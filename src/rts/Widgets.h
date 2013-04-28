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

class UIWidget {
 public:
  virtual ~UIWidget() { }

  // Return true if the click has been handled
  virtual bool handleClick(const glm::vec2 &pos) = 0;
  virtual void render(float dt) = 0;

  UI *getUI() {
    return ui_;
  }
  void setUI(UI *ui) {
    ui_ = ui;
  }

 private:
  UI *ui_ = nullptr;
};

class ClickableWidget : public UIWidget {
 public:
  typedef std::function<bool(const glm::vec2 &)> OnClickListener;

  virtual bool handleClick(const glm::vec2 &pos) override final;

  ClickableWidget* setOnClickListener(OnClickListener onClickListener);

 protected:
  virtual bool isClick(const glm::vec2 &pos) const = 0;
  OnClickListener onClickListener_;
};

class SizedWidget : public ClickableWidget {
 public:
  SizedWidget(const std::string &name);

  const glm::vec2 &getCenter() const {
    return center_;
  }
  const glm::vec2 &getSize() const {
    return size_;
  }

 protected:
  virtual bool isClick(const glm::vec2 &pos) const override final;

 private:
  glm::vec2 center_;
  glm::vec2 size_;
};

class TextureWidget : public SizedWidget {
 public:
  TextureWidget(const std::string &name);

  void render(float dt) override;

 private:
  std::string texName_;
};

class DepthFieldWidget : public SizedWidget {
 public:
   DepthFieldWidget(const std::string &name);

   void render(float dt);

 private:
   DepthField *depthField_;
};

class TextWidget : public SizedWidget {
 public:
  typedef std::function<std::string()> TextFunc;

  TextWidget(const std::string &name);

  TextWidget *setTextFunc(const TextFunc &func);
  TextWidget *setText(const std::string &s);

  void render(float dt);

 private:
  float height_;
  glm::vec4 bgcolor_;
  TextFunc textFunc_;
  std::string text_;
};

template<class T>
class CustomWidget : public UIWidget {
public:
  CustomWidget(T func)
    : func_(func) {
  }

  void render(float dt) {
    func_(dt);
  }

  virtual bool handleClick(const glm::vec2 &pos) override final {
    return false;
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
glm::vec2 uiPosParam(const std::string &name);
glm::vec2 uiSizeParam(const std::string &name);
};  // rts
#endif  // SRC_RTS_WIDGETS_H_
