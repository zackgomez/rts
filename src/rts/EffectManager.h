#ifndef SRC_RTS_EFFECTMANAGER_H_
#define SRC_RTS_EFFECTMANAGER_H_
#include <functional>
#include <string>
#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

namespace rts {

class Effect {
 public:
  Effect(float t) : startTime_(t) { }
  virtual ~Effect() { }

  virtual bool render(float t) const = 0;

  float getStartingTime() const {
    return startTime_;
  }

 private:
  float startTime_;
};

class CustomizableEffect : public Effect {
 public:
  typedef std::function<void(float t)> RenderFunc;
  typedef std::function<bool(float t)> AliveFunc;
  
  CustomizableEffect(float t, const RenderFunc, const AliveFunc);

  virtual bool render(float t) const final override;

 private:
  const RenderFunc renderFunc_;
  const AliveFunc aliveFunc_;
};


struct ParticleInfo {
  glm::vec3 pos;
  glm::vec2 size;
  glm::vec4 color;
  GLuint texture;
  glm::vec4 texcoord;

  enum BillboardType {
    NONE,
    SPHERICAL,
    CYLINDRICAL,
  };
  BillboardType billboard_type;
  glm::vec3 billboard_vec;
};
Effect *makeParticleEffect(
    float duration,
    std::function<ParticleInfo(float t)> particle_func);

class EffectManager {
 public:
  ~EffectManager();

  void addEffect(Effect* effect) {
    effects_.push_back(effect);
  }
  void render(float t);
  void clear();

 private:
  std::vector<Effect*> effects_;
};

}  // rts
#endif  // SRC_RTS_EFFECTMANAGER_H_
