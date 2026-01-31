#pragma once

#include "mesh_cache.hpp"

namespace Subdiv::Control 
{

/**
 * @brief Core subdivision mesh - topology and attributes only.
 * 
 * Design:
 * - Flat index-based arrays (GPU-ready)
 * - SOA layout (cache-optimized)
 * - No rendering code (use RenderMesh for that)
 * - Direct GPU data access (zero-copy)
 * 
 * For rendering, wrap with RenderMesh which generates indices.
 */
class Mesh
{
public:
    // Construction & Editing =================================================
    
    Mesh() = default;
    ~Mesh() { clear(); }

    VertexIndex addVertex(const glm::vec3& pos);
    FaceIndex   addFace(std::initializer_list<VertexIndex> verts); 
    FaceIndex   addFace(std::span<const VertexIndex> verts);

    void setPosition(VertexIndex v, const glm::vec3& pos);
    void setEdgeSharpness(EdgeIndex e, float sharpness);
    void setEdgeCrease(EdgeIndex e, bool crease);
    
    void clear();

    // Queries ================================================================

    size_t numVertices()  const { return vertices.size(); }
    size_t numHalfEdges() const { return halfEdges.size(); }
    size_t numEdges()     const { return edges.size(); }
    size_t numFaces()     const { return faces.size(); }
    bool   isEmpty()      const { return vertices.empty(); }

    uint16_t getValence(VertexIndex v) const;
    bool     isBoundaryVertex(VertexIndex v) const;

    std::span<const VertexIndex> getOneRing(VertexIndex v) const;
    std::array<VertexIndex, 2>   getEdgeVertices(EdgeIndex e) const;

    HalfEdgeIndex findHalfEdge(VertexIndex v0, VertexIndex v1) const;
    EdgeIndex     findEdge(VertexIndex v0, VertexIndex v1) const;
    VertexIndex   getFromVertex(HalfEdgeIndex he) const;

    // Topology Cache =========================================================

    void buildCache();

    // Utilities ==============================================================

    void   computeNormals();
    bool   validate() const;
    size_t getMemoryUsage() const;

    // Direct Data Access (for GPU upload, subdivision, etc.) =================

    const void* getPositionsData()  const { return positions.data(); }
    size_t      getPositionsBytes() const { return positions.size() * sizeof(glm::vec3); }
    
    const void* getNormalsData()  const { return normals.data(); }
    size_t      getNormalsBytes() const { return normals.size() * sizeof(glm::vec3); }
    
    const void* getUVsData()  const { return uvs.data(); }
    size_t      getUVsBytes() const { return uvs.size() * sizeof(glm::vec2); }
    
    // Topology cache data (for GPU subdivision)
    const void* getValencesData()  const { return cache.getValences().data(); }
    size_t      getValencesBytes() const { return cache.getValences().size() * sizeof(uint16_t); }
    
    const void* getOneRingsData()  const { return cache.getOneRings().data(); }
    size_t      getOneRingsBytes() const { return cache.getOneRings().size() * sizeof(VertexIndex); }
    
    const void* getOneRingOffsetsData()  const { return cache.getOneRingOffsets().data(); }
    size_t      getOneRingOffsetsBytes() const { return cache.getOneRingOffsets().size() * sizeof(uint32_t); }

private:
    std::unordered_map<uint64_t, HalfEdgeIndex> halfEdgeMap_;
    
    void invalidateCache() { cache.clear(); }
    
    static uint64_t makeDirectedEdgeKey(VertexIndex v0, VertexIndex v1) { return (static_cast<uint64_t>(v0) << 32) | v1; }

public:
    // Public Data (Direct Access) ============================================
    
    // Topology
    std::vector<Vertex>     vertices;
    std::vector<HalfEdge>   halfEdges;
    std::vector<Edge>       edges;
    std::vector<Face>       faces;
    
    // Attributes (SOA - separate from topology)
    std::vector<glm::vec3>  positions;     // Vertex positions
    std::vector<glm::vec3>  normals;       // Vertex normals
    std::vector<glm::vec2>  uvs;           // Vertex UVs

    TopologyCache cache;
};

} // namespace Subdiv::Control