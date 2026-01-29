#include "control/mesh.hpp"

#include <cassert>

namespace Subdiv::Control 
{

// --- Element creation ---

VertexIndex Mesh::addVertex(const glm::vec3& pos)
{
    VertexIndex idx = static_cast<VertexIndex>(vertices_.size());
    vertices_.push_back(Vertex{pos});
    vertexAttributes_.push_back(VertexAttributes{});
    return idx;
}

FaceIndex Mesh::addFace(const std::vector<VertexIndex>& verts)
{
    assert(verts.size() > 2 &&
        "Faces must have at least 3 vertices");
    
    // Check all vertices exist
    for (VertexIndex v : verts)
        assert(v < vertices_.size() && "Invalid vertex index");
    
    // Check for duplicate vertices in face
    for (size_t i = 0; i < verts.size(); ++i) {
        for (size_t j = i + 1; j < verts.size(); ++j)
            assert(verts[i] != verts[j] && "Face has duplicate vertices");
    }
    
    FaceIndex faceIdx = static_cast<FaceIndex>(faces_.size());
    Face face;
    face.valence = static_cast<uint32_t>(verts.size());
    
    // Create half-edges for this face
    std::vector<HalfEdgeIndex> faceHalfEdges;
    faceHalfEdges.reserve(verts.size());
    
    for (size_t i = 0; i < verts.size(); ++i) 
    {
        VertexIndex v0 = verts[i];
        VertexIndex v1 = verts[(i + 1) % verts.size()];
        
        HalfEdgeIndex heIdx = static_cast<HalfEdgeIndex>(halfEdges_.size());
        
        HalfEdge he;
        he.to   = v1;
        he.face = faceIdx;
        
        halfEdges_.push_back(he);
        faceHalfEdges.push_back(heIdx);

        // Update vertex outgoing if not set
        if(vertices_[v0].outgoing == INVALID_INDEX) 
            vertices_[v0].outgoing = heIdx;
        
        // Check if the opposite half-edge (v1 -> v0) already exists
        uint64_t twinKey = makeDirectedEdgeKey(v1, v0);
        auto it = halfEdgeMap_.find(twinKey);

        if(it != halfEdgeMap_.end())
        {
            // Twin half-edge found (v1->v0)
            HalfEdgeIndex twinIdx = it->second;

            // MANIFOLD CHECK: twin should not already have a twin
            assert(halfEdges_[twinIdx].twin == INVALID_INDEX &&
                "Non-Manifold edge detected: edge has more than 2 faces");

            // Twin must already have an edge (edges are created alongside half-edges)
            assert(halfEdges_[twinIdx].edge != INVALID_INDEX && 
                   "Twin half-edge found without edge");
        
            // Link twins
            halfEdges_[heIdx].twin   = twinIdx;
            halfEdges_[twinIdx].twin = heIdx;
            halfEdges_[heIdx].edge   = halfEdges_[twinIdx].edge; 
        }
        else 
        {
            // No twin found yet - this is either a boundary edge or the first half-edge
            EdgeIndex edgeIdx = static_cast<EdgeIndex>(edges_.size());
            edges_.push_back(Edge{});
            halfEdges_[heIdx].edge = edgeIdx;
            
            // Store this half-edge in the map for future twin lookup
            uint64_t directedKey = makeDirectedEdgeKey(v0, v1);
            halfEdgeMap_[directedKey] = heIdx;
        }
    }
    
    // Link half-edges in a loop (next/prev)
    for (size_t i = 0; i < faceHalfEdges.size(); ++i) {
        HalfEdgeIndex curr = faceHalfEdges[i];
        HalfEdgeIndex next = faceHalfEdges[(i + 1) % faceHalfEdges.size()];
        HalfEdgeIndex prev = faceHalfEdges[(i + faceHalfEdges.size() - 1) % faceHalfEdges.size()];
        
        halfEdges_[curr].next = next;
        halfEdges_[curr].prev = prev;
    }
    
    // Set face's half-edge pointer
    face.edge = faceHalfEdges[0];
    faces_.push_back(face);
    faceAttributes_.push_back(FaceAttributes{});
    
    return faceIdx;
}

VertexIndex Mesh::getFromVertex(HalfEdgeIndex heIdx) const
{
    assert(heIdx < halfEdges_.size());

    const HalfEdge& he = halfEdges_[heIdx];
    
    if (he.prev == INVALID_INDEX)
        return INVALID_INDEX;
    
    return halfEdges_[he.prev].to;
}

int Mesh::getVertexValence(VertexIndex vIdx) const
{
    assert(vIdx < vertices_.size());
    
    const Vertex& v = vertices_[vIdx];
    if (v.outgoing == INVALID_INDEX)
        return 0; // Isolated vertex
    
    int valence = 0;
    HalfEdgeIndex start = v.outgoing;
    HalfEdgeIndex current = start;
    
    do 
    {
        valence++;
        
        // Move to next outgoing half-edge
        HalfEdgeIndex twin = halfEdges_[current].twin;

        if (twin == INVALID_INDEX) 
        {
            // Boundary vertex - count edges in other direction
            current = v.outgoing;
            // Walk backwards until we hit boundary
            while (true) 
            {
                HalfEdgeIndex prev = halfEdges_[current].prev;
                if (prev == INVALID_INDEX) break;
                
                HalfEdgeIndex prevTwin = halfEdges_[prev].twin;
                if (prevTwin == INVALID_INDEX) break;
                
                current = prevTwin;
                valence++;
            }
            break;
        }
        
        current = halfEdges_[twin].next;
        
        // Safety check for infinite loop
        if (valence > 1000)
            return -1; // Invalid mesh
        
    } 
    while (current != start && current != INVALID_INDEX);
    
    return valence;
}

bool Mesh::isBoundaryVertex(VertexIndex vIdx) const
{
    assert(vIdx < vertices_.size());
    
    const Vertex& v = vertices_[vIdx];
    if (v.outgoing == INVALID_INDEX)
        return true; // Isolated vertex is considered boundary
    
    // Walk around vertex and check if any half-edge has no twin
    HalfEdgeIndex start = v.outgoing;
    HalfEdgeIndex current = start;
    
    do 
    {
        if (halfEdges_[current].twin == INVALID_INDEX)
            return true;
        
        HalfEdgeIndex twin = halfEdges_[current].twin;
        if (twin == INVALID_INDEX)
            return true;
        
        current = halfEdges_[twin].next;
        
        // Safety check
        if (current == INVALID_INDEX)
            return true;
        
    } 
    while (current != start);
    
    return false;
}

HalfEdgeIndex Mesh::findHalfEdge(VertexIndex v0, VertexIndex v1) const
{
    uint64_t key = makeDirectedEdgeKey(v0, v1);
    auto it = halfEdgeMap_.find(key);
    
    if (it == halfEdgeMap_.end())
        return INVALID_INDEX;
    
    return it->second;
}

EdgeIndex Mesh::findEdge(VertexIndex v0, VertexIndex v1) const
{
    // Try to find half-edge in either direction
    HalfEdgeIndex he = findHalfEdge(v0, v1);
    if (he == INVALID_INDEX)
        he = findHalfEdge(v1, v0);
    
    if (he == INVALID_INDEX)
        return INVALID_INDEX;
    
    return halfEdges_[he].edge;
}

// --- Validation ---

bool Mesh::validate() const
{
    // Check all half-edges
    for (size_t i = 0; i < halfEdges_.size(); ++i) {
        const HalfEdge& he = halfEdges_[i];
        
        // Check 'to' vertex exists
        if (he.to >= vertices_.size())
            return false;
        
        // Check twin is valid
        if (he.twin != INVALID_INDEX) 
        {
            if (he.twin >= halfEdges_.size())
                return false;

            // Twin's twin should point back
            if (halfEdges_[he.twin].twin != i)
                return false;

            // Twins should share the same edge
            if (he.edge != INVALID_INDEX && halfEdges_[he.twin].edge != he.edge)
                return false;

            // Twin should point back to the 'from' vertex
            VertexIndex from = getFromVertex(static_cast<HalfEdgeIndex>(i));
            if (from != INVALID_INDEX && halfEdges_[he.twin].to != from)
                return false;
        }
        
        // Check next/prev if they exist
        if (he.next != INVALID_INDEX && he.next >= halfEdges_.size())
            return false;

        if (he.prev != INVALID_INDEX && he.prev >= halfEdges_.size())
            return false;
        
        // Check edge exists
        if (he.edge != INVALID_INDEX && he.edge >= edges_.size())
            return false;
        
        // Check face exists
        if (he.face != INVALID_INDEX && he.face >= faces_.size())
            return false;
        
        // If has face, must have next and prev
        if (he.face != INVALID_INDEX) 
        {
            if (he.next == INVALID_INDEX || he.prev == INVALID_INDEX)
                return false;
        }
        
        // Check next->prev consistency
        if (he.next != INVALID_INDEX) 
        {
            if (halfEdges_[he.next].prev != i)
                return false;
        }
        
        // Check prev->next consistency
        if (he.prev != INVALID_INDEX) 
        {
            if (halfEdges_[he.prev].next != i)
                return false;
        }
    }
    
    // Check all faces
    for (size_t i = 0; i < faces_.size(); ++i) 
    {
        const Face& f = faces_[i];
        
        if (f.edge >= halfEdges_.size())
            return false;
        
        // Walk around face and verify loop
        HalfEdgeIndex start = f.edge;
        HalfEdgeIndex current = start;
        uint32_t count = 0;
        
        do 
        {
            if (halfEdges_[current].face != i) 
                return false;
            
            current = halfEdges_[current].next;
            count++;
            
            if (count > f.valence + 1)
                return false; // Loop too long
            
        } 
        while (current != start && current != INVALID_INDEX);
        
        if (count != f.valence)
            return false; // Valence mismatch
    }
    
    // Check manifoldness: each edge should have at most 2 half-edges
    for (size_t i = 0; i < edges_.size(); ++i) 
    {
        int heCount = 0;
        for (const HalfEdge& he : halfEdges_) 
        {
            if (he.edge == i)
                heCount++;
        }
        
        if (heCount > 2 || heCount == 0) // Non-manifold or Orphaned edge
            return false; 
    }
    
    // Check attribute array sizes match
    if (vertexAttributes_.size() != vertices_.size())
        return false;

    if (faceAttributes_.size() != faces_.size())
        return false;
    
    return true;
}

// --- Editing support ---

void Mesh::clear()
{
    vertices_.clear();
    halfEdges_.clear();
    edges_.clear();
    faces_.clear();
    vertexAttributes_.clear();
    faceAttributes_.clear();
    halfEdgeMap_.clear();
    faceGroups_.clear();
}

void Mesh::rebuildEdgeMap()
{
    halfEdgeMap_.clear();
    
    for (size_t i = 0; i < halfEdges_.size(); ++i) 
    {
        const HalfEdge& he = halfEdges_[i];
        
        if (he.prev == INVALID_INDEX) continue;
        
        VertexIndex v0 = halfEdges_[he.prev].to;
        VertexIndex v1 = he.to;
        
        uint64_t key = makeDirectedEdgeKey(v0, v1);
        halfEdgeMap_[key] = static_cast<HalfEdgeIndex>(i);
    }
}

// --- Attribute management ---

void Mesh::computeFaceNormals()
{
    for (size_t i = 0; i < faces_.size(); ++i) 
    {
        const Face& face = faces_[i];
        
        // Get three vertices of the face to compute normal
        HalfEdgeIndex he0 = face.edge;
        HalfEdgeIndex he1 = halfEdges_[he0].next;
        HalfEdgeIndex he2 = halfEdges_[he1].next;
        
        VertexIndex v0 = getFromVertex(he0);
        VertexIndex v1 = halfEdges_[he0].to;
        VertexIndex v2 = halfEdges_[he1].to;
        
        if (v0 == INVALID_INDEX) continue;
        
        const glm::vec3& p0 = vertices_[v0].position;
        const glm::vec3& p1 = vertices_[v1].position;
        const glm::vec3& p2 = vertices_[v2].position;
        
        glm::vec3 edge1 = p1 - p0;
        glm::vec3 edge2 = p2 - p0;
        glm::vec3 normal = glm::cross(edge1, edge2);
        
        float length = glm::length(normal);
        if (length > 1e-6f) {
            normal /= length;
        } else {
            normal = glm::vec3(0.0f, 1.0f, 0.0f); // Degenerate face
        }
        
        faceAttributes_[i].normal = normal;
    }
}

void Mesh::computeVertexNormals()
{
    // First compute face normals
    computeFaceNormals();
    
    // Reset vertex normals
    for (auto& attrib : vertexAttributes_)
        attrib.normal = glm::vec3(0.0f);
    
    // Accumulate face normals to vertices (area-weighted)
    for (size_t i = 0; i < faces_.size(); ++i) 
    {
        const Face& face = faces_[i];
        const glm::vec3& faceNormal = faceAttributes_[i].normal;
        
        // Walk around face and add normal to each vertex
        HalfEdgeIndex current = face.edge;
        HalfEdgeIndex start = current;
        
        do 
        {
            VertexIndex vIdx = getFromVertex(current);
            if (vIdx != INVALID_INDEX)
                vertexAttributes_[vIdx].normal += faceNormal;
            
            current = halfEdges_[current].next;
        } 
        while (current != start && current != INVALID_INDEX);
    }
    
    // Normalize vertex normals
    for (auto& attrib : vertexAttributes_) 
    {
        float length = glm::length(attrib.normal);
        if (length > 1e-6f)
            attrib.normal /= length;
        else
            attrib.normal = glm::vec3(0.0f, 1.0f, 0.0f);
    }
}
