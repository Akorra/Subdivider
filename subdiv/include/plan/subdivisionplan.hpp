#pragma once

#include <vector>

namespace Subdiv::Plan
{

enum class NodeType : uint8_t {
    Internal     = 0,  // has 4 children, recurse
    Regular      = 1,  // bicubic B-spline, 16 control points
    Boundary     = 2,  // like Regular, extrapolated phantom pts
    Crease       = 3,  // Regular + one semi-sharp edge
    Terminal     = 4,  // collapses n levels of EV subtree
    Extraordinary= 5,  // EV corner: limit pos + 2 tangents only
};

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
 class SubdivisionPlan 
 {

 };

} // Subdiv::Plan 