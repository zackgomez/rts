#pragma once
#include "rts/Widgets.h"

namespace rts {

class BorderWidget : public UIWidget {
 public:
  BorderWidget(
      const glm::vec4 &border_color,
      UIWidget *child);
  virtual ~BorderWidget();

  UIWidget *getChild() const {
    return child_;
  }
  virtual bool handleClick(const glm::vec2 &pos, int button) override;
  virtual void render(float dt) override;
  virtual void update(const glm::vec2 &pos, int buttons) override;

 private:
  glm::vec2 getInnerSize() const;

  glm::vec4 borderColor_;
  UIWidget *child_;
};

class TooltipWidget : public UIWidget {
 public:
  TooltipWidget(
      UIWidget *child);
  virtual ~TooltipWidget();

  UIWidget *getChild() const {
    return child_;
  }
  virtual bool handleClick(const glm::vec2 &pos, int button) override;
  virtual void render(float dt) override;
  virtual void update(const glm::vec2 &pos, int buttons) override;

  typedef std::function<std::string()> TooltipFunc;
  void setTooltipFunc(TooltipFunc func) {
    tooltipFunc_ = func;
  }

 private:
  glm::vec2 getInnerSize() const;

  TooltipFunc tooltipFunc_;
  UIWidget *child_;
};

};  // rts
