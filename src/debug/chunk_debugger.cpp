#include "./chunk_debugger.hpp"

#include "ecs/components/rectangle.hpp"
#include "imgui.h"
#include "scenes/gameplay.hpp"

namespace
{
std::vector<entt::entity> chunk_quads{};
}

namespace dl
{
ChunkDebugger::ChunkDebugger(Gameplay& gameplay) : m_gameplay(gameplay) {}

void ChunkDebugger::update()
{
  if (!open)
  {
    return;
  }

  const auto& grid_size = m_gameplay.m_world.get_tile_size();

  for (const auto& chunk : m_gameplay.m_world.chunk_manager.chunks)
  {
    bool found = false;

    for (const auto entity : chunk_quads)
    {
      const auto& position = m_gameplay.m_registry.get<Position>(entity);

      if (position.x == chunk->position.x && position.y == chunk->position.y && position.z == chunk->position.z)
      {
        auto& rect = m_gameplay.m_registry.get<Rectangle>(entity);

        if (chunk->active && rect.quad.color.int_color != 0xCC8844FF)
        {
          rect.quad.color.set(0xCC8844FF);
        }
        else if (!chunk->active && rect.quad.color.int_color != 0x4477AAFF)
        {
          rect.quad.color.set(0x4477AAFF);
        }

        found = true;
        break;
      }
    }

    if (!found)
    {
      auto color = chunk->active ? 0xCC8844FF : 0x4477AAFF;
      auto entity = m_gameplay.m_registry.create();
      m_gameplay.m_registry.emplace<Rectangle>(entity,
                                               m_gameplay.m_world.chunk_manager.chunk_size.x * grid_size.x,
                                               m_gameplay.m_world.chunk_manager.chunk_size.y * grid_size.y,
                                               color);
      m_gameplay.m_registry.emplace<Position>(entity, chunk->position.x, chunk->position.y, chunk->position.z);
      chunk_quads.push_back(entity);
    }
  }

  for (const auto entity : chunk_quads)
  {
    const auto& position = m_gameplay.m_registry.get<Position>(entity);
    bool found = false;

    for (const auto& chunk : m_gameplay.m_world.chunk_manager.chunks)
    {
      if (position.x == chunk->position.x && position.y == chunk->position.y && position.z == chunk->position.z)
      {
        found = true;
        break;
      }
    }

    if (!found)
    {
      m_gameplay.m_registry.destroy(entity);
    }
  }

  std::erase_if(chunk_quads, [this](const auto entity) { return !m_gameplay.m_registry.valid(entity); });
}
}  // namespace dl
