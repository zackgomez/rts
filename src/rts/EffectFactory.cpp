#include "rts/EffectFactory.h"
#include <sstream>
#include "common/ParamReader.h"
#include "rts/Renderer.h"
#include "rts/EffectManager.h"
#include "rts/FontManager.h"
#include "rts/Game.h"
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

void add_jseffect(const std::string &name, v8::Handle<v8::Object> params) {
  auto *script = Game::get()->getScript();
  auto json_params = script->jsToJSON(params);
  if (name == "teleport") {
    glm::vec3 start = glm::vec3(toVec2(json_params["start"]), 0.5f);
    glm::vec3 end = glm::vec3(toVec2(json_params["end"]), 0.5f);
    Renderer::get()->getEffectManager()->addEffect(
        makeCannedParticleEffect("effects.teleport", start, end, glm::vec3(0.f)));
  } else if (name == "snipe") {
    id_t eid = params->Get(v8::String::New("source_eid"))->IntegerValue();
    auto *entity = Game::get()->getEntity(eid);
    if (!entity) {
      return;
    }
    // TODO(zack): get this position from the model (weapon tip)
    auto pos = glm::vec3(
        entity->getPosition2() + 1.0f * entity->getDirection(),
        1.0f);
    auto shot_pos = pos + 1.5f * glm::vec3(entity->getDirection(), 0.f);
    Renderer::get()->getEffectManager()->addEffect(
        makeCannedParticleEffect("effects.muzzle_flash", pos, pos, glm::vec3(0.f)));
    // TODO(zack): make this be cylindrically billboarded
    Renderer::get()->getEffectManager()->addEffect(
        makeCannedParticleEffect(
          "effects.snipe_shot",
          pos,
          shot_pos,
          glm::normalize(shot_pos - pos)));
  } else {
    invariant_violation("Unknown effect " + name);
  }
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

		auto entityPos = entity->getPosition(Renderer::get()->getSimDT());
		auto entitySize = entity->getSize();
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
  glm::vec3 pos = e->getPosition(Renderer::get()->getSimDT())
    + glm::vec3(0.f, 0.f, e->getHeight());
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

RenderFunction makeEntityEffect(
		const ModelEntity *entity,
    const std::string &name,
    v8::Handle<v8::Object> params) {
	using namespace v8;

	if (name == "heal") {
		return makeTextureBelowEffect(entity, 3.5f, "heal_icon", 2.f);
  } else if (name == "on_damage") {
    auto parts_handle = params->Get(String::New("parts"));
    invariant(!parts_handle.IsEmpty(), "missing 'parts' for on_damage message");
    auto parts = Handle<Array>::Cast(parts_handle);
    invariant(!parts.IsEmpty(), "expected 'parts' array for on_damage message");
    std::vector<int> parts_vec;
    float amount = params->Get(String::New("amount"))->NumberValue();
    for (int i = 0; i < parts->Length(); i++) {
      int idx = parts->Get(i)->IntegerValue();
      ((GameEntity *)entity)->setTookDamage(idx);
      parts_vec.push_back(idx);
      Renderer::get()->getEffectManager()->addEffect(makeOnDamageEffect(
            entity,
            amount));
    }
    return nullptr;
	} else {
		invariant_violation("unknown effect " + name);
	}
}

}  // rts
