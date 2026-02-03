#include "graphics/drawables/text.hpp"

using namespace ui;

// Define static members
std::shared_ptr<util::Font> TextDrawable::fallbackFont = nullptr;
std::once_flag TextDrawable::fallbackInitFlag;

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
  std::call_once(fallbackInitFlag, []() {
    fallbackFont = std::make_shared<util::Font>(); // Initialize with default constructor
  });
}

void TextDrawable::draw(const graphics::RenderContext& context) const {
  drawTextInternal(text.c_str(), context);
}

// Note: drawF is a template and is defined in the header (must be visible at
// call sites). The implementation was moved to the header to avoid linker
// issues.
void TextDrawable::drawTextInternal(const char* str, const graphics::RenderContext& context) const {
  if (!visible) {
    return;
  }

  auto [x, y] = getPosition().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
  auto [w, h] = getSize().toScreenPos(static_cast<float>(context.screenWidth), static_cast<float>(context.screenHeight));
  x += context.offsetX;
  y += context.offsetY;
  // Obtain the actual ALLEGRO_FONT pointer at draw time. This avoids holding
  // onto a raw pointer that could be invalidated if the Font resource is
  // reloaded elsewhere.
  ALLEGRO_FONT* font_ptr = nullptr;
  if (font) {
    font_ptr = font->getFont(font_size);
  }
  if (!font_ptr && fallbackFont) {
    font_ptr = fallbackFont->getFont(font_size);
  }
  if (!font_ptr) {
    return; // no font available to draw
  }

  int font_height = al_get_font_line_height(font_ptr);
  float y_offset = calculateVerticalOffset(verticalAlignment, h, font_height);

  int allegro_align = ALLEGRO_ALIGN_CENTER;
  switch (textAlignment) {
    case TextDrawable::HorizontalAlignment::Left:
      allegro_align = ALLEGRO_ALIGN_LEFT;
      break;
    case TextDrawable::HorizontalAlignment::Center:
      allegro_align = ALLEGRO_ALIGN_CENTER;
      break;
    case TextDrawable::HorizontalAlignment::Right:
      allegro_align = ALLEGRO_ALIGN_RIGHT;
      break;
  }

  if (!multiline) {
    al_draw_text(font_ptr, color, x, y + y_offset, allegro_align | ALLEGRO_ALIGN_INTEGER, str);
  } else {
    al_draw_multiline_text(font_ptr, color, x, y + y_offset, w, font_height + line_height,
                           allegro_align | ALLEGRO_ALIGN_INTEGER, str);
  }
}