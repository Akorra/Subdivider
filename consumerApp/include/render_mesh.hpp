#pragma once

#include "control/mesh.hpp"

/**
 * @brief Rendering wrapper for Mesh.
 * 
 * Generates GPU-ready index buffers for rendering:
 * - Triangle indices (for solid rendering)
 * - Wireframe indices (for edge rendering)
 * 
 * Usage:
 *   Mesh mesh;
 *   // ... build mesh ...
 *   
 *   RenderMesh renderMesh(mesh);
 *   renderMesh.build();
 *   
 *   // Upload to GPU
 *   glBufferData(..., renderMesh.getTriangleIndicesData(), ...);
 */

class RenderMesh
{
public:
    /**
     * @brief Construct from mesh reference.
     * Does not take ownership. Mesh must outlive RenderMesh.
     */
    explicit RenderMesh(const Subdiv::Control::Mesh& mesh);
    
    /**
     * @brief Build index buffers from mesh.
     * Call after mesh topology changes.
     */
    void build();
    
    /**
     * @brief Clear all index buffers.
     */
    void clear();
    
    /**
     * @brief Check if indices are valid.
     */
    bool isValid() const { return valid_; }
    
    // Triangle Indices (for solid rendering) =================================
    
    /**
     * @brief Get triangle indices (fan triangulation).
     * Each face is triangulated as a fan from the first vertex.
     */
    const std::vector<uint32_t>& getTriangleIndices() const { 
        return triangleIndices_; 
    }
    
    const void* getTriangleIndicesData() const { 
        return triangleIndices_.data(); 
    }
    
    size_t getTriangleIndicesBytes() const { 
        return triangleIndices_.size() * sizeof(uint32_t); 
    }
    
    size_t numTriangles() const { 
        return triangleIndices_.size() / 3; 
    }
    
    // Wireframe Indices (for edge rendering) =================================
    
    /**
     * @brief Get wireframe indices (unique edges).
     * Each edge appears once as [v0, v1].
     */
    const std::vector<uint32_t>& getWireframeIndices() const { 
        return wireframeIndices_; 
    }
    
    const void* getWireframeIndicesData() const { 
        return wireframeIndices_.data(); 
    }
    
    size_t getWireframeIndicesBytes() const { 
        return wireframeIndices_.size() * sizeof(uint32_t); 
    }
    
    size_t numWireframeLines() const { 
        return wireframeIndices_.size() / 2; 
    }
    
    // Statistics =============================================================
    
    size_t getMemoryUsage() const;
    
private:
    const Subdiv::Control::Mesh& mesh_;
    
    std::vector<uint32_t> triangleIndices_;
    std::vector<uint32_t> wireframeIndices_;
    
    bool valid_ = false;
    
    void buildTriangleIndices();
    void buildWireframeIndices();
};
