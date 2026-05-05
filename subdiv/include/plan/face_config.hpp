#pragma once

#include <vector>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <functional>

namespace Subdiv::Plan
{

/**
 * wire layout of config buffer
 * 
 * byte 0 : valence
 * byte 1 : boundaryRules (none = 0, smooth, sharp pinned, ...)
 * byte 2 : boundaryMask  (bit i - corner i is on boundary edge)
 * byte 3 : rotation      (canonical rotation - corner that ended up at position 0) 
 * 
 * bytes 4 .. 4+ n*4 - 1 :
 *      Per-corner record, n records of 4 bytes each:
 *          [0] corner_valence    (vertex valence, clamped to 255)
 *          [1] vertex_sharpness  (0=smooth, 255=inf-sharp corner)
 *          [2] edge_sharpness    (sharpness of edge i→i+1, quantised)
 *          [3] edge_tag          (0=smooth, 1=crease, 2=semi, from EdgeTag)
 */

static constexpr size_t CONFIG_HEADER_SIZE = 4;
static constexpr size_t CONFIG_CORNER_SIZE = 4; // per corner

inline size_t  config_byte_size(int valence) 
{ 
    return CONFIG_HEADER_SIZE + valence * CONFIG_CORNER_SIZE; 
}

// float sharpness [0, inf[ to uint8. (granularity of ~0.02 sharpness units)
inline uint8_t quantise_sharpness(float s)
{
    if (s <= 0.0f) return 0;
    if (s >= 5.0f) return 255;
    return static_cast<uint8_t>(s * 51.0f);
}

/**
 * @brief FaceConfigBuilder
 * 
 * canonical description of a face's 1-ring topology.
 * 
 * N-Gon are dealt with by rotating untill the lowest vallence extraordinary vertex comes first to maximize plan sharing.
 * Quad face with EV at corner 2 maps to same plan as face with EV at corner 0 - plan stores rotation offset so traversal
 * can un-rotate the uv domain at query time.
 * 
 * flat byte buffer (std::vector<uint8_t>) for single FNV pass hashing.
 */
struct FaceConfig 
{
    static std::vector<uint8_t> build(
        uint8_t valence, 
        uint8_t boundary_rule, 
        uint8_t boundary_mask,
        const std::vector<uint8_t>& corner_valences,    // length = valence 
        const std::vector<uint8_t>& vertex_sharpnesses, // length = valence 
        const std::vector<uint8_t>& edge_sharpnesses,   // length = valence 
        const std::vector<uint8_t>& edge_tags)          // length = valence 
    {
        assert((int)corner_valences.size()    == valence);
        assert((int)vertex_sharpnesses.size() == valence);
        assert((int)edge_sharpnesses.size()   == valence);
        assert((int)edge_tags.size()          == valence);
        
        std::vector<uint8_t> buf(config_byte_size(valence), 0);
        buf[0] = valence;
        buf[1] = boundary_rule;
        buf[2] = boundary_mask;
        buf[3] = 0; //< canonical rotation
        
        for (int i = 0; i < valence; ++i) 
        {
            uint8_t* c = &buf[CONFIG_HEADER_SIZE + i * CONFIG_CORNER_SIZE];
            c[0] = corner_valences[i];
            c[1] = vertex_sharpnesses[i];
            c[2] = edge_sharpnesses[i];
            c[3] = edge_tags[i];
        }
        
        canonicalize(buf, valence);
        
        return buf;
    }

private:
    // Rotate the per-corner arrays so that the configuration is in canonical form. 
    // Produces the smallest byte sequence such that topologically equivalent faces map to the same key.
    static void canonicalize(std::vector<uint8_t>& fc, int n) 
    {
        auto corner = [&](int rot, int i) -> const uint8_t* {
            return &fc[CONFIG_HEADER_SIZE + ((rot + i) % n) * CONFIG_CORNER_SIZE];
        };

        // Find the rotation offset that gives lexicographical min sequence.
        int best = 0;
        for (int r = 1; r < n; ++r) 
        {
            // Compare rotation r against current best lexicographically
            for (int i = 0; i < n; ++i) 
            {
                const uint8_t* a = corner(best, i);
                const uint8_t* b = corner(r,    i);

                int cmp = memcmp(b, a, CONFIG_CORNER_SIZE);
                if (cmp < 0) { best = r; break; }
                if (cmp > 0) break;
                // equal so far, continue to next corner
            }
        }
        
        fc[3] = static_cast<uint8_t>(best); // record canonical rotation

        if (best == 0) return;
        
        // Apply rotation: copy into temp buffer then write back
        std::vector<uint8_t> tmp(n * CONFIG_CORNER_SIZE);
        for (int i = 0; i < n; ++i) 
            memcpy(&tmp[i * CONFIG_CORNER_SIZE], corner(best, i), CONFIG_CORNER_SIZE);
        memcpy(&fc[CONFIG_HEADER_SIZE], tmp.data(), n * CONFIG_CORNER_SIZE);
        
        // Also rotate boundary_mask bits
        uint8_t bm = 0, orig_bm = fc[2];
        for (int i = 0; i < n; ++i)
            if (orig_bm & (1 << ((i + best) % n)))
                bm |= static_cast<uint8_t>(1 << i);
        fc[2] = bm;
    }
};

// Hash and equality for use as unordered_map key. Keys are stored as vector<uint8_t> (the raw buffer).
struct FaceConfigHash 
{
    size_t operator()(const std::vector<uint8_t>& buf) const noexcept 
    {
        // FNV-1a — fast, good distribution, no dependencies
        size_t h = 0xcbf29ce484222325ULL;
        for (uint8_t b : buf)
            h = (h ^ b) * 0x100000001b3ULL;
        return h;
    }
};

/**
 * @brief Accessor Helpers 
 * 
 * Some helpers provided moslty for readibility
 */

 //! face confige header data:
inline int     cfg_valence      (const std::vector<uint8_t>& cfg) { return cfg[0]; }
inline uint8_t cfg_boundary_rule(const std::vector<uint8_t>& cfg) { return cfg[1]; }
inline uint8_t cfg_boundary_mask(const std::vector<uint8_t>& cfg) { return cfg[2]; }
inline uint8_t cfg_rotation     (const std::vector<uint8_t>& cfg) { return cfg[3]; }

//! face config corner data:
inline const uint8_t* cfg_corner(const std::vector<uint8_t>& cfg, int i) { return &cfg[CONFIG_HEADER_SIZE + i * CONFIG_CORNER_SIZE]; }

//! corner_valence, vertex_sharpness, edge_sharpness, edge_tag for corner i:
inline uint8_t cfg_corner_valence(const std::vector<uint8_t>& cfg, int i) { return cfg_corner(cfg, i)[0]; }
inline uint8_t cfg_corner_vsharp (const std::vector<uint8_t>& cfg, int i) { return cfg_corner(cfg, i)[1]; }
inline uint8_t cfg_edge_sharpness(const std::vector<uint8_t>& cfg, int i) { return cfg_corner(cfg, i)[2]; }
inline uint8_t cfg_edge_tag      (const std::vector<uint8_t>& cfg, int i) { return cfg_corner(cfg, i)[3]; }

} // Subdiv::Plan 