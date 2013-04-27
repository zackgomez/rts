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

  void render(float dt);

  virtual bool isClick(const glm::vec2 &pos) const;

 private:
  ActionFunc actionsFunc_;
  ActionExecutor actionExecutor_;
  glm::vec2 center_;
  glm::vec2 size_;
  glm::vec4 bgcolor_;

  bool handleRealClick(const glm::vec2 &pos) const;
};

};  // rts
