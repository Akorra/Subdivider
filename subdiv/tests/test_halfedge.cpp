#include <catch2/catch_test_macros.hpp>
#include "control/mesh.hpp"
#include "diagnostics/context.hpp"

using namespace Subdiv::Control;
using namespace Subdiv::Diagnostics;

// RAII helper for tests
class DiagnosticTestScope {
public:
    DiagnosticTestScope() {
        Context::enable(Context::Mode::ERRORS_ONLY);
        Context::clear();
    }
    ~DiagnosticTestScope() {
        Context::disable();
    }
};

TEST_CASE("HalfEdge - Basic Properties", "[halfedge]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    SECTION("Half-edge has correct destination")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        REQUIRE(he != INVALID_INDEX);
        
        const auto& halfEdge = mesh.halfEdges[he];
        REQUIRE(halfEdge.to == v1);
    }
    
    SECTION("Half-edge has next/prev connectivity")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        REQUIRE(he != INVALID_INDEX);
        
        const auto& halfEdge = mesh.halfEdges[he];
        
        REQUIRE(halfEdge.next != INVALID_INDEX);
        REQUIRE(halfEdge.prev != INVALID_INDEX);
        
        // Next should point to edge from v1 to v2
        const auto& nextHe = mesh.halfEdges[halfEdge.next];
        REQUIRE(nextHe.to == v2);
        
        // Prev should point from v2 to v0
        const auto& prevHe = mesh.halfEdges[halfEdge.prev];
        REQUIRE(prevHe.to == v0);
    }
    
    SECTION("Half-edge references correct face")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        const auto& halfEdge = mesh.halfEdges[he];
        
        REQUIRE(halfEdge.face != INVALID_INDEX);
        REQUIRE(halfEdge.face < mesh.numFaces());
    }
    
    SECTION("Half-edge references edge")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        const auto& halfEdge = mesh.halfEdges[he];
        
        REQUIRE(halfEdge.edge != INVALID_INDEX);
        REQUIRE(halfEdge.edge < mesh.numEdges());
    }
}

TEST_CASE("HalfEdge - Boundary Half-Edges", "[halfedge][boundary]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    SECTION("All half-edges in single face are boundary")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he12 = mesh.findHalfEdge(v1, v2);
        HalfEdgeIndex he20 = mesh.findHalfEdge(v2, v0);
        
        REQUIRE(mesh.halfEdges[he01].twin == INVALID_INDEX);
        REQUIRE(mesh.halfEdges[he12].twin == INVALID_INDEX);
        REQUIRE(mesh.halfEdges[he20].twin == INVALID_INDEX);
    }
    
    SECTION("Boundary half-edge has no twin")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        REQUIRE(mesh.halfEdges[he].twin == INVALID_INDEX);
    }
}

TEST_CASE("HalfEdge - Twin Half-Edges", "[halfedge][twins]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
    // Create two triangles sharing edge v0-v1
    mesh.addFace({v0, v1, v2});
    mesh.addFace({v1, v0, v3});
    
    SECTION("Twins exist and reference each other")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(he01 != INVALID_INDEX);
        REQUIRE(he10 != INVALID_INDEX);
        
        // They should be twins
        REQUIRE(mesh.halfEdges[he01].twin == he10);
        REQUIRE(mesh.halfEdges[he10].twin == he01);
    }
    
    SECTION("Twins point in opposite directions")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        // Check directions
        REQUIRE(mesh.halfEdges[he01].to == v1);
        REQUIRE(mesh.halfEdges[he10].to == v0);
        
        // Verify "from" vertices
        VertexIndex from01 = mesh.getFromVertex(he01);
        VertexIndex from10 = mesh.getFromVertex(he10);
        
        REQUIRE(from01 == v0);
        REQUIRE(from10 == v1);
        
        // Twin's 'to' should equal our 'from'
        REQUIRE(mesh.halfEdges[he10].to == from01);
        REQUIRE(mesh.halfEdges[he01].to == from10);
    }
    
    SECTION("Twins share the same edge")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(mesh.halfEdges[he01].edge == mesh.halfEdges[he10].edge);
    }
    
    SECTION("Twins belong to different faces")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(mesh.halfEdges[he01].face != mesh.halfEdges[he10].face);
    }
    
    SECTION("Interior edge is not boundary")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(mesh.halfEdges[he01].twin != INVALID_INDEX);
        REQUIRE(mesh.halfEdges[he10].twin != INVALID_INDEX);
    }
}

TEST_CASE("HalfEdge - Face Loop", "[halfedge][face][loop]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    SECTION("Triangle face loop")
    {
        mesh.addFace({v0, v1, v2});
        
        FaceIndex face = 0;
        HalfEdgeIndex start = mesh.faces[face].edge;
        HalfEdgeIndex current = start;
        
        int count = 0;
        std::vector<VertexIndex> vertices;
        
        do {
            vertices.push_back(mesh.getFromVertex(current));
            current = mesh.halfEdges[current].next;
            count++;
        } while (current != start && count < 10);
        
        REQUIRE(count == 3);
        REQUIRE(vertices.size() == 3);
        
        // Vertices should be v0, v1, v2 in order
        REQUIRE(vertices[0] == v0);
        REQUIRE(vertices[1] == v1);
        REQUIRE(vertices[2] == v2);
    }
    
    SECTION("Quad face loop")
    {
        mesh.addFace({v0, v1, v2, v3});
        
        FaceIndex face = 0;
        HalfEdgeIndex start = mesh.faces[face].edge;
        HalfEdgeIndex current = start;
        
        int count = 0;
        do {
            current = mesh.halfEdges[current].next;
            count++;
        } while (current != start && count < 10);
        
        REQUIRE(count == 4);
    }
    
    SECTION("All half-edges in loop reference same face")
    {
        mesh.addFace({v0, v1, v2});
        
        FaceIndex faceIdx = 0;
        HalfEdgeIndex start = mesh.faces[faceIdx].edge;
        HalfEdgeIndex current = start;
        
        do {
            REQUIRE(mesh.halfEdges[current].face == faceIdx);
            current = mesh.halfEdges[current].next;
        } while (current != start);
    }
}

TEST_CASE("HalfEdge - Next/Prev Connectivity", "[halfedge][connectivity]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    SECTION("next->prev points back")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex next = mesh.halfEdges[he].next;
        
        REQUIRE(mesh.halfEdges[next].prev == he);
    }
    
    SECTION("prev->next points back")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex prev = mesh.halfEdges[he].prev;
        
        REQUIRE(mesh.halfEdges[prev].next == he);
    }
    
    SECTION("twin->twin points back")
    {
        auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
        mesh.addFace({v1, v0, v3});
        
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex twin = mesh.halfEdges[he01].twin;
        
        REQUIRE(twin != INVALID_INDEX);
        REQUIRE(mesh.halfEdges[twin].twin == he01);
    }
}

TEST_CASE("HalfEdge - getFromVertex", "[halfedge][query]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    SECTION("getFromVertex returns correct source")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        REQUIRE(he01 != INVALID_INDEX);
        
        VertexIndex from = mesh.getFromVertex(he01);
        REQUIRE(from == v0);
        
        // Verify it forms correct edge
        REQUIRE(mesh.halfEdges[he01].to == v1);
    }
    
    SECTION("getFromVertex via prev->to")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v1, v2);
        REQUIRE(he != INVALID_INDEX);
        
        VertexIndex from = mesh.getFromVertex(he);
        REQUIRE(from == v1);
    }
}

TEST_CASE("HalfEdge - Validation", "[halfedge][validation]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    mesh.addFace({v1, v3, v2});
    
    REQUIRE(mesh.validate());
    REQUIRE(!Context::hasErrors());
}