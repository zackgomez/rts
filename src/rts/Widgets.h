#ifndef SRC_RTS_WIDGETS_H_
#define SRC_RTS_WIDGETS_H_

#include <functional>
#include <string>
#include <glm/glm.hpp>
#include "rts/ResourceManager.h"

namespace rts {

class UIWidget;
UIWidget *createWidget(const std::string &paramName);
void createWidgets(const std::string &widgetGroupName);

class UIWidget {
 public:
  UIWidget();
  virtual ~UIWidget() { }

  virtual bool isClick(const glm::vec2 &pos) const {
    return false;
  }
  bool handleClick(const glm::vec2 &pos);

  typedef std::function<bool(const glm::vec2 &)> OnClickListener;

  UIWidget* setOnClickListener(OnClickListener onClickListener);
  UIWidget* setClickable();
  UIWidget* setUnClickable();
  bool isClickable() const {
    return clickable_;
  }


  virtual void render(float dt) = 0;

 private:
   bool clickable_;
   OnClickListener onClickListener_;
   glm::vec2 pos_, size_;
};

class SizedWidget : public UIWidget {
 public:
  SizedWidget(const std::string &name);

  virtual bool isClick(const glm::vec2 &pos) const;

  const glm::vec2 &getCenter() const { return
    center_;
  }

  const glm::vec2 &getSize() const { return
    size_;
  }


 private:
  glm::vec2 center_;
  glm::vec2 size_;
};

class TextureWidget : public SizedWidget {
 public:
  TextureWidget(const std::string &name);

  void render(float dt);

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

private:
  T func_;
};
// This does template type deduction so you don't have to figure out wtf T is
// when you want to make a custom widget
template<typename T>
CustomWidget<T>* createCustomWidget(T&& func) {
  return new CustomWidget<T>(func);
}
};  // rts
#endif  // SRC_RTS_WIDGETS_H_
