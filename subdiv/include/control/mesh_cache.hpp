#pragma once

#include "mesh_types.hpp"

#include <span>
#include <array>

namespace Subdiv::Control 
{
// Forward declaration
class Mesh;

/**
 * @brief Precomputed topology cache for fast mesh queries.
 * 
 * Provides O(1) access to:
 * - Vertex valences (number of incident edges)
 * - Boundary status (vertex/edge on boundary)
 * - Vertex one-rings (neighboring vertices in CCW order)
 * - Edge vertices (canonical ordering v0 < v1)
 * 
 * Built once after mesh construction/modification, then used
 * for subdivision, rendering, and queries.
 * 
 * Memory layout:
 * - SOA (Structure of Arrays) for cache efficiency
 * - CSR (Compressed Sparse Row) for one-rings
 * - GPU-ready (can upload directly)
 */
class TopologyCache
{
public:
    TopologyCache() = default;
    ~TopologyCache() { clear(); }

    /**
     * @brief Build cache from mesh.
     * 
     * Algorithm:
     * 1. Single pass over half-edges to extract edge data
     * 2. Count valences via edge counting (O(E))
     * 3. Mark boundary vertices/edges
     * 4. Build CSR offsets for one-rings
     * 5. Fill one-rings via half-edge traversal (ordered CCW)
     * 
     * Complexity: O(V + E + H) where H = number of half-edges
     * 
     * @param mesh Source mesh
     */
    void build(const Mesh& mesh);
    
    /**
     * @brief Clear all cached data.
     */
    void clear();

    /**
     * @brief Check if cache is built.
     */
    bool isValid() const { return !valences_.empty(); }

    // ===== Vertex Queries ===== //

    /**
     * @brief Get vertex valence (number of incident edges).
     * @return Valence (0 for isolated vertices)
     */
    uint16_t getValence(VertexIndex v) const { return (v < valences_.size()) ? valences_[v] : 0; }

    /**
     * @brief Check if vertex is on boundary.
     */
    bool isBoundaryVertex(VertexIndex v) const { return (v < boundaryFlags_.size()) ? boundaryFlags_[v] : false; }

    /**
     * @brief Get vertex one-ring (neighboring vertices in CCW order).
     * 
     * Zero-copy - returns view into internal array.
     * Order is consistent for subdivision algorithms.
     * 
     * @return Span of neighbor indices (empty if invalid vertex)
     */
    std::span<const VertexIndex> getVertexOneRing(VertexIndex v) const;

    /**
     * @brief Get faces incident to a vertex.
     * Used for computing vertex points in subdivision.
     * 
     * @return Span of faces that share vertex v (empty if invalid vertex)
     */
    std::span<const FaceIndex> getVertexFaces(VertexIndex v) const;

    // ===== Edge Queries ===== //

    /**
     * @brief Check if edge is on boundary.
     */
    bool isBoundaryEdge(EdgeIndex e) const {  return (e < edgeBoundaryFlags_.size()) ? edgeBoundaryFlags_[e] : false; }

    /**
     * @brief Get edge vertices in canonical order (v0 < v1).
     * 
     * @return [v0, v1] or [INVALID_INDEX, INVALID_INDEX] if invalid edge
     */
    std::array<VertexIndex, 2> getEdgeVertices(EdgeIndex e) const 
    {
        if(e < edgeVertices_.size())
            return edgeVertices_[e];
        return {INVALID_INDEX, INVALID_INDEX};
    }

    /**
     * @brief Get faces incident to an edge (1 for boundary, 2 for interior).
     * Used for computing edge points in subdivision.
     */
    std::span<const FaceIndex> getEdgeFaces(EdgeIndex e) const;

    // ===== Face Queries ===== //

    /**
     * @brief Get face vertices in CCW order.
     * Fast alternative to half-edge traversal.
     */
    std::span<const VertexIndex> getFaceVertices(FaceIndex f) const;
    
    /**
     * @brief Get face edges in CCW order.
     */
    std::span<const EdgeIndex> getFaceEdges(FaceIndex f) const;

    // ===== Statistics ===== //

    size_t numVertices()         const { return valences_.size(); }
    size_t numEdges()            const { return edgeVertices_.size(); }
    size_t numFaces()            const { return faceVertexOffsets_.empty() ? 0 : faceVertexOffsets_.size() - 1; }
    size_t numBoundaryVertices() const { return numBoundaryVertices_; }
    size_t numBoundaryEdges()    const { return numBoundaryEdges_; }

    /**
     * @brief Compute memory usage in bytes.
     */
    size_t memoryUsage() const;

    // ===== Direct Array Access (for GPU upload) ===== //

    const std::vector<uint16_t>& getValences()       const { return valences_; }
    const std::vector<uint8_t>&  getBoundaryFlags()  const { return boundaryFlags_; }
    
    const std::vector<VertexIndex>& getOneRings()       const { return oneRings_; }
    const std::vector<uint32_t>&    getOneRingOffsets() const { return oneRingOffsets_; }
    
    const std::vector<std::array<VertexIndex, 2>>& getEdgeVerticesArray() const { return edgeVertices_; }
    const std::vector<uint8_t>&                    getEdgeBoundaryFlags() const { return edgeBoundaryFlags_; }

private:
    // Vertex data (SOA layout)
    std::vector<uint16_t> valences_;                        // Number of incident edges
    std::vector<uint8_t>  boundaryFlags_;                   // 0 = interior, 1 = boundary
    
    // Vertex one-rings (CSR format)
    std::vector<VertexIndex> oneRings_;        // Flattened neighbor lists
    std::vector<uint32_t>    oneRingOffsets_;  // Start index per vertex (+1 sentinel)

    // Vertex-face incidence (CSR format)
    std::vector<FaceIndex> vertexFaces_;       // Faces incident to each vertex
    std::vector<uint32_t>  vertexFaceOffsets_; // Start index per vertex (+1 sentinel)
    
    // Edge data
    std::vector<std::array<VertexIndex, 2>> edgeVertices_;      // [v0, v1] canonical order
    std::vector<uint8_t>                    edgeBoundaryFlags_; // 0 = interior, 1 = boundary

    // Edge-face incidence (CSR format)
    std::vector<FaceIndex> edgeFaces_;         // Faces incident to each edge
    std::vector<uint32_t>  edgeFaceOffsets_;   // Start index per edge (+1 sentinel)

    // Face vertices (CSR format)
    std::vector<VertexIndex> faceVertices_;    // Vertices of each face (CCW)
    std::vector<uint32_t>    faceVertexOffsets_; // Start index per face (+1 sentinel)
    
    // Face edges (CSR format)
    std::vector<EdgeIndex> faceEdges_;         // Edges of each face (CCW)
    std::vector<uint32_t>  faceEdgeOffsets_;   // Start index per face (+1 sentinel)    
    
    // Statistics
    size_t numBoundaryVertices_ = 0;
    size_t numBoundaryEdges_    = 0;

    // Mesh validity flag
    bool   valid_ = false;
};

} // namespace Subdiv::Control