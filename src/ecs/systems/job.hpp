#pragma once

#include <entt/entity/registry.hpp>

#include "world/society/job.hpp"
#include "world/world.hpp"

namespace dl
{
class JobSystem
{
 public:
  JobSystem(World& world);

  void update(entt::registry& registry, const double delta);

 private:
  World& m_world;

  void m_update_tile_job(const Job& job, const double delta, const entt::entity agent, entt::registry& registry);
  void m_create_or_assign_job_progress(const Job& job, entt::registry& registry);
};
}  // namespace dl
