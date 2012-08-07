#pragma once
#include <string>
#include "Message.h"
#include "glm.h"

namespace rts {

class Entity;

/* This class represents a weapon, melee or ranged, and is intended for use in
 * the actor class.  It has methods for querying the capabilities for a weapon
 * as well as its use.
 */
class Weapon
{
public:
  Weapon(const std::string &name, const Entity *owner);

  // State transitions
  // READY -> fire() -> WINDUP -> actually fire -> WINDDOWN -> COOLDOWN
  // eventually reloading will be incorporated here.  The attack can be
  // canceled by calling interrupt() during the WINDUP state.
  enum WeaponState
  {
    READY,
    WINDUP,
    WINDDOWN,
    COOLDOWN,
  };

  float getMaxRange() const;
  float getArc() const;
  WeaponState getState() const;
  const std::string & getName() const { return name_; }

  // Returns true if the target is in range, and this weapon is capable
  // of firing
  bool canAttack(const Entity *target) const;

  // Should be called once per tick
  void update(float dt);
  // Start a shot, by running through the state transitions that govern weapons
  // firing
  void fire(const Entity *target);
  // Interrupt the current action, i.e. if the weapon was winding up, it would
  // cease doing so
  // TODO(zack) use this when a state changes in unit, i.e. a transition from
  // idle/moving should cancel a shot in WINDUP
  void interrupt();

private:
  float param(const std::string &p) const;
  std::string strParam(const std::string &p) const;
  bool hasParam(const std::string &p) const;
  bool hasStrParam(const std::string &p) const;

  const std::string name_;
  const Entity *owner_;
  WeaponState state_;
  float t_;
  Message msg_;
};

} // rts
