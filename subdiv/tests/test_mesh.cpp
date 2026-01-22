#include <catch2/catch_test_macros.hpp>
#include "control/mesh.hpp"

using namespace Subdiv::Control;

TEST_CASE("Mesh: vertex creation", "[Mesh]")
{
    Mesh mesh;

    Vertex* v0 = mesh.addVertex({0,0,0});
    Vertex* v1 = mesh.addVertex({1,0,0});
    Vertex* v2 = mesh.addVertex({0,1,0});

    REQUIRE(mesh.vertices.size() == 3);
    REQUIRE(v0->position == glm::vec3(0,0,0));
    REQUIRE(v1->position == glm::vec3(1,0,0));
    REQUIRE(v2->position == glm::vec3(0,1,0));
}

TEST_CASE("Mesh: single triangle face", "[Mesh]")
{
    Mesh mesh;

    Vertex* v0 = mesh.addVertex({0,0,0});
    Vertex* v1 = mesh.addVertex({1,0,0});
    Vertex* v2 = mesh.addVertex({0,1,0});

    // add face
    Face* f = mesh.addFace({0, 1, 2});
    REQUIRE(mesh.faces.size() == 1);
    REQUIRE(mesh.faces[0]->valence() == 3);

    // build connectivity
    REQUIRE(mesh.buildConnectivity());

    // validate half-edge twin pointers
    for(HalfEdge* he : mesh.halfEdges)
    {
        if(he->isBoundary())
        {
            REQUIRE(he->twin == nullptr);
        }
        else
        {
            REQUIRE(he->twin != nullptr);
            REQUIRE(he->twin->twin == he);
        }
    }
}

TEST_CASE("Mesh: quad face with twin edges", "[Mesh]") 
{
    Mesh mesh;

    Vertex* v0 = mesh.addVertex({0,0,0});
    Vertex* v1 = mesh.addVertex({1,0,0});
    Vertex* v2 = mesh.addVertex({1,1,0});
    Vertex* v3 = mesh.addVertex({0,1,0});

    // add two faces sharing one edge
    Face* f0 = mesh.addFace({0, 1, 2, 3});
    Face* f1 = mesh.addFace({2, 1, 0}); // shares edge 1->2 and 0->1

    REQUIRE(mesh.buildConnectivity());

    // twin verification
    for(HalfEdge* he : mesh.halfEdges) 
    {
        if(he->isBoundary()) 
        {
            REQUIRE(he->twin == nullptr);
        }
        else
        {
            REQUIRE(he->twin != nullptr);
            REQUIRE(he->twin->twin == he);
        }
    }
}

TEST_CASE("Mesh: non-manifold detection", "[Mesh]") {
    Mesh mesh;

    Vertex* v0 = mesh.addVertex({0,0,0});
    Vertex* v1 = mesh.addVertex({1,0,0});
    Vertex* v2 = mesh.addVertex({0,1,0});
    Vertex* v3 = mesh.addVertex({1,1,0});

    // add overlapping faces on the same edge
    mesh.addFace({0,1,2});
    mesh.addFace({0,1,3}); // shares 0->1 edge

    REQUIRE(mesh.buildConnectivity() == false); // should detect non-manifold
}

TEST_CASE("OBJ: single triangle loads correctly") {
    Mesh mesh;

    std::stringstream obj(R"(
        v 0 0 0
        v 1 0 0
        v 0 1 0
        f 1 2 3
    )");

    REQUIRE(mesh.loadOBJ(obj));
    REQUIRE(mesh.validate());
    REQUIRE(mesh.vertices.size() == 3);
    REQUIRE(mesh.faces.size() == 1);
    REQUIRE(mesh.halfEdges.size() == 3);
}

TEST_CASE("OBJ: quad face supported") {
    Mesh mesh;

    std::stringstream obj(R"(
        v 0 0 0
        v 1 0 0
        v 1 1 0
        v 0 1 0
        f 1 2 3 4
    )");

    REQUIRE(mesh.loadOBJ(obj));
    REQUIRE(mesh.validate());
    REQUIRE(mesh.faces[0]->valence() == 4);
}

TEST_CASE("OBJ: shared edge creates twins") {
    Mesh mesh;

    std::stringstream obj(R"(
        v 0 0 0
        v 1 0 0
        v 0 1 0
        v 1 1 0
        f 1 2 3
        f 2 4 3
    )");

    REQUIRE(mesh.loadOBJ(obj));
    REQUIRE(mesh.validate());

    int twinCount = 0;
    for (HalfEdge* he : mesh.halfEdges) {
        if (he->twin)
            ++twinCount;
    }

    REQUIRE(twinCount > 0);
}

TEST_CASE("OBJ: non-manifold edge fails") {
    Mesh mesh;

    std::stringstream obj(R"(
        v 0 0 0
        v 1 0 0
        v 0 1 0
        v 0 -1 0
        f 1 2 3
        f 1 2 4
    )");

    REQUIRE_FALSE(mesh.loadOBJ(obj));
}