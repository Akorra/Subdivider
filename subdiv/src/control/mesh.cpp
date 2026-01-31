#include "control/mesh.hpp"
#include "diagnostics/context.hpp"

#include <unordered_set>
#include <algorithm>

namespace Subdiv::Control
{
// Construction & Editing =====================================================

VertexIndex Mesh::addVertex(const glm::vec3& pos)
{
    VertexIndex idx = static_cast<VertexIndex>(vertices.size());

    Vertex v;
    v.outgoing  = INVALID_INDEX;
    v.sharpness = 0.0f;
    v.isCorner  = 0;

    vertices.push_back(v);
    positions.push_back(pos);
    normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
    uvs.push_back(glm::vec2(0.0f));

    invalidateCache();
    
    return idx;
}

FaceIndex Mesh::addFace(std::span<const VertexIndex> verts)
{
    // Validation =============================================================
    if(verts.size() < 3)
    {
        SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"FACE_TOO_FEW_VERTICES", "Face must have at least 3 vertices", "Got " + std::to_string(verts.size()) + " vertices");
        return INVALID_INDEX;
    }

    // Check all vertices exist
    for (size_t i = 0; i < verts.size(); ++i) 
    {
        if (verts[i] >= vertices.size()) 
        {
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"INVALID_VERTEX_INDEX","Vertex index out of bounds", "Vertex " + std::to_string(verts[i]) + " at position " + std::to_string(i));
            return INVALID_INDEX;
        }
    }

    // Check for duplicate vertices in face
    std::unordered_set<VertexIndex> vertSet;
    for (size_t i = 0; i < verts.size(); ++i) 
    {
        if (!vertSet.insert(verts[i]).second) 
        {
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"DUPLICATE_VERTEX_IN_FACE", "Face contains duplicate vertex", "Vertex " + std::to_string(verts[i]) + " appears multiple times");
            return INVALID_INDEX;
        }
    }

    // Check for duplicate directed edges and non-manifold edges
    for (size_t i = 0; i < verts.size(); ++i) 
    {
        VertexIndex v0 = verts[i];
        VertexIndex v1 = verts[(i + 1) % verts.size()];

        uint64_t key     = makeDirectedEdgeKey(v0, v1);
        uint64_t twinKey = makeDirectedEdgeKey(v1, v0);
        
        // Check if this directed edge already exists
        if (halfEdgeMap_.find(key) != halfEdgeMap_.end()) 
        {
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"DUPLICATE_DIRECTED_EDGE", "Directed edge already exists", "Edge " + std::to_string(v0) + "->" + std::to_string(v1) + " at position " + std::to_string(i));
            return INVALID_INDEX;
        }

        // Check if twin edge exists and already has a twin (non-manifold)
        auto twinIt = halfEdgeMap_.find(twinKey);
        if (twinIt != halfEdgeMap_.end()) 
        {
            HalfEdgeIndex twinHe = twinIt->second;
            if (halfEdges[twinHe].twin != INVALID_INDEX) 
            {
                SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"NON_MANIFOLD_EDGE", "Edge would have more than 2 faces", "Edge " + std::to_string(v0) + "->" +  std::to_string(v1));
                return INVALID_INDEX;
            }
        }
    }

    // Create Face ============================================================
    
    FaceIndex faceIdx = static_cast<FaceIndex>(faces.size());
    
    Face face;
    face.valence = static_cast<uint32_t>(verts.size());
    face.edge    = static_cast<HalfEdgeIndex>(halfEdges.size()); // Will be first half-edge
    
    faces.push_back(face);

    // Create Half-Edges ======================================================

    HalfEdgeIndex firstHe = static_cast<HalfEdgeIndex>(halfEdges.size());

    for(size_t i=0; i < verts.size(); ++i)
    {
        VertexIndex v0 = verts[i];
        VertexIndex v1 = verts[(i + 1) % verts.size()];

        HalfEdgeIndex heIdx = static_cast<HalfEdgeIndex>(halfEdges.size());

        HalfEdge he;
        he.to = v1;
        he.face = faceIdx;
        he.next = (i + 1 < verts.size()) ? heIdx + 1 : firstHe;
        he.prev = (i > 0) ? heIdx - 1 : firstHe + static_cast<HalfEdgeIndex>(verts.size() - 1);
        he.twin = INVALID_INDEX;
        he.edge = INVALID_INDEX;

        // Check if twin exists
        uint64_t twinKey = makeDirectedEdgeKey(v1, v0);
        auto twinIt = halfEdgeMap_.find(twinKey);

        if (twinIt != halfEdgeMap_.end()) 
        {
            // Twin exists - link them and reuse edge
            HalfEdgeIndex twinIdx = twinIt->second;

            // Ensure twin doesn't already have a twin
            if (halfEdges[twinIdx].twin != INVALID_INDEX) 
            {
                // This should never happen due to pre-validation, but double-check
                // Roll back the face we just added
                faces.pop_back();
                SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"NON_MANIFOLD_EDGE_DURING_CREATION", "Found twin that already has a twin during half-edge creation", "Edge between vertices " + std::to_string(v0) + " and " + std::to_string(v1) + " - this indicates a validation bug or concurrent modification");
                return INVALID_INDEX;
            }

            he.twin = twinIdx;
            he.edge = halfEdges[twinIdx].edge;
            
            halfEdges.push_back(he);
            
            // Link twin back
            halfEdges[twinIdx].twin = heIdx;
        } 
        else 
        {
            // No twin - create new edge
            EdgeIndex edgeIdx = static_cast<EdgeIndex>(edges.size());
            he.edge = edgeIdx;
            
            Edge edge;
            edge.tag = EdgeTag::SMOOTH;
            edge.sharpness = 0.0f;
            edges.push_back(edge);
            
            halfEdges.push_back(he);
        }

        // Add to map
        uint64_t directedKey = makeDirectedEdgeKey(v0, v1);
        halfEdgeMap_[directedKey] = heIdx;
        
        // Update vertex outgoing
        if (vertices[v0].outgoing == INVALID_INDEX)
            vertices[v0].outgoing = heIdx;
    }

    invalidateCache();

    return faceIdx;
}

void Mesh::setPosition(VertexIndex v, const glm::vec3& pos)
{
    if (v >= positions.size()) return;
    positions[v] = pos;
}

void Mesh::setEdgeSharpness(EdgeIndex e, float sharpness)
{
    if (e >= edges.size()) return;
    edges[e].sharpness = sharpness;
    edges[e].tag       = (sharpness > 0.0f) ? EdgeTag::SEMI : EdgeTag::SMOOTH;
}

void Mesh::setEdgeCrease(EdgeIndex e, bool crease)
{
    if (e >= edges.size()) return;
    edges[e].tag = crease ? EdgeTag::CREASE : EdgeTag::SMOOTH;
    if (crease)
        edges[e].sharpness = 1.0f;
}

void Mesh::clear()
{
    vertices.clear();
    halfEdges.clear();
    edges.clear();
    faces.clear();
    positions.clear();
    normals.clear();
    uvs.clear();
    halfEdgeMap_.clear();
    cache.clear();
}

// Queries (delegate to cache) ================================================

uint16_t Mesh::getValence(VertexIndex v) const
{
    if (!cache.isValid())
        const_cast<Mesh*>(this)->buildCache();
    return cache.getValence(v);
}

bool Mesh::isBoundaryVertex(VertexIndex v) const
{
    if (!cache.isValid())
        const_cast<Mesh*>(this)->buildCache();
    return cache.isBoundaryVertex(v);
}

std::span<const VertexIndex> Mesh::getOneRing(VertexIndex v) const
{
    if (!cache.isValid())
        const_cast<Mesh*>(this)->buildCache();
    return cache.getVertexOneRing(v);
}

std::array<VertexIndex, 2> Mesh::getEdgeVertices(EdgeIndex e) const
{
    if (!cache.isValid())
        const_cast<Mesh*>(this)->buildCache();
    return cache.getEdgeVertices(e);
}

HalfEdgeIndex Mesh::findHalfEdge(VertexIndex v0, VertexIndex v1) const
{
    uint64_t key = makeDirectedEdgeKey(v0, v1);
    auto it = halfEdgeMap_.find(key);
    
    if (it != halfEdgeMap_.end())
        return it->second;
    
    // Try twin direction and return twin
    uint64_t twinKey = makeDirectedEdgeKey(v1, v0);
    it = halfEdgeMap_.find(twinKey);
    
    if (it != halfEdgeMap_.end())
        return halfEdges[it->second].twin;
    
    return INVALID_INDEX;
}

VertexIndex Mesh::getFromVertex(HalfEdgeIndex he) const
{
    if (he >= halfEdges.size()) return INVALID_INDEX;
    
    HalfEdgeIndex prev = halfEdges[he].prev;
    if (isValidIndex(prev, halfEdges.size())) 
        return halfEdges[prev].to;
    return INVALID_INDEX;
}

// Topology Cache =============================================================

void Mesh::buildCache()
{
    cache.build(*this);
}

// Utilities ==================================================================
void Mesh::computeNormals()
{
    // Reset normals
    std::fill(normals.begin(), normals.end(), glm::vec3(0.0f));
    
    // Accumulate face normals (area-weighted)
    for (FaceIndex f = 0; f < faces.size(); ++f) 
    {
        const Face& face = faces[f];
        
        if (face.valence < 3) continue;
        
        HalfEdgeIndex he = face.edge;
        if (he == INVALID_INDEX || he >= halfEdges.size()) continue;
        
        // Get first 3 vertices to compute normal
        VertexIndex v0 = getFromVertex(he);
        he = halfEdges[he].next;
        if (he == INVALID_INDEX || he >= halfEdges.size()) continue;
        
        VertexIndex v1 = getFromVertex(he);
        he = halfEdges[he].next;
        if (he == INVALID_INDEX || he >= halfEdges.size()) continue;
        
        VertexIndex v2 = getFromVertex(he);
        
        if (v0 >= positions.size() || v1 >= positions.size() || v2 >= positions.size()) continue;
        
        glm::vec3 p0 = positions[v0];
        glm::vec3 p1 = positions[v1];
        glm::vec3 p2 = positions[v2];
        
        glm::vec3 normal = glm::cross(p1 - p0, p2 - p0);
        
        // Accumulate to all face vertices
        he = face.edge;
        size_t count = 0;
        do 
        {
            VertexIndex v = getFromVertex(he);
            if (v < normals.size())
                normals[v] += normal;
            he = halfEdges[he].next;
            count++;
        }
        while (he != face.edge && he != INVALID_INDEX && count < face.valence + 10);
    }
    
    // Normalize
    for (glm::vec3& n : normals) 
    {
        float len = glm::length(n);
        if (len > 1e-6f)
            n /= len;
        else
            n = glm::vec3(0.0f, 1.0f, 0.0f);

    }
}

bool Mesh::validate() const
{
    bool valid = true;
    
    // Check vertex outgoing
    for (size_t v = 0; v < vertices.size(); ++v) {
        HalfEdgeIndex out = vertices[v].outgoing;
        if (out != INVALID_INDEX && out >= halfEdges.size()) 
        {
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"INVALID_VERTEX_OUTGOING", "Vertex outgoing half-edge out of bounds", "Vertex " + std::to_string(v));
            valid = false;
        }
    }
    
    // Check half-edge connectivity
    for (size_t h = 0; h < halfEdges.size(); ++h) {
        const HalfEdge& he = halfEdges[h];
        
        // Check next/prev
        if (he.next != INVALID_INDEX) {
            if (he.next >= halfEdges.size()) 
            {
                SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"INVALID_HALFEDGE_NEXT", "Half-edge next out of bounds", "HalfEdge " + std::to_string(h));
                valid = false;
            } 
            else if (halfEdges[he.next].prev != h) 
            {
                SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"HALFEDGE_NEXT_PREV_MISMATCH", "next->prev doesn't point back", "HalfEdge " + std::to_string(h));
                valid = false;
            }
        }
        
        // Check prev/next
        if (he.prev != INVALID_INDEX) {
            if (he.prev >= halfEdges.size()) 
            {
                SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"INVALID_HALFEDGE_PREV", "Half-edge prev out of bounds", "HalfEdge " + std::to_string(h));
                valid = false;
            } 
            else if (halfEdges[he.prev].next != h) 
            {
                SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"HALFEDGE_PREV_NEXT_MISMATCH", "prev->next doesn't point back", "HalfEdge " + std::to_string(h));
                valid = false;
            }
        }
        
        // Check twin
        if (he.twin != INVALID_INDEX) 
        {
            if (he.twin >= halfEdges.size()) 
            {
                SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"INVALID_HALFEDGE_TWIN", "Half-edge twin out of bounds", "HalfEdge " + std::to_string(h));
                valid = false;
            } 
            else if (halfEdges[he.twin].twin != h) 
            {
                SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"HALFEDGE_TWIN_MISMATCH", "twin->twin doesn't point back", "HalfEdge " + std::to_string(h));
                valid = false;
            }
        }
        
        // Check edge
        if (he.edge != INVALID_INDEX && he.edge >= edges.size()) {
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR, "INVALID_HALFEDGE_EDGE", "Half-edge edge out of bounds", "HalfEdge " + std::to_string(h));
            valid = false;
        }
        
        // Check face
        if (he.face != INVALID_INDEX && he.face >= faces.size()) {
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"INVALID_HALFEDGE_FACE", "Half-edge face out of bounds", "HalfEdge " + std::to_string(h));
            valid = false;
        }
        
        // Check to vertex
        if (he.to != INVALID_INDEX && he.to >= vertices.size()) 
        {
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR, "INVALID_HALFEDGE_TO","Half-edge destination vertex out of bounds", "HalfEdge " + std::to_string(h));
            valid = false;
        }
    }
    
    // Check face edges reference valid half-edges
    for (size_t f = 0; f < faces.size(); ++f) 
    {
        const Face& face = faces[f];
        if (face.edge != INVALID_INDEX && face.edge >= halfEdges.size()) {
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR,"INVALID_FACE_EDGE", "Face edge half-edge out of bounds", "Face " + std::to_string(f));
            valid = false;
        }
    }
    
    return valid;
}

size_t Mesh::getMemoryUsage() const
{
    size_t total = 0;
    
    total += vertices.size() * sizeof(Vertex);
    total += halfEdges.size() * sizeof(HalfEdge);
    total += edges.size() * sizeof(Edge);
    total += faces.size() * sizeof(Face);
    
    total += positions.size() * sizeof(glm::vec3);
    total += normals.size() * sizeof(glm::vec3);
    total += uvs.size() * sizeof(glm::vec2);
    
    total += cache.memoryUsage();
    
    // Half-edge map overhead
    total += halfEdgeMap_.size() * (sizeof(uint64_t) + sizeof(HalfEdgeIndex));
    
    return total;
}

}