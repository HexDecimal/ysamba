#pragma once

#include <entt/entity/registry.hpp>
#include <memory>

#include "./scene.hpp"
#include "core/input_manager.hpp"
#include "ecs/systems/action.hpp"
#include "ecs/systems/game.hpp"
#include "ecs/systems/inspector.hpp"
#include "ecs/systems/inventory.hpp"
#include "ecs/systems/job.hpp"
#include "ecs/systems/physics.hpp"
#include "ecs/systems/pickup.hpp"
#include "ecs/systems/render.hpp"
#include "ecs/systems/society.hpp"
#include "ecs/systems/walk.hpp"
#include "graphics/batch.hpp"
#include "ui/ui_manager.hpp"
#include "world/world.hpp"

namespace dl
{
struct GameContext;

class Gameplay : public Scene
{
 public:
  Gameplay(GameContext& game_context);

  void load() override;
  void update() override;
  void render() override;

  void save_world(const std::string& file_path);
  void load_world(const std::string& file_path);

 private:
  enum State
  {
    PLAYING,
    PAUSED,
  };

  entt::registry m_registry;
  World m_world{m_game_context};
  ui::UIManager m_ui_manager{};
  std::shared_ptr<InputManager> m_input_manager = InputManager::get_instance();
  State m_current_state = State::PAUSED;
  entt::entity m_fps_text;
  double m_turn_delay = 0.0;

  GameSystem m_game_system{m_registry, m_world};
  PhysicsSystem m_physics_system{m_world};
  RenderSystem m_render_system{m_world};
  SocietySystem m_society_system{};
  InspectorSystem m_inspector_system{m_world, m_ui_manager};
  ActionSystem m_action_system{m_world, m_ui_manager};
  PickupSystem m_pickup_system{m_world};
  WalkSystem m_walk_system{m_world};
  JobSystem m_job_system{m_world};
  InventorySystem m_inventory_system{m_world, m_ui_manager};

  void m_update_input(GameContext& game_context);
};
}  // namespace dl
