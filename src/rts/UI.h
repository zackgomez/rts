#ifndef SRC_RTS_UI_H_
#define SRC_RTS_UI_H_
#include <map>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include "common/Types.h"
#include "common/util.h"
#include "common/Types.h"
#include "rts/FontManager.h"
#include "rts/Graphics.h"

namespace rts {

class ModelEntity;
class UIWidget;
struct MapHighlight;

class UI {
 public:
  static glm::vec2 convertUIPos(const glm::vec2 &pos);

  UI();
  ~UI();

  static UI* get() {
    if (!instance_) {
      instance_ = new UI;
    }
    return instance_;
  }

  void addWidget(const std::string &name, UIWidget *widget);
  UIWidget *getWidget(const std::string &name);
  void clearWidgets();

  bool handleMousePress(const glm::vec2 &loc);

  void initGameWidgets(id_t playerID);
  void clearGameWidgets();

  void render(float dt);
  void renderEntity(const ModelEntity *e, const glm::mat4 &transform, float dt);
  void highlight(const glm::vec2 &mapCoord);
  void highlightEntity(id_t eid);

  void setChatActive(bool active) {
    chatActive_ = active;
  }
  void setChatBuffer(const std::string &buffer) {
    chatBuffer_ = buffer;
  }

 private:
  static UI* instance_;
  static void renderChat(float dt);
  static void renderMinimap(id_t localPlayerID, float dt);
  static void renderHighlights(float dt);
  static void renderDragRect(float dt);

  std::map<std::string, UIWidget *> widgets_;

  std::vector<MapHighlight> highlights_;
  std::map<id_t, float> entityHighlights_;
  bool chatActive_;
  std::string chatBuffer_;

  id_t playerID_;
};

struct MapHighlight {
  glm::vec2 pos;
  float remaining;
};
};  // rts
#endif  // SRC_RTS_UI_H_
