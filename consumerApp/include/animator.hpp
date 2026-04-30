#pragma once 

#include <vector>
#include <glm/glm.hpp>

namespace Subdiv::Control { class Mesh; }

/**
 * @brief Animates vertex positions independently of mesh topology.
 *
 * Stores a copy of rest positions and produces a posed buffer each frame.
 * Mesh-agnostic: works for any geometry.
 *
 * Usage:
 *   VertexAnimator anim;
 *   anim.init(mesh);
 * 
 *   anime.update(); //< per frame
 */
class VertexAnimator 
{
public:
    bool isActive()          const { return (active_ && target_); }
    void setActive(bool val);

    void init(Subdiv::Control::Mesh* target);
    void update(float time);

private:
    bool  active_ = false;

    std::vector<glm::vec3> cached_;  // original positions
    Subdiv::Control::Mesh* target_ = nullptr;
};