#include <catch2/catch_test_macros.hpp>
#include "control/utils.hpp"

using namespace Subdiv::Control;

/**
 * Helper: Build a single polygon face with N vertices.
 * - All edges are boundary edges (no twins)
 * - next / prev are wired correctly
 */
static Face* buildPolygon(
    const std::vector<glm::vec3>& positions,
    std::vector<Vertex*>& outVertices,
    std::vector<HalfEdge*>& outHalfEdges
) {
    REQUIRE(positions.size() >= 3);

    Face* face = new Face();

    // Create vertices
    for (const auto& p : positions) {
        outVertices.push_back(new Vertex(p));
    }

    const size_t n = outVertices.size();

    // Create half-edges
    for (size_t i = 0; i < n; ++i) {
        outHalfEdges.push_back(new HalfEdge());
    }

    // Wire half-edges into a loop
    for (size_t i = 0; i < n; ++i) {
        HalfEdge* he   = outHalfEdges[i];
        HalfEdge* next = outHalfEdges[(i + 1) % n];
        HalfEdge* prev = outHalfEdges[(i + n - 1) % n];

        he->to   = outVertices[(i + 1) % n];
        he->next = next;
        he->prev = prev;
        he->face = face;

        // Assign an outgoing edge to each vertex
        outVertices[i]->outgoing = he;
    }

    face->edge = outHalfEdges[0];
    return face;
}

// ------------------------------------------------------------
// Face tests
// ------------------------------------------------------------

TEST_CASE("Face loop is valid and valence matches vertex count", "[face]") {
    std::vector<Vertex*> vertices;
    std::vector<HalfEdge*> edges;

    Face* face = buildPolygon(
        {
            {0,0,0},
            {1,0,0},
            {2,1,0},
            {1,2,0},
            {0,1,0} // pentagon
        },
        vertices,
        edges
    );

    REQUIRE(face->valence() == 5);
    REQUIRE(face->isValidLoop());

    HalfEdge* e = face->edge;
    int count = 0;

    do {
        REQUIRE(e != nullptr);
        REQUIRE(e->face == face);
        e = e->next;
        ++count;
    } while (e != face->edge);

    REQUIRE(count == 5);
}

// ------------------------------------------------------------
// HalfEdge consistency tests
// ------------------------------------------------------------

TEST_CASE("HalfEdge next/prev pointers are consistent", "[halfedge]") {
    std::vector<Vertex*> vertices;
    std::vector<HalfEdge*> edges;

    buildPolygon(
        {
            {0,0,0},
            {1,0,0},
            {1,1,0}
        },
        vertices,
        edges
    );

    for (HalfEdge* he : edges) {
        REQUIRE(he->next != nullptr);
        REQUIRE(he->prev != nullptr);
        REQUIRE(he->next->prev == he);
        REQUIRE(he->prev->next == he);
    }
}

TEST_CASE("HalfEdge from() and to pointers are valid", "[halfedge]") {
    std::vector<Vertex*> vertices;
    std::vector<HalfEdge*> edges;

    buildPolygon(
        {
            {0,0,0},
            {1,0,0},
            {1,1,0},
            {0,1,0}
        },
        vertices,
        edges
    );

    for (HalfEdge* he : edges) {
        REQUIRE(he->to != nullptr);
        REQUIRE(he->from() != nullptr);
        REQUIRE(he->from() != he->to);
    }
}

// ------------------------------------------------------------
// Boundary tests
// ------------------------------------------------------------

TEST_CASE("Boundary edges and vertices are detected correctly", "[boundary]") {
    std::vector<Vertex*> vertices;
    std::vector<HalfEdge*> edges;

    buildPolygon(
        {
            {0,0,0},
            {1,0,0},
            {1,1,0},
            {0,1,0}
        },
        vertices,
        edges
    );

    for (HalfEdge* he : edges) {
        REQUIRE(he->isBoundary());
        REQUIRE(he->twin == nullptr);
    }

    for (Vertex* v : vertices) {
        REQUIRE(v->isBoundary());
    }
}

// ------------------------------------------------------------
// Twin tests
// ------------------------------------------------------------

TEST_CASE("Twin edges are symmetric", "[twin]") {
    Vertex v0({0,0,0});
    Vertex v1({1,0,0});

    HalfEdge e0;
    HalfEdge e1;

    e0.to = &v1;
    e1.to = &v0;

    e0.twin = &e1;
    e1.twin = &e0;

    REQUIRE(e0.twin == &e1);
    REQUIRE(e1.twin == &e0);
    REQUIRE(e0.twin->twin == &e0);
    REQUIRE(e1.twin->twin == &e1);
}