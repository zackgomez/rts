#pragma once
#include "rts/Widgets.h"
#include "common/Types.h"
#include "rts/Actor.h"
#include "rts/UIAction.h"

namespace rts {

class BorderWidget;

class ActorPanelWidget : public UIWidget {
 public:
  typedef std::function<const Actor*()> ActorFunc;
  ActorPanelWidget(const std::string &name, ActorFunc f);
  virtual ~ActorPanelWidget();

  virtual bool handleClick(const glm::vec2 &pos, int button) override;
  virtual void render(float dt) override;
  virtual void update(const glm::vec2 &pos, int buttons) override;

 private:
  std::string name_;
  glm::vec4 bgcolor_;

  ActorFunc actorFunc_;

  std::vector<BorderWidget *> partWidgets_;
};

class PartWidget : public UIWidget {
 public:
   PartWidget(glm::vec4 bgcolor) : bgcolor_(bgcolor) { }
  virtual ~PartWidget() { }

  void setPart(const Actor::UIPart &part) {
    part_ = part;
  }
  virtual bool handleClick(const glm::vec2 &pos, int button) override;
  virtual void render(float dt) override;
  virtual void update(const glm::vec2 &pos, int buttons) override { }

 private:
  glm::vec4 bgcolor_;
  Actor::UIPart part_;
};

};  // rts
