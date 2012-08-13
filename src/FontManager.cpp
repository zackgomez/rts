#include "FontManager.h"
#include "Engine.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <iomanip>
#include <fstream>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include "util.h"
#include "Logger.h"
#include "ParamReader.h"

const float widthRatio = 0.75f;

FontManager::FontManager() :
  glyphTexSize_(0.f),
  glyphTex_(0) {
  initialize();
}

FontManager::~FontManager() {
  freeTexture(glyphTex_);
}

void FontManager::drawString(const std::string &s, const glm::vec2 &pos,
    float height) {
  glm::vec2 p(pos);
  float fact = height / glyphSize_;
  for (char c : s) {
    p.x += drawCharacter(c, p, fact);
  }
}

float FontManager::drawCharacter(char c, const glm::vec2 &pos, float fact) {

  invariant(c >= 32 && c < 32 + 96, "invalid character to render");
  stbtt_bakedchar bc = cdata_[c - 32];

  glm::vec4 texcoord = glm::vec4(bc.x0, bc.y0, bc.x1, bc.y1) / glyphTexSize_;
  glm::vec2 size = fact * glm::vec2(bc.x1 - bc.x0, bc.y1 - bc.y0);
  glm::vec2 offset = fact * glm::vec2(bc.xoff, bc.yoff);
  float advance = fact * bc.xadvance;

  drawTexture(pos + offset, size, glyphTex_, texcoord);

  return advance;
}

void FontManager::drawTex() const {
  drawTexture(glm::vec2(400, 400), glm::vec2(400, 400), glyphTex_);
}

FontManager * FontManager::get() {
  static FontManager fm;
  return &fm;
}

void FontManager::initialize() {
  // TODO(zack): Read the various fonts from config, right now only one,
  // hardcoded
  std::string font_file = strParam("fonts.FreeSans.file");
  std::ifstream f(font_file.c_str(), std::ios_base::binary);
  if (!f) {
    throw file_exception("file: " + font_file + " not found");
  }

  std::string fontstr;
  f.seekg(0, std::ios::end);
  fontstr.reserve(f.tellg());
  f.seekg(0, std::ios::beg);
  fontstr.assign((std::istreambuf_iterator<char>(f)),
      std::istreambuf_iterator<char>());

  glyphTexSize_ = intParam("fonts.FreeSans.texsize");
  glyphSize_ = fltParam("fonts.FreeSans.size");
  unsigned char temp_bitmap[glyphTexSize_ * glyphTexSize_];
  stbtt_BakeFontBitmap((unsigned char*) fontstr.c_str(), 0, // font data
      glyphSize_, // font size
      temp_bitmap,
      glyphTexSize_, glyphTexSize_,
      32, 96, // character range
      cdata_);

  glGenTextures(1, &glyphTex_);
  glBindTexture(GL_TEXTURE_2D, glyphTex_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
      glyphTexSize_, glyphTexSize_, 0,
      GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
