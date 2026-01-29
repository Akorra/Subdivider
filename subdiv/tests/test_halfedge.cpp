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
        
        const auto& halfEdge = mesh.halfEdge(he);
        REQUIRE(halfEdge.to == v1);
    }
    
    SECTION("Half-edge has next/prev connectivity")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        REQUIRE(he != INVALID_INDEX);
        
        const auto& halfEdge = mesh.halfEdge(he);
        
        REQUIRE(halfEdge.next != INVALID_INDEX);
        REQUIRE(halfEdge.prev != INVALID_INDEX);
        
        // Next should point to edge from v1 to v2
        const auto& nextHe = mesh.halfEdge(halfEdge.next);
        REQUIRE(nextHe.to == v2);
        
        // Prev should point from v2 to v0
        const auto& prevHe = mesh.halfEdge(halfEdge.prev);
        REQUIRE(prevHe.to == v0);
    }
    
    SECTION("Half-edge references correct face")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        const auto& halfEdge = mesh.halfEdge(he);
        
        REQUIRE(halfEdge.face != INVALID_INDEX);
        REQUIRE(halfEdge.face < mesh.numFaces());
    }
    
    SECTION("Half-edge references edge")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        const auto& halfEdge = mesh.halfEdge(he);
        
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
        
        REQUIRE(mesh.halfEdge(he01).isBoundary());
        REQUIRE(mesh.halfEdge(he12).isBoundary());
        REQUIRE(mesh.halfEdge(he20).isBoundary());
    }
    
    SECTION("Boundary half-edge has no twin")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        REQUIRE(mesh.halfEdge(he).twin == INVALID_INDEX);
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
        REQUIRE(mesh.halfEdge(he01).twin == he10);
        REQUIRE(mesh.halfEdge(he10).twin == he01);
    }
    
    SECTION("Twins point in opposite directions")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        // Check directions
        REQUIRE(mesh.halfEdge(he01).to == v1);
        REQUIRE(mesh.halfEdge(he10).to == v0);
        
        // Verify "from" vertices
        VertexIndex from01 = mesh.getFromVertex(he01);
        VertexIndex from10 = mesh.getFromVertex(he10);
        
        REQUIRE(from01 == v0);
        REQUIRE(from10 == v1);
        
        // Twin's 'to' should equal our 'from'
        REQUIRE(mesh.halfEdge(he10).to == from01);
        REQUIRE(mesh.halfEdge(he01).to == from10);
    }
    
    SECTION("Twins share the same edge")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(mesh.halfEdge(he01).edge == mesh.halfEdge(he10).edge);
    }
    
    SECTION("Twins belong to different faces")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(mesh.halfEdge(he01).face != mesh.halfEdge(he10).face);
    }
    
    SECTION("Interior edge is not boundary")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(!mesh.halfEdge(he01).isBoundary());
        REQUIRE(!mesh.halfEdge(he10).isBoundary());
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
        HalfEdgeIndex start = mesh.face(face).edge;
        HalfEdgeIndex current = start;
        
        int count = 0;
        std::vector<VertexIndex> vertices;
        
        do {
            vertices.push_back(mesh.getFromVertex(current));
            current = mesh.halfEdge(current).next;
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
        HalfEdgeIndex start = mesh.face(face).edge;
        HalfEdgeIndex current = start;
        
        int count = 0;
        do {
            current = mesh.halfEdge(current).next;
            count++;
        } while (current != start && count < 10);
        
        REQUIRE(count == 4);
    }
    
    SECTION("All half-edges in loop reference same face")
    {
        mesh.addFace({v0, v1, v2});
        
        FaceIndex faceIdx = 0;
        HalfEdgeIndex start = mesh.face(faceIdx).edge;
        HalfEdgeIndex current = start;
        
        do {
            REQUIRE(mesh.halfEdge(current).face == faceIdx);
            current = mesh.halfEdge(current).next;
        } while (current != start);
    }
}

TEST_CASE("HalfEdge - Vertex Star (One-Ring)", "[halfedge][vertex][star]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({-1.0f, 0.0f, 0.0f});
    
    SECTION("Boundary vertex star")
    {
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v0, v2, v3});
        
        // v0 is connected to v1, v2, v3
        int valence = mesh.getVertexValence(v0);
        REQUIRE(valence == 3);
        
        // v0 should be a boundary vertex (edges v0->v1 and v0->v3 are boundary)
        REQUIRE(mesh.isBoundaryVertex(v0));
    }
    
    SECTION("Interior vertex star")
    {
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v0, v2, v3});
        mesh.addFace({v0, v3, v1}); // Close the fan
        
        // v0 is connected to v1, v2, v3
        int valence = mesh.getVertexValence(v0);
        REQUIRE(valence == 3);
        
        // v0 should be interior (all edges have twins)
        REQUIRE(!mesh.isBoundaryVertex(v0));
    }
    
    SECTION("Collect outgoing edges")
    {
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v0, v2, v3});
        
        // Manual collection of outgoing half-edges
        std::vector<VertexIndex> neighbors;
        HalfEdgeIndex start = mesh.vertex(v0).outgoing;
        
        // Collect via twin->next traversal
        if (start != INVALID_INDEX) {
            HalfEdgeIndex current = start;
            
            // Clockwise direction
            do {
                neighbors.push_back(mesh.halfEdge(current).to);
                
                HalfEdgeIndex twin = mesh.halfEdge(current).twin;
                if (twin == INVALID_INDEX) {
                    // Hit boundary, walk counter-clockwise
                    current = start;
                    while (true) {
                        HalfEdgeIndex prev = mesh.halfEdge(current).prev;
                        if (prev == INVALID_INDEX) break;
                        
                        HalfEdgeIndex prevTwin = mesh.halfEdge(prev).twin;
                        if (prevTwin == INVALID_INDEX) {
                            // Hit other boundary
                            break;
                        }
                        
                        neighbors.push_back(mesh.halfEdge(prevTwin).to);
                        current = prevTwin;
                    }
                    break;
                }
                
                current = mesh.halfEdge(twin).next;
                
            } while (current != start);
        }
        
        // Should have collected v1, v2, v3 (order may vary)
        REQUIRE(neighbors.size() == 3);
        REQUIRE(std::find(neighbors.begin(), neighbors.end(), v1) != neighbors.end());
        REQUIRE(std::find(neighbors.begin(), neighbors.end(), v2) != neighbors.end());
        REQUIRE(std::find(neighbors.begin(), neighbors.end(), v3) != neighbors.end());
    }
}

TEST_CASE("HalfEdge - Edge Properties", "[halfedge][edge]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
    SECTION("Boundary edge has one half-edge")
    {
        mesh.addFace({v0, v1, v2});
        
        EdgeIndex edge = mesh.findEdge(v0, v1);
        REQUIRE(edge != INVALID_INDEX);
        
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        REQUIRE(mesh.halfEdge(he).edge == edge);
        
        // No twin exists for boundary
        REQUIRE(mesh.halfEdge(he).twin == INVALID_INDEX);
    }
    
    SECTION("Interior edge has two half-edges")
    {
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v1, v3, v2});
        
        EdgeIndex edge = mesh.findEdge(v1, v2);
        REQUIRE(edge != INVALID_INDEX);
        
        HalfEdgeIndex he12 = mesh.findHalfEdge(v1, v2);
        HalfEdgeIndex he21 = mesh.findHalfEdge(v2, v1);
        
        // Both half-edges reference the same edge
        REQUIRE(mesh.halfEdge(he12).edge == edge);
        REQUIRE(mesh.halfEdge(he21).edge == edge);
        
        // They are twins
        REQUIRE(mesh.halfEdge(he12).twin == he21);
        REQUIRE(mesh.halfEdge(he21).twin == he12);
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
        REQUIRE(mesh.halfEdge(he01).to == v1);
    }
    
    SECTION("getFromVertex via prev->to")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v1, v2);
        REQUIRE(he != INVALID_INDEX);
        
        VertexIndex from = mesh.getFromVertex(he);
        REQUIRE(from == v1);
    }
}

TEST_CASE("HalfEdge - Map Storage", "[halfedge][map]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
    SECTION("Map stores first half-edge of each edge")
    {
        mesh.addFace({v0, v1, v2});
        
        // v0->v1 should be stored in map
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        REQUIRE(he01 != INVALID_INDEX);
        REQUIRE(mesh.halfEdge(he01).to == v1);
    }
    
    SECTION("Twin is found via map lookup")
    {
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v1, v0, v3});
        
        // Both directions should be findable
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(he01 != INVALID_INDEX);
        REQUIRE(he10 != INVALID_INDEX);
        
        // One is stored directly, the other via twin
        // Both should be found correctly
        REQUIRE(mesh.halfEdge(he01).to == v1);
        REQUIRE(mesh.halfEdge(he10).to == v0);
    }
    
    SECTION("Boundary half-edge only in one direction")
    {
        mesh.addFace({v0, v1, v2});
        
        // v0->v1 exists
        REQUIRE(mesh.findHalfEdge(v0, v1) != INVALID_INDEX);
        
        // v1->v0 does not exist (no face created it)
        REQUIRE(mesh.findHalfEdge(v1, v0) == INVALID_INDEX);
    }
}

TEST_CASE("HalfEdge - Complex Topology", "[halfedge][complex]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    SECTION("Triangle strip")
    {
        auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
        auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
        auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
        auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
        auto v4 = mesh.addVertex({2.0f, 0.0f, 0.0f});
        
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v1, v3, v2});
        mesh.addFace({v1, v4, v3});
        
        // All interior edges should have twins
        HalfEdgeIndex he12 = mesh.findHalfEdge(v1, v2);
        HalfEdgeIndex he21 = mesh.findHalfEdge(v2, v1);
        REQUIRE(mesh.halfEdge(he12).twin == he21);
        
        HalfEdgeIndex he13 = mesh.findHalfEdge(v1, v3);
        HalfEdgeIndex he31 = mesh.findHalfEdge(v3, v1);
        REQUIRE(mesh.halfEdge(he13).twin == he31);
        
        REQUIRE(!Context::hasErrors());
    }
    
    SECTION("Vertex with multiple incident faces")
    {
        // Create a fan of triangles around v0
        auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
        auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
        auto v2 = mesh.addVertex({0.7f, 0.7f, 0.0f});
        auto v3 = mesh.addVertex({0.0f, 1.0f, 0.0f});
        auto v4 = mesh.addVertex({-0.7f, 0.7f, 0.0f});
        
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v0, v2, v3});
        mesh.addFace({v0, v3, v4});
        
        // v0 should have high valence
        int valence = mesh.getVertexValence(v0);
        REQUIRE(valence >= 3);
        
        REQUIRE(!Context::hasErrors());
    }
}

TEST_CASE("HalfEdge - Iterator Pattern", "[halfedge][iteration]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2, v3});
    
    SECTION("Iterate around face")
    {
        FaceIndex face = 0;
        HalfEdgeIndex start = mesh.face(face).edge;
        HalfEdgeIndex current = start;
        
        std::vector<VertexIndex> vertices;
        int count = 0;
        
        do {
            vertices.push_back(mesh.getFromVertex(current));
            current = mesh.halfEdge(current).next;
            count++;
        } while (current != start && count < 100);
        
        REQUIRE(count == 4);
        REQUIRE(vertices == std::vector<VertexIndex>{v0, v1, v2, v3});
    }
    
    SECTION("Iterate using prev")
    {
        FaceIndex face = 0;
        HalfEdgeIndex start = mesh.face(face).edge;
        HalfEdgeIndex current = start;
        
        std::vector<VertexIndex> vertices;
        int count = 0;
        
        do {
            vertices.push_back(mesh.halfEdge(current).to);
            current = mesh.halfEdge(current).prev;
            count++;
        } while (current != start && count < 100);
        
        REQUIRE(count == 4);
        // Reverse order when using prev
        REQUIRE(vertices == std::vector<VertexIndex>{v1, v0, v3, v2});
    }
}

TEST_CASE("HalfEdge - Consistency", "[halfedge][validation]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    mesh.addFace({v1, v3, v2});
    
    SECTION("next->prev points back")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex next = mesh.halfEdge(he).next;
        
        REQUIRE(mesh.halfEdge(next).prev == he);
    }
    
    SECTION("prev->next points back")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex prev = mesh.halfEdge(he).prev;
        
        REQUIRE(mesh.halfEdge(prev).next == he);
    }
    
    SECTION("twin->twin points back")
    {
        HalfEdgeIndex he12 = mesh.findHalfEdge(v1, v2);
        HalfEdgeIndex twin = mesh.halfEdge(he12).twin;
        
        REQUIRE(twin != INVALID_INDEX);
        REQUIRE(mesh.halfEdge(twin).twin == he12);
    }
    
    SECTION("Mesh validates successfully")
    {
        REQUIRE(mesh.validate());
        REQUIRE(!Context::hasErrors());
    }
}