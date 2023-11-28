#include "./label.hpp"

#include "graphics/batch.hpp"

namespace dl::ui
{
Label::Label(const std::string& text, const std::string& typeface)
    : UIComponent(ComponentType::Text), text(Text(text, typeface))
{
}
void Label::render(const std::shared_ptr<Batch> batch) { batch->text(text, position.x, position.y, position.z); }
}  // namespace dl::ui
