#include "rts/GameEntityRenderShim.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "common/Clock.h"
#include "common/ParamReader.h"
#include "rts/Actor.h"
#include "rts/Building.h"
#include "rts/CollisionObject.h"
#include "rts/Graphics.h"
#include "rts/Game.h"
#include "rts/Map.h"
#include "rts/Player.h"
#include "rts/Projectile.h"
#include "rts/Renderer.h"
#include "rts/ResourceManager.h"
#include "rts/Unit.h"

namespace rts {

glm::mat4 GameEntityRenderShim::getTransform(float dt) const {
  const glm::vec2 pos2 = getPosition(dt);
  const float rotAngle = getAngle(dt);
  const glm::vec3 pos = glm::vec3(pos2, Game::get()->getMap()->getMapHeight(pos2));
  float modelSize = 1.f;
  if (hasParam(getName() + ".modelSize")) {
    modelSize = fltParam(getName() + ".modelSize");
  }

  return
    glm::scale(
      glm::rotate(
        glm::rotate(
          glm::translate(glm::mat4(1.f), pos),
          rotAngle, glm::vec3(0, 0, 1)),
        90.f, glm::vec3(1, 0, 0)),
      glm::vec3(modelSize));
}

std::string GameEntityRenderShim::getShaderName() const {
  return "unit";
}

std::string GameEntityRenderShim::getMeshName() const {
  return strParam(getName() + ".model");
}

glm::vec4 GameEntityRenderShim::getColor() const {
  const Player *player = Game::get()->getPlayer(getPlayerID());
  glm::vec3 pcolor = player ? player->getColor() :
    vec3Param("global.defaultColor");
  return glm::vec4(pcolor, 1.f);
}

void GameEntityRenderShim::render(float dt) {
  glm::mat4 transform = getTransform(dt);

  if (hasProperty(GameEntity::P_COLLIDABLE)) {
    record_section("collidable");
    // render collision rect
    glm::mat4 transform =
      glm::scale(
          glm::rotate(
            glm::translate(glm::mat4(1.f),
              glm::vec3(getPosition(dt), 0.005f)),
            getAngle(dt), glm::vec3(0, 0, 1)),
          glm::vec3(getSize(), 1.f));
    glm::vec4 color(0, 0, 0, 1);
    renderRectangleColor(transform, color);
  }

  // Collision objects only need to render their collision rectangle, so we
  // can return after we do that.
  auto type = getType();
  if (type == CollisionObject::TYPE) return;

  renderMesh(transform);

  if (type == Unit::TYPE || type == Building::TYPE) {
    //renderActor(transform);
  } else if (type != Projectile::TYPE) {
    LOG(WARNING) << "Unable to render entity " << getName() << '\n';
  }
}

void GameEntityRenderShim::renderMesh(const glm::mat4 &transform) {
    GLuint meshProgram = ResourceManager::get()->getShader(getShaderName());
    glUseProgram(meshProgram);
    GLuint colorUniform = glGetUniformLocation(meshProgram, "color");
    GLuint lightPosUniform = glGetUniformLocation(meshProgram, "lightPos");
    glUniform4fv(colorUniform, 1, glm::value_ptr(getColor()));
    auto lightPos = vec3Param("renderer.lightPos");
    glUniform3fv(lightPosUniform, 1, glm::value_ptr(lightPos));
    Mesh * mesh = ResourceManager::get()->getMesh(getMeshName());
    ::renderMesh(transform, mesh);
}

}  // namespace rts
