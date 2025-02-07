#pragma once

#include <glm/vec2.hpp>

#include "./color.hpp"
#include "./texture.hpp"
#include "./transform.hpp"

namespace dl
{
class MultiSprite
{
 public:
  std::string resource_id;
  std::shared_ptr<Texture> texture = nullptr;
  std::unique_ptr<Transform> transform = nullptr;
  Color color{0xFFFFFFFF};
  uint32_t frame = 0;
  FrameAngle frame_angle = FrameAngle::Parallel;

  MultiSprite() {}
  MultiSprite(const std::string& resource_id, const uint32_t frame, const uint32_t width, const uint32_t height)
      : resource_id(resource_id), frame(frame), m_width(width), m_height(height)
  {
  }

  uint32_t get_frame() const { return frame; };
  glm::vec2 get_size() const;
  // Get top-left, top-right, bottom-right and bottom-left uv coordinates
  std::array<glm::vec2, 4> get_texture_coordinates() const;

 private:
  int m_width = 0;
  int m_height = 0;
};
}  // namespace dl
