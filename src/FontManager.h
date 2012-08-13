#pragma once
#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <stb_truetype.h>

class FontManager {
public:
  static FontManager * get();

  // Pos is top left, height top of capital letters to bottom of letters that
  // drop below
  void drawString(const std::string &s, 
      const glm::vec2 &pos, float height);

  void drawTex() const;

private:
  FontManager();
  ~FontManager();

  void initialize();
  float drawCharacter(char c, const glm::vec2 &pos, float height);

  float glyphSize_;
  unsigned glyphTexSize_;
  stbtt_bakedchar cdata_[96];
  GLuint glyphTex_;
};
