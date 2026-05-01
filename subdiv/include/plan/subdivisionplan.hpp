#pragma once

#include <vector>

namespace Subdiv::Plan
{

// QuadTree Node Types
enum class NodeType : uint8_t {
    Internal     = 0,  // has 4 children, recurse
    Regular      = 1,  // bicubic B-spline, 16 control points
    Boundary     = 2,  // like Regular, extrapolated phantom pts
    Crease       = 3,  // Regular + one semi-sharp edge
    Terminal     = 4,  // collapses n levels of EV subtree
    Extraordinary= 5,  // EV corner: limit pos + 2 tangents only
};

/**
 * @brief QuadNode
 * 
 * 8-byte node for a flattened quadtree array
 * Tree structure is encoded via integer indices rather than pointers.
 * Internal nodes store the index of their first child
 * children are of node[i] are at node[child_offset + 0 .. 3].
 * Total Size: 8 bytes -> 2 nodes per cache line pair
 * (children are allocated in groups of 4 -> 1 cache line holds 2 sibling groups)
 */
struct alignas(8) QuadNode {
    NodeType  type;
    uint8_t   crease_edge;    // which edge (0-3), only for Crease
    uint8_t   rotation;       // EV corner rotation, Terminal/Extraordinary
    uint8_t   depth;          // subdivision depth of this node
    uint32_t  payload;        // meaning depends on type:
                              //   Internal:      index of first child (children are payload+0..3)
                              //   Regular/Boundary/Crease: stencil_offset (16 stencils)
                              //   Terminal:      stencil_offset (24 * num_levels stencils)
                              //   Extraordinary: stencil_offset (3 stencils: pos, tan0, tan1)
};

/**
 * @brief CreaseData
 * 
 * Data for crease nodes 
 */ 
struct CreaseData {
    float     sharpness;
    uint8_t   edge;         // which edge (0-3) is creased
    uint8_t   _pad[3];
};

static_assert(sizeof(QuadNode) == 8);

/**
 * @brief SubdivisionPlan
 *  
 * subdivision plan for face configuration
 * 
 * Design:
 * - Flat index-based dense arrays
 * - SOA layout (cache-optimized)
 * - Immutable after construction
 * - Shared across faces with the same 1-ring topology
 */
struct SubdivisionPlan 
{
    std::vector<QuadNode>   nodes;        // QuadTree nodes with root at nodes_[0]
    std::vector<float>      weights;      // Packed stencil weights: all stencils concatenated. stencil i starts at:  weights.data() + i * ringSize
    std::vector<uint16_t>   levelCounts;  // Number of stencils per-level (max 6 levels for factor 64)
    std::vector<CreaseData> creaseData;   // Crease data, indexed by crease node's stencil_offset

    uint16_t ringSize       = 0; // Size of 1-ring neighborhood     
    uint8_t  maxDepth       = 0; // Maximum subdivision depth stored in this plan
    uint8_t  terminalLevels = 0; // Number of terminal node levels stored
    uint32_t stencilCount   = 0; // Total number of stencils
};

} // Subdiv::Plan 