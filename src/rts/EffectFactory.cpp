#include "rts/EffectFactory.h"
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

RenderFunction makeEntityEffect(
		const ModelEntity *entity,
    const std::string &name,
    v8::Handle<v8::Object> params) {
	using namespace v8;

	if (name == "heal") {
		return makeTextureBelowEffect(entity, 1.5f, "heal_icon", 2.f);
	} else {
		invariant_violation("unknown effect " + name);
	}
}

}  // rts
