#pragma once
#include "rts/Widgets.h"
#include <vector>
#include <mutex>

namespace rts {

class CommandWidget : public StaticWidget {
 public:
  CommandWidget(const std::string &name);
  virtual ~CommandWidget();

  CommandWidget* show(float duration);
  CommandWidget* hide();
  // Will capture text until submitted or ESC pressed
  CommandWidget* captureText(const std::string &prefix = "");
  void stopCapturing();

  void setCloseOnSubmit(bool close) {
    closeOnSubmit_ = close;
  }

  // Thread-safe
  CommandWidget* addMessage(const std::string &msg);

  typedef std::function<void(const std::string &newText)> OnTextSubmittedHandler;
  void setOnTextSubmittedHandler(OnTextSubmittedHandler h) {
    textSubmittedHandler_ = h;
  }

  virtual void render(float dt);

 private:
  std::string name_;
  bool capturing_;
  bool closeOnSubmit_;
  float showDuration_;
  OnTextSubmittedHandler textSubmittedHandler_;
  std::string buffer_;
  std::vector<std::string> messages_;
  std::mutex lock_;
};
};  // rts
