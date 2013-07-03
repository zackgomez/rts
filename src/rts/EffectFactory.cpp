#include "rts/EffectFactory.h"
#include "rts/Actor.h"
#include "rts/Renderer.h"
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
    const std::vector<int> &parts) {
  return [=](float dt) -> bool {
    return false;
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
      LOG(DEBUG) << "part damage: " << idx << '\n';
      ((Actor *)entity)->setTookDamage(idx);
      parts_vec.push_back(idx);
    }
    return makeOnDamageEffect(entity, name, parts_vec);
	} else {
		invariant_violation("unknown effect " + name);
	}
}

}  // rts
