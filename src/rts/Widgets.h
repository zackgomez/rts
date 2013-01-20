#ifndef SRC_RTS_WIDGETS_H_
#define SRC_RTS_WIDGETS_H_

#include <functional>
#include <string>
#include <glm/glm.hpp>

namespace rts {

class UIWidget;
UIWidget *createWidget(const std::string &paramName);
void createWidgets(const std::string &widgetGroupName);

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

class SizedWidget : public UIWidget {
 public:
  SizedWidget(const std::string &name);

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
