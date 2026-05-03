#pragma once

#include <vector>
#include <cstdint>
#include <cstring>
#include <functional>

namespace Subdiv::Plan
{

/**
 * @brief FaceConfig
 * 
 * canonical description of a face's 1-ring topology.
 * 
 * N-Gon are dealt with by rotating untill the lowest vallence 
 * extraordinary vertex comes first to maximize plan sharing.
 * 
 * flat byte buffer for hashing 
 */
struct FaceConfig {
    uint8_t  face_valence;      // number of corners (3, 4, 5, ...)
    uint8_t  boundary_rule;     // boundary interpolation mode
    uint8_t  boundary_mask;     // bit i = corner i is on boundary

    // Per-corner data, length = face_valence each:
    //   corner_valence[i]    -- vertex valence at corner i
    //   vertex_sharpness[i]  -- 0=smooth, 255=inf sharp
    //   edge_sharpness[i]    -- sharpness of edge i->i+1
    // Stored interleaved: [val0, vsharp0, esharp0, val1, vsharp1, esharp1, ...]
    uint8_t  per_corner[0];     // flexible array member

    // ---- Factory + storage ----
    static size_t byteSize(int valence) { return offsetof(FaceConfig, per_corner) + valence * 3; }

    static std::vector<uint8_t> make(int valence, uint8_t boundary_rule, uint8_t boundary_mask,
        const std::vector<uint8_t>&  corner_valences, const std::vector<uint8_t>&  vertex_sharpnesses, const std::vector<uint8_t>&  edge_sharpnesses)
    {
        std::vector<uint8_t> buf(byteSize(valence), 0);
        FaceConfig* fc = reinterpret_cast<FaceConfig*>(buf.data());
        fc->face_valence   = (uint8_t)valence;
        fc->boundary_rule  = boundary_rule;
        fc->boundary_mask  = boundary_mask;
        for (int i = 0; i < valence; ++i) {
            fc->per_corner[i * 3 + 0] = corner_valences[i];
            fc->per_corner[i * 3 + 1] = vertex_sharpnesses[i];
            fc->per_corner[i * 3 + 2] = edge_sharpnesses[i];
        }
        canonicalize(fc, valence);
        return buf;
    }

private:
    // Rotate the per-corner arrays so that the configuration is in canonical form. 
    // Produces the smallest byte sequence such that topologically equivalent faces map to the same key.
    static void canonicalize(FaceConfig* fc, int n) 
    {
        // Find the rotation offset that gives lexicographical min sequence.
        int best = 0;
        for (int r = 1; r < n; ++r) 
        {
            for (int i = 0; i < n; ++i) 
            {
                int ai = (best + i) % n;
                int bi = (r    + i) % n;
                
                uint8_t a0 = fc->per_corner[ai*3], b0 = fc->per_corner[bi*3];
                if (b0 < a0) { best = r; break; }
                if (b0 > a0) break;
                
                // tie-break with next fields...
                uint8_t a1 = fc->per_corner[ai*3+1], b1 = fc->per_corner[bi*3+1];
                if (b1 < a1) { best = r; break; }
                if (b1 > a1) break;
                
                uint8_t a2 = fc->per_corner[ai*3+2], b2 = fc->per_corner[bi*3+2];
                if (b2 < a2) { best = r; break; }
                if (b2 > a2) break;
            }
        }

        if (best == 0) return;
        
        // Apply rotation in-place
        std::vector<uint8_t> tmp(n * 3);
        for (int i = 0; i < n; ++i)
            memcpy(&tmp[i*3], &fc->per_corner[((i+best)%n)*3], 3);
        memcpy(fc->per_corner, tmp.data(), n * 3);
        
        // Also rotate boundary_mask
        uint8_t bm = 0;
        for (int i = 0; i < n; ++i)
            if (fc->boundary_mask & (1 << ((i + best) % n)))
                bm |= (1 << i);
        fc->boundary_mask = bm;
    }
};

// Hash and equality for use as unordered_map key. Keys are stored as vector<uint8_t> (the raw buffer).
struct FaceConfigHash 
{
    size_t operator()(const std::vector<uint8_t>& buf) const noexcept 
    {
        size_t h = 0xcbf29ce484222325ULL;
        for (uint8_t b : buf)
            h = (h ^ b) * 0x100000001b3ULL;
        return h;
    }
};

} // Subdiv::Plan 