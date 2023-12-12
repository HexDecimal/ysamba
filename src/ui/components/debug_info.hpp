#pragma once

#include "./component.hpp"

namespace dl::ui
{
class Label;
class Container;

class DebugInfo : public UIComponent
{
 public:
  DebugInfo();

  void set_content(const std::string& text);

 private:
  Container* m_container;
  Label* m_label;
};

}  // namespace dl::ui
