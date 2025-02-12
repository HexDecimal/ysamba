#include "./inventory.hpp"

#include <spdlog/spdlog.h>

#include <entt/core/hashed_string.hpp>

#include "ecs/components/carried_items.hpp"
#include "ecs/components/item.hpp"
#include "ecs/components/position.hpp"
#include "ecs/components/selectable.hpp"
#include "ecs/components/weared_items.hpp"
#include "ecs/components/wielded_items.hpp"
#include "ui/compositions/inventory.hpp"
#include "ui/ui_manager.hpp"
#include "world/world.hpp"

namespace dl
{
InventorySystem::InventorySystem(World& world, ui::UIManager& ui_manager) : m_world(world), m_ui_manager(ui_manager)
{
  auto on_select = [this](const int i) { m_state = static_cast<InventoryState>(i); };
  m_inventory = m_ui_manager.emplace<ui::Inventory>(on_select);
}

void InventorySystem::update(entt::registry& registry)
{
  if (m_state == InventoryState::Open)
  {
    m_update_inventory(registry);
  }
  else if (m_state == InventoryState::Closed)
  {
    m_update_closed_inventory(registry);
  }
}

void InventorySystem::m_update_inventory(entt::registry& registry)
{
  using namespace entt::literals;

  m_open_inventory(registry);

  if (!m_input_manager.is_context("inventory"_hs))
  {
    return;
  }

  if (m_input_manager.poll_action("close_inventory"_hs))
  {
    m_dispose();
  }
}

void InventorySystem::m_update_closed_inventory(entt::registry& registry)
{
  using namespace entt::literals;

  const auto& current_context = m_input_manager.get_current_context();

  if (current_context == nullptr || current_context->key != "gameplay"_hs)
  {
    return;
  }

  if (m_input_manager.poll_action("open_inventory"_hs))
  {
    m_update_items(registry);

    m_state = InventoryState::Open;
    m_input_manager.push_context("inventory"_hs);
  }
}

void InventorySystem::m_open_inventory(entt::registry& registry)
{
  assert(m_inventory != nullptr);

  if (m_inventory->state == ui::UIComponent::State::Hidden)
  {
    m_weared_items_names.clear();
    m_weared_items_names.reserve(m_weared_items.size());
    m_carried_items_names.clear();
    m_carried_items_names.reserve(m_carried_items.size());

    for (const auto entity : m_weared_items)
    {
      if (!registry.all_of<Item>(entity))
      {
        continue;
      }

      const auto& item = registry.get<Item>(entity);
      const auto& item_data = m_world.get_item_data(item.id);
      m_weared_items_names.push_back({static_cast<uint32_t>(entity), item_data.name});
    }

    for (const auto entity : m_carried_items)
    {
      if (!registry.all_of<Item>(entity))
      {
        continue;
      }

      const auto& item = registry.get<Item>(entity);
      const auto& item_data = m_world.get_item_data(item.id);
      m_carried_items_names.push_back({static_cast<uint32_t>(entity), item_data.name});
    }

    m_inventory->set_weared_items(m_weared_items_names);
    m_inventory->set_carried_items(m_carried_items_names);
    m_inventory->show();
  }
}

void InventorySystem::m_close_inventory() { m_inventory->hide(); }

void InventorySystem::m_dispose()
{
  m_close_inventory();
  m_state = InventoryState::Closed;
  m_input_manager.pop_context();
}

void InventorySystem::m_update_items(entt::registry& registry)
{
  m_carried_items.clear();
  m_weared_items.clear();
  std::vector<entt::entity> selected_entities{};
  auto view = registry.view<Selectable>();

  for (const auto entity : view)
  {
    const auto& selectable = registry.get<Selectable>(entity);

    if (!selectable.selected)
    {
      continue;
    }

    selected_entities.push_back(entity);
  }

  if (!selected_entities.empty())
  {
    for (const auto entity : selected_entities)
    {
      if (registry.all_of<WieldedItems>(entity))
      {
        const auto& items = registry.get<WieldedItems>(entity);
        const auto left_hand = items.left_hand;
        const auto right_hand = items.right_hand;

        if (registry.valid(left_hand))
        {
          m_weared_items.push_back(left_hand);
        }
        if (registry.valid(right_hand) && right_hand != left_hand)
        {
          m_weared_items.push_back(right_hand);
        }
      }
      if (registry.all_of<WearedItems>(entity))
      {
        const auto& items = registry.get<WearedItems>(entity);
        m_weared_items.insert(m_weared_items.end(), items.items.begin(), items.items.end());
      }
      if (registry.all_of<CarriedItems>(entity))
      {
        const auto& items = registry.get<CarriedItems>(entity);
        m_carried_items.insert(m_carried_items.begin(), items.items.begin(), items.items.end());
      }
    }
    return;
  }

  // TODO: Implement society inventory
}
}  // namespace dl
