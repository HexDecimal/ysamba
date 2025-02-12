#include "./component.hpp"

#include <spdlog/spdlog.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/asset_manager.hpp"
#include "core/display.hpp"
#include "graphics/renderer.hpp"

namespace dl::ui
{
InputManager& UIComponent::m_input_manager = InputManager::get_instance();

void UIComponent::m_init()
{
  init();

  for (const auto& child : children)
  {
    if (!child->has_initialized)
    {
      child->m_init();
    }
  }

  has_initialized = true;
  dirty = true;
}

void UIComponent::m_update_geometry()
{
  if (state == State::Hidden)
  {
    return;
  }

  if (dirty)
  {
    const auto& window_size = Display::get_window_size();
    const auto& matrix = m_context.matrix_stack->back();

    glm::vec3 transformed_position{position.x + margin.x, position.y + margin.y, position.z};

    if (x_alignment == XAlignement::Right)
    {
      const int anchor_w = (parent == nullptr || parent->size.x == 0) ? window_size.x : parent->size.x;
      transformed_position.x = anchor_w - size.x - position.x - margin.x;
    }
    else if (x_alignment == XAlignement::Center)
    {
      const int anchor_w = (parent == nullptr || parent->size.x == 0) ? window_size.x : parent->size.x;
      transformed_position.x = anchor_w / 2 - size.x / 2 + position.x + margin.x;
    }
    if (y_alignment == YAlignement::Bottom)
    {
      const int anchor_h = (parent == nullptr || parent->size.y == 0) ? window_size.y : parent->size.y;
      transformed_position.y = anchor_h - size.y - position.y - margin.y;
    }
    else if (y_alignment == YAlignement::Center)
    {
      const int anchor_h = (parent == nullptr || parent->size.y == 0) ? window_size.y : parent->size.y;
      transformed_position.y = anchor_h / 2 - size.y / 2 + position.y + margin.y;
    }
    m_transform_matrix =
        glm::translate(matrix, glm::vec3(transformed_position.x, transformed_position.y, transformed_position.z));
    const auto top_left = m_transform_matrix * glm::vec4(0.f, 0.f, 1.f, 1.f);
    absolute_position.x = top_left.x;
    absolute_position.y = top_left.y;
  }
}

void UIComponent::m_update()
{
  if (!has_initialized)
  {
    m_init();
  }

  m_update_geometry();

  if (m_is_positioned())
  {
    m_context.matrix_stack->push_back(m_transform_matrix);
  }

  update_geometry();

  if (state == State::Visible)
  {
    update();
  }

  for (const auto& child : children)
  {
    child->dirty = dirty;
    child->opacity = opacity;
    child->m_update();
  }

  if (m_is_positioned())
  {
    m_context.matrix_stack->pop_back();
  }

  dirty = false;
}

void UIComponent::render(Batch& batch)
{
  if (state == State::Hidden)
  {
    return;
  }

  for (const auto& child : children)
  {
    child->render(batch);
  }
}

void UIComponent::show() { state = State::Visible; }

void UIComponent::hide() { state = State::Hidden; }

void UIComponent::propagate_state()
{
  for (auto& child : children)
  {
    child->state = state;
    child->propagate_state();
  }
}

bool UIComponent::m_is_positioned()
{
  return position.x != 0 || position.y != 0 || x_alignment != XAlignement::Left || y_alignment != YAlignement::Top;
}
}  // namespace dl::ui
