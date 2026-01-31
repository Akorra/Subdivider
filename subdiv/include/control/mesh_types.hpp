#pragma once

#include <vector>       
#include <string>   
#include <cstdint>
#include <glm/glm.hpp> // GLM math

namespace Subdiv::Control
{

// #define IS_VALID_INDEX(idx, maxSize) ((idx) != INVALID_INDEX && (idx) < (maxSize))
template <typename IndexT, typename SizeT>
constexpr bool isValidIndex(IndexT idx, SizeT maxSize) noexcept { return idx != INVALID_INDEX && idx < maxSize; }

// Type Aliases
using VertexIndex   = uint32_t;
using HalfEdgeIndex = uint32_t;
using FaceIndex     = uint32_t;
using EdgeIndex     = uint32_t;

static constexpr uint32_t INVALID_INDEX = 0xFFFFFFFF;

/**
 * @brief Edge sharpness classification for subdivision.
 */
enum class EdgeTag : uint8_t 
{
    SMOOTH = 0,  ///< Smooth edge (default)
    CREASE = 1,  ///< Hard crease (infinitely sharp)
    SEMI   = 2,  ///< Semi-sharp (sharpness decreases each subdivision)
};

/**
 * @brief Vertex structure - GPU friendly layout.
 * 
 * Memory layout:   16 bytes
 * - vec3 position: 12 bytes
 * - HalfEdgeIndex: 4 bytes
 */
struct Vertex 
{
    HalfEdgeIndex outgoing   = INVALID_INDEX; // One outgoing half-edge
    float         sharpness  = 0.0f;          // Corner sharpness
    uint8_t       isCorner   = 0;             // Dart vertex flag
    uint8_t       padding[3] = {0};           // Explicit padding
};
static_assert(sizeof(Vertex) == 12, "Vertex should be 12 bytes");

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
 * Memory layout: 8 bytes
 */
struct Edge 
{
    EdgeTag tag = EdgeTag::SMOOTH;
    uint8_t padding[3] = {0};
    float sharpness = 0.0f;
};
static_assert(sizeof(Edge) == 8, "Edge should be 8 bytes");

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