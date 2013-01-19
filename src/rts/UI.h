#ifndef SRC_RTS_UI_H_
#define SRC_RTS_UI_H_
#include <functional>
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

class UIWidget {
 public:
  UIWidget();
  virtual ~UIWidget() { }

  bool isClick(const glm::vec2 &pos) const;
  bool handleClick(const glm::vec2 &pos);

  typedef std::function<bool(const glm::vec2 &)> OnClickListener;

  void setOnClickListener(OnClickListener onClickListener);
  void setClickable(const glm::vec2 &pos, const glm::vec2 &size);
  void setUnClickable();


  virtual void render(float dt) = 0;

 private:
   bool clickable_;
   OnClickListener onClickListener_;
   glm::vec2 pos_, size_;
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
      T& textGetter)
  : pos_(pos),
    size_(size),
    height_(fontHeight),
    bgcolor_(bgcolor),
    textFunc_(textGetter) {
  }

  void render(float dt) {
    const std::string text = textFunc_();
    drawRect(pos_, size_, bgcolor_);
    FontManager::get()->drawString(text, pos_, height_);
  }

 private:
  glm::vec2 pos_;
  glm::vec2 size_;
  float height_;
  glm::vec4 bgcolor_;
  T textFunc_;
};

template<typename T>
TextWidget<T>* createTextWidget(
  const glm::vec2& pos,
  const glm::vec2& size,
  float fontHeight,
  const glm::vec4 &bgcolor,
  T textGetter) {
  return new TextWidget<T>(pos, size, fontHeight, bgcolor, textGetter);
}

template<class T>
class CustomWidget : public UIWidget {
public:
  CustomWidget(T func)
    : func_(func) {
  }

  void render(float dt) {
    func_(dt);
  }

private:
  T func_;
};
// This does template type deduction so you don't have to figure out wtf T is
// when you want to make a custom widget
template<typename T>
CustomWidget<T>* createCustomWidget(T&& func) {
  return new CustomWidget<T>(func);
}

struct MapHighlight {
  glm::vec2 pos;
  float remaining;
};
};  // rts
#endif  // SRC_RTS_UI_H_
