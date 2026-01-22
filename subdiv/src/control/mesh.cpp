#include "control/mesh.hpp"

namespace Subdiv::Control 
{

Mesh::~Mesh()
{
    clear();
}

void Mesh::clear() 
{
    for(auto* v : vertices)     delete v;
    for(auto* e : halfEdges)    delete e;
    for(auto* f : faces)        delete f;

    vertices.clear();
    halfEdges.clear();
    faces.clear();
    edgeMap.clear();
}

/** 
 * Vertex creation
*/
Vertex* Mesh::addVertex(const glm::vec3& pos)
{
    Vertex* v = new Vertex(pos);
    vertices.push_back(v);
    return v;
}

Vertex* Mesh::addVertex(const float x, const float y, const float z)
{
    return addVertex({x, y, z});
}

/** 
 * Face creation (ngons supported)
*/
Face* Mesh::addFace(const std::vector<uint32_t>& indices) 
{
    assert(indices.size() > 2);

    Face* f = new Face();
    faces.push_back(f);
    
    const size_t n = indices.size();
    std::vector<HalfEdge*> localEdges(n);

    // Create half edges:
    for(size_t i = 0; i < n; ++i)
    {
        HalfEdge* he = new HalfEdge();
        he->face     = f;
        halfEdges.push_back(he);
        localEdges[i] = he;
    }

    // Wire next/prev/to
    for (size_t i = 0; i < n; ++i) {
        size_t curr = i;
        size_t next = (i + 1) % n;
        size_t prev = (i + n - 1) % n;

        HalfEdge* he = localEdges[curr];

        he->to   = vertices[indices[next]];
        he->next = localEdges[next];
        he->prev = localEdges[prev];

        // Assign outgoing edge if not set
        Vertex* from = vertices[indices[curr]];
        if (!from->outgoing)
            from->outgoing = he;

        // Store edge for twin resolution
        edgeMap[{ indices[curr], indices[next] }] = he;
    }

    face->edge = localEdges[0];
    return face;
}

/** 
 * Connectivity build (twin resolution)
*/
bool Mesh::buildConnectivity() {
    for (auto& [key, he] : edgeMap) {
        const uint32_t from = key.first;
        const uint32_t to   = key.second;

        auto it = edgeMap.find({to, from});
        if (it != edgeMap.end()) {
            HalfEdge* twin = it->second;

            // Non-manifold: more than one twin
            if (he->twin && he->twin != twin)
                return false;

            he->twin = twin;
            twin->twin = he;
        }
    }

    return validate();
}

/** 
 * Validation
*/
bool Mesh::validate() const {
    // Check half-edge invariants
    for (HalfEdge* he : halfEdges) {
        if (!he->to || !he->next || !he->prev || !he->face)
            return false;

        if (he->next->prev != he)
            return false;

        if (he->prev->next != he)
            return false;

        if (he->twin && he->twin->twin != he)
            return false;
    }

    // Check face loops
    for (Face* f : faces) {
        if (!f->edge || !f->isValidLoop())
            return false;
    }

    return true;
}

}