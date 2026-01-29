#include "control/mesh.hpp"

#include "diagnostics/context.hpp"

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
    SUBDIV_PROFILE_FUNCTION();
    
    // Helper lambda to return error
    auto returnError = [&](const std::string& code, const std::string& message, 
                          const std::string& context = "") -> FaceIndex {
        SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR, code, message, context);
        return INVALID_INDEX;
    };

    // Validate input
    if (verts.size() < 3)
        return returnError("FACE_TOO_FEW_VERTICES", 
                           "Face must have at least 3 vertices",
                           "vertex count: " + std::to_string(verts.size()));
    
    // Check for duplicate vertices
    for (size_t i = 0; i < verts.size(); ++i) 
    {
        for (size_t j = i + 1; j < verts.size(); ++j) 
        {
            if (verts[i] == verts[j]) 
            {
                std::ostringstream oss;
                oss << "vertex " << verts[i] << " at positions " << i << " and " << j;
                return returnError("DUPLICATE_VERTEX_IN_FACE", "Face contains duplicate vertex", oss.str());
            }
        }
    }
    
    FaceIndex faceIdx = static_cast<FaceIndex>(faces_.size());
    Face face;
    face.valence = static_cast<uint32_t>(verts.size());

    // Track memory allocation
    SUBDIV_TRACK_ALLOC("HalfEdges", sizeof(HalfEdge) * verts.size());
    
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

            if (halfEdges_[twinIdx].twin != INVALID_INDEX) 
            {
                // Rollback
                halfEdges_.resize(faceHalfEdges[0]);
                SUBDIV_TRACK_DEALLOC("HalfEdges", sizeof(HalfEdge) * faceHalfEdges.size());
                
                std::ostringstream oss;
                oss << "edge (" << v0 << ", " << v1 << ")";
                return returnError("NON_MANIFOLD_EDGE", "Edge already has two faces (non-manifold)", oss.str());
            }
            
            SUBDIV_ASSERT(halfEdges_[twinIdx].edge != INVALID_INDEX, "Twin half-edge found without edge");
        
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

            SUBDIV_TRACK_ALLOC("Edges", sizeof(Edge));
            
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

    SUBDIV_TRACK_ALLOC("Faces", sizeof(Face) + sizeof(FaceAttributes));
    
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
    SUBDIV_PROFILE_FUNCTION();
    
    bool valid = true;
    
    auto addValidationError = [&](const std::string& code, const std::string& message, 
                                  const std::string& context = "") {
        valid = false;
        SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::ERROR, code, message, context);
    };
    
    // Check all half-edges
    for (size_t i = 0; i < halfEdges_.size(); ++i) {
        const HalfEdge& he = halfEdges_[i];
        
        if (he.to >= vertices_.size()) {
            addValidationError("INVALID_HALFEDGE_TO", "Half-edge 'to' vertex out of bounds", "halfedge " + std::to_string(i));
            continue;
        }
        
        if (he.twin != INVALID_INDEX) {
            if (he.twin >= halfEdges_.size()) 
                addValidationError("INVALID_HALFEDGE_TWIN", "Half-edge twin out of bounds", "halfedge " + std::to_string(i));
            else if (halfEdges_[he.twin].twin != i)
                addValidationError("BROKEN_TWIN_LINK", "Twin half-edge doesn't point back", "halfedge " + std::to_string(i));
            else if (he.edge != INVALID_INDEX && halfEdges_[he.twin].edge != he.edge)
                addValidationError("TWIN_EDGE_MISMATCH", "Twin half-edges have different edges", "halfedge " + std::to_string(i));
        }
        
        if (he.next != INVALID_INDEX && he.next >= halfEdges_.size()) 
            addValidationError("INVALID_HALFEDGE_NEXT", "Half-edge next out of bounds", "halfedge " + std::to_string(i));
        
        if (he.prev != INVALID_INDEX && he.prev >= halfEdges_.size()) 
            addValidationError("INVALID_HALFEDGE_PREV", "Half-edge prev out of bounds", "halfedge " + std::to_string(i));
        
        if (he.edge != INVALID_INDEX && he.edge >= edges_.size())
            addValidationError("INVALID_HALFEDGE_EDGE", "Half-edge edge out of bounds", "halfedge " + std::to_string(i));
        
        if (he.face != INVALID_INDEX && he.face >= faces_.size()) {
            addValidationError("INVALID_HALFEDGE_FACE", "Half-edge face out of bounds", "halfedge " + std::to_string(i));
        }
        
        if (he.face != INVALID_INDEX) {
            if (he.next == INVALID_INDEX || he.prev == INVALID_INDEX)
                addValidationError("INCOMPLETE_FACE_LOOP", "Half-edge with face missing next/prev", "halfedge " + std::to_string(i));
        }
        
        if (he.next != INVALID_INDEX && he.next < halfEdges_.size()) {
            if (halfEdges_[he.next].prev != i)
                addValidationError("BROKEN_NEXT_LINK", "next->prev doesn't point back", "halfedge " + std::to_string(i));
        }
        
        if (he.prev != INVALID_INDEX && he.prev < halfEdges_.size()) {
            if (halfEdges_[he.prev].next != i)
                addValidationError("BROKEN_PREV_LINK", "prev->next doesn't point back", "halfedge " + std::to_string(i));
        }
    }
    
    // Check all faces
    for (size_t i = 0; i < faces_.size(); ++i) 
    {
        const Face& f = faces_[i];
        
        if (f.edge >= halfEdges_.size()) 
        {
            addValidationError("INVALID_FACE_EDGE", "Face edge out of bounds", "face " + std::to_string(i));
            continue;
        }
        
        HalfEdgeIndex start = f.edge;
        HalfEdgeIndex current = start;
        uint32_t count = 0;
        
        do 
        {
            if (current >= halfEdges_.size()) 
            {
                addValidationError("FACE_LOOP_INVALID_INDEX", "Face loop contains invalid index", "face " + std::to_string(i));
                break;
            }
            
            if (halfEdges_[current].face != i) 
                addValidationError("FACE_LOOP_WRONG_FACE", "Face loop half-edge points to wrong face", "face " + std::to_string(i));
            
            current = halfEdges_[current].next;
            count++;
            
            if (count > f.valence + 1) 
            {
                addValidationError("FACE_LOOP_TOO_LONG", "Face loop is too long", "face " + std::to_string(i));
                break;
            }
            
        } 
        while (current != start && current != INVALID_INDEX);
        
        if (count != f.valence)
            addValidationError("FACE_VALENCE_MISMATCH", 
                             "Face valence doesn't match loop length",
                             "face " + std::to_string(i) + 
                             " (expected " + std::to_string(f.valence) + 
                             ", got " + std::to_string(count) + ")");
    }
    
    // Check manifoldness
    for (size_t i = 0; i < edges_.size(); ++i) 
    {
        int heCount = 0;
        for (const HalfEdge& he : halfEdges_) 
        {
            if (he.edge == i)
                heCount++;
        }
        
        if (heCount > 2) 
            addValidationError("NON_MANIFOLD_EDGE", "Edge has more than 2 half-edges", "edge " + std::to_string(i));
        
        if (heCount == 0) 
            addValidationError("ORPHANED_EDGE", "Edge has no half-edges", "edge " + std::to_string(i));
        
    }
    
    // Check attribute array sizes
    if (vertexAttributes_.size() != vertices_.size()) 
        addValidationError("VERTEX_ATTRIB_SIZE_MISMATCH", 
                         "Vertex attributes size doesn't match vertices",
                         "vertices: " + std::to_string(vertices_.size()) + 
                         ", attribs: " + std::to_string(vertexAttributes_.size()));
    
    if (faceAttributes_.size() != faces_.size()) 
        addValidationError("FACE_ATTRIB_SIZE_MISMATCH", 
                         "Face attributes size doesn't match faces",
                         "faces: " + std::to_string(faces_.size()) + 
                         ", attribs: " + std::to_string(faceAttributes_.size()));
    
    return valid;
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
    SUBDIV_PROFILE_FUNCTION();

    for (size_t i = 0; i < faces_.size(); ++i) 
    {
        const Face& face = faces_[i];
        
        // Get three vertices of the face to compute normal
        HalfEdgeIndex he0 = face.edge;
        HalfEdgeIndex he1 = halfEdges_[he0].next;
        
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
    SUBDIV_PROFILE_FUNCTION();

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

}
