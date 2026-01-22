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
}

/** 
 * Vertex creation
*/
Vertex* Mesh::addVertex(const glm::vec3& pos)
{
    Vertex* v = new Vertex(pos);
    vertices.push_back(v);
    indicesDirty = true;
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
    assert(indices.size() > 2 && "Faces must have at least 3 vertices");

    Face* f = new Face();
    faces.push_back(f);
    
    const size_t n = indices.size();
    std::vector<HalfEdge*> loop(n);

    // Create half edges:
    for(size_t i = 0; i < n; ++i)
    {
        loop[i] = new HalfEdge();
        halfEdges.push_back(loop[i]);
    }

    // Wire next/prev/to
    for (size_t i = 0; i < n; ++i) 
    {
        HalfEdge* he   = loop[i];

        he->to   = vertices[indices[(i+1)%n]];
        he->next = loop[(i+1)%n];
        he->prev = loop[(i+n-1)%n];
        he->face = f;

        // Assign outgoing edge if not set
        Vertex* from = vertices[indices[i]];
        if (!from->outgoing)
            from->outgoing = he;
    }

    f->edge = loop[0];
    indicesDirty = true;

    return f;
}

/**
 * Index Cache 
 */
void Mesh::rebuildIndexCache() 
{
    vertexToIndex.clear();
    vertexToIndex.reserve(vertices.size());

    indexToVertex = vertices;

    for (uint32_t i = 0; i < vertices.size(); ++i)
        vertexToIndex[vertices[i]] = i;

    indicesDirty = false;
}

/** 
 * Connectivity build (twin resolution) (O(E))
*/
bool Mesh::buildConnectivity() 
{
    if(indicesDirty)
        rebuildIndexCache();

    // temporary half edge map helper
    std::unordered_map<uint64_t, HalfEdge*> edgeMap;
    edgeMap.reserve(halfEdges.size());

    for (HalfEdge* he : halfEdges) 
    {
        // get directed half edge vertex indices
        const uint32_t from = vertexToIndex[he->from()];
        const uint32_t to   = vertexToIndex[he->to];

        // calculate halfEdge keys for edgeMap
        uint64_t key     = makeEdgeKey(from, to);
        uint64_t twinKey = makeEdgeKey(to, from);
        
        // search for twin in edgeMap
        auto it = edgeMap.find(twinKey);
        if(it != edgeMap.end())
        {
            // if twin is in edge map -> set twin data, and remove twin from map since we found its pair
            he->twin         = it->second;
            it->second->twin = he;
            edgeMap.erase(it);
        }
        else
        {
            //first time seeing this edge?
            if(edgeMap.contains(key))
                return false; // non-manifold
            // otherwise place key in map so it's twin can find it
            edgeMap.emplace(key, he);
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