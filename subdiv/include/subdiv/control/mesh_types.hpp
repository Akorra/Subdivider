#pragma once

#include <vector>       
#include <string>   
#include <cstdint>
#include <glm/glm.hpp> // GLM math

namespace Subdiv::Control
{

static constexpr uint32_t INVALID_INDEX = 0xFFFFFFFF;
static constexpr float    HARD_CREASE   = -1.0f;
static constexpr float    SMOOTH_CREASE = 0.0f;
static constexpr float    MAX_SHARPNESS = 10.0f;

// #define IS_VALID_INDEX(idx, maxSize) ((idx) != INVALID_INDEX && (idx) < (maxSize))
template <typename IndexT>
constexpr bool isValidIndex(IndexT idx, size_t maxSize) noexcept  {  return idx != INVALID_INDEX && static_cast<size_t>(idx) < maxSize; }

// Type Aliases
using VertexIndex   = uint32_t;
using HalfEdgeIndex = uint32_t;
using FaceIndex     = uint32_t;
using EdgeIndex     = uint32_t;

/**
 * @brief Vertex structure - GPU friendly layout.
 * 
 * Memory layout:   8 bytes
 * - HalfEdgeIndex: 4 bytes
 * - sharpness:     4 bytes
 */
struct Vertex 
{
    HalfEdgeIndex outgoing   = INVALID_INDEX; // One outgoing half-edge
    float         sharpness  = SMOOTH_CREASE; // 0=smooth, >0=semi-sharp corner, -1=hard corner
};
static_assert(sizeof(Vertex) == 8, "Vertex should be 8 bytes");

/**
 * @brief Half-edge structure.
 * 
 * Memory layout: 24 bytes (6 x uint32_t)
 * Tightly packed for cache efficiency.
 */
struct HalfEdge 
{
    VertexIndex   to   = INVALID_INDEX; // Destination vertex
    HalfEdgeIndex next = INVALID_INDEX; // Next in face loop
    HalfEdgeIndex prev = INVALID_INDEX; // Previous in face loop
    HalfEdgeIndex twin = INVALID_INDEX; // Opposite half-edge
    EdgeIndex     edge = INVALID_INDEX; // Parent edge
    FaceIndex     face = INVALID_INDEX; // Adjacent face
};
static_assert(sizeof(HalfEdge) == 24, "HalfEdge should be 24 bytes");

/**
 * @brief Edge attributes - shared between twin half-edges.
 * Stores crease information for subdivision.
 * 
 * Memory layout: 4 bytes
 */
struct Edge 
{
    float sharpness = SMOOTH_CREASE; // 0 = smooth, >0 = semi-sharp, -1 = hard crease 
};
static_assert(sizeof(Edge) == 4, "Edge should be 4 bytes");

/**
 * @brief Face structure.
 * 
 * Memory layout: 8 bytes (2 x uint32_t)
 */
struct Face 
{
    HalfEdgeIndex edge    = INVALID_INDEX; // One boundary half-edge
    uint32_t      valence = 0;             // Number of vertices
};
static_assert(sizeof(Face) == 8, "Face should be 8 bytes");

/**
 * @brief Face group for materials/selections.
 */
struct FaceGroup 
{
    std::string            name;
    std::vector<FaceIndex> faces;
};

/**
 * @brief Per-face attributes for rendering.
 * 
 * Memory layout: 16 bytes
 */
struct FaceAttributes
{
    glm::vec3 normal{0.0f, 1.0f, 0.0f};  // Face normal (for flat shading)
    uint32_t  materialId = 0;             // Material/texture ID
};


// Core data arrays - these map directly to GPU buffers
using Vertices      = std::vector<Vertex>;
using HalfEdges     = std::vector<HalfEdge>;
using Edges         = std::vector<Edge>;
using Faces         = std::vector<Face>;

} // namespace Subdiv::Control