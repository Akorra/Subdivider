#include <catch2/catch_test_macros.hpp>
#include "control/mesh.hpp"
#include "control/mesh_cache.hpp"
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

TEST_CASE("TopologyCache - Lazy Building", "[cache]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    SECTION("Cache not built initially")
    {
        REQUIRE_FALSE(mesh.cache.isValid());
    }
    
    SECTION("Cache built on first query")
    {
        uint16_t valence = mesh.getValence(v0);
        REQUIRE(mesh.cache.isValid());
        REQUIRE(valence == 2);
    }
}

TEST_CASE("TopologyCache - Invalidation", "[cache]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    mesh.buildCache();
    
    REQUIRE(mesh.cache.isValid());
    
    SECTION("Adding vertex invalidates")
    {
        mesh.addVertex({1.0f, 1.0f, 0.0f});
        REQUIRE_FALSE(mesh.cache.isValid());
    }
    
    SECTION("Adding face invalidates")
    {
        auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
        mesh.buildCache();
        REQUIRE(mesh.cache.isValid());
        
        mesh.addFace({v1, v3, v2});
        REQUIRE_FALSE(mesh.cache.isValid());
    }
}

TEST_CASE("TopologyCache - Vertex Valence", "[cache][valence]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
    SECTION("Isolated vertex has valence 0")
    {
        mesh.buildCache();
        REQUIRE(mesh.getValence(v0) == 0);
    }
    
    SECTION("Triangle vertices have valence 2")
    {
        mesh.addFace({v0, v1, v2});
        mesh.buildCache();
        
        REQUIRE(mesh.getValence(v0) == 2);
        REQUIRE(mesh.getValence(v1) == 2);
        REQUIRE(mesh.getValence(v2) == 2);
    }
    
    SECTION("Shared edge vertices have higher valence")
    {
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v1, v3, v2});
        mesh.buildCache();
        
        // Corner vertices: valence 2
        REQUIRE(mesh.getValence(v0) == 2);
        REQUIRE(mesh.getValence(v3) == 2);
        
        // Shared edge vertices: valence 3
        REQUIRE(mesh.getValence(v1) == 3);
        REQUIRE(mesh.getValence(v2) == 3);
    }
}

TEST_CASE("TopologyCache - Boundary Detection", "[cache][boundary]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    mesh.buildCache();
    
    SECTION("All vertices on single face are boundary")
    {
        REQUIRE(mesh.isBoundaryVertex(v0));
        REQUIRE(mesh.isBoundaryVertex(v1));
        REQUIRE(mesh.isBoundaryVertex(v2));
    }
    
    SECTION("All edges are boundary")
    {
        REQUIRE(mesh.cache.numBoundaryEdges() == 3);
    }
}

TEST_CASE("TopologyCache - Interior Vertex", "[cache][boundary]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    // Create 4 triangles around a central vertex
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});     // Center
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({-1.0f, 0.0f, 0.0f});
    auto v4 = mesh.addVertex({0.0f, -1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    mesh.addFace({v0, v2, v3});
    mesh.addFace({v0, v3, v4});
    mesh.addFace({v0, v4, v1});
    
    mesh.buildCache();
    
    SECTION("Center vertex is interior")
    {
        REQUIRE_FALSE(mesh.isBoundaryVertex(v0));
        REQUIRE(mesh.getValence(v0) == 4);
    }
    
    SECTION("Outer vertices are boundary")
    {
        REQUIRE(mesh.isBoundaryVertex(v1));
        REQUIRE(mesh.isBoundaryVertex(v2));
        REQUIRE(mesh.isBoundaryVertex(v3));
        REQUIRE(mesh.isBoundaryVertex(v4));
    }
}

TEST_CASE("TopologyCache - Vertex One-Ring", "[cache][onering]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});     // Center
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({-1.0f, 0.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    mesh.addFace({v0, v2, v3});
    mesh.buildCache();
    
    SECTION("One-ring size matches valence")
    {
        auto oneRing = mesh.getOneRing(v0);
        REQUIRE(oneRing.size() == mesh.getValence(v0));
    }
    
    SECTION("One-ring contains all neighbors")
    {
        auto oneRing = mesh.getOneRing(v0);
        std::vector<VertexIndex> neighbors(oneRing.begin(), oneRing.end());
        
        REQUIRE(std::find(neighbors.begin(), neighbors.end(), v1) != neighbors.end());
        REQUIRE(std::find(neighbors.begin(), neighbors.end(), v2) != neighbors.end());
        REQUIRE(std::find(neighbors.begin(), neighbors.end(), v3) != neighbors.end());
    }
}

TEST_CASE("TopologyCache - Edge Vertices", "[cache][edge]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    mesh.buildCache();
    
    SECTION("Edge vertices in canonical order")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        EdgeIndex e = mesh.halfEdges[he].edge;
        
        auto [ev0, ev1] = mesh.getEdgeVertices(e);
        
        // Should be in order (v0 < v1)
        REQUIRE(ev0 == v0);
        REQUIRE(ev1 == v1);
    }
}

TEST_CASE("TopologyCache - Face Vertices", "[cache][face]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2, v3});
    mesh.buildCache();
    
    SECTION("Get face vertices")
    {
        auto faceVerts = mesh.cache.getFaceVertices(0);
        
        REQUIRE(faceVerts.size() == 4);
        REQUIRE(faceVerts[0] == v0);
        REQUIRE(faceVerts[1] == v1);
        REQUIRE(faceVerts[2] == v2);
        REQUIRE(faceVerts[3] == v3);
    }
}

TEST_CASE("TopologyCache - Statistics", "[cache][stats]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    mesh.buildCache();
    
    SECTION("Cache memory usage")
    {
        size_t memory = mesh.cache.memoryUsage();
        REQUIRE(memory > 0);
    }
    
    SECTION("Cache statistics")
    {
        REQUIRE(mesh.cache.numVertices() == 3);
        REQUIRE(mesh.cache.numEdges() == 3);
        REQUIRE(mesh.cache.numBoundaryVertices() == 3);
        REQUIRE(mesh.cache.numBoundaryEdges() == 3);
    }
}