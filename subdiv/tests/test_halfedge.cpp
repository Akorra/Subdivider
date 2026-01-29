#include <catch2/catch_test_macros.hpp>
#include "subdiv/include/control/mesh.hpp"

using namespace Subdiv::Control;

TEST_CASE("HalfEdge - Basic Properties", "[halfedge]")
{
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    SECTION("Half-edge connectivity")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        REQUIRE(he != INVALID_INDEX);
        
        const auto& halfEdge = mesh.halfEdge(he);
        
        // Check to vertex
        REQUIRE(halfEdge.to == v1);
        
        // Check next/prev connectivity
        REQUIRE(halfEdge.next != INVALID_INDEX);
        REQUIRE(halfEdge.prev != INVALID_INDEX);
        
        // Next should point to edge from v1 to v2
        const auto& nextHe = mesh.halfEdge(halfEdge.next);
        REQUIRE(nextHe.to == v2);
        
        // Prev should point from v2 to v0
        const auto& prevHe = mesh.halfEdge(halfEdge.prev);
        REQUIRE(prevHe.to == v0);
    }
    
    SECTION("Twin half-edges")
    {
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(he01 != INVALID_INDEX);
        REQUIRE(he10 != INVALID_INDEX);
        
        // Twins should reference each other
        REQUIRE(mesh.halfEdge(he01).twin == he10);
        REQUIRE(mesh.halfEdge(he10).twin == he01);
        
        // Boundary edges have no twin
        REQUIRE(mesh.halfEdge(he01).isBoundary());
        REQUIRE(mesh.halfEdge(he10).isBoundary());
    }
    
    SECTION("Face reference")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        const auto& halfEdge = mesh.halfEdge(he);
        
        REQUIRE(halfEdge.face != INVALID_INDEX);
        REQUIRE(halfEdge.face < mesh.numFaces());
    }
}

TEST_CASE("HalfEdge - Closed Mesh", "[halfedge]")
{
    Mesh mesh;
    
    // Create a closed quad (two triangles)
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    mesh.addFace({v0, v2, v3});
    
    SECTION("Interior edge has twins on both sides")
    {
        HalfEdgeIndex he02 = mesh.findHalfEdge(v0, v2);
        HalfEdgeIndex he20 = mesh.findHalfEdge(v2, v0);
        
        REQUIRE(he02 != INVALID_INDEX);
        REQUIRE(he20 != INVALID_INDEX);
        
        // These should be twins
        REQUIRE(mesh.halfEdge(he02).twin == he20);
        REQUIRE(mesh.halfEdge(he20).twin == he02);
        
        // Both should have faces
        REQUIRE(mesh.halfEdge(he02).face != INVALID_INDEX);
        REQUIRE(mesh.halfEdge(he20).face != INVALID_INDEX);
        
        // Interior edge is not boundary
        REQUIRE(!mesh.halfEdge(he02).isBoundary());
        REQUIRE(!mesh.halfEdge(he20).isBoundary());
    }
}