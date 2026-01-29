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

TEST_CASE("ControlMesh - Basic Construction", "[controlmesh]")
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
    }
}

TEST_CASE("ControlMesh - Triangle Face", "[controlmesh][face]")
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
    }
}

TEST_CASE("ControlMesh - Manifold Mesh", "[controlmesh][manifold]")
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
        REQUIRE(mesh.validate());
    }
    
    SECTION("Non-manifold edge should fail")
    {
        mesh.addFace({v0, v1, v2});
        mesh.addFace({v1, v3, v2});
        FaceIndex f2 = mesh.addFace({v1, v2, v3});
        
        REQUIRE(f2 == INVALID_INDEX);
        REQUIRE(Context::hasErrors());
        
        const auto* err = Context::getLastError();
        REQUIRE(err->code == "NON_MANIFOLD_EDGE");
    }
}

/**
TEST_CASE("ControlMesh - Result Type", "[controlmesh][result]")
{
    Mesh mesh;
    
    auto v0 = mesh.addVertex({0.0f, 0.0f, 0.0f});
    auto v1 = mesh.addVertex({1.0f, 0.0f, 0.0f});
    auto v2 = mesh.addVertex({0.0f, 1.0f, 0.0f});
    
    SECTION("Successful face creation")
    {
        auto result = mesh.tryAddFace({v0, v1, v2});
        
        REQUIRE(result.isOk());
        REQUIRE(!result.isError());
    }
    
    SECTION("Failed face creation")
    {
        auto result = mesh.tryAddFace({v0, v1});
        
        REQUIRE(!result.isOk());
        REQUIRE(result.isError());
        REQUIRE(result.error().code == "FACE_TOO_FEW_VERTICES");
    }
}
/**/