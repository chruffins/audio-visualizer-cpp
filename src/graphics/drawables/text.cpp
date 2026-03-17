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

// helper to truncate text with ellipsis if it exceeds width
std::string truncateTextWithEllipsis(ALLEGRO_FONT* font, const char* str, int maxWidth) {
  int textWidth = al_get_text_width(font, str);
  if (textWidth <= maxWidth) {
    return std::string(str);
  }

  std::string ellipsis = "...";
  int ellipsisWidth = al_get_text_width(font, ellipsis.c_str());
  if (ellipsisWidth >= maxWidth) {
    return "";
  }

  int availableWidth = maxWidth - ellipsisWidth;
  std::string truncated;
  for (const char* p = str; *p != '\0'; ) {
    // utf-8 crap
    unsigned char c = static_cast<unsigned char>(*p);
    int charLen = 1;
    if      ((c & 0xF8) == 0xF0) charLen = 4; // 11110xxx
    else if ((c & 0xF0) == 0xE0) charLen = 3; // 1110xxxx
    else if ((c & 0xE0) == 0xC0) charLen = 2; // 110xxxxx

    std::string candidate = truncated + std::string(p, charLen);
    if (al_get_text_width(font, candidate.c_str()) > availableWidth) {
      break;
    }
    truncated = std::move(candidate);
    p += charLen;
  }
  return truncated + ellipsis;
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
  
  // Culling: skip off-screen text
  if (isOffScreen(context)) {
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
  std::string truncatedStr;

  if (truncateText) {
    truncatedStr = truncateTextWithEllipsis(font_ptr, str, static_cast<int>(w));
    str = truncatedStr.c_str();
  }

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