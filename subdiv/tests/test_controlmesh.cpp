#include <catch2/catch_test_macros.hpp>

#include "control/mesh.hpp"
#include "diagnostics/context.hpp"

#include <glm/gtc/constants.hpp>

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

TEST_CASE("Mesh - Basic Construction", "[mesh][construction]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    SECTION("Empty mesh has zero elements")
    {
        REQUIRE(mesh.numVertices()  == 0);
        REQUIRE(mesh.numFaces()     == 0);
        REQUIRE(mesh.numEdges()     == 0);
        REQUIRE(mesh.numHalfEdges() == 0);
        REQUIRE(mesh.isEmpty());
    }
    
    SECTION("Add single vertex")
    {
        VertexIndex v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
        
        REQUIRE(v0 == 0);
        REQUIRE(mesh.numVertices() == 1);
        REQUIRE(mesh.positions[v0].x == 0.0f);
        REQUIRE(mesh.positions[v0].y == 0.0f);
        REQUIRE(mesh.positions[v0].z == 0.0f);
    }
    
    SECTION("Add multiple vertices")
    {
        auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
        auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
        auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
        
        REQUIRE(mesh.numVertices() == 3);
        REQUIRE(v0 == 0);
        REQUIRE(v1 == 1);
        REQUIRE(v2 == 2);

        REQUIRE(mesh.positions[v0] == glm::vec3(0.0f, 0.0f, 0.0f));
        REQUIRE(mesh.positions[v1] == glm::vec3(1.0f, 0.0f, 0.0f));
        REQUIRE(mesh.positions[v2] == glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

TEST_CASE("Mesh - Triangle Face", "[mesh][face][construction]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    SECTION("Add valid triangle face")
    {
        FaceIndex face = mesh.addFace({v0, v1, v2});
        
        REQUIRE(face != INVALID_INDEX);
        REQUIRE(!Context::hasErrors());
        REQUIRE(mesh.numFaces()          == 1);
        REQUIRE(mesh.numEdges()          == 3);
        REQUIRE(mesh.numHalfEdges()      == 3);
        REQUIRE(mesh.faces[face].valence == 3);
    }
    
    SECTION("Face with too few vertices should fail")
    {
        FaceIndex face = mesh.addFace({v0, v1});
        
        REQUIRE(face == INVALID_INDEX);
        REQUIRE(Context::hasErrors());
        
        const auto* err = Context::getLastError();
        REQUIRE(err != nullptr);
        REQUIRE(err->code == "FACE_TOO_FEW_VERTICES");
    }
    
    SECTION("Face with duplicate vertices should fail")
    {
        FaceIndex face = mesh.addFace({v0, v1, v0});
        
        REQUIRE(face == INVALID_INDEX);
        REQUIRE(Context::hasErrors());
        
        const auto* err = Context::getLastError();
        REQUIRE(err != nullptr);
        REQUIRE(err->code == "DUPLICATE_VERTEX_IN_FACE");
    }
    
    SECTION("Face with invalid vertex should fail")
    {
        FaceIndex face = mesh.addFace({v0, v1, 999});
        
        REQUIRE(face == INVALID_INDEX);
        REQUIRE(Context::hasErrors());
        
        const auto* err = Context::getLastError();
        REQUIRE(err != nullptr);
        REQUIRE(err->code == "INVALID_VERTEX_INDEX");
    }
}

TEST_CASE("Mesh - Quad Face", "[mesh][face][construction]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({-1.0f, -1.0f, 0.0f});
    auto v1 = mesh.addVertex({ 1.0f, -1.0f, 0.0f});
    auto v2 = mesh.addVertex({ 1.0f,  1.0f, 0.0f});
    auto v3 = mesh.addVertex({-1.0f,  1.0f, 0.0f});
    
    FaceIndex face = mesh.addFace({v0, v1, v2, v3});
    
    REQUIRE(face != INVALID_INDEX);
    REQUIRE(!Context::hasErrors());
    REQUIRE(mesh.numFaces()          == 1);
    REQUIRE(mesh.numEdges()          == 4);
    REQUIRE(mesh.numHalfEdges()      == 4);
    REQUIRE(mesh.faces[face].valence == 4);
}

TEST_CASE("Mesh - NGon Face", "[mesh][face][construction]")
{
    DiagnosticTestScope diag;
    Mesh mesh;

    std::vector<VertexIndex> verts;
    for(int i = 0; i < 5; ++i)
    {
        float angle = i * 2.0f * glm::pi<float>() / 5;
        verts.push_back(mesh.addVertex({glm::cos(angle), glm::sin(angle), 0}));
    }
    
    FaceIndex face = mesh.addFace(verts);
    
    REQUIRE(face != INVALID_INDEX);
    REQUIRE(!Context::hasErrors());
    REQUIRE(mesh.numFaces()          == 1);
    REQUIRE(mesh.numEdges()          == 5);
    REQUIRE(mesh.numHalfEdges()      == 5);
    REQUIRE(mesh.faces[face].valence == 5);
}

TEST_CASE("Mesh - Manifold Mesh", "[mesh][manifold]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
    SECTION("Two triangles sharing an edge")
    {
        FaceIndex f0 = mesh.addFace({v0, v1, v2});
        FaceIndex f1 = mesh.addFace({v1, v3, v2});
        
        REQUIRE(f0 != INVALID_INDEX);
        REQUIRE(f1 != INVALID_INDEX);
        REQUIRE(!Context::hasErrors());
        
        // Should have 2 faces, 5 edges (3 + 3 - 1 shared)
        REQUIRE(mesh.numFaces() == 2);
        REQUIRE(mesh.numEdges() == 5);
        
        // Validate mesh
        REQUIRE(mesh.validate());
    }
    
    SECTION("Non-manifold edge should fail - third face on edge")
    {
        auto v4 = mesh.addVertex({0.5f, 0.5f, 1.0f});
        
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v1, v0, v3}); // Twin edge v0-v1
        
        // Third face sharing edge v0-v1 would be non-manifold
        FaceIndex f2 = mesh.addFace({v0, v1, v4});
        
        REQUIRE(f2 == INVALID_INDEX);
        REQUIRE(Context::hasErrors());
        
        const auto* err = Context::getLastError();
        REQUIRE(err != nullptr);
        REQUIRE(err->code == "NON_MANIFOLD_EDGE");
    }
}

TEST_CASE("Mesh - Duplicate Directed Edge", "[mesh][direction][error]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
    SECTION("Same direction edge is detected")
    {
        // First face: v0 -> v1 -> v2
        FaceIndex f1 = mesh.addFace({v0, v1, v2});
        REQUIRE(f1 != INVALID_INDEX);
        REQUIRE(!Context::hasErrors());
        
        size_t initialHalfEdges = mesh.numHalfEdges();
        
        // Second face: v0 -> v1 -> v3 (tries to create v0->v1 again)
        FaceIndex f2 = mesh.addFace({v0, v1, v3});
        
        REQUIRE(f2 == INVALID_INDEX);
        REQUIRE(Context::hasErrors());
        
        const auto* err = Context::getLastError();
        REQUIRE(err != nullptr);
        REQUIRE(err->code == "NON_MANIFOLD_EDGE");
        REQUIRE(err->context.find("position 0") != std::string::npos);
        
        // Verify mesh wasn't modified
        REQUIRE(mesh.numHalfEdges() == initialHalfEdges);
    }
    
    SECTION("Opposite direction succeeds")
    {
        mesh.addFace({v0, v1, v2});
        
        // This should succeed - v1->v0 is opposite to v0->v1
        FaceIndex f2 = mesh.addFace({v1, v0, v3});
        
        REQUIRE(f2 != INVALID_INDEX);
        REQUIRE(!Context::hasErrors());
        
        // Verify twins were linked
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(he01 != INVALID_INDEX);
        REQUIRE(he10 != INVALID_INDEX);
        REQUIRE(mesh.halfEdges[he01].twin == he10);
        REQUIRE(mesh.halfEdges[he10].twin == he01);
    }
}

TEST_CASE("Mesh - findHalfEdge", "[mesh][halfedge][query]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
    SECTION("Find half-edge stored directly in map")
    {
        mesh.addFace({v0, v1, v2});
        
        // v0->v1 should be in the map
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        
        REQUIRE(he != INVALID_INDEX);
        REQUIRE(mesh.halfEdges[he].to == v1);
    }
    
    SECTION("Find half-edge via twin (not in map)")
    {
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v1, v0, v3});
        
        // Both directions should be findable
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(he01 != INVALID_INDEX);
        REQUIRE(he10 != INVALID_INDEX);
        
        // Verify they point in correct directions
        REQUIRE(mesh.halfEdges[he01].to == v1);
        REQUIRE(mesh.halfEdges[he10].to == v0);
        
        // Verify they are twins
        REQUIRE(mesh.halfEdges[he01].twin == he10);
        REQUIRE(mesh.halfEdges[he10].twin == he01);
    }
    
    SECTION("Boundary edge only findable in one direction")
    {
        mesh.addFace({v0, v1, v2});
        
        // v0->v1 exists (boundary)
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        REQUIRE(he01 != INVALID_INDEX);
        REQUIRE(mesh.halfEdges[he01].twin == INVALID_INDEX); //< is boundary test
        
        // v1->v0 doesn't exist
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        REQUIRE(he10 == INVALID_INDEX);
    }
    
    SECTION("Non-existent edge returns INVALID")
    {
        mesh.addFace({v0, v1, v2});
        
        // Edge v0->v3 doesn't exist
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v3);
        REQUIRE(he == INVALID_INDEX);
    }
    
    SECTION("After adding twin, both directions findable")
    {
        mesh.addFace({v0, v1, v2});
        
        // Initially only v0->v1 exists
        REQUIRE(mesh.findHalfEdge(v0, v1) != INVALID_INDEX);
        REQUIRE(mesh.findHalfEdge(v1, v0) == INVALID_INDEX);
        
        // Add face with opposite edge
        mesh.addFace({v1, v0, v3});
        
        // Now both directions exist
        REQUIRE(mesh.findHalfEdge(v0, v1) != INVALID_INDEX);
        REQUIRE(mesh.findHalfEdge(v1, v0) != INVALID_INDEX);
    }
}

TEST_CASE("Mesh - findEdge", "[mesh][edge][query]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    SECTION("Find edge in either direction")
    {
        EdgeIndex edge01 = mesh.findEdge(v0, v1);
        EdgeIndex edge10 = mesh.findEdge(v1, v0);
        
        REQUIRE(edge01 != INVALID_INDEX);
        REQUIRE(edge10 != INVALID_INDEX);
        
        // Should be the same edge
        REQUIRE(edge01 == edge10);
    }
    
    SECTION("Non-existent edge returns INVALID")
    {
        auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
        EdgeIndex edge = mesh.findEdge(v0, v3);
        REQUIRE(edge == INVALID_INDEX);
    }
}

TEST_CASE("Mesh - Clear", "[mesh]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    mesh.addFace({v0, v1, v2});
    
    REQUIRE(mesh.numVertices() > 0);
    REQUIRE(mesh.numFaces() > 0);
    
    mesh.clear();
    
    REQUIRE(mesh.numVertices() == 0);
    REQUIRE(mesh.numFaces() == 0);
    REQUIRE(mesh.numEdges() == 0);
    REQUIRE(mesh.numHalfEdges() == 0);
    REQUIRE(!mesh.cache.isValid());
}

TEST_CASE("Mesh - Complex Geometry", "[mesh][complex]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    SECTION("Grid of quads")
    {
        // Create 3x3 grid of vertices
        std::vector<VertexIndex> verts;
        for (int y = 0; y < 3; ++y) {
            for (int x = 0; x < 3; ++x) {
                verts.push_back(mesh.addVertex({float(x), float(y), 0.0f}));
            }
        }
        
        // Create quads
        int faceCount = 0;
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                VertexIndex v0 = verts[y * 3 + x];
                VertexIndex v1 = verts[y * 3 + (x + 1)];
                VertexIndex v2 = verts[(y + 1) * 3 + (x + 1)];
                VertexIndex v3 = verts[(y + 1) * 3 + x];
                
                FaceIndex f = mesh.addFace({v0, v1, v2, v3});
                REQUIRE(f != INVALID_INDEX);
                faceCount++;
            }
        }
        
        REQUIRE(faceCount == 4);
        REQUIRE(!Context::hasErrors());
        REQUIRE(mesh.validate());
    }
}

TEST_CASE("Mesh - Vertex Position", "[mesh][attributes]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    
    SECTION("Set position")
    {
        mesh.setPosition(v0, {1.0f, 2.0f, 3.0f});
        REQUIRE(mesh.positions[v0] == glm::vec3(1.0f, 2.0f, 3.0f));
    }
}

TEST_CASE("Mesh - Edge Attributes", "[mesh][attributes]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    EdgeIndex e = mesh.findEdge(v0, v1);
    
    SECTION("Default is smooth")
    {
        REQUIRE(mesh.edges[e].tag == EdgeTag::SMOOTH);
        REQUIRE(mesh.edges[e].sharpness == 0.0f);
    }
    
    SECTION("Set crease")
    {
        mesh.setEdgeCrease(e, true);
        REQUIRE(mesh.edges[e].tag == EdgeTag::CREASE);
        REQUIRE(mesh.edges[e].sharpness == 1.0f);
    }
    
    SECTION("Set semi-sharp")
    {
        mesh.setEdgeSharpness(e, 0.5f);
        REQUIRE(mesh.edges[e].tag == EdgeTag::SEMI);
        REQUIRE(mesh.edges[e].sharpness == 0.5f);
    }
}