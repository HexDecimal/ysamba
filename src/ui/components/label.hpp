#pragma once

#include "./component.hpp"
#include "graphics/text.hpp"
#include "ui/style/text.hpp"

namespace dl::ui
{
class Label : public UIComponent
{
 public:
  Text text;
  TextStyle style;

  Label(const std::string& text, const std::string& typeface = "font-1980");

  void render(Renderer& renderer, const uint32_t layer);

 private:
};

}  // namespace dl::ui
