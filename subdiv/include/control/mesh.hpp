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

    /**
     * Add vertices to Mesh from positions
     */
    Vertex* addVertex(const glm::vec3& pos);
    Vertex* addVertex(const float x, const float y, const float y);

    /**
     * Add a face from vertex indices
     * supports ngons (n>2 ofcourse)
     */
    Face* addFace(const std::vector<uint32_t>& indices);

    /**
     * Resolve half edge structure (twins, boundaries, validate manifoldness)
     */
    bool buildConnectivity();

    /**
     * validate mesh
     */
    bool validate() const;

    /**
     * clean out mesh components
     */
    void clear();


public:
    // storage (owning)
    std::vector<Vertex*>    vertices;
    std::vector<HalfEdge*>  halfEdges;
    std::vector<Face*>      faces;

private:
    // Helper for twin resolution (add readibility)
    using EdgeKey = std::pair<uint32_t, uint32_t>;

    struct EdgeKeyHash 
    {
        size_t operator()(const EdgeKey& k) const 
        {
            return (size_t(k.first) << 32) ^ size_t(k.second);
        }
    };

    std::unordered_map<EdgeKey, HalfEdge*, EdgeKeyHash> edgeMap;

};

} // namespace Subdiv::Control