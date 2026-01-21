#pragma once

#include <vector>

struct Vertex;
struct HalfEdge;
struct Face;

namespace subdiv 
{
class BaseMesh {
public:
    BaseMesh();  // constructor
    ~BaseMesh(); // destructor


private:
    std::vector<Vertex*>    vertices;
    std::vector<HalfEdge*>  halfedges;
    std::vector<Face*>      faces;
};

} // namespace subdiv