#include "./button.hpp"

#include <spdlog/spdlog.h>

#include "./container.hpp"
#include "./label.hpp"
#include "graphics/batch.hpp"

namespace dl::ui
{
Button::Button() : UIComponent()
{
  m_container = emplace<Container>();
  m_label = m_container->emplace<Label>();
}

void Button::init()
{
  m_container->set_size(size);
  m_container->set_color(background_color);
  m_label->set_text(text);
  m_label->x_alignment = label_x_alignment;
  m_label->y_alignment = label_y_alignment;
}

void Button::update(std::vector<glm::mat4>& matrix_stack)
{
  if (!on_click)
  {
    return;
  }

  if (m_input_manager.has_clicked(InputManager::MouseButton::Left))
  {
    const auto& matrix = matrix_stack.back();
    const auto top_left = matrix * glm::vec4(0.f, 0.f, 1.f, 1.f);
    const auto& mouse_position = m_input_manager.get_mouse_position();

    if (mouse_position.x > top_left.x && mouse_position.x < top_left.x + size.x && mouse_position.y > top_left.y &&
        mouse_position.y < top_left.y + size.y)
    {
      on_click();
    }
  }
}

}  // namespace dl::ui
