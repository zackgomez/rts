#ifndef SRC_RTS_MATCHMAKERCONTROLLER_H_
#define SRC_RTS_MATCHMAKERCONTROLLER_H_

#include "rts/Controller.h"

namespace rts {

class Matchmaker;
class CommandWidget;

class MatchmakerController : public Controller {
 public:
  MatchmakerController(Matchmaker *mm);
  ~MatchmakerController();

  virtual void onCreate() override;
  virtual void onDestroy() override;

  virtual void quitEvent() override;
  virtual void keyPress(const KeyEvent &ev) override;

 protected:
  virtual void renderExtra(float dt) override;

 private:
   Matchmaker *matchmaker_;
   CommandWidget *infoWidget_;
};
};  // rts
#endif
