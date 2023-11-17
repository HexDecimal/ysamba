#pragma once

#include <string>
#include <array>
#include <glm/vec2.hpp>
#include "./asset.hpp"

namespace dl
{
enum class TextureType
{
  DIFFUSE,
  SPECULAR,
  NORMAL,
};

class Texture : public Asset
{
public:
  // Create single texture
  Texture (const std::string& filepath, const TextureType type);
  // Create uniform texture atlas
  Texture (const std::string& filepath, const TextureType type, const int horizontal_frames, const int vertical_frames);
  // Create empty texture in order to load it later
  Texture (const int width, const int height, const TextureType type);
  ~Texture();

  void load (const std::string& filepath);
  void bind();
  void unbind();
  inline unsigned int get_id() const { return m_id; }
  inline int get_width() const { return m_width; }
  inline int get_height() const { return m_height; }
  inline int get_horizontal_frames() const { return m_horizontal_frames; }
  inline int get_vertical_frames() const { return m_vertical_frames; }
  inline TextureType get_type() const { return m_type; }
  // TODO: Implement irregular frame calculations
  float get_frame_width (const int frame) const;
  float get_frame_height (const int frame) const;
  // Get top-left, top-right, bottom-right and bottom-left uv coordinates
  std::array<glm::vec2, 4> get_frame_coords (const int frame) const;
  /* void set_custom_uv (const glm::vec2& uv, const float width, const float height); */

protected:
  const TextureType m_type;
  const int m_horizontal_frames;
  const int m_vertical_frames;
  unsigned int m_id = 0;
  int m_width       = 0;
  int m_height      = 0;
  /* bool m_has_custom_uv = false; */
  /* glm::vec2 m_custom_uv{}; */
  /* float m_frame_width = 0.0f; */
  /* float m_frame_height = 0.0f; */

  void m_load_empty();
};
}

