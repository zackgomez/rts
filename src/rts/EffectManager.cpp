#include "rts/EffectManager.h"
#include "common/Logger.h"
#include "rts/Graphics.h"
#include "rts/Renderer.h"

namespace rts {

CustomizableEffect::CustomizableEffect(float t, RenderFunc rf, AliveFunc af)
  : Effect(t),
    renderFunc_(rf),
    aliveFunc_(af) {
}

bool CustomizableEffect::render(float t) const {
  float subt = t - getStartingTime();
  if (!aliveFunc_(subt)) {
    return false;
  }
  renderFunc_(subt);
  return true;
}


Effect *makeParticleEffect(
    float duration,
    std::function<ParticleInfo(float t)> particle_func) {
  return new CustomizableEffect(
    Renderer::get()->getRenderTime(),
    [=](float t) -> void {
      auto particle = particle_func(t);

      // TODO(zack): add billboarding options
      glm::mat4 view_transform = getViewStack().current();
      glm::mat4 transform = 
        glm::scale(
          glm::translate(glm::mat4(1.f), particle.pos),
          glm::vec3(particle.size, 1.f));
      for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
          transform[i][j] = view_transform[j][i];
        }
      }
      if (!particle.texture) {
        renderRectangleColor(
          transform,
          particle.color);
      } else {
        renderRectangleTexture(transform, particle.texture, particle.texcoord);
      }
    },
    [=](float t) -> bool {
      return t <= duration;
    });
}

EffectManager::~EffectManager() {
  clear();
}

void EffectManager::clear() {
  for (auto *effect : effects_) {
    delete effect;
  }
  effects_.clear();
}

void EffectManager::render(float t) {
  std::vector<int> dead_effects;
  for (int i = 0; i < effects_.size(); i++)  {
    auto *effect = effects_[i];
    if (!effect->render(t)) {
      dead_effects.push_back(i);
    }
  }

  int n = 0;
  for (auto i : dead_effects) {
    std::swap(effects_[i - n], effects_.back());
    delete effects_.back();
    effects_.pop_back();
    n++;
  }
}

}  // rts
