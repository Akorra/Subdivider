#include "mesh_cache.hpp"

#include "mesh.hpp"

#include "diagnostics/context.hpp"

namespace Subdiv::Control 
{

void TopologyCache::build(const Mesh& mesh) 
{
    SUBDIV_PROFILE_FUNCTION();
    
    clear();
    
    const size_t numVerts     = mesh.vertices.size();
    const size_t numEdges     = mesh.edges.size();
    const size_t numFaces     = mesh.faces.size();
    const size_t numHalfEdges = mesh.halfEdges.size();
    
    if (numVerts == 0) return;
    
    // Edge data
    edgeVertices_.resize(numEdges, {INVALID_INDEX, INVALID_INDEX});
    edgeBoundaryFlags_.resize(numEdges, 1);  
    edgeFaceOffsets_.resize(numEdges + 1, 0);
    
    // PHASE 1: Build edge data (single pass over half-edges) =================
    
    std::vector<size_t> edgeFaceCounts(numEdges, 0);

    // Single pass over half-edges
    for(HalfEdgeIndex h = 0; h < mesh.halfEdges.size(); ++h)
    {
        const HalfEdge& he = mesh.halfEdges[h];

        if(he.edge == INVALID_INDEX || he.edge >= numEdges) continue;

        // Set edge vertices
        if(edgeVertices_[he.edge][0] == INVALID_INDEX)
        {
            VertexIndex v0 = mesh.getFromVertex(h);
            VertexIndex v1 = he.to;

            // Store in canonical order (v0 < v1)
            if(v0 > v1) std::swap(v0, v1);
            edgeVertices_[he.edge] = {v0, v1};
        }

        // Count adjacent faces
        if(he.face != INVALID_INDEX)
            edgeFaceCounts[he.edge]++;
        
        // If he has twin, then it is not boundary
        if(he.twin != INVALID_INDEX)
            edgeBoundaryFlags_[he.edge] = 0; 
    }

#if SUBDIV_ENABLE_PROFILING
    // Detect non-manifold edges
    for (EdgeIndex e = 0; e < numEdges; ++e) {
        if (edgeFaceCounts[e] > 2)
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::WARNING,"NON_MANIFOLD_EDGE_DETECTED", "Edge has more than 2 faces", "Edge " + std::to_string(e) + " has " + std::to_string(edgeFaceCounts[e]) + " faces");
    }
#endif

    // Count boundary edges
    numBoundaryEdges_ = std::count(edgeBoundaryFlags_.begin(), edgeBoundaryFlags_.end(), 1);

    // PHASE 2: Compute valences (uniform definition: number of incident edges)
    
    valences_.resize(numVerts, 0);

    for (EdgeIndex e = 0; e < numEdges; ++e) 
    {
        auto [v0, v1] = edgeVertices_[e];
        if (isValidIndex(v0, numVerts)) valences_[v0]++; 
        if (isValidIndex(v1, numVerts)) valences_[v1]++;
    }

    // PHASE 3: Determine boundary vertices (separate from valence) ===========
    
    boundaryFlags_.resize(numVerts, 0);
    
    // A vertex is boundary if ANY incident edge is boundary
    for (EdgeIndex e = 0; e < numEdges; ++e) 
    {
        if (edgeBoundaryFlags_[e] != 0) 
        {
            auto [v0, v1] = edgeVertices_[e];
            if(isValidIndex(v0, numVerts)) boundaryFlags_[v0] = 1;
            if(isValidIndex(v1, numVerts)) boundaryFlags_[v1] = 1;
        }
    }

    numBoundaryVertices_ = std::count(boundaryFlags_.begin(), boundaryFlags_.end(), 1);

    // PHASE 4: Count vertex-face incidence ===================================
    
    std::vector<size_t> vertexFaceCounts(numVerts, 0);
    
    // Count faces per vertex
    for (FaceIndex f = 0; f < numFaces; ++f) 
    {
        const Face& face = mesh.faces[f];
        
        HalfEdgeIndex start = face.edge;
        if (start == INVALID_INDEX) continue;
        
        HalfEdgeIndex current = start;
        std::vector<bool> visited(numHalfEdges, false); // Stack-allocated for speed
        
        do 
        {
            if (visited[current]) 
            {
                SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::WARNING, "CYCLE_IN_FACE", "Detected cycle in face loop", "Face " + std::to_string(f));
                break;
            }

            visited[current] = true;

            VertexIndex v = mesh.getFromVertex(current);
            if (isValidIndex(v, numVerts))
                vertexFaceCounts[v]++;
            
            current = mesh.halfEdges[current].next;
        } 
        while (current != start && isValidIndex(current, numHalfEdges));
    }
    
    // PHASE 5: Build CSR offsets =============================================
    
    // Vertex one-ring offsets
    oneRingOffsets_.resize(numVerts + 1, 0);
    oneRingOffsets_[0] = 0;
    for (size_t v = 0; v < numVerts; ++v)
        oneRingOffsets_[v + 1] = oneRingOffsets_[v] + static_cast<uint32_t>(valences_[v]);
    
    // Vertex-face offsets
    vertexFaceOffsets_.resize(numVerts + 1, 0);
    vertexFaceOffsets_[0] = 0;
    for (size_t v = 0; v < numVerts; ++v)
        vertexFaceOffsets_[v + 1] = vertexFaceOffsets_[v] + vertexFaceCounts[v];
    
    // Edge-face offsets
    edgeFaceOffsets_[0] = 0;
    for (size_t e = 0; e < numEdges; ++e)
        edgeFaceOffsets_[e + 1] = edgeFaceOffsets_[e] + edgeFaceCounts[e];
    
    // Face-vertex offsets
    faceVertexOffsets_.resize(numFaces + 1, 0);
    faceVertexOffsets_[0] = 0;
    for (size_t f = 0; f < numFaces; ++f)
        faceVertexOffsets_[f + 1] = faceVertexOffsets_[f] + mesh.faces[f].valence;
    
    // Face-edge offsets
    faceEdgeOffsets_.resize(numFaces + 1, 0);
    faceEdgeOffsets_[0] = 0;
    for (size_t f = 0; f < numFaces; ++f) 
        faceEdgeOffsets_[f + 1] = faceEdgeOffsets_[f] + mesh.faces[f].valence;

    // PHASE 6: Allocate flattened arrays =====================================
    
    oneRings_.resize(oneRingOffsets_.back());
    vertexFaces_.resize(vertexFaceOffsets_.back());
    edgeFaces_.resize(edgeFaceOffsets_.back());
    faceVertices_.resize(faceVertexOffsets_.back());
    faceEdges_.resize(faceEdgeOffsets_.back());

    // PHASE 7: Fill vertex one-rings (ORDERED CCW, half-edge-based) ==========

    std::vector<uint32_t> oneRingWritePos = oneRingOffsets_;
    std::vector<bool>     visitedHalfEdges(numHalfEdges); // Reusable buffer

    for (VertexIndex v = 0; v < numVerts; ++v)
    {
        const Vertex& vert = mesh.vertices[v];
        if (vert.outgoing == INVALID_INDEX) continue;
        
        const uint32_t writeStart = oneRingWritePos[v];
        const uint32_t writeEnd = oneRingOffsets_[v + 1];
        
        HalfEdgeIndex start = vert.outgoing;
        HalfEdgeIndex current = start;

        // Clear visited flags for this vertex
        std::fill(visitedHalfEdges.begin(), visitedHalfEdges.end(), false);
        
        // First, check if we're on a boundary by walking forward
        // Walk clockwise (twin->next)
        bool hitBoundary = false;
        do {
            if (visitedHalfEdges[current]) break; // Cycle detected

            visitedHalfEdges[current] = true;
            
            const VertexIndex neighbor = mesh.halfEdges[current].to;
            if (oneRingWritePos[v] < writeEnd)
                oneRings_[oneRingWritePos[v]++] = neighbor;
            
            const HalfEdgeIndex twin = mesh.halfEdges[current].twin;
            if (twin == INVALID_INDEX) 
            {
                hitBoundary = true;
                break;
            }
            
            current = mesh.halfEdges[twin].next;
            
        } while (current != start);
        
        // If boundary, walk BACKWARDS to find the first boundary half-edge
        if (hitBoundary) 
        {
            while (true) 
            {
                const HalfEdgeIndex prev = mesh.halfEdges[current].prev;
                if (!isValidIndex(prev, numHalfEdges)) break;
                
                const HalfEdgeIndex prevTwin = mesh.halfEdges[prev].twin;
                if (prevTwin == INVALID_INDEX) 
                {
                    const VertexIndex lastNeighbour = mesh.getFromVertex(prev);
                    if(oneRingWritePos[v] < writeEnd)
                        oneRings_[oneRingWritePos[v]++] = lastNeighbour;
                    break;
                }
                
                if (visitedHalfEdges[prevTwin]) break; // Already processed
                visitedHalfEdges[prevTwin] = true;

                const VertexIndex neighbor = mesh.halfEdges[prevTwin].to;
                if (oneRingWritePos[v] < writeEnd)
                    oneRings_[oneRingWritePos[v]++] = neighbor;
                current = prevTwin;
            }
        }

#if SUBDIV_ENABLE_PROFILING
        // Verify count
        const size_t written = oneRingWritePos[v] - writeStart;
        if (written != valences_[v]) 
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::WARNING, "ONE_RING_COUNT_MISMATCH", "One-ring doesn't match valence", "Vertex " + std::to_string(v) +  ": expected " + std::to_string(valences_[v]) + ", got " + std::to_string(written));
#endif
    }

    // PHASE 8: Fill face data and incidence relationships ====================
    
    std::vector<uint32_t> vertexFaceWritePos = vertexFaceOffsets_;
    std::vector<uint32_t> edgeFaceWritePos = edgeFaceOffsets_;
    
    for (FaceIndex f = 0; f < numFaces; ++f) 
    {
        const Face& face = mesh.faces[f];
        
        HalfEdgeIndex start = face.edge;
        if (!isValidIndex(start, numHalfEdges)) continue;
        
        HalfEdgeIndex current = start;
        uint32_t faceVertPos = faceVertexOffsets_[f];
        uint32_t faceEdgePos = faceEdgeOffsets_[f];

        const uint32_t faceVertEnd = faceVertexOffsets_[f + 1];
        const uint32_t faceEdgeEnd = faceEdgeOffsets_[f + 1];
        
        std::fill(visitedHalfEdges.begin(), visitedHalfEdges.end(), false);

        do 
        {
            if (visitedHalfEdges[current]) break;
            visitedHalfEdges[current] = true;

            const HalfEdge& he = mesh.halfEdges[current];
            
            // Add vertex to face
            const VertexIndex v = mesh.getFromVertex(current);
            if (isValidIndex(v, numVerts)) 
            {
                if (faceVertPos < faceVertEnd) 
                    faceVertices_[faceVertPos++] = v;
                
                // Add face to vertex
                if (vertexFaceWritePos[v] < vertexFaceOffsets_[v + 1]) 
                    vertexFaces_[vertexFaceWritePos[v]++] = f;
            }
            
            // Add edge to face
            EdgeIndex e = he.edge;
            if (isValidIndex(e, numEdges)) 
            {
                if (faceEdgePos < faceEdgeEnd)
                    faceEdges_[faceEdgePos++] = e;
                
                // Add face to edge
                if (edgeFaceWritePos[e] < edgeFaceOffsets_[e + 1])
                    edgeFaces_[edgeFaceWritePos[e]++] = f;
            }
            
            current = he.next;

        } while (current != start && isValidIndex(current, numHalfEdges));
    }

    // PHASE 9: Verify CSR consistency (debug only) ===========================

#if SUBDIV_ENABLE_PROFILING
    for (size_t v = 0; v < numVerts; ++v) 
    {
        if (vertexFaceWritePos[v] != vertexFaceOffsets_[v + 1])
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::WARNING,"VERTEX_FACE_CSR_INCOMPLETE", "Vertex-face array not fully filled", "Vertex " + std::to_string(v));
    }   

    for (size_t e = 0; e < numEdges; ++e) 
    {
        if (edgeFaceWritePos[e] != edgeFaceOffsets_[e + 1])
            SUBDIV_ADD_ERROR(Subdiv::Diagnostics::ErrorSeverity::WARNING, "EDGE_FACE_CSR_INCOMPLETE", "Edge-face array not fully filled", "Edge " + std::to_string(e));
    }
#endif
    
    valid_ = true;
}

void TopologyCache::clear()
{
    valences_.clear();
    boundaryFlags_.clear();
    oneRings_.clear();
    oneRingOffsets_.clear();
    vertexFaces_.clear();
    vertexFaceOffsets_.clear();
    edgeVertices_.clear();
    edgeBoundaryFlags_.clear();
    edgeFaces_.clear();
    edgeFaceOffsets_.clear();
    faceVertices_.clear();
    faceVertexOffsets_.clear();
    faceEdges_.clear();
    faceEdgeOffsets_.clear();

    numBoundaryVertices_ = 0;
    numBoundaryEdges_ = 0;
    
    valid_ = false;
}

std::span<const VertexIndex> TopologyCache::getVertexOneRing(VertexIndex v) const
{
    SUBDIV_ASSERT(valid_, "TopologyCache not built");

    if (v >= oneRingOffsets_.size() - 1) return {};
    
    uint32_t start = oneRingOffsets_[v];
    uint32_t end   = oneRingOffsets_[v + 1];
    return { oneRings_.data() + start, end - start };
}

std::span<const FaceIndex> TopologyCache::getVertexFaces(VertexIndex v) const
{
    SUBDIV_ASSERT(valid_, "TopologyCache not built");
    
    if (v >= vertexFaceOffsets_.size() - 1) return {};
    
    uint32_t start = vertexFaceOffsets_[v];
    uint32_t end   = vertexFaceOffsets_[v + 1];
    return { vertexFaces_.data() + start, end - start };
}

std::span<const FaceIndex> TopologyCache::getEdgeFaces(EdgeIndex e) const
{
    SUBDIV_ASSERT(valid_, "TopologyCache not built");
    
    if (e >= edgeFaceOffsets_.size() - 1) return {};
    
    uint32_t start = edgeFaceOffsets_[e];
    uint32_t end   = edgeFaceOffsets_[e + 1];
    return { edgeFaces_.data() + start, end - start };
}

std::span<const VertexIndex> TopologyCache::getFaceVertices(FaceIndex f) const
{
    SUBDIV_ASSERT(valid_, "TopologyCache not built");
    
    if (f >= faceVertexOffsets_.size() - 1) return {};
    
    uint32_t start = faceVertexOffsets_[f];
    uint32_t end   = faceVertexOffsets_[f + 1];
    return { faceVertices_.data() + start, end - start };
}

std::span<const EdgeIndex> TopologyCache::getFaceEdges(FaceIndex f) const
{
    SUBDIV_ASSERT(valid_, "TopologyCache not built");
    
    if (f >= faceEdgeOffsets_.size() - 1) return {};
    
    uint32_t start = faceEdgeOffsets_[f];
    uint32_t end = faceEdgeOffsets_[f + 1];
    return { faceEdges_.data() + start, end - start };
}

size_t TopologyCache::memoryUsage() const
{
    // byte allocation
    return valences_.size() * sizeof(uint16_t) +
           boundaryFlags_.size() * sizeof(uint8_t) +
           oneRings_.size() * sizeof(VertexIndex) +
           oneRingOffsets_.size() * sizeof(uint32_t) +
           vertexFaces_.size() * sizeof(FaceIndex) +
           vertexFaceOffsets_.size() * sizeof(uint32_t) +
           edgeVertices_.size() * sizeof(std::array<VertexIndex, 2>) +
           edgeBoundaryFlags_.size() * sizeof(uint8_t) +
           edgeFaces_.size() * sizeof(FaceIndex) +
           edgeFaceOffsets_.size() * sizeof(uint32_t) +
           faceVertices_.size() * sizeof(VertexIndex) +
           faceVertexOffsets_.size() * sizeof(uint32_t) +
           faceEdges_.size() * sizeof(EdgeIndex) +
           faceEdgeOffsets_.size() * sizeof(uint32_t);
}

} // namespace Subdiv::Control