#ifndef SRC_RTS_WIDGETS_H_
#define SRC_RTS_WIDGETS_H_

#include <functional>
#include <string>
#include <glm/glm.hpp>
#include "rts/ResourceManager.h"

namespace rts {

class UI;
class UIWidget;
UIWidget *createWidget(const std::string &paramName);
void createWidgets(UI *ui, const std::string &widgetGroupName);
glm::vec2 uiVec2Param(const std::string &name);

class UIWidget {
 public:
  virtual ~UIWidget() { }

  // Return true if the click has been handled
  virtual bool handleClick(const glm::vec2 &pos, int button) = 0;
  virtual void update(const glm::vec2 &pos, int buttons) = 0;
  virtual void render(float dt) = 0;

  virtual glm::vec2 getCenter() const = 0;
  virtual glm::vec2 getSize() const = 0;


  UI *getUI() {
    return ui_;
  }
  void setUI(UI *ui) {
    ui_ = ui;
  }

 private:
  UI *ui_;
};

class StaticWidget : public UIWidget {
 public:
  StaticWidget(const std::string &name);

  virtual glm::vec2 getCenter() const override final {
    return center_;
  }
  virtual glm::vec2 getSize() const override final {
    return size_;
  }

  virtual void update(const glm::vec2 &pos, int buttons) override { }
  virtual bool handleClick(const glm::vec2 &pos, int button) override;

  typedef std::function<bool()> OnPressListener;
  void setOnPressListener(OnPressListener l) {
    onPressListener_ = l;
  }

 protected:
  bool isClick(const glm::vec2 &pos) const;

 private:
  glm::vec2 center_;
  glm::vec2 size_;
  OnPressListener onPressListener_;
};

class TextureWidget : public StaticWidget {
 public:
  TextureWidget(const std::string &name);

  void render(float dt) override;

 private:
  std::string texName_;
};

class DepthFieldWidget : public StaticWidget {
 public:
   DepthFieldWidget(const std::string &name);

   void render(float dt);

 private:
   DepthField *depthField_;
};

class TextWidget : public StaticWidget {
 public:
  typedef std::function<std::string()> TextFunc;

  TextWidget(const std::string &name);

  TextWidget *setTextFunc(const TextFunc &func);

  void render(float dt);

  void setColor(const glm::vec4 &color) { bgcolor_ = color; }

 private:
  float height_;
  glm::vec4 bgcolor_;
  TextFunc textFunc_;
};

glm::vec2 uiPosParam(const std::string &name);
glm::vec2 uiSizeParam(const std::string &name);
};  // rts
#endif  // SRC_RTS_WIDGETS_H_
