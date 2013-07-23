#include "rts/EffectFactory.h"
#include <sstream>
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/Renderer.h"
#include "rts/FontManager.h"
#include "rts/Graphics.h"
#include "rts/ResourceManager.h"

namespace rts {

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

RenderFunction makeOnDamageEffect(
    const ModelEntity *e,
    const std::string &name,
    float amount,
    const std::vector<int> &parts) {
  glm::vec3 pos = e->getPosition(Renderer::get()->getSimDT())
    + glm::vec3(0.f, 0.f, e->getHeight());
  glm::vec3 dir = glm::normalize(glm::vec3(
        frand() * 2.f - 1.f,
        frand() * 2.f - 1.f,
        frand() + 1.f));
  float t = 0.f;
  float a = fltParam("hud.damageTextGravity");
  float duration = 2.f;
  // TODO(zack): adjust color for AOE damage
  glm::vec3 color = glm::vec3(0.9f, 0.5f, 0.1f);
  //glm::vec3 color = glm::vec3(1.f, 1.f, 1.f);
  return [=](float dt) mutable -> bool {
    t += dt;
    if (t > duration) {
      return false;
    }

    auto resolution = Renderer::get()->getResolution();
    auto screen_pos = applyMatrix(
        getProjectionStack().current() * getViewStack().current(),
        pos + dir * (a * t * t + t));
    // ndc to resolution
    auto text_pos = (glm::vec2(screen_pos.x, -screen_pos.y) + 1.f) / 2.f * resolution;

    float font_height = fltParam("hud.damageTextFontHeight");
    std::stringstream ss;
    ss << FontManager::get()->makeColorCode(color)
      << glm::floor(amount);
    glDisable(GL_DEPTH_TEST);
    FontManager::get()->drawString(ss.str(), glm::vec2(text_pos), font_height);
    glEnable(GL_DEPTH_TEST);

    return true;
  };
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
    for (int i = 0; i < parts->Length(); i++) {
      int idx = parts->Get(i)->IntegerValue();
      ((Actor *)entity)->setTookDamage(idx);
      parts_vec.push_back(idx);
    }
    float amount = params->Get(String::New("amount"))->NumberValue();
    return makeOnDamageEffect(entity, name, amount, parts_vec);
	} else {
		invariant_violation("unknown effect " + name);
	}
}

}  // rts
