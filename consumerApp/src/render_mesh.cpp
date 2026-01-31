#include "render_mesh.hpp"
#include "config.hpp"

RenderMesh::RenderMesh(const Subdiv::Control::Mesh& mesh) : mesh_(mesh)
{
}

void RenderMesh::build()
{
    SUBDIV_PROFILE_FUNCTION();
    
    clear();
    
    buildTriangleIndices();
    buildWireframeIndices();
    
    valid_ = true;
}

void RenderMesh::clear()
{
    triangleIndices_.clear();
    wireframeIndices_.clear();
    valid_ = false;
}

void RenderMesh::buildTriangleIndices()
{
    // Fan triangulation for each face
    for (Subdiv::Control::FaceIndex f = 0; f < mesh_.numFaces(); ++f) {
        const auto& face = mesh_.faces[f];
        
        if (face.valence < 3) continue;
        
        // Collect face vertices via half-edge traversal
        std::vector<Subdiv::Control::VertexIndex> faceVerts;
        faceVerts.reserve(face.valence);
        
        Subdiv::Control::HalfEdgeIndex start = face.edge;
        if (start == Subdiv::Control::INVALID_INDEX || start >= mesh_.halfEdges.size()) continue;
        
        Subdiv::Control::HalfEdgeIndex current = start;
        size_t count = 0;
        
        do {
            Subdiv::Control::VertexIndex v = mesh_.getFromVertex(current);
            if (v != Subdiv::Control::INVALID_INDEX) {
                faceVerts.push_back(v);
            }
            
            current = mesh_.halfEdges[current].next;
            count++;
            
            if (count > face.valence + 10) break; // Safety
            
        } while (current != start && current != Subdiv::Control::INVALID_INDEX);
        
        // Fan triangulation: (v0, v1, v2), (v0, v2, v3), ...
        if (faceVerts.size() >= 3) {
            for (size_t i = 1; i + 1 < faceVerts.size(); ++i) {
                triangleIndices_.push_back(faceVerts[0]);
                triangleIndices_.push_back(faceVerts[i]);
                triangleIndices_.push_back(faceVerts[i + 1]);
            }
        }
    }
}

void RenderMesh::buildWireframeIndices()
{
    // Ensure cache is built
    if (!mesh_.cache.isValid()) {
        const_cast<Subdiv::Control::Mesh&>(mesh_).buildCache();
    }
    
    // One line per edge (v0, v1)
    for (Subdiv::Control::EdgeIndex e = 0; e < mesh_.numEdges(); ++e) {
        auto [v0, v1] = mesh_.cache.getEdgeVertices(e);
        if (v0 != Subdiv::Control::INVALID_INDEX && v1 != Subdiv::Control::INVALID_INDEX) {
            wireframeIndices_.push_back(v0);
            wireframeIndices_.push_back(v1);
        }
    }
}

size_t RenderMesh::getMemoryUsage() const
{
    return triangleIndices_.size() * sizeof(uint32_t) +
           wireframeIndices_.size() * sizeof(uint32_t);
}
