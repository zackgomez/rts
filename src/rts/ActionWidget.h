#pragma once
#include "rts/Widgets.h"
#include "common/Types.h"
#include "rts/UIAction.h"

namespace rts {

class ActionWidget : public UIWidget {
 public:
  typedef std::function<std::vector<UIAction>()> ActionFunc;
  typedef std::function<void(const UIAction& action)> ActionExecutor;

  ActionWidget(
      const std::string &name,
      ActionFunc actions,
      ActionExecutor executor);
  virtual ~ActionWidget();

  virtual void render(float dt) override;

  virtual bool handleClick(const glm::vec2 &pos) override;

  virtual glm::vec2 getCenter() const override final;
  virtual glm::vec2 getSize() const override final;

 private:
  ActionFunc actionsFunc_;
  ActionExecutor actionExecutor_;
  glm::vec2 center_;
  glm::vec2 size_;
  glm::vec4 bgcolor_;

};

};  // rts
