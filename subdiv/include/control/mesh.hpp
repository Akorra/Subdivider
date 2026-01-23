#pragma once

#include "utils.hpp"

#include <unordered_map>

namespace Subdiv::Control 
{

class Mesh 
{

public:
    Mesh() = default;
    ~Mesh();

    // for now lets not allow for copy
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;

    //! Add vertices to Mesh from positions
    Vertex* addVertex(const glm::vec3& pos);
    Vertex* addVertex(const float x, const float y, const float z);

    //! clean out mesh components
    void clear();

    //! Add a face from vertex indices - supports ngons (n>2 ofcourse)
    Face* addFace(const std::vector<uint32_t>& vertexIndices);

    //! Resolve half edge structure (twins, boundaries, validate manifoldness)
    bool buildConnectivity();

    //! validate mesh
    bool validate() const;

    //! import from .obj file
    bool loadOBJ(const std::string&  filepath, bool flipYZ=false, bool clearMesh=true);
    bool loadOBJ(      std::istream& in,       bool flipYZ=false, bool clearMesh=true);

    //! apply crease to edge
    void applyCrease(uint32_t a, uint32_t b, float sharpness);

    //! get half-edge from vertex indices
    HalfEdge* getHalfEdge(uint32_t from, uint32_t to);

private:
    //! rebuild index cache - call on topology cahnge
    void rebuildIndexCache();

    //! halfedge map helper -> generates flat key from vertex index pair
    static inline uint64_t makeEdgeKey(uint32_t from, uint32_t to) 
    {
        return (uint64_t(from) << 32) | uint64_t(to);
    }

public:
    // storage (owning)
    std::vector<Vertex*>    vertices;
    std::vector<HalfEdge*>  halfEdges;
    std::vector<Face*>      faces;

private:
    // Index cache (rebuilt only when topology changes)
    std::unordered_map<const Vertex*, uint32_t> vertexToIndex;
    bool indicesDirty = true;

    std::vector<FaceGroup> groups;
    std::unordered_map<uint64_t, HalfEdge*> edgeLookup;
};

} // namespace Subdiv::Control