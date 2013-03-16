#pragma once
#include <string>
#include "common/util.h"

namespace rts {

class Entity
{
public:
  Entity(id_t id) : id_(id) { }
  virtual ~Entity() { }

  id_t getID() const { return id_; }
  virtual bool hasProperty(uint32_t property) const {
      return false;
  }

private:
  id_t id_;
};
}  // rts
