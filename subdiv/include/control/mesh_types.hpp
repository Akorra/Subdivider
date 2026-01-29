#pragma once

#include <vector>       
#include <string>   
#include <cstdint>
#include <glm/glm.hpp> // GLM math

namespace Subdiv::Control
{
// Indices for cache-friendly storage
using VertexIndex   = uint32_t;
using HalfEdgeIndex = uint32_t;
using FaceIndex     = uint32_t;
using EdgeIndex     = uint32_t;

static constexpr uint32_t INVALID_INDEX = 0xFFFFFFFF;

/**
 * @brief Edge type for subdivision surfaces.
 */
enum class EdgeTag : uint8_t 
{
    EDGE_SMOOTH = 0,    // Normal smooth edge.
    EDGE_HARD   = 1,    // Fully sharp edge.    
    EDGE_SEMI   = 2,    // Semi-sharp edge, controlled by sharpness var
};


/**
 * @brief Vertex structure - GPU friendly layout.
 * 
 * Memory layout: 24 bytes
 * - vec3 position: 12 bytes
 * - HalfEdgeIndex: 4 bytes
 * - float sharpness: 4 bytes
 * - bool isCorner: 1 byte
 * - padding: 3 bytes
 */
struct Vertex 
{
    glm::vec3     position{0.0f};             // Vertex position in 3D space
    HalfEdgeIndex outgoing   = INVALID_INDEX; // One outgoing half-edge (arbitrary)
    float         sharpness  = 0.0f;          // Corner sharpness (0 = smooth, higher = sharper)
    bool          corner     = false;         // Corner vertex (don't smooth)
    uint8_t       padding[3] = {0};           // Explicit padding for alignment
    
    bool isBoundary() const { return outgoing == INVALID_INDEX; }
};

/**
 * @brief Half-edge structure.
 * 
 * Memory layout: 20 bytes (5 x uint32_t)
 * Tightly packed for cache efficiency.
 */
struct HalfEdge 
{
    VertexIndex   to   = INVALID_INDEX; // Destination vertex
    HalfEdgeIndex next = INVALID_INDEX; // Next half-edge in face loop
    HalfEdgeIndex prev = INVALID_INDEX; // Previous half-edge in face loop
    HalfEdgeIndex twin = INVALID_INDEX; // Opposite half-edge (INVALID if boundary)
    FaceIndex     face = INVALID_INDEX;
    EdgeIndex     edge = INVALID_INDEX; // Parent edge (for shared attributes)
    
    bool isBoundary() const { return twin == INVALID_INDEX; }
};

/**
 * @brief Edge attributes - shared between twin half-edges.
 * Stores crease information for subdivision.
 * 
 * Memory layout: 8 bytes
 */
struct Edge
{
    EdgeTag tag        = EdgeTag::EDGE_SMOOTH;
    float   sharpness  = 0.0f;                  // Semi-sharp edge sharpness (0 = smooth, higher = sharper)
    uint8_t padding[3] = {0};                   // Explicit padding
};

/**
 * @brief Face structure.
 * 
 * Memory layout: 8 bytes (2 x uint32_t)
 */
struct Face 
{
    HalfEdgeIndex edge    = INVALID_INDEX; // One half-edge on this face
    uint32_t      valence = 0;             // Number of edges (cached)
    
    bool isQuad() const { return valence == 4; }
};

/**
 * @brief Face group for materials/selections.
 */
struct FaceGroup 
{
    std::string            name;
    std::vector<FaceIndex> faces;
};

/**
 * @brief Per-vertex attributes for rendering.
 * Stored separately from topology for flexibility and GPU alignment.
 * These can be subdivided using various rules (linear, smooth, etc.)
 * 
 * Memory layout: 32 bytes (cache-line friendly)
 */
struct VertexAttributes
{
    glm::vec3 normal{0.0f, 1.0f, 0.0f};  // 12 bytes - Vertex normal
    glm::vec2 uv{0.0f};                  // 8 bytes  - Texture coordinates
    glm::vec4 color{1.0f};               // 16 bytes - Vertex color (RGBA)
    
    // Could add tangent/bitangent here if needed
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
using VertexAttribs = std::vector<VertexAttributes>;
using FaceAttribs   = std::vector<FaceAttributes>;

/**
 * @brief Create a directed key for a half-edge (v0 -> v1).
 * This maintains direction, so v0->v1 and v1->v0 have different keys.
 */
inline uint64_t makeDirectedEdgeKey(VertexIndex v0, VertexIndex v1) 
{
    return (static_cast<uint64_t>(v0) << 32) | static_cast<uint64_t>(v1);
}


/**
 * @brief Create an undirected key for an edge (unordered pair of vertices).
 * Used for finding edges regardless of direction.
 */
inline uint64_t makeUndirectedEdgeKey(VertexIndex v0, VertexIndex v1) 
{
    if (v0 > v1) std::swap(v0, v1);
    return (static_cast<uint64_t>(v0) << 32) | static_cast<uint64_t>(v1);
}

} // namespace Subdiv::Control