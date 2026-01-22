#pragma once

#include <vector>
#include <cassert>
#include <glm/glm.hpp> // GLM math

namespace Subdiv::Control
{

/**
 * @brief Edge type for subdivision surfaces.
 * EDGE_SMOOTH : Normal smooth edge.
 * EDGE_HARD   : Fully sharp edge.
 * EDGE_SEMI   : Semi-sharp edge, controlled by sharpness.
 */
enum class EdgeTag : uint8_t {
    EDGE_SMOOTH = 0,
    EDGE_HARD   = 1,
    EDGE_SEMI   = 2,
    EDGE_UNKNOWN = 255 // default uninitialized
};

// Forward declarations
struct Vertex;
struct HalfEdge;
struct Face;

/**
 * @brief Vertex of a half-edge mesh.
 * Stores position and one outgoing half-edge.
 * Optional: mark as crease/corner vertex.
 */
struct Vertex {
    glm::vec3 position;             ///< Vertex position
    HalfEdge* outgoing = nullptr;   ///< One outgoing half-edge (nullptr if isolated)
    bool isCorner = false;          ///< True if vertex is a crease/corner

    // Constructors
    Vertex() = default;
    explicit Vertex(const glm::vec3& pos) : position(pos) {}
    
    /**
     * @brief Checks if this vertex is on a boundary (has at least one boundary edge)
     */
    bool isBoundary() const;
};

/**
 * @brief Half-edge structure for mesh connectivity.
 * Represents an edge directed from its previous half-edge vertex to `to`.
 */
struct HalfEdge {
    Vertex*   to      = nullptr; ///< Destination vertex of this half-edge
    HalfEdge* next    = nullptr; ///< Next half-edge around the face
    HalfEdge* prev    = nullptr; ///< Previous half-edge around the face
    HalfEdge* twin    = nullptr; ///< Opposite half-edge
    Face*     face    = nullptr; ///< Face this half-edge borders

    EdgeTag tag       = EdgeTag::EDGE_SMOOTH; ///< Edge tag for subdivision
    float   sharpness = 0.0f;                 ///< Only valid for EDGE_SEMI

    HalfEdge() = default;

    /**
     * @brief Checks if this edge is on a boundary (no twin).
     */
    bool isBoundary() const { return twin == nullptr; }

    /**
     * @brief Safely sets sharpness, only valid for EDGE_SEMI.
     */
    void setSharpness(float s) {
        assert(tag == EdgeTag::EDGE_SEMI && "Sharpness can only be set on semi-sharp edges");
        sharpness = glm::max(0.0f, s);
    }

    /**
     * @brief Returns the vertex at the start of this half-edge (prev->to)
     */
    Vertex* from() const {
        assert(prev && prev->to);
        return prev->to;
    }
};

/**
 * @brief Face of a half-edge mesh.
 * Stores one half-edge of its loop.
 */
struct Face {
    HalfEdge* edge = nullptr; ///< One half-edge of the face loop

    Face() = default;

    /**
     * @brief Returns the number of edges (vertices) in the face.
     */
    [[nodiscard]] int valence() const {
        if (!edge) return 0;
        int count = 0;
        HalfEdge* e = edge;
        do {
            assert(e && "Half-edge loop is broken");
            ++count;
            e = e->next;
        } 
        while (e != edge);

        return count;
    }

    /**
     * @brief Checks if the face is valid (all edges connected in a loop)
     */
    bool isValidLoop() const 
    {
        if (!edge) return false;
        HalfEdge* e = edge->next;
        while (e && e != edge) e = e->next;
        return e == edge;
    }
};

/**
 * Implementation of Vertex helper
 */
inline bool Vertex::isBoundary() const {
    if (!outgoing) return true; // isolated vertex
    const HalfEdge* e = outgoing;
    do {
        if (e->isBoundary()) 
            return true;
        e = e->twin ? e->twin->next : nullptr;
    } 
    while (e && e != outgoing);
    
    return false;
}

} // namespace subdiv