#include "./texture.hpp"

#include <glad/glad.h>
#include <spdlog/spdlog.h>

extern "C"
{
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
}

namespace dl
{
// Load single texture
Texture::Texture(const std::string& filepath, const TextureType type)
    : m_type(type), m_horizontal_frames(1), m_vertical_frames(1)
{
  load(filepath);
}

// Load uniform texture atlas
Texture::Texture(const std::string& filepath,
                 const TextureType type,
                 const int horizontal_frames,
                 const int vertical_frames)
    : m_type(type), m_horizontal_frames(horizontal_frames), m_vertical_frames(vertical_frames)
{
  load(filepath);
}

// Load with raw data
Texture::Texture(const std::vector<unsigned char>& data, const int width, const int height)
    : m_horizontal_frames(1), m_vertical_frames(1)
{
  load(data.data(), width, height, GL_RGBA);
}

Texture::Texture(const int width, const int height, const TextureType type)
    : m_type(type), m_horizontal_frames(1), m_vertical_frames(1), m_width(width), m_height(height)
{
  m_load_empty();
}

Texture::~Texture() { glDeleteTextures(1, &m_id); }

void Texture::load(const std::string& filepath)
{
  int width = 0;
  int height = 0;
  int channels = 0;
  unsigned char* image_data;

  // image_data = stbi_load (filepath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
  image_data = stbi_load(filepath.c_str(), &width, &height, &channels, 0);
  if (image_data == nullptr)
  {
    throw std::runtime_error("It wasn't possible to load " + filepath);
  }

  GLenum format;
  switch (channels)
  {
  case 1:
    format = GL_RED;
    break;

  case 3:
    format = GL_RGB;
    break;

  case 4:
    format = GL_RGBA;
    break;

  default:
    format = GL_RGBA;
    break;
  }

  load(image_data, width, height, format);

  stbi_image_free(image_data);
}

void Texture::load(const unsigned char* data, const int width, const int height, unsigned int format)
{
  m_width = width;
  m_height = height;

  glGenTextures(1, &m_id);
  glBindTexture(GL_TEXTURE_2D, m_id);
  glTexImage2D(GL_TEXTURE_2D, 0, format, m_width, m_height, 0, format, GL_UNSIGNED_BYTE, data);
  glGenerateMipmap(GL_TEXTURE_2D);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  glBindTexture(GL_TEXTURE_2D, 0);
}

void Texture::bind() { glBindTexture(GL_TEXTURE_2D, m_id); }

void Texture::unbind() { glBindTexture(GL_TEXTURE_2D, 0); }

// TODO: Implement irregular frame calculations
float Texture::get_frame_width(const int frame) const { return (m_width / static_cast<float>(m_horizontal_frames)); }

// TODO: Implement irregular frame calculations
float Texture::get_frame_height(const int frame) const { return (m_height / static_cast<float>(m_vertical_frames)); }

// TODO: Implement irregular frame calculations
// Get top-left, top-right, bottom-right and bottom-left uv coordinates
std::array<glm::vec2, 4> Texture::get_frame_coords(const int frame) const
{
  const auto frame_width = get_frame_width(frame) / static_cast<float>(m_width);
  const auto frame_height = get_frame_height(frame) / static_cast<float>(m_height);
  const int max_frames = m_horizontal_frames * m_vertical_frames;
  const float frame_x = static_cast<float>(frame % m_horizontal_frames);
  const float frame_y = static_cast<float>((frame % max_frames) / m_horizontal_frames);
  // Multiply the x coord of the frame in the texture atlas by the normalized value of the width one frame.
  const float top_left_x = frame_x * (m_width / static_cast<float>(m_horizontal_frames)) / m_width;
  // Multiply the y coord of the frame in the tile map by the normalized value of the height one frame.
  // Invert the value as the y axis is upwards for OpenGL
  const float top_left_y = frame_y * (m_height / static_cast<float>(m_vertical_frames)) / m_height;

  return std::array<glm::vec2, 4>{
      glm::vec2{top_left_x, top_left_y},
      glm::vec2{top_left_x + frame_width, top_left_y},
      glm::vec2{top_left_x + frame_width, top_left_y + frame_height},
      glm::vec2{top_left_x, top_left_y + frame_height},
  };
}

void Texture::m_load_empty()
{
  glGenTextures(1, &m_id);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glBindTexture(GL_TEXTURE_2D, m_id);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, m_width, m_height, 0, GL_RED, GL_UNSIGNED_BYTE, 0x0000);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glBindTexture(GL_TEXTURE_2D, 0);

  GLenum err;
  while ((err = glGetError()) != GL_NO_ERROR)
  {
    spdlog::critical("Failed to create empty Texture. OpenGL error code: 0x{0:04x}", err);
  }
}
}  // namespace dl
