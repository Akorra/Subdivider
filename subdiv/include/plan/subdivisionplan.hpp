#pragma once

#include <vector>

namespace Subdiv::Plan
{

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