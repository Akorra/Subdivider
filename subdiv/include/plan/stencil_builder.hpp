#pragma once

#include "plan/plan_types.hpp"
#include "control/mesh_types.hpp"

#include <span>
#include <array>
#include <unordered_map>
#include <glm/gtc/constants.hpp>

// Computes subdivision stencil weights for the control points of each leaf node in the subdivision plan.
//
// A stencil for control point P at subdivision depth d expresses P as a linear combination of the base-mesh 1-ring vertices:
//     P = Sum_i { w_i * base_vertex[i] }
//
// We compute weights *symbolically*: represent each vertex at depth d
// as a sparse map {base_vertex_index -> weight}, then apply Catmull-Clark
// rules to propagate weights down through the levels.
//
// This is pure CPU work done once at plan-build time.  
// The resulting dense float arrays are what the GPU hull shader evaluates at runtime.

namespace Subdiv::Plan
{

// Symbolic vertex: a sparse linear combination over base vertices. key = base VertexIndex, value = weight (float)
using SymVert = std::unordered_map<Control::VertexIndex, float>;

// Multiply all weights by scalar:
inline SymVert scale(const SymVert& v, float s)
{
    SymVert out(v.size());
    for (auto& [k, w] : v) out[k] = w*s;
    return out;
}

// Accumulate src * s into dst
inline void madd(SymVert& dst, const SymVert& src, float s)
{
    for (auto& [k, w] : src) 
        dst[k] += w*s;
}

// Convert SymVert to a dense float array of length ring_size. ring_map maps base VertexIndex -> local 1-ring index.
inline void densify(
    const SymVert& sym,
    const std::unordered_map<Control::VertexIndex, uint32_t>& ring_map,
    std::vector<float>& out_weights,   // append here
    uint32_t ring_size)
{
    // Append ring_size zeros, then fill from sym
    size_t base = out_weights.size();
    
    out_weights.resize(base + ring_size, 0.0f);

    for (auto& [v, w] : sym) 
    {
        auto it = ring_map.find(v);
        assert(it != ring_map.end() && "stencil references vertex outside 1-ring");
        out_weights[base + it->second] += w;
    }
}

/**
 * @brief SubdivLevel
 * 
 * 1 lvl of the symbolic subdivision hierarchy.
 * Stores symbolic positions for every vertex in subdivided mesh at lvl (only those in 1-ring region of face)
 */
struct SubdivLevel
{
    // Positions of vertices accessible at this level.
    // Indexed by their index *within this level's local mesh*.
    std::vector<SymVert> verts;

    // Topology mirroring the actual subdivided mesh at this level:
    //   face_verts[f][0..3] = local vertex indices for face f (we only track faces within the 1-ring region)
    std::vector<std::array<int,4>> face_verts; // quads only after level 1
    
    // Edge data: pairs of local vert indices
    std::vector<std::pair<int,int>> edges;
};

/**
 * @brief StencilBuilder
 *
 * Public API
 *  
 * Given the base-mesh 1-ring of a face, builds symbolic Catmull-Clark 
 * subdivisions down to a requested depth and extracts stencil weights 
 * for the control points of each leaf node in the plan.
 */
class StencilBuilder
{
public:
    //! Compute 16 stencil arrays for regular 4x4 patch
    uint32_t ComputeRegularStencils(const std::vector<SymVert>& level_verts, const std::array<int,16>& patch_local_indices,
        const std::unordered_map<Control::VertexIndex, uint32_t>& ring_map, uint32_t ring_size, std::vector<float>& out_weights)
    {
        uint32_t offset = (uint32_t)(out_weights.size() / ring_size);
        for(int i=0; i<16; ++i)
            densify(level_verts[patch_local_indices[i]], ring_map, out_weights, ring_size);
        return offset;
    }

    //! COmpute 24 stencil weight arrays for Terminal node (5x5 grid shared across 3 regular paches)
    uint32_t ComputeTerminalStencils(const std::vector<SymVert>& level_verts, const std::array<int,24>& grid_local_indices,
        const std::unordered_map<Control::VertexIndex, uint32_t>& ring_map, uint32_t ring_size, std::vector<float>& out_weights) 
    { 
        uint32_t offset = (uint32_t)(out_weights.size() / ring_size);
        for (int i = 0; i < 24; ++i)
            densify(level_verts[grid_local_indices[i]], ring_map, out_weights, ring_size);
        return offset; 
    }

    //! Compute 3 stencils for an extraordinary vertex (limit position, limit tangent 1 and limit tangent 2)
    uint32_t ComputeEVStencils(const SymVert& ev_sym, const std::vector<SymVert>& ring_syms /*1-ring of EV*/, uint32_t valence,
        const std::unordered_map<Control::VertexIndex, uint32_t>& ring_map, uint32_t ring_size, std::vector<float>& out_weights)
    { 
        uint32_t offset = (uint32_t)(out_weights.size() / ring_size);

        // limit position stencil - Catmull-Clark limit position for an interior vertex of valence n
        {
            float n     = (float)valence;
            float denom = n * (n + 5.0f);

            SymVert lim_pos;
            
            madd(lim_pos, ev_sym, (n * n) / denom);
            // ring_syms: interleaved edge-adjacent, face-diagonal (length = 2n)
            for (uint32_t i = 0; i < valence; ++i) 
            {
                madd(lim_pos, ring_syms[2*i],     4.0f / denom); // edge-adjacent
                madd(lim_pos, ring_syms[2*i + 1], 1.0f / denom); // face-diagonal
            }
            densify(lim_pos, ring_map, out_weights, ring_size);
        }

        // limit tangent 0 stencil - Catmull-Clark limit tangent (first): T0 = sum_i cos(2*pi*i/n) * e_i
        // e_i are edge-adjacent vertices, normalised by the EV
        {
            SymVert t0;
            for (uint32_t i = 0; i < valence; ++i) 
            {
                float angle = glm::two_pi<float>() * (float)i / (float)valence;
                madd(t0, ring_syms[2*i], glm::cos(angle));
            }
            densify(t0, ring_map, out_weights, ring_size);
        }

        // limit tangent 1 stencil - Catmull-Clark limit tangent (second): T1 = sum_i sin(2*pi*i/n) * e_i
        {
            SymVert t1;
            for (uint32_t i = 0; i < valence; ++i) 
            {
                float angle = glm::two_pi<float>() * (float)i / (float)valence;
                madd(t1, ring_syms[2*i], glm::sin(angle));
            }
            densify(t1, ring_map, out_weights, ring_size);
        }

        return offset; 
    }
};

} // namespace Subdiv::Plan
