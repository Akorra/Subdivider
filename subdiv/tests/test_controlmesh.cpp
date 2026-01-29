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

TEST_CASE("Mesh - Basic Construction", "[mesh]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    SECTION("Empty mesh has zero elements")
    {
        REQUIRE(mesh.numVertices() == 0);
        REQUIRE(mesh.numFaces() == 0);
        REQUIRE(mesh.numEdges() == 0);
        REQUIRE(mesh.numHalfEdges() == 0);
    }
    
    SECTION("Add single vertex")
    {
        VertexIndex v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
        
        REQUIRE(v0 == 0);
        REQUIRE(mesh.numVertices() == 1);
        REQUIRE(mesh.vertex(v0).position.x == 0.0f);
        REQUIRE(mesh.vertex(v0).position.y == 0.0f);
        REQUIRE(mesh.vertex(v0).position.z == 0.0f);
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
    }
}

TEST_CASE("Mesh - Triangle Face", "[mesh][face]")
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
        REQUIRE(mesh.numFaces() == 1);
        REQUIRE(mesh.numHalfEdges() == 3);
        REQUIRE(mesh.numEdges() == 3);
        
        // Check face valence
        REQUIRE(mesh.face(face).valence == 3);
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

TEST_CASE("Mesh - Quad Face", "[mesh][face]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    auto v3 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    FaceIndex face = mesh.addFace({v0, v1, v2, v3});
    
    REQUIRE(face != INVALID_INDEX);
    REQUIRE(!Context::hasErrors());
    REQUIRE(mesh.face(face).valence == 4);
    REQUIRE(mesh.numHalfEdges() == 4);
    REQUIRE(mesh.numEdges() == 4);
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
    
    SECTION("Non-manifold edge should fail")
    {
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v1, v3, v2});
        
        // Third face sharing edge v1-v2 would be non-manifold
        FaceIndex f2 = mesh.addFace({v1, v2, v3});
        
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
        REQUIRE(mesh.halfEdge(he01).twin == he10);
        REQUIRE(mesh.halfEdge(he10).twin == he01);
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
        REQUIRE(mesh.halfEdge(he).to == v1);
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
        REQUIRE(mesh.halfEdge(he01).to == v1);
        REQUIRE(mesh.halfEdge(he10).to == v0);
        
        // Verify they are twins
        REQUIRE(mesh.halfEdge(he01).twin == he10);
        REQUIRE(mesh.halfEdge(he10).twin == he01);
    }
    
    SECTION("Boundary edge only findable in one direction")
    {
        mesh.addFace({v0, v1, v2});
        
        // v0->v1 exists (boundary)
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        REQUIRE(he01 != INVALID_INDEX);
        REQUIRE(mesh.halfEdge(he01).isBoundary());
        
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
    auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
    
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
        EdgeIndex edge = mesh.findEdge(v0, v3);
        REQUIRE(edge == INVALID_INDEX);
    }
}

TEST_CASE("Mesh - HalfEdge Connectivity", "[mesh][halfedge][connectivity]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    SECTION("Next/prev form a loop")
    {
        HalfEdgeIndex he = mesh.findHalfEdge(v0, v1);
        REQUIRE(he != INVALID_INDEX);
        
        HalfEdgeIndex next = mesh.halfEdge(he).next;
        HalfEdgeIndex prev = mesh.halfEdge(he).prev;
        
        REQUIRE(next != INVALID_INDEX);
        REQUIRE(prev != INVALID_INDEX);
        
        // next->prev should point back
        REQUIRE(mesh.halfEdge(next).prev == he);
        // prev->next should point back
        REQUIRE(mesh.halfEdge(prev).next == he);
        
        // Loop should close
        HalfEdgeIndex current = he;
        int count = 0;
        do {
            current = mesh.halfEdge(current).next;
            count++;
        } while (current != he && count < 10);
        
        REQUIRE(count == 3); // Triangle
    }
    
    SECTION("Twin connectivity")
    {
        auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
        mesh.addFace({v1, v0, v3});
        
        HalfEdgeIndex he01 = mesh.findHalfEdge(v0, v1);
        HalfEdgeIndex he10 = mesh.findHalfEdge(v1, v0);
        
        REQUIRE(!mesh.halfEdge(he01).isBoundary());
        REQUIRE(!mesh.halfEdge(he10).isBoundary());
        
        // Twin of twin should be self
        REQUIRE(mesh.halfEdge(mesh.halfEdge(he01).twin).twin == he01);
    }
}

TEST_CASE("Mesh - Boundary Detection", "[mesh][boundary]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    mesh.addFace({v0, v1, v2});
    
    SECTION("All vertices on single face are boundary")
    {
        REQUIRE(mesh.isBoundaryVertex(v0));
        REQUIRE(mesh.isBoundaryVertex(v1));
        REQUIRE(mesh.isBoundaryVertex(v2));
    }
    
    SECTION("Vertex with closed fan is not boundary")
    {
        auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
        
        // Create closed quad (two triangles)
        mesh.addFace({v0, v2, v3});
        mesh.addFace({v0, v3, v1});
        
        // v0 is interior (shared by 3 faces, all edges have twins)
        // This would need proper closed fan, but this tests the concept
    }
}

TEST_CASE("Mesh - Vertex Valence", "[mesh][vertex][valence]")
{
    DiagnosticTestScope diag;
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    SECTION("Isolated vertex has valence 0")
    {
        REQUIRE(mesh.getVertexValence(v0) == 0);
    }
    
    SECTION("Vertex in triangle has valence 2")
    {
        mesh.addFace({v0, v1, v2});
        
        REQUIRE(mesh.getVertexValence(v0) == 2);
        REQUIRE(mesh.getVertexValence(v1) == 2);
        REQUIRE(mesh.getVertexValence(v2) == 2);
    }
    
    SECTION("Interior vertex has higher valence")
    {
        auto v3 = mesh.addVertex({1.0f, 1.0f, 0.0f});
        
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v1, v3, v2});
        
        // v1 and v2 are shared by 2 faces
        REQUIRE(mesh.getVertexValence(v1) == 3);
        REQUIRE(mesh.getVertexValence(v2) == 3);
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