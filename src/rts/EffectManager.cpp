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

glm::mat4 makeSphericalBillboardTransform(const glm::vec3 &pos) {
  glm::mat4 view_transform = getViewStack().current();
  glm::mat4 ret(1.f);
  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < 3; j++) {
      ret[i][j] = view_transform[j][i];
    }
  }
  ret[3] = glm::vec4(pos, 1.f);
  return ret;
}

glm::mat4 makeCylindricalBillboardTransform(
    const glm::vec3 &pos,
    const glm::vec3 &rot_axis) {
  glm::mat4 view_transform = getViewStack().current();
  glm::vec3 up = glm::normalize(rot_axis);

  glm::vec3 cpos = applyMatrix(glm::inverse(view_transform), glm::vec3(0.f));
  glm::vec3 look = pos - cpos;
  glm::vec3 right = glm::normalize(glm::cross(look, up));
  look = glm::normalize(glm::cross(up, right));

  glm::mat4 transform = glm::mat4(1.f);
  transform[0] = glm::vec4(up, 0.f);
  transform[1] = glm::vec4(right, 0.f);
  transform[2] = glm::vec4(look, 0.f);
  transform[3] = glm::vec4(pos, 1.f);

  return transform;
}

Effect *makeParticleEffect(
    float duration,
    std::function<ParticleInfo(float t)> particle_func) {
  return new CustomizableEffect(
    Renderer::get()->getRenderTime(),
    [=](float t) -> void {
      auto particle = particle_func(t);

      glm::mat4 transform(1.f);
      if (particle.billboard_type == ParticleInfo::SPHERICAL) {
        transform = makeSphericalBillboardTransform(particle.pos);
      } else if (particle.billboard_type == ParticleInfo::CYLINDRICAL) {
        transform = makeCylindricalBillboardTransform(
          particle.pos,
          particle.billboard_vec);
      } else if (particle.billboard_type == ParticleInfo::NONE) {
        // TODO(zack): maybe use orientation here?
       transform = glm::mat4(1.f);
      } else {
        invariant_violation("Unknown billboarding type");
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
