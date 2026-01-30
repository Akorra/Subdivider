#pragma once

#include "mesh_types.hpp"

#include <span>
#include <array>

namespace Subdiv::Control 
{
// Forward declaration
class Mesh;

/**
 * @brief Precomputed topology cache for fast subdivision queries.
 * 
 * Stores frequently-accessed topology information in cache-friendly
 * formats (SOA, CSR). Built once, used many times during subdivision.
 * 
 * Features:
 * - O(1) valence/boundary queries
 * - Cache-friendly vertex one-rings (CSR format)
 * - SIMD-ready data layout
 * - GPU-uploadable arrays
 */
class TopologyCache
{
public:
    TopologyCache() = default;
    ~TopologyCache() { clear(); }

    /**
     * @brief Build cache from mesh topology.
     * Should be called after mesh construction/modification.
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

    // --- Vertex queries ---
    bool     isBoundaryVertex(VertexIndex v) const { return boundaryFlags_[v]; }
    uint16_t getValence(VertexIndex v)       const { return valences_[v]; }

    /**
     * @brief Get vertex one-ring (neighboring vertices in Counter Clock Wise [CCW] order).
     * Zero-copy - returns view into internal array.
     */
    std::span<const VertexIndex> getVertexOneRing(VertexIndex v) const;

    /**
     * @brief Get faces incident to a vertex.
     */
    std::span<const FaceIndex>   getVertexFaces(VertexIndex v) const;

    // --- Edge queries ---
    bool     isBoundaryEdge(EdgeIndex e)     const { return boundaryEdges_[e]; }

    /**
     * @brief Get the two vertices of an edge.
     * @return [v0, v1] where v0 < v1 (canonical ordering)
     */
    const std::array<VertexIndex, 2>& getEdgeVertices(EdgeIndex e) const { return edgeVertices_[e]; }

    /**
     * @brief Get the one or two faces adjacent to an edge.
     * Boundary edges have one face, interior edges have two.
     * @return Span of 1 or 2 face indices
     */
    std::span<const FaceIndex> getEdgeFaces(EdgeIndex e) const;
    
    // --- Face queries ---
    
    /**
     * @brief Get face vertices (in CCW order).
     */
    std::span<const VertexIndex> getFaceVertices(FaceIndex f) const;

    /**
     * @brief Get the edges of a face (in CCW order).
     */
    std::span<const EdgeIndex> getFaceEdges(FaceIndex f) const;
    

    // --- Statistics ---
    size_t numVertices()         const { return valences_.size(); }
    size_t numEdges()            const { return edgeVertices_.size(); }
    size_t numFaces()            const { return faceVertexOffsets_.size() - 1; }
    size_t numBoundaryVertices() const { return boundaryVertexCount_; }
    size_t numBoundaryEdges()    const { return boundaryEdgeCount_; }
    size_t memoryUsage()         const;

    // --- Direct array access (for GPU upload) ---
    const std::vector<uint16_t>& getValences()     const { return valences_; }
    const std::vector<uint8_t>& getBoundaryFlags() const { return boundaryFlags_; }
    const std::vector<std::array<VertexIndex, 2>>& getEdgeVerticesArray() const { return edgeVertices_; }

private:
    // Vertex data (SOA layout)
    std::vector<uint16_t> valences_;
    std::vector<uint8_t>  boundaryFlags_;
    
    // Vertex one-ring (CSR - Compressed Sparse Row)
    std::vector<VertexIndex> vertexOneRing_;
    std::vector<uint32_t>    vertexOneRingOffsets_;
    
    // Vertex-face incidence
    std::vector<FaceIndex> vertexFaces_;
    std::vector<size_t>    vertexFaceOffsets_;
    
    // Edge data
    std::vector<uint8_t> boundaryEdges_;
    std::vector<std::array<VertexIndex, 2>> edgeVertices_;

    // Edge-face incidence (CSR)
    std::vector<FaceIndex> edgeFaces_;        // Faces adjacent to each edge
    std::vector<size_t>    edgeFaceOffsets_;  // CSR offsets

    // Face vertices
    std::vector<VertexIndex> faceVertices_;
    std::vector<uint32_t>    faceVertexOffsets_;

    std::vector<EdgeIndex> faceEdges_;        // Edges of each face
    std::vector<size_t>    faceEdgeOffsets_;  // CSR offsets
    
    // Statistics
    size_t boundaryVertexCount_ = 0;  
    size_t boundaryEdgeCount_   = 0;

    // Mesh validity flag
    bool   valid_ = false;
};

} // namespace Subdiv::Control