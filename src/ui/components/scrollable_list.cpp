#include "./scrollable_list.hpp"

#include <spdlog/spdlog.h>

#include "./button_list.hpp"
#include "./container.hpp"
#include "./scrollable.hpp"
#include "core/display.hpp"
#include "core/maths/vector.hpp"

namespace dl::ui
{
ScrollableList::ScrollableList()
{
  m_container = emplace<Container>();
  m_scrollable = m_container->emplace<Scrollable>();
  m_list = m_scrollable->emplace<ButtonList>();
}

void ScrollableList::init()
{
  m_container->set_size(size);
  m_container->set_color(color);
  m_scrollable->size = size;
  m_list->button_size = Vector2i{size.x, 32};
  m_has_initialized = true;
}

void ScrollableList::set_items(const ItemList& items) { m_list->set_items(items); }

void ScrollableList::set_on_select(const std::function<void(const uint32_t)>& on_select)
{
  m_list->set_on_select(on_select);
}

}  // namespace dl::ui
