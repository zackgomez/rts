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
  glm::vec4 borderColor_;

  UIWidget *child_;

  glm::vec2 getInnerSize() const;
};

};  // rts
