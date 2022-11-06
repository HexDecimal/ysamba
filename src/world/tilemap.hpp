#pragma once

#include <string>
#include <map>
#include <libtcod.hpp>
#include "../core/lua_api.hpp"
#include "../components/size.hpp"
#include "./tile_data.hpp"

namespace dl
{
  class Tilemap
  {
    public:
      Tilemap(std::vector<int> tiles, const int width, const int height);
      ~Tilemap();

      int get(const int x, const int y) const;
      Size get_size() const;

    private:
      LuaAPI m_lua;
      std::vector<int> m_tiles;
      const int m_width = 0;
      const int m_height = 0;
      const int m_null_tile = 0;
  };
}
