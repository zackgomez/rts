#define GLM_SWIZZLE_XYZW
#include "Renderer.h"
#include <SDL/SDL.h>
#include <iostream>
#include <GL/glew.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "util.h"
#include "Engine.h"
#include "Entity.h"
#include "Map.h"
#include "Game.h"
#include "Unit.h"
#include "Building.h"
#include "ParamReader.h"
#include "Projectile.h"
#include "Player.h"
#include "MessageHub.h"
#include "FontManager.h"

namespace rts {

LoggerPtr OpenGLRenderer::logger_;

OpenGLRenderer::OpenGLRenderer(const glm::vec2 &resolution) :
  cameraPos_(0.f, 5.f, 0.f),
  resolution_(resolution),
  dragStart_(HUGE_VAL),
  dragEnd_(HUGE_VAL),
  lastRender_(0) {
  if (!logger_.get()) {
    logger_ = Logger::getLogger("OGLRenderer");
  }

  if (!initEngine(resolution_)) {
    logger_->fatal() << "Unable to initialize graphics resources\n";
    exit(1);
  }
  // Initialize font manager, if necessary
  FontManager::get();

  // Load resources
  mapProgram_ = loadProgram("shaders/map.v.glsl", "shaders/map.f.glsl");
  meshProgram_ = loadProgram("shaders/unit.v.glsl", "shaders/unit.f.glsl");
  // TODO(zack) read these and the transforms from params
  meshes_["unit"] = loadMesh("models/soldier.obj");
  meshes_["melee_unit"] = loadMesh("models/melee_unit.obj");
  meshes_["building"] = loadMesh("models/building.obj");
  meshes_["basic_bullet"] = loadMesh("models/projectile.obj");
  // unit model is based at 0, height 1, translate to center of model
  glm::mat4 unitMeshTrans = glm::scale(glm::mat4(1.f), glm::vec3(1, 0.5f, 1));
  setMeshTransform(meshes_["unit"], unitMeshTrans);
  setMeshTransform(
    meshes_["melee_unit"],
    glm::scale(unitMeshTrans, glm::vec3(2.f))
  );

  // TODO(zack) resource manager or something
  textures_["minimap"] = makeTexture(strParam("ui.minimap.image"));
  textures_["unitinfo"] = makeTexture(strParam("ui.unitinfo.image"));
  textures_["topbar"] = makeTexture(strParam("ui.topbar.image"));

  glm::mat4 projMeshTrans =
    glm::rotate(glm::mat4(1.f), 90.f, glm::vec3(1, 0, 0));
  setMeshTransform(meshes_["basic_bullet"], projMeshTrans);
}

OpenGLRenderer::~OpenGLRenderer() {
  for(Effect *effect : effects_) {
    delete effect;
  }
}

void OpenGLRenderer::renderMessages(const std::set<Message> &messages) {
  for (const Message &msg : messages) {
    if (msg["type"] == MessageTypes::ATTACK) {
      for (Json::Value _victim : msg["to"]) {
        id_t victim = toID(_victim);
        effects_.push_back(new BloodEffect(victim));
      }
    }
  }
}

void OpenGLRenderer::renderEntity(const Entity *entity) {
  glm::vec3 pos = entity->getPosition(simdt_);
  float rotAngle = entity->getAngle(simdt_);
  const std::string &type = entity->getType();

  // Interpolate if they move
  glm::mat4 transform = glm::scale(
                          glm::rotate(
                            glm::translate(glm::mat4(1.f), pos),
                            // TODO(zack) why does rotAngle need to be negative here?
                            // I think openGL may use a "left handed" coordinate system...
                            -rotAngle, glm::vec3(0, 1, 0)),
                          glm::vec3(entity->getRadius() / 0.5f));

  if (type == Unit::TYPE || type == Building::TYPE) {
    renderActor((const Actor *) entity, transform);
  } else if (type == Projectile::TYPE) {
    // TODO(zack): move to renderProjectile
    // TODO(zack): make this color to a param in Projectile
    glm::vec4 color = glm::vec4(0.5, 0.7, 0.5, 1);
    glUseProgram(meshProgram_);
    GLuint colorUniform = glGetUniformLocation(meshProgram_, "color");
    GLuint lightPosUniform = glGetUniformLocation(meshProgram_, "lightPos");
    glUniform4fv(colorUniform, 1, glm::value_ptr(color));
    glUniform3fv(lightPosUniform, 1, glm::value_ptr(lightPos_));
    invariant(meshes_.find(entity->getName()) != meshes_.end(),
              "cannot find mesh for entity " + entity->getName());
    renderMesh(transform, meshes_[entity->getName()]);
  } else {
    logger_->warning() << "Unable to render entity " << entity->getName() << '\n';
  }
}

void OpenGLRenderer::renderUI() {
  glm::vec2 res = vec2Param("window.resolution");

  glDisable(GL_DEPTH_TEST);

  // TODO(connor) there may be a better way to do this
  // perhaps store the names of all the UI elements in an array
  // and iterate over it?

  // top bar:
  glm::vec2 pos = vec2Param("ui.topbar.pos");
  if (pos.x < 0) {
    pos.x += res.x;
  }
  if (pos.y < 0) {
    pos.y += res.y;
  }
  glm::vec2 size = vec2Param("ui.topbar.dim");
  GLuint tex = textures_["topbar"];
  drawTexture(pos, size, tex);

  // minimap underlay
  pos = vec2Param("ui.minimap.pos");
  if (pos.x < 0) {
    pos.x += res.x;
  }
  if (pos.y < 0) {
    pos.y += res.y;
  }
  size = vec2Param("ui.minimap.dim");
  tex = textures_["minimap"];
  drawTexture(pos, size, tex);

  // unit info underlay:
  pos = vec2Param("ui.unitinfo.pos");
  if (pos.x < 0) {
    pos.x += res.x;
  }
  if (pos.y < 0) {
    pos.y += res.y;
  }
  size = vec2Param("ui.unitinfo.dim");
  tex = textures_["unitinfo"];
  drawTexture(pos, size, tex);

  glEnable(GL_DEPTH_TEST);
}

void OpenGLRenderer::renderMap(const Map *map) {
  const glm::vec2 &mapSize = map->getSize();

  const glm::vec4 mapColor(0.25, 0.2, 0.15, 1.0);

  glUseProgram(mapProgram_);
  GLuint colorUniform = glGetUniformLocation(mapProgram_, "color");
  GLuint mapSizeUniform = glGetUniformLocation(mapProgram_, "mapSize");
  glUniform4fv(colorUniform, 1, glm::value_ptr(mapColor));
  glUniform2fv(mapSizeUniform, 1, glm::value_ptr(mapSize));

  glm::mat4 transform = glm::rotate(
                          glm::scale(glm::mat4(1.f), glm::vec3(mapSize.x, 1.f, mapSize.y)),
                          90.f, glm::vec3(1, 0, 0));
  renderRectangleProgram(transform);

  // Render each of the highlights
for (auto& hl : highlights_) {
    hl.remaining -= renderdt_;
    glm::mat4 transform =
      glm::scale(
        glm::rotate(
          glm::translate(glm::mat4(1.f),
                         glm::vec3(hl.pos.x, 0.01f, hl.pos.y)),
          90.f, glm::vec3(1, 0, 0)),
        glm::vec3(0.33f));
    renderRectangleColor(transform, glm::vec4(1, 0, 0, 1));
  }
  // Remove done highlights
  for (size_t i = 0; i < highlights_.size(); ) {
    if (highlights_[i].remaining <= 0.f) {
      std::swap(highlights_[i], highlights_[highlights_.size() - 1]);
      highlights_.pop_back();
    } else {
      i++;
    }
  }

}

void OpenGLRenderer::startRender(float dt) {
  simdt_ = dt;
  if (lastRender_)
    renderdt_ = (SDL_GetTicks() - lastRender_) / 1000.f;
  else
    renderdt_ = 0;

  if (game_->isPaused()) {
    simdt_ = renderdt_ = 0.f;
  }

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Set up matrices
  float aspectRatio = resolution_.x / resolution_.y;
  float fov = 90.f;
  getProjectionStack().clear();
  getProjectionStack().current() =
    glm::perspective(fov, aspectRatio, 0.1f, 100.f);
  getViewStack().clear();
  getViewStack().current() =
    glm::lookAt(cameraPos_,
                glm::vec3(cameraPos_.x, 0, cameraPos_.z - 0.5f),
                glm::vec3(0, 0, -1));

  // Set up lights
  lightPos_ = applyMatrix(getViewStack().current(), glm::vec3(-5, 10, -5));

  // Display effects
  for (size_t i = 0; i < effects_.size(); i++) {
    Effect *effect = effects_[i];

    effect->render(renderdt_);

    if (effect->needsRemoval()) {
      delete effect;
      // Swap trick
      std::swap(effects_[i], effects_[effects_.size() - 1]);
      effects_.pop_back();
    }
  }

  // Clear coordinates
  ndcCoords_.clear();
  lastRender_ = SDL_GetTicks();
}

void OpenGLRenderer::renderActor(const Actor *actor, glm::mat4 transform) {
  const Player *player = game_->getPlayer(actor->getPlayerID());
  glm::vec3 pcolor = player ? player->getColor() : glm::vec3(0.f);
  // if selected draw as green
  glm::vec4 color = selection_.count(actor->getID())
                    ? glm::vec4(vec3Param("colors.selected"), 1.f)
                    : glm::vec4(pcolor, 1.f);
  // TODO(zack): Flash units white if damage taken
  const std::string &name = actor->getName();

  // TODO(zack) parameterize shaders on name
  glUseProgram(meshProgram_);
  GLuint colorUniform = glGetUniformLocation(meshProgram_, "color");
  GLuint lightPosUniform = glGetUniformLocation(meshProgram_, "lightPos");
  glUniform4fv(colorUniform, 1, glm::value_ptr(color));
  glUniform3fv(lightPosUniform, 1, glm::value_ptr(lightPos_));
  invariant(meshes_.find(name) != meshes_.end(),
            "missing mesh for actor " + name);
  renderMesh(transform, meshes_[name]);

  glm::vec4 ndc = getProjectionStack().current() * getViewStack().current() *
                  transform * glm::vec4(0, 0, 0, 1);
  ndc /= ndc.w;
  ndcCoords_[actor] = glm::vec3(ndc);

  // display health bar
  glDisable(GL_DEPTH_TEST);
  float healthFact = glm::max(0.f, actor->getHealth() / actor->getMaxHealth());
  glm::vec2 size = vec2Param("hud.actor_health.dim");
  glm::vec2 offset = vec2Param("hud.actor_health.pos");
  glm::vec2 pos = (glm::vec2(ndc.x, -ndc.y) / 2.f + glm::vec2(0.5f)) * resolution_;
  pos += offset;
  // Red underneath for max health
  drawRect(pos, size, glm::vec4(1, 0, 0, 1));
  // Green on top for current health
  pos.x -= size.x * (1.f - healthFact) / 2.f;
  size.x *= healthFact;
  drawRect(pos, size, glm::vec4(0, 1, 0, 1));

  std::queue<Actor::Production> queue = actor->getProductionQueue();
  if (!queue.empty()) {
    // display production bar
    float prodFactor = 1.f - queue.front().time / queue.front().max_time;
    size = vec2Param("hud.actor_prod.dim");
    offset = vec2Param("hud.actor_prod.pos");
    pos = (glm::vec2(ndc.x, -ndc.y) / 2.f + glm::vec2(0.5f)) * resolution_;
    pos += offset;
    // Purple underneath for max time
    drawRect(pos, size, glm::vec4(0.5, 0, 1, 1));
    // Blue on top for time elapsed
    pos.x -= size.x * (1.f - prodFactor) / 2.f;
    size.x *= prodFactor;
    drawRect(pos, size, glm::vec4(0, 0, 1, 1));
  }
  glEnable(GL_DEPTH_TEST);
}

void OpenGLRenderer::endRender() {
  glm::mat4 fullmat =
    getProjectionStack().current() * getViewStack().current();
  // Render drag selection if it exists
  if (dragStart_ != glm::vec3(HUGE_VAL)) {
    glm::vec2 start = applyMatrix(fullmat, dragStart_).xy;
    glm::vec2 end = applyMatrix(fullmat, dragEnd_).xy;
    start = (glm::vec2(start.x, -start.y) + glm::vec2(1.f)) * 0.5f * resolution_;
    end = (glm::vec2(end.x, -end.y) + glm::vec2(1.f)) * 0.5f * resolution_;

    glDisable(GL_DEPTH_TEST);

    // TODO(zack): make this color a param
    drawRect((start + end) / 2.f, end - start, glm::vec4(0.2f, 0.6f, 0.2f, 0.3f));
    // Reset each frame
    dragStart_ = glm::vec3(HUGE_VAL);
  }

  renderUI();

  SDL_GL_SwapBuffers();
}

void OpenGLRenderer::updateCamera(const glm::vec3 &delta) {
  cameraPos_ += delta;

  glm::vec2 mapSize = game_->getMap()->getSize() / 2.f;
  cameraPos_ = glm::clamp(
                 cameraPos_,
                 glm::vec3(-mapSize.x, 0.f, -mapSize.y),
                 glm::vec3(mapSize.x, 20.f, mapSize.y));
}

id_t OpenGLRenderer::selectEntity(const glm::vec2 &screenCoord) const {
  glm::vec2 pos = glm::vec2(screenToNDC(screenCoord));

  id_t eid = NO_ENTITY;
  float bestDist = HUGE_VAL;
  // Find the best entity
  for (auto& pair : ndcCoords_) {
    float dist = glm::distance(pos, glm::vec2(pair.second));
    // TODO(zack) transform radius and use it instead of hardcoded number..
    // Must be inside the entities radius and better than previous
    const float thresh = sqrtf(0.009f);
    if (dist < thresh && dist < bestDist) {
      bestDist = dist;
      eid = pair.first->getID();
    }
  }

  return eid;
}

std::set<id_t> OpenGLRenderer::selectEntities(
  const glm::vec3 &start, const glm::vec3 &end, id_t pid) const {
  assertPid(pid);
  glm::mat4 fullMatrix =
    getProjectionStack().current() * getViewStack().current();
  glm::vec2 s = applyMatrix(fullMatrix, start).xy;
  glm::vec2 e = applyMatrix(fullMatrix, end).xy;
  // Defines the rectangle
  glm::vec2 center = (e + s) / 2.f;
  glm::vec2 size = glm::abs(e - s);
  std::set<id_t> ret;

  for (auto &pair : ndcCoords_) {
    glm::vec2 p = glm::vec2(pair.second);
    // Inside rect and owned by player
    // TODO(zack) make this radius aware, right now the center must be in
    // their.
    if (fabs(p.x - center.x) < size.x / 2.f
        && fabs(p.y - center.y) < size.y / 2.f
        && pair.first->getPlayerID() == pid) {
      ret.insert(pair.first->getID());
    }
  }

  return ret;
}

void OpenGLRenderer::setSelection(const std::set<id_t> &select) {
  selection_ = select;
}

void OpenGLRenderer::highlight(const glm::vec2 &mapCoord) {
  MapHighlight hl;
  hl.pos = mapCoord;
  hl.remaining = 0.5f;
  highlights_.push_back(hl);
}

void OpenGLRenderer::setDragRect(const glm::vec3 &s, const glm::vec3 &e) {
  dragStart_ = s;
  dragEnd_ = e;
}

glm::vec3 OpenGLRenderer::screenToTerrain(const glm::vec2 &screenCoord) const {
  glm::vec4 ndc = glm::vec4(screenToNDC(screenCoord), 1.f);

  ndc = glm::inverse(getProjectionStack().current() * getViewStack().current()) * ndc;
  ndc /= ndc.w;

  glm::vec3 dir = glm::normalize(glm::vec3(ndc) - cameraPos_);

  glm::vec3 terrain = glm::vec3(ndc) - dir * ndc.y / dir.y;

  const glm::vec2 mapSize = game_->getMap()->getSize() / 2.f;
  // Don't return a non terrain coordinate
  if (terrain.x < -mapSize.x || terrain.x > mapSize.x
      || terrain.z < -mapSize.y || terrain.z > mapSize.y) {
    return glm::vec3(HUGE_VAL);
  }

  return glm::vec3(terrain);
}

glm::vec3 OpenGLRenderer::screenToNDC(const glm::vec2 &screen) const {
  float z;
  glReadPixels(screen.x, resolution_.y - screen.y, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &z);
  return glm::vec3(
      2.f * (screen.x / resolution_.x) - 1.f,
      2.f * (1.f - (screen.y / resolution_.y)) - 1.f,
      z);
}

BloodEffect::BloodEffect(id_t aid) :
  aid_(aid),
  t_(0.f) {

  assertEid(aid);
}

BloodEffect::~BloodEffect() {
  // nop
}

void BloodEffect::render(float dt) {
  // TODO(zack) replace this with a cool blood effect and or damage text
  t_ += dt;

  Actor *a = (Actor *) MessageHub::get()->getEntity(aid_);
  // TODO(zack): assert that this entity IS an actor
  if (!a) {
    // If entity died, no more effect
    t_ = HUGE_VAL;
    return;
  }

  glm::vec3 pos = a->getPosition() + glm::vec3(0.f, a->getHeight(), 0.f);
  float size = a->getRadius() / 2.5f;
  glm::mat4 transform =
    glm::rotate(
      glm::rotate(
        glm::scale(
          glm::translate(glm::mat4(1.f), pos),
          glm::vec3(size)
        ),
        rad2deg(M_PI * t_),
        glm::vec3(0, 1, 0)
      ),
      90.f,
      glm::vec3(1.f, 0.f, 0.f)
   );

  renderRectangleColor(transform, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
}

bool BloodEffect::needsRemoval() const {
  return t_ >= fltParam("effects.damageRenderDuration");
}

};
