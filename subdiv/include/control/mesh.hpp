#pragma once

#include "mesh_types.hpp"

#include <unordered_map>

namespace Subdiv::Control 
{
/**
 * @brief Catmull-Clark mesh.
 * Optimized for GPU upload and real-time editing.
 * Errors are logged to global Diagnostics singleton if enabled.
 */
class Mesh
{
public:
    Mesh() = default;
    ~Mesh() = default;

    // Delete copy
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    
    // Allow move
    Mesh(Mesh&&) = default;
    Mesh& operator=(Mesh&&) = default;

    // --- Element creation ---
    VertexIndex   addVertex(const glm::vec3& pos);
    FaceIndex     addFace(const std::vector<VertexIndex>& verts);

    // --- Accessors (topology) ---
    const Vertices&  getVertices()  const { return vertices_; }
    const HalfEdges& getHalfEdges() const { return halfEdges_; }
    const Edges&     getEdges()     const { return edges_; }
    const Faces&     getFaces()     const { return faces_; }

    Vertex&           vertex(VertexIndex i)       { return vertices_[i]; }
    const Vertex&     vertex(VertexIndex i) const { return vertices_[i]; }
    
    HalfEdge&         halfEdge(HalfEdgeIndex i)       { return halfEdges_[i]; }
    const HalfEdge&   halfEdge(HalfEdgeIndex i) const { return halfEdges_[i]; }
    
    Edge&             edge(EdgeIndex i)       { return edges_[i]; }
    const Edge&       edge(EdgeIndex i) const { return edges_[i]; }
    
    Face&             face(FaceIndex i)       { return faces_[i]; }
    const Face&       face(FaceIndex i) const { return faces_[i]; }

    // --- Accessors (attributes) ---
    const VertexAttribs& getVertexAttributes() const { return vertexAttributes_; }
    const FaceAttribs&   getFaceAttributes()   const { return faceAttributes_; }
    
    VertexAttributes&       vertexAttrib(VertexIndex i)       { return vertexAttributes_[i]; }
    const VertexAttributes& vertexAttrib(VertexIndex i) const { return vertexAttributes_[i]; }
    
    FaceAttributes&       faceAttrib(FaceIndex i)       { return faceAttributes_[i]; }
    const FaceAttributes& faceAttrib(FaceIndex i) const { return faceAttributes_[i]; }
    
    // --- Topology queries ---
    VertexIndex   getFromVertex(HalfEdgeIndex heIdx) const;
    int           getVertexValence(VertexIndex vIdx) const;
    bool          isBoundaryVertex(VertexIndex vIdx) const;
    
    // Find half-edge from v0 to v1 (returns INVALID_INDEX if not found)
    HalfEdgeIndex findHalfEdge(VertexIndex v0, VertexIndex v1) const;
    
    // Find edge between v0 and v1 (returns INVALID_INDEX if not found)
    EdgeIndex     findEdge(VertexIndex v0, VertexIndex v1) const;
    
    // --- Validation ---
    bool validate() const;
    
    // --- Statistics ---
    size_t numVertices()  const { return vertices_.size(); }
    size_t numHalfEdges() const { return halfEdges_.size(); }
    size_t numEdges()     const { return edges_.size(); }
    size_t numFaces()     const { return faces_.size(); }
    
    // --- Editing support ---
    void clear();
    void rebuildEdgeMap(); // Rebuild after bulk modifications

    // --- Attribute management ---
    void computeVertexNormals();  // Compute smooth vertex normals from face normals
    void computeFaceNormals();    // Compute face normals from geometry

private:
    Vertices  vertices_;
    HalfEdges halfEdges_;
    Edges     edges_;
    Faces     faces_;

    // Rendering attributes (GPU-ready, separate buffers)
    VertexAttribs vertexAttributes_;
    FaceAttribs   faceAttributes_;
    
    std::vector<FaceGroup> faceGroups_;
    
    // Mapping: directed (v0 -> v1) -> HalfEdgeIndex (This stores the half-edge FROM v0 TO v1)
    std::unordered_map<uint64_t, HalfEdgeIndex> halfEdgeMap_;
};

} // namespace Subdiv::Control