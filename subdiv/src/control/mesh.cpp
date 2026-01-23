#include "control/mesh.hpp"

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
    edgeLookup.clear();
    groups.clear();

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

    edgeLookup.clear();
    edgeLookup.reserve(halfEdges.size());

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

        if (edgeLookup.contains(key))
            return false; // duplicate directed edge
        edgeLookup.emplace(key, he);
        
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

    struct CreaseCmd { uint32_t a, b; float sharp; };
    std::vector<CreaseCmd> creases;
    std::vector<uint32_t> faceIndices;
    std::string line;
    FaceGroup* currentGroup = nullptr;

    // handle indices
    auto resolveIndex = [&](int idx) -> uint32_t {
        if (idx > 0) return uint32_t(idx - 1);  // OBJ is 1-based
        return uint32_t(vertices.size() + idx); // negative index
    };

    while(std::getline(in, line))
    {
        if (line.empty() || line[0] == '#')
        {
            // ---- Crease import via comment ----
            // # crease v0 v1 sharpness - store after connectivity
            if (line.rfind("# crease", 0) == 0)
            {
                std::stringstream cs(line);
                std::string _;
                uint32_t a, b;
                float sharp;

                cs >> _ >> _ >> a >> b >> sharp;

                creases.push_back({resolveIndex(a), resolveIndex(b), sharp});
            }

            continue;
        }
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
                uint32_t idx = std::stoi(vert.substr(0, slash));
                faceIndices.push_back(resolveIndex(idx));
            }
            
            if(faceIndices.size() > 2)
            {
                Face* f = addFace(faceIndices);
                if(currentGroup)
                    currentGroup->faces.push_back(f);
            }
        }
        else if(tag == "g" || tag == "o")
        {
            std::string name;
            ss >> name;
            groups.push_back({name});
            currentGroup = &groups.back();
        }
    }

    if(!buildConnectivity())
        return false;

    for(const auto& crease : creases)
        applyCrease(crease.a, crease.b, crease.sharp);                  

    return true;
}

void Mesh::applyCrease(uint32_t a, uint32_t b, float sharpness)
{
    if (a >= vertices.size() || b >= vertices.size())
        return;

    uint64_t key     = makeEdgeKey(a, b);
    uint64_t twinKey = makeEdgeKey(b, a);

    auto it = edgeLookup.find(key);
    if (it == edgeLookup.end())
        return;

    HalfEdge* he = it->second;
    he->tag = EdgeTag::EDGE_SEMI;
    he->sharpness = sharpness;

    auto jt = edgeLookup.find(twinKey);
    if (jt != edgeLookup.end()) {
        jt->second->tag = EdgeTag::EDGE_SEMI;
        jt->second->sharpness = sharpness;
    }
}

HalfEdge* Mesh::getHalfEdge(uint32_t from, uint32_t to)
{
    auto it = edgeLookup.find(makeEdgeKey(from, to));
    if(it == edgeLookup.end())
        return nullptr;
    return it->second;
}

}