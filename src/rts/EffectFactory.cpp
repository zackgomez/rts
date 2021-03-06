#include "rts/EffectFactory.h"
#include <sstream>
#include "common/ParamReader.h"
#include "rts/Renderer.h"
#include "rts/EffectManager.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
#include "rts/GameEntity.h"
#include "rts/GameScript.h"
#include "rts/Graphics.h"
#include "rts/ResourceManager.h"

namespace rts {

Effect *makeCannedParticleEffect(
    const std::string &name,
    const glm::vec3 &start,
    const glm::vec3 &end,
    const glm::vec3 &orientation) {
  auto duration = fltParam(name + ".duration");
  auto texture = ResourceManager::get()->getTexture(
      strParam(name + ".texture"));
  glm::vec2 size(fltParam(name + ".scale"));
  std::vector<glm::vec4> coords;
  Json::Value json_coord_array = getParam(name + ".coords");
  for (auto i = 0; i < json_coord_array.size(); i++) {
    coords.push_back(toVec4(json_coord_array[i]));
  }

  auto part_info_func = [=](float t) -> ParticleInfo {
    float u = t / duration;
    size_t coord_idx = glm::floor(u * coords.size());
    ParticleInfo ret;
    ret.pos = (1.f - u) * start + u * end;
    ret.size = size;
    ret.texture = texture;
    ret.texcoord = coords[coord_idx];
    ret.color = glm::vec4(1.f);
    ret.billboard_type = orientation == glm::vec3(0.f) ?
      ParticleInfo::SPHERICAL :
      ParticleInfo::CYLINDRICAL;
    ret.billboard_vec = orientation;
    return ret;
  };

  return makeParticleEffect(
      duration,
      part_info_func);
}

RenderFunction makeTextureBelowEffect(
		const ModelEntity *entity,
		float duration,
		const std::string &texture,
		float scale = 1.f) {
	GLuint tex = ResourceManager::get()->getTexture(texture);
	return [=](float dt) mutable -> bool {
		duration -= dt;
		if (duration < 0.f) {
			return false;
		}

    const float t = Renderer::get()->getGameTime();
		auto entityPos = entity->getPosition(t);
		auto entitySize = entity->getSize2(t);
		auto transform =
			glm::scale(
				glm::translate(
					glm::mat4(1.f),
					entityPos + glm::vec3(0, 0.f, 0.05f)),
				glm::vec3(0.25f + scale * sqrt(entitySize.x * entitySize.y)));

		renderRectangleTexture(transform, tex, glm::vec4(0, 0, 1, 1));

		return true;
	};
}

Effect * makeOnDamageEffect(
    const ModelEntity *e,
    float amount) {
  const float t = Renderer::get()->getGameTime();
  glm::vec3 pos = e->getPosition(t)
    + glm::vec3(0.f, 0.f, e->getHeight(t));
  glm::vec3 dir = glm::normalize(glm::vec3(
        frand() * 2.f - 1.f,
        frand() * 2.f - 1.f,
        frand() + 1.f));
  float a = fltParam("hud.damageTextGravity");
  float duration = 2.f;
  // TODO(zack): adjust color for AOE damage
  glm::vec3 color = glm::vec3(0.9f, 0.5f, 0.1f);
  std::stringstream ss;
  ss << FontManager::get()->makeColorCode(color)
    << glm::floor(amount);
  const std::string str = ss.str();
  return new CustomizableEffect(
      Renderer::get()->getRenderTime(),
      [=] (float t) -> void {
        auto resolution = Renderer::get()->getResolution();
        auto screen_pos = applyMatrix(
          getProjectionStack().current() * getViewStack().current(),
          pos + dir * (a * t * t + t));
        // ndc to resolution
        auto text_pos = resolution
          * (glm::vec2(screen_pos.x, -screen_pos.y) + 1.f) / 2.f;

        float font_height = fltParam("hud.damageTextFontHeight");
        glDisable(GL_DEPTH_TEST);
        FontManager::get()->drawString(str, glm::vec2(text_pos), font_height);
        glEnable(GL_DEPTH_TEST);
      },
      [=] (float t) -> bool {
        return t < duration;
      });
}

  void add_effect(const std::string &name, const Json::Value& params) {
  using namespace v8;
  // TODO(zack): this should be a param passed to this function
  float t = Renderer::get()->getGameTime();

  if (name == "teleport") {
    glm::vec3 start = glm::vec3(toVec2(params["start"]), 0.5f);
    glm::vec3 end = glm::vec3(toVec2(params["end"]), 0.5f);
    Renderer::get()->getEffectManager()->addEffect(
        makeCannedParticleEffect("effects.teleport", start, end, glm::vec3(0.f)));
  } else if (name == "snipe") {
    auto eid = must_have_idx(params, "eid").asString();
    auto *entity = Game::get()->getEntity(eid);
    if (!entity) {
      return;
    }
    // TODO(zack): get this position from the model (weapon tip)
    auto pos = glm::vec3(
        entity->getPosition2(t) + 1.0f * entity->getDirection(t),
        1.0f);
    auto shot_pos = pos + 1.5f * glm::vec3(entity->getDirection(t), 0.f);
    Renderer::get()->getEffectManager()->addEffect(
        makeCannedParticleEffect("effects.muzzle_flash", pos, pos, glm::vec3(0.f)));
    // TODO(zack): make this be cylindrically billboarded
    Renderer::get()->getEffectManager()->addEffect(
        makeCannedParticleEffect(
          "effects.snipe_shot",
          pos,
          shot_pos,
          glm::normalize(shot_pos - pos)));
  } else if (name == "heal_target") {
    auto eid = must_have_idx(params, "eid").asString();
    auto *entity = Game::get()->getEntity(eid);
		entity->addExtraEffect(
        makeTextureBelowEffect(entity, 3.5f, "heal_icon", 2.f));
  } else if (name == "on_damage") {
    auto eid = params["eid"].asString();
    auto *entity = Game::get()->getEntity(eid);
    if (!entity) {
      return;
    }
    auto parts = must_have_idx(params, "parts");
    std::vector<int> parts_vec;
    float amount = must_have_idx(params, "amount").asFloat();
    for (auto &&part_json : parts) {
      int idx = part_json.asInt();
      ((GameEntity *)entity)->setTookDamage(idx);
      parts_vec.push_back(idx);
      Renderer::get()->getEffectManager()->addEffect(makeOnDamageEffect(
            entity,
            amount));
    }
  } else {
    invariant_violation("Unknown effect " + name);
  }
}

}  // rts
