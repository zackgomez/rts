#pragma once
#include "rts/Widgets.h"
#include "common/Types.h"
#include "rts/UIAction.h"

namespace rts {

class Actor;

class ActorPanelWidget : public UIWidget {
 public:
  typedef std::function<const Actor*()> ActorFunc;
  ActorPanelWidget(const std::string &name, ActorFunc f);
  virtual ~ActorPanelWidget();

  virtual bool handleClick(const glm::vec2 &pos, int button) override { return true; }
  virtual void render(float dt) override;
  virtual void update(const glm::vec2 &pos, int buttons) override;

  virtual glm::vec2 getCenter() const override final;
  virtual glm::vec2 getSize() const override final;

 private:
  std::string name_;
  glm::vec2 center_;
  glm::vec2 size_;
  glm::vec4 bgcolor_;

  ActorFunc actorFunc_;
  bool hover_;
  bool press_;
  glm::vec2 hoverPos_;
  float hoverTimer_;
};

};  // rts
