#ifndef SRC_RTS_FONTMANAGER_H_
#define SRC_RTS_FONTMANAGER_H_

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>
#include <stb_truetype.h>

class Shader;

class FontManager {
 public:
  static FontManager * get();

  // When present in a string, the color of the remaining text is determined
  // by the next 3 bytes, encoding RGB as 1 byte values.
  static const char COLOR_CNTRL_CH = '\1';
  // THREAD UNSAFE
  // Returns a static buffer of size 4 containing encoding a color change to
  // the passed in color.
  static const char *makeColorCode(const glm::vec3 &color);

  // Pos is top left, height top of capital letters to bottom of letters that
  // drop below
  void drawString(const std::string &s,
                  const glm::vec2 &pos, float height);

  // Debug function, draws the glyph texture in the center of the screen
  void drawTex() const;

 private:
  FontManager();
  ~FontManager();

  void initialize();
  float drawCharacter(char c, const glm::vec2 &pos, float height,
      Shader *program);
  glm::vec3 parseColor(const std::string &s, size_t i);

  float glyphSize_;
  unsigned glyphTexSize_;
  stbtt_bakedchar cdata_[96];
  GLuint glyphTex_;
};

#endif  // SRC_RTS_FONTMANAGER_H_
