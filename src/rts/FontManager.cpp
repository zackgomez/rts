#include "rts/FontManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <iomanip>
#include <fstream>
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>
#include "common/Logger.h"
#include "common/ParamReader.h"
#include "common/util.h"
#include "rts/Graphics.h"
#include "rts/ResourceManager.h"

const float widthRatio = 0.75f;

FontManager::FontManager()
  : glyphTexSize_(0.f),
    glyphTex_(0) {
  initialize();
}

FontManager::~FontManager() {
  freeTexture(glyphTex_);
}

void FontManager::drawString(const std::string &s, const glm::vec2 &pos,
    float height) {
  glm::vec4 color(0.f, 0.f, 0.f, 1.f);

  auto program = ResourceManager::get()->getShader("font");
  program->makeActive();
  program->uniform4f("color", color);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, glyphTex_);


  glm::vec2 p(pos.x, pos.y + height);
  float fact = height / glyphSize_;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    if (c == COLOR_CNTRL_CH) {
      color = glm::vec4(parseColor(s, i + 1), 1.f);
      program->uniform4f("color", color);
      // advance past next 3 characters
      i += 3;
      continue;
    }
    p.x += drawCharacter(c, p, fact, program);
  }
}

float FontManager::computeStringWidth(
    const std::string &s,
    const glm::vec2 &pos,
    float height) {
  float width = 0.f;
  const float fact = height / glyphSize_;
  for (size_t i = 0; i < s.length(); i++) {
    char c = s[i];
    invariant(c >= 32 && c < 32 + 96, "invalid character to render");
    if (c == COLOR_CNTRL_CH) {
      // advance past next 3 characters
      i += 3;
      continue;
    }
    width += fact * ((stbtt_bakedchar*)cdata_)[c - 32].xadvance;
  }
  return width;
}

const char * FontManager::makeColorCode(const glm::vec3 &color) {
  static char buf[5];
  buf[0] = COLOR_CNTRL_CH;
  for (int i = 0; i < 3; i++) {
    buf[i+1] = static_cast<char>(color[i] * 255);
  }
  // TODO(zack): these lines are an embarassing hack to make the string always
  // be of length 4 to strlen
  buf[1] = buf[1] ? buf[1] : '\x1';
  buf[2] = buf[2] ? buf[2] : '\x1';
  buf[3] = buf[3] ? buf[3] : '\x1';
  buf[4] = '\0';

  return buf;
}

glm::vec3 FontManager::parseColor(const std::string &s, size_t i) {
  glm::vec3 color;
  size_t j = 0;
  for (; j < 3 && i < s.length(); i++, j++) {
    color[j] = static_cast<uint8_t>(s[i]) / 255.f;
  }
  if (j != 3) {
    LOG(WARNING) << "String ended while attempting to parse color\n";
  }

  return color;
}

float FontManager::drawCharacter(char c, const glm::vec2 &pos, float fact,
    Shader *program) {
  invariant(c >= 32, "invalid character to render");
  stbtt_bakedchar bc = ((stbtt_bakedchar*)cdata_)[c - 32];

  glm::vec4 texcoord = glm::vec4(bc.x0, bc.y0, bc.x1, bc.y1) / glyphTexSize_;
  glm::vec2 size = fact * glm::vec2(bc.x1 - bc.x0, bc.y1 - bc.y0);
  glm::vec2 offset = fact * glm::vec2(bc.xoff, bc.yoff);
  float advance = fact * bc.xadvance;

  program->uniform4f("texcoord", texcoord);
  drawShader(pos + offset, size);

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
  unsigned char *temp_bitmap = (unsigned char *)
      malloc(sizeof(*temp_bitmap) * glyphTexSize_ * glyphTexSize_);
  cdata_ = malloc(sizeof(stbtt_bakedchar) * 96);
  stbtt_BakeFontBitmap((unsigned char*) fontstr.c_str(), 0,  // font data
      glyphSize_,  // font size
      temp_bitmap,
      glyphTexSize_, glyphTexSize_,
      32, 96,  // character range
      (stbtt_bakedchar*)cdata_);

  glGenTextures(1, &glyphTex_);
  glBindTexture(GL_TEXTURE_2D, glyphTex_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA,
      glyphTexSize_, glyphTexSize_, 0,
      GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  free(temp_bitmap);
}
