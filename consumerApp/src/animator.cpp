#include "animator.hpp"
#include "control/mesh.hpp"
#include <cmath>

void VertexAnimator::setActive(bool val)
{
    if(!target_ && val == active_) return;
    if(active_ && !val)
        target_->setPositions(cached_);
    active_ = val;
}

void VertexAnimator::init(Subdiv::Control::Mesh *target)
{
    if(!target) return;

    target_ = target;
    cached_ = target->getPositions();
}

void VertexAnimator::update(float time)
{
    if(!active_) return;

    for (size_t i = 0; i < cached_.size(); ++i)
    {
        const glm::vec3& r = cached_[i];

        // Radial pulse: vertices breathe outward along their rest normal (origin→vertex)
        float dist   = glm::length(r);
        float phase  = dist * 1.5f; // stagger by distance from origin
        float pulse  = 0.15f * std::sin(time * 2.0f + phase);

        glm::vec3 dir = (dist > 1e-5f) ? r / dist : glm::vec3(0, 1, 0);
        glm::vec3 pos = r + dir * pulse;
        target_->setPosition(i, pos);
    }
}