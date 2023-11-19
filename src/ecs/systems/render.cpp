#include <tgmath.h>
#include <iostream>
#include "./render.hpp"
#include "../../world/world.hpp"
#include "../../world/camera.hpp"
#include "../components/position.hpp"
#include "../components/visibility.hpp"
#include "../components/selectable.hpp"
#include "../../graphics/renderer.hpp"
#include "../../graphics/camera.hpp"

namespace dl
{
  RenderSystem::RenderSystem(World& world)
    : m_world(world)
  {

  }

  void RenderSystem::update(entt::registry &registry, Renderer& renderer, const ViewCamera& camera)
  {
    // TODO: Cache values so we don't have to calculate them every frame
    const auto& camera_position = camera.get_position();
    const auto& camera_dimensions = camera.get_dimensions();
    const auto& world_texture_id = m_world.get_texture_id();
    const auto& world_texture = renderer.get_texture(world_texture_id);
    const auto tile_width = world_texture->get_frame_width(0);
    const auto tile_height = world_texture->get_frame_height(0);
    const auto horizontal_tiles = std::ceil(camera_dimensions.x / tile_width);
    const auto vertical_tiles = std::ceil(camera_dimensions.y / tile_height);

    renderer.init("world");

    for (int i = 0; i < horizontal_tiles; ++i)
    {
      for (int j = 0; j < vertical_tiles; ++j)
      {
        const auto& tile = m_world.get(i + camera_position.x, j + camera_position.y, 0.0);
        const auto& sprite = std::make_shared<Sprite>(world_texture_id, tile.id);

        if (sprite->texture == nullptr)
        {
          sprite->texture = world_texture;
        }

        const auto sprite_size = sprite->get_size();
        renderer.batch("world", sprite, i*sprite_size.x, j*sprite_size.y, 0.0);
      }
    }

    auto view = registry.view<const Position, const Visibility, const Selectable>();
    view.each([&renderer, camera_position](const auto &position, const auto &visibility, const auto& selectable) { 
      if (visibility.sprite->texture == nullptr)
      {
        visibility.sprite->texture = renderer.get_texture(visibility.sprite->resource_id);
      }

      if (selectable.selected)
      {
        visibility.sprite->set_frame(1);
      }

      const auto sprite_size = visibility.sprite->get_size();
      const auto position_x = static_cast<int>(std::round(position.x - camera_position.x)) * sprite_size.x;
      const auto position_y = static_cast<int>(std::round(position.y - camera_position.y)) * sprite_size.y;

      renderer.batch("world", visibility.sprite, position_x, position_y, 1.);
    });
    renderer.finalize("world");
  }
}

