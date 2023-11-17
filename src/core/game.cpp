#include "game.hpp"

#include "./clock.hpp"
#include "./file_manager.hpp"

// TEMP
#include "../graphics/sprite.hpp"
// TEMP

namespace dl
{
  Game::Game() { }

  void Game::load(int argc, char* argv[])
  {

    try
    {
      m_camera.set_frustrum (0.0f, 800.f, 600.f, 0.0f);
      m_world_renderer.add_layer("world", "world");

      /* auto params = TCOD_ContextParams{}; */
      /* params.tcod_version = TCOD_COMPILEDVERSION; */
      /* params.argc = argc; */
      /* params.argv = argv; */
      /* params.renderer_type = TCOD_RENDERER_SDL2; */
      /* params.vsync = 1; */
      /* /1* params.sdl_window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED; *1/ */
      /* params.sdl_window_flags = SDL_WINDOW_RESIZABLE; */
      /* params.window_title = m_lua.get_variable<const char*>("title"); */

      /* const std::string tilesheet_path = m_lua.get_variable<std::string>("tilesheet_path"); */
      /* const int tilesheet_width = m_lua.get_variable<int>("tilesheet_width"); */
      /* const int tilesheet_height = m_lua.get_variable<int>("tilesheet_height"); */
      /* auto tileset = tcod::load_tilesheet(dl::FileManager::get_full_path(tilesheet_path), {tilesheet_width, tilesheet_height}, tcod::CHARMAP_TCOD); */
      /* params.tileset = tileset.get(); */

      /* const int console_width = m_lua.get_variable<int>("console_width"); */
      /* const int console_height = m_lua.get_variable<int>("console_height"); */

      /* m_console = tcod::Console{console_width, console_height}; */
      /* params.console = m_console.get(); */

      /* m_context = tcod::Context(params); */
    }
    catch (const std::exception& exc)
    {
      std::cerr << exc.what() << "\n";
      throw;
    }
  }

  void Game::run()
  {
    try
    {
      dl::Clock clock{};

      while (!m_input_manager->should_quit())
      {
        clock.tick();

        m_scene_manager.update(clock.delta);
        m_input_manager->update();

        auto sprite = std::make_shared<Sprite>("spritesheet-tileset", 1);

        m_display.clear();
        m_world_renderer.init("world");
        m_world_renderer.batch("world", sprite, 0., 0., 0.);
        m_world_renderer.finalize("world");
        m_world_renderer.render(m_camera);
        /* m_scene_manager.render(m_context, m_console); */
        m_display.render();
      }
    }
    catch (const std::exception& exc)
    {
      std::cerr << exc.what() << "\n";
      throw;
    }
  }
}
