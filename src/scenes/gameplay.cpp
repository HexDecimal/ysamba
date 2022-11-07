#include "./gameplay.hpp"

#include <fstream>
#include <cereal/archives/binary.hpp>

namespace dl
{
  Gameplay::Gameplay()
    : Scene("scenes/gameplay.lua")
  { }

  void Gameplay::load()
  {
    Scene::load();

    m_world.generate(100, 100);
    m_physics_layer.add(&m_player.body);
    m_camera.size.w = m_lua.get_variable<int>("camera_width");
    m_camera.size.h = m_lua.get_variable<int>("camera_height");
    m_has_loaded = true;
  }

  void Gameplay::update(const uint32_t delta, std::function<void(const std::string&)> set_scene)
  {
    if (!has_loaded())
    {
      return;
    }

    // If G is pressed
    if (m_input_manager->is_key_down(10))
    {
      m_world.generate(100, 100);
    }
    // If S is pressed
    else if (m_input_manager->is_key_down(22))
    {
      save_world("./world.dl");
    }
    // If C is pressed
    else if (m_input_manager->is_key_down(6))
    {
      load_world("./world.dl");
    }

    m_camera.update(m_player.body.position, m_world.get_tilemap_size(m_player.body.position.z));
    m_player.update(delta);

    if (m_player.should_advance_turn())
    {
      m_physics_layer.update(delta);
      m_world.update(delta);
    }
  }

  void Gameplay::render(TCOD_Console& console)
  {
    if (!has_loaded())
    {
      return;
    }

    m_world.render(console, m_camera);
    m_player.render(console, m_camera);
  }

  void Gameplay::save_world(const std::string& file_path)
  {
    std::ofstream output_stream(file_path);
    cereal::BinaryOutputArchive archive (output_stream);
    archive(m_world);
  }

  void Gameplay::load_world(const std::string& file_path)
  {
    std::ifstream input_stream(file_path);
    cereal::BinaryInputArchive archive (input_stream);
    archive(m_world);
    m_has_loaded = true;
  }
}

