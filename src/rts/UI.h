#ifndef SRC_RTS_UI_H_
#define SRC_RTS_UI_H_
#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "common/Types.h"
#include "common/util.h"
#include "common/Types.h"

namespace rts {

class RenderEntity;
class UIWidget;
struct MapHighlight;

class UI {
 public:
  UI();
  ~UI();

  void render(float dt);
  void renderEntity(const RenderEntity *e, const glm::mat4 &transform, float dt);
  void highlight(const glm::vec2 &mapCoord);
  void highlightEntity(id_t eid);

  void initGameWidgets(id_t playerID);

  void setChatActive(bool active) {
    chatActive_ = active;
  }
  void setChatBuffer(const std::string &buffer) {
    chatBuffer_ = buffer;
  }
  void setDragRect(const glm::vec3 &start, const glm::vec3 &end);

 private:
  void renderChat();
  void renderMinimap();
  void renderHighlights(float dt);
  void renderDragRect(float dt);

  std::vector<UIWidget *> widgets_;

  std::vector<MapHighlight> highlights_;
  std::map<id_t, float> entityHighlights_;

  bool chatActive_;
  std::string chatBuffer_;

  id_t playerID_;

  glm::vec3 dragStart_, dragEnd_;
};

class UIWidget {
 public:
  virtual ~UIWidget() { }

  virtual void render(float dt) = 0;
};

class TextureWidget : public UIWidget {
 public:
  TextureWidget(
      const glm::vec2 &pos,
      const glm::vec2 &size,
      const std::string& texName);

  void render(float dt);

 private:
  glm::vec2 pos_;
  glm::vec2 size_;
  std::string texName_;
};

template<class T>
class TextWidget : public UIWidget {
 public:
  TextWidget(
      const glm::vec2 &pos,
      const glm::vec2 &size,
      float fontHeight,
      const glm::vec4 &bgcolor,
      T& textGetter);

  void render(float dt);

 private:
  glm::vec2 pos_;
  glm::vec2 size_;
  float height_;
  glm::vec4 bgcolor_;
  T textFunc_;
};

struct MapHighlight {
  glm::vec2 pos;
  float remaining;
};
};  // rts
#endif  // SRC_RTS_UI_H_
