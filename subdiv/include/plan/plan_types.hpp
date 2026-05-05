#pragma once

#include <span>
#include <vector>
#include <cassert>
#include <glm/glm.hpp>

namespace Subdiv::Plan 
{
//! GPU Tesselator supports factor 64 -> 6 levels (2^6 = 64)
static constexpr int MAX_SUBDIV_DEPTH = 6;

//! Control points per regular B-Spline patch (4x4 grid)
static constexpr int REGULAR_CP_COUNT = 16;

//! Control points per terminal node level (5x5 grid, 24 unique - 3 adjacent patches share edge control points)
static constexpr int TERMINAL_CP_PER_LVL = 24;

//! Stencils for an extraordinary vertex node: limit pos + 2 tangents.
static constexpr int EXTRAORDINARY_STENCIL_COUNT = 3;

/**
 * @brief NodeType
 * 
 * - Internal - No directly evaluable children ( #children = valence )
 * - Regular       - Regular B-Spline Patch (valence = 4 on all corners, 4x4=16 control points, no crease/boundary features) 
 * - Boundary      - Like Regular but on boundary or hard-crease edge (Kobbelt-extrapolated phantom vertices control points - 4x4 still applies)
 * - Crease        - Like Regular with exactly one semi-sharp crease edge (4x4 control points + crease metadata)
 * - Terminal      - Colapses repeating 3 Reg + 1 Int pattern around extraordinary vertex into single node (24 cp's per level on 5x5 grid + 3 EV stencils as fallback) 
 * - Extraordinary - Fallback for reaching maximum depth at Extraordinary corner. (limit position + 2 tangent stencils)
 */
enum class NodeType : uint8_t { Internal = 0, Regular, Boundary, Crease, Terminal, Extraordinary };

/**
 * @brief QuadNode (8 bytes)
 * 
 * QuadTree Structure stored in flat array with root at nodes[0]
 * Internal nodes reference children by index:
 *      children at nodes[payload + 0 ... (#children - 1)]
 * Leaf nodes reference stencils by offset:
 *      stencil i starts at wheights[payload + i * ring_size]
 * 
 * Payload by type:
 *      - Internal      - index of first child in nodes[]  (children are contiguous)
 *      - Regular       - stencil offset (16 stencils × ring_size weights each)
 *      - Boundary      - stencil offset (16 stencils × ring_size weights each)
 *      - Crease        - stencil offset (16 stencils), crease_data indexed separately
 *      - Terminal      - stencil offset (terminal_levels × 24 + 3 stencils)
 *      - Extraordinary - stencil offset (3 stencils: pos, tan0, tan1)
 */
struct alignas(8) QuadNode
{
    NodeType type       = NodeType::Internal;
    uint8_t  childCount = 4;    
    uint8_t  creaseEdge = 0xFF; // which edge is creased
    uint8_t  rotation   = 0;    // Terminal/Extraordinary: EV corner index or canocical rotation for n-gon root
    uint32_t payload    = 0xFFFFFFFF;
};
static_assert(sizeof(QuadNode) == 8, "QuadNode must be 8 bytes.");

/**
 * @brief CreaseData
 * 
 * stored in parallel array.
 */
struct CreaseData 
{
    float   sharpness = 0.0f;
    uint8_t edge      = 0xFF; // which edge is creased in 4x4 pach
    uint8_t _pad[3]   = {};
};
static_assert(sizeof(CreaseData) == 8, "CreaseData must be 8 bytes.");

/**
 * @brief SubdivisionPlan
 * 
 * Immutable after construction.
 * Shared across all faces with the same 1-ring topology.
 * 
 * Memory layout summary:
 *      nodes[]        — flat quadtree (8 bytes/node)
 *      weights[]      — packed stencil weights (float per 1-ring vertex per stencil)
 *      level_counts[] — cumulative stencil count per depth (LOD skipping)
 *      crease_data[]  — one entry per Crease node
 *      crease_node_to_data[] — maps node index -> crease_data index
 */
struct SubdivisionPlan
{
    // Contiguous Stencil weights 
    // (Stencil i: weights[payload + i*ring_size .. payload + (i+1)*ring_size - 1])
    // (weights[j] coefficient for 1-ring vertex j)
    std::vector<float>      weights; 
    std::vector<QuadNode>   nodes;            // Quadtree (flat, root at 0)
    std::vector<uint32_t>   levelCounts;      // levelCounts[d] = total stencils needed for maxDepth d. (LOD skipping)
    std::vector<CreaseData> creaseData;
    std::vector<uint32_t>   creaseNodeToData;

    uint16_t ringSize     = 0; // #vertices in 1-ring of this face configuration.
    uint8_t  faceValence  = 4; // base face valence
    uint8_t  maxDepth     = 0; // subdiv depth (stop storage -> EV eval fallback)
    uint8_t  terminalLvls = 0; // levels stored in terminal nodes = maxDepth - depth of terminal node
    uint8_t  _pad[3]      = {};

    uint32_t stencilCount = 0; // Total stencils stored in weights[] (= weights.size() / ring_size).
};

// Get a single stencil's weights as a span.
inline std::span<const float> getStencil(const SubdivisionPlan& plan, uint32_t stencil_offset, uint32_t stencil_index)
{
    uint32_t start = stencil_offset + stencil_index * plan.ringSize;

    assert(start + plan.ringSize <= plan.weights.size());

    return { plan.weights.data() + start, plan.ringSize };
}

// Evaluate a stencil: dot-product weights against provided positions. positions[i] is the world-space (or object-space) position of 1-ring vertex i.
inline glm::vec3 evalStencil(const SubdivisionPlan& plan, uint32_t stencil_offset, uint32_t stencil_index, std::span<const glm::vec3> positions)
{
    auto w = getStencil(plan, stencil_offset, stencil_index);

    assert(w.size() == positions.size());

    glm::vec3 result{0.0f};
    for (uint32_t i = 0; i < (uint32_t)w.size(); ++i)
        result += w[i] * positions[i];
    
    return result;
}

} // namespace Subdiv::Plan