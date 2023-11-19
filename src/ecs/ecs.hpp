#pragma once

#include <entt/entity/registry.hpp>
#include "./systems/game.hpp"
#include "./systems/physics.hpp"
#include "./systems/render.hpp"
#include "./systems/society.hpp"

namespace dl
{
  class ViewCamera;
  class World;
  class Renderer;

  class ECS
  {
    public:
      ECS(entt::registry& registry, World& world, ViewCamera& camera);
      void load();
      void update(const uint32_t delta);
      void render(Renderer& renderer);

    private:
      entt::registry& m_registry;
      World& m_world;
      ViewCamera& m_camera;
      GameSystem m_game_system{m_registry, m_world};
      PhysicsSystem m_physics_system{m_world};
      RenderSystem m_render_system{m_world};
      SocietySystem m_society_system;
      int m_turn_delay{0};
  };
}

