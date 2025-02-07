#pragma once

#include <entt/entity/registry.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <optional>
#include <vector>

#include "core/input_manager.hpp"
#include "core/maths/vector.hpp"
#include "ui/animation.hpp"
#include "ui/context.hpp"

namespace dl
{
class Batch;
class Renderer;
class AssetManager;
}  // namespace dl

namespace dl::ui
{
enum class YAlignement
{
  Bottom,
  Center,
  Top,
};

enum class XAlignement
{
  Left,
  Center,
  Right,
};

enum class Placement
{
  Absolute,
  Relative,
};

class UIComponent
{
 public:
  friend class UIManager;

  enum class State
  {
    Hidden,
    Visible,
    Animating,
  };

  bool has_initialized = false;
  bool dirty = true;
  State state = State::Visible;
  Vector3i position{};
  Vector3i absolute_position{};
  Vector2i size{};
  Vector2i margin{};
  XAlignement x_alignment = XAlignement::Left;
  YAlignement y_alignment = YAlignement::Top;
  Placement placement = Placement::Relative;
  entt::entity animations = entt::null;
  double opacity = 1.0;

  UIComponent* parent = nullptr;
  std::vector<std::unique_ptr<UIComponent>> children;

  UIComponent(UIContext& context) : m_context(context) {}
  virtual ~UIComponent() {}

  virtual void init() {}
  virtual void update_geometry() {}
  virtual void update() {}
  virtual void render(Batch& batch);
  virtual void show();
  virtual void hide();

  // Propagates state to children
  void propagate_state();

  template <typename T, typename... Args>
  T* emplace(Args&&... args)
  {
    auto component = std::make_unique<T>(m_context, std::forward<Args>(args)...);
    component->parent = this;
    children.push_back(std::move(component));
    return dynamic_cast<T*>(&(*children.back()));
  }

  template <typename T, typename... Args>
  void erase(const T* component)
  {
    children.erase(std::find_if(children.begin(), children.end(), [component](std::unique_ptr<UIComponent>& c) {
      return c.get() == component;
    }));
  }

  template <typename T, typename... Args>
  void animate(Args&&... args)
  {
    if (!m_context.animator->valid(animations))
    {
      animations = m_context.animator->create();
      m_context.animator->emplace<AnimationTarget>(animations, this);
    }

    m_context.animator->emplace<T>(animations, std::forward<Args>(args)...);

    if (state != State::Animating)
    {
      state = State::Animating;
    }
  }

 protected:
  UIContext& m_context;
  static InputManager& m_input_manager;
  glm::mat4 m_transform_matrix{};

  void m_init();
  void m_update();
  void m_update_geometry();
  bool m_is_positioned();
};

}  // namespace dl::ui
