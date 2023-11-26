#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "core/maths/vector.hpp"

namespace dl
{
class Display;

enum class CameraState
{
  IDLE,
  FOLLOW,
};

class Camera
{
 public:
  Camera();
  Camera(const Display& display);

  void move(const Vector3& quantity);
  const Vector3& get_position() const { return m_position; }
  const Vector2i get_position_in_tiles() const;
  glm::mat4 get_view_matrix() const
  {
    return glm::lookAt(glm::vec3(m_position.x, m_position.y, m_position.z), m_front, m_up);
  }
  glm::mat4 get_default_view_matrix() const { return glm::lookAt({0.f, 0.f, 500.f}, {0.f, 0.f, -1.f}, m_up); }
  glm::mat4 get_projection_matrix() const
  {
    return glm::ortho(m_frustrum_left, m_frustrum_right, m_frustrum_bottom, m_frustrum_top, m_near, m_far);
  }
  glm::mat4 get_vp_matrix() const
  {
    return (glm::ortho(m_frustrum_left, m_frustrum_right, m_frustrum_bottom, m_frustrum_top, m_near, m_far) *
            glm::lookAt(glm::vec3(m_position.x, m_position.y, m_position.z), m_front, m_up));
  }
  const Vector3& get_saved_position() const { return m_saved_position; }
  const Vector2& get_size() const { return m_size; }
  const Vector2i& get_size_in_tiles() const { return m_size_in_tiles; }
  const Vector2i& get_tile_size() const { return m_tile_size; }
  void set_position(const Vector3& position);
  void set_size(const Vector2& size);
  void set_frustrum(const float left, const float right, const float bottom, const float top);
  void set_tile_size(const Vector2i& size);
  void save_position() { m_saved_position = m_position; }

 private:
  float m_frustrum_left = 0.0f;
  float m_frustrum_right = 0.0f;
  float m_frustrum_bottom = 0.0f;
  float m_frustrum_top = 0.0f;
  float m_near = 0.1f;
  float m_far = 1000.0f;
  Vector3 m_position{0.0, 0.0, 500.0};
  glm::vec3 m_front{0.0f, 0.0f, -1.0f};
  const glm::vec3 m_up{0.0f, 1.0f, 0.0f};
  Vector3 m_saved_position{0., 0., 0.};
  Vector2 m_size{};
  Vector2i m_tile_size{0, 0};
  Vector2i m_size_in_tiles{0, 0};
};
}  // namespace dl
