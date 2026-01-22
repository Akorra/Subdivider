#include "control/mesh.hpp"

#include <string>
#include <iostream>
#include <istream>
#include <fstream>
#include <sstream>

namespace Subdiv::Control 
{

Mesh::~Mesh()
{
    clear();
}

void Mesh::clear() 
{
    // Delete half-edges
    for (auto* he : halfEdges) delete he;
    halfEdges.clear();

    // Delete faces
    for (auto* f : faces) delete f;
    faces.clear();

    // Delete vertices
    for (auto* v : vertices) delete v;
    vertices.clear();

    // Clear caches
    vertexToIndex.clear();
    indicesDirty = true;
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
        HalfEdge* he = loop[i];
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

/** 
 * OBJ Import - for now we only import vertex and face data
*/
bool Mesh::loadOBJ(const std::string& filepath, bool flipYZ, bool clearMesh)
{
    std::ifstream file(filepath);
    if(!file.is_open())
    {
        std::cerr << "Cannot open OBJ file: " << filepath << "\n";
        return false;
    }

    return loadOBJ(file, flipYZ, clearMesh);
}

bool Mesh::loadOBJ(std::istream& in, bool flipYZ, bool clearMesh)
{
    if(clearMesh) clear();

    std::vector<uint32_t> faceIndices;
    std::string line;

    while(std::getline(in, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        std::string tag;
        ss >> tag;

        if(tag == "v") //< vertex 
        {
            float x, y, z;
            ss >> x >> y >> z;
            if(flipYZ) 
                std::swap(y,z);
            addVertex(x, y, z);
        }
        else if(tag == "f")
        {
            faceIndices.clear();
            std::string vert;
            while(ss >> vert)
            {
                size_t slash = vert.find('/');
                uint32_t idx = std::stoi(vert.substr(0, slash)) - 1;
                faceIndices.push_back(idx);
            }
            
            if(!faceIndices.empty())
                addFace(faceIndices);
        }
    }

    return buildConnectivity();
}

}