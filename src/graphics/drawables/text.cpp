#include "graphics/drawables/text.hpp"

using namespace ui;

// Define static members
std::shared_ptr<util::Font> TextDrawable::fallbackFont = nullptr;
bool TextDrawable::fallbackFontInitialized = false;

namespace {
// Helper to calculate vertical offset
float calculateVerticalOffset(graphics::VerticalAlignment alignment, float height, int font_height) {
  switch (alignment) {
    case graphics::VerticalAlignment::CENTER:
      return (height - font_height) / 2.0f;
    case graphics::VerticalAlignment::BOTTOM:
      return height - font_height;
    case graphics::VerticalAlignment::TOP:
    default:
      return 0.0f;
  }
}
} // anonymous namespace

void TextDrawable::initializeFallbackFont() {
  if (!fallbackFontInitialized) {
    fallbackFont = std::make_shared<util::Font>(); // Initialize with default constructor
    fallbackFontInitialized = true;
  }
}

void TextDrawable::draw(const graphics::RenderContext& context) const {
  drawTextInternal(text.c_str());
}

template <typename... Args>
void TextDrawable::drawF(const char *fmt, Args &&...args) {
  int needed = std::snprintf(nullptr, 0, fmt, std::forward<Args>(args)...);
  if (needed <= 0) {
    return; // formatting failed or empty
  }
  std::string buffer;
  buffer.resize(static_cast<size_t>(needed) + 1);
  std::snprintf(buffer.data(), buffer.size(), fmt, std::forward<Args>(args)...);
  buffer.resize(static_cast<size_t>(needed));

  drawTextInternal(buffer.c_str());
}

void TextDrawable::drawTextInternal(const char* str) const {
  auto [x, y] = position.toScreenPos();
  auto [w, h] = size.toScreenPos();
  float y_offset = calculateVerticalOffset(verticalAlignment, h, _font_height);

  if (!multiline) {
    al_draw_text(lastUsedFont, color, x, y + y_offset, textAlignment | ALLEGRO_ALIGN_INTEGER, str);
  } else {
    al_draw_multiline_text(lastUsedFont, color, x, y + y_offset, w, _font_height + line_height,
                           textAlignment | ALLEGRO_ALIGN_INTEGER, str);
  }
}