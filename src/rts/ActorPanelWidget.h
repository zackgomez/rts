#pragma once
#include "rts/Widgets.h"
#include "common/Types.h"
#include "rts/GameEntity.h"
#include "rts/UIAction.h"

namespace rts {

class BorderWidget;

class ActorPanelWidget : public UIWidget {
 public:
  typedef std::function<const GameEntity*()> ActorFunc;
  typedef std::function<void(const GameEntity::UIPartUpgrade &)> PartUpgradeHandler;
  ActorPanelWidget(
      const std::string &name,
      ActorFunc f,
      PartUpgradeHandler upgrade_handler);
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

  void setPart(const GameEntity::UIPart &part) {
    part_ = part;
  }
  void setUpgradeHandler(ActorPanelWidget::PartUpgradeHandler handler) {
    upgradeHandler_ = handler;
  }
  virtual bool handleClick(const glm::vec2 &pos, int button) override;
  virtual void render(float dt) override;
  virtual void update(const glm::vec2 &pos, int buttons) override { }

 private:
  glm::vec4 bgcolor_;
  ActorPanelWidget::PartUpgradeHandler upgradeHandler_;
  GameEntity::UIPart part_;
};

};  // rts
