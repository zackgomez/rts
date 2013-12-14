#define GLM_SWIZZLE
#include "rts/GameController.h"
#include <iomanip>
#include <sstream>
#include <glm/gtx/norm.hpp>
#include "common/util.h"
#include "common/ParamReader.h"
#include "rts/ActionWidget.h"
#include "rts/ActorPanelWidget.h"
#include "rts/CommandWidget.h"
#include "rts/Game.h"
#include "rts/GameScript.h"
#include "rts/Input.h"
#include "rts/Map.h"
#include "rts/Matchmaker.h"
#include "rts/MinimapWidget.h"
#include "rts/Player.h"
#include "rts/PlayerAction.h"
#include "rts/Renderer.h"
#include "rts/UI.h"
#include "rts/Widgets.h"

#ifdef _MSC_VER
#define isinf(x) (!_finite(x))
#endif

namespace rts {

namespace PlayerState {
const std::string DEFAULT = "DEFAULT";
const std::string CHATTING = "CHATTING";
}

struct VPInfo {
  id_t owner;
  id_t capper;
  float cap_fact;
};

static void renderEntity(
    const LocalPlayer *localPlayer,
    const std::map<id_t, float>& entityHighlights,
    const ModelEntity *e,
    float t);

static void renderHighlights(
    std::vector<MapHighlight> &highlights,
    float dt) {
  // Render each of the highlights
  for (auto& hl : highlights) {
    hl.remaining -= dt;
    glm::mat4 transform =
      glm::scale(
          glm::translate(
            glm::mat4(1.f),
            glm::vec3(hl.pos.x, hl.pos.y, 0.01f)),
          glm::vec3(0.33f));
    renderCircleColor(
        transform,
        glm::vec4(1, 0, 0, 1),
        fltParam("ui.highlight.thickness"));
  }
}

void renderDragRect(bool enabled, const glm::vec2 &start, const glm::vec2 &end, float dt) {
  if (!enabled) {
    return;
  }
  float dist = glm::distance(start, end);
  if (dist < fltParam("hud.minDragDistance")) {
    return;
  }
  // TODO(zack): make this color a param
  auto color = glm::vec4(0.2f, 0.6f, 0.2f, 0.3f);
  drawRect(start, end - start, color);
}

// Todo. This gets called before JS is initialized. Return 0 until we know 
// what the hell is going on.
std::string getVPString(id_t team) {
  return std::to_string((int)Game::get()->getVictoryPoints(team));
}

GameController::GameController(LocalPlayer *player)
  : player_(player),
    shift_(false),
    ctrl_(false),
    alt_(false),
    leftDrag_(false),
    leftDragMinimap_(false),
    state_(PlayerState::DEFAULT),
    renderNavMesh_(false),
    visData_(nullptr),
    visDataLength_(0),
    order_(),
    zoom_(0.f),
    action_() {
  gameScript_ = new GameScript();
}

GameController::~GameController() {
}

void call_js_controller_init(v8::Handle<v8::Object> controller) {
  using namespace v8;
  HandleScope handle_scope(Isolate::GetCurrent());
  TryCatch try_catch;
  auto js_init_func = controller->Get(v8::String::New("init"));
  invariant(
    js_init_func->IsFunction(),
    "must have jsController init func");
  auto jsparams = v8::Object::New();
  jsparams->Set(
    v8::String::New("map_size"),
    vec2ToJS(Game::get()->getMap()->getSize()));
  const int argc = 1;
  v8::Handle<v8::Value> argv[] = {
    jsparams,
  };
  auto js_ret = v8::Handle<v8::Function>::Cast(js_init_func)
    ->Call(controller, argc, argv);
  checkJSResult(js_ret, try_catch, "js controller init");
}

  v8::Handle<v8::Object> GameController::getJSController() {
    return v8::Handle<v8::Object>::Cast(gameScript_->getInitReturn());
  }

void GameController::onCreate() {
  gameScript_->init("ui-main");
  ENTER_GAMESCRIPT(gameScript_);
  call_js_controller_init(getJSController());

  grab_mouse();
  // TODO(zack): delete texture
  glGenTextures(1, &visTex_);
  glBindTexture(GL_TEXTURE_2D, visTex_);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  createWidgets(getUI(), "ui.widgets");

  using namespace std::placeholders;

  auto minimapWidget =
      new MinimapWidget("ui.widgets.minimap", player_->getPlayerID());
  getUI()->addWidget("ui.widgets.minimap", minimapWidget);
  minimapWidget->setMinimapListener(
    [&](const glm::vec2 &pos, int button) {
      if (button == MouseButton::LEFT) {
        leftDragMinimap_ = true;
      }
      if (button == MouseButton::RIGHT) {
        glm::vec3 v3;
        v3.x = pos.x * Renderer::get()->getMapSize().x;
        v3.y = -1 * pos.y * Renderer::get()->getMapSize().y;
        Json::Value order = handleRightClick(nullptr, v3);
        attemptIssueOrder(order);
      }
    });

  auto actionFunc = [=]() -> std::vector<UIAction> {
    auto selection = player_->getSelection();
    if (selection.empty()) {
      return std::vector<UIAction>();
    }
    auto *entity = Game::get()->getEntity(*selection.begin());
    if (!entity) {
      return std::vector<UIAction>();
    }
    return entity->getActions();
  };
  auto actionExecutor = [=](const UIAction &action) {
    handleUIAction(action);
  };

  auto actionWidget = new ActionWidget("ui.widgets.action", actionFunc, actionExecutor);
  getUI()->addWidget("ui.widgets.action", actionWidget);

  auto panelWidget = new ActorPanelWidget(
      "ui.widgets.actor_panel",
      [=]() -> const GameEntity * {
        const auto &sel = player_->getSelection();
        if (sel.empty()) {
          return nullptr;
        }
        return Game::get()->getEntity(*sel.begin());
      },
      [=](const GameEntity::UIPartUpgrade &upgrade) {
        if (player_->getSelection().empty()) {
          return;
        }
        Json::Value order;
        order["type"] = OrderTypes::UPGRADE;
        order["entity"] = toJson(player_->getSelection());
        order["part"] = upgrade.part;
        order["upgrade"] = upgrade.name;

        attemptIssueOrder(order);
      });
  getUI()->addWidget("ui.widgets.actor_panel", panelWidget);

  auto chatWidget = new CommandWidget("ui.chat");
  chatWidget->setCloseOnSubmit(true);
  chatWidget->setOnTextSubmittedHandler([&, chatWidget](const std::string &t) {
    PlayerAction msg;
    msg["type"] = ActionTypes::CHAT;

    std::string text = t;
    if (t.find("/all ") == 0) {
      msg["target"] = "all";
      // Remove command text
      text = t.substr(5);
    } else {
      msg["target"] = "team";
    }
    // In future add additional check for whisper
    if (!text.empty()) {
      msg["chat"] = text;
      Game::get()->addAction(player_->getPlayerID(), msg);
    }
  });
  getUI()->addWidget("ui.widgets.chat", chatWidget);

  Game::get()->setChatListener([&](id_t pid, const Json::Value &m) {
    const Player* from = Game::get()->getPlayer(pid);
    invariant(from, "No playyayayaya");
    std::stringstream ss;
    if (m["target"] == "all" || (m["target"] == "team" && from->getTeamID() == player_->getTeamID())) {
      ss << from->getName();
      if (m["target"] == "all") {
        ss << " (all)";
      }
      ss << ": " << m["chat"].asString();
      ((CommandWidget *)getUI()->getWidget("ui.widgets.chat"))
        ->addMessage(ss.str())
        ->show(fltParam("ui.chat.chatDisplayTime"));
    } // Note: When we add whisper, append additional conditional
  });

  Renderer::get()->setEntityOverlayRenderer(
      std::bind(
        renderEntity,
        player_,
        std::ref(entityHighlights_),
        _1,
        _2));

  ((TextWidget *)getUI()->getWidget("ui.widgets.vicdisplay-1"))
    ->setTextFunc(std::bind(getVPString, STARTING_TID));
  ((TextWidget *)getUI()->getWidget("ui.widgets.vicdisplay-1"))
    ->setColor(getTeamColor(STARTING_TID));

  ((TextWidget *)getUI()->getWidget("ui.widgets.vicdisplay-2"))
    ->setTextFunc(std::bind(getVPString, STARTING_TID + 1));
  ((TextWidget *)getUI()->getWidget("ui.widgets.vicdisplay-2"))
    ->setColor(getTeamColor(STARTING_TID + 1));

  ((TextWidget *)getUI()->getWidget("ui.widgets.reqdisplay"))
    ->setTextFunc([&]() -> std::string {
      int req = Game::get()->getRequisition(player_->getPlayerID());
      return "Req: " + std::to_string(req);
    });

  ((TextWidget *)getUI()->getWidget("ui.widgets.perfinfo"))
    ->setTextFunc([]() -> std::string {
        glm::vec3 color(0.1f, 1.f, 0.1f);
        int fps = Renderer::get()->getAverageFPS();
        return std::string() + FontManager::makeColorCode(color)
        + "FPS: " + std::to_string(fps);
    });


  ((TextWidget *)getUI()->getWidget("ui.widgets.clock"))
    ->setTextFunc([]() -> std::string {
      glm::vec3 color(0.1f, 1.f, 0.1f);
      float t = Game::get()->getElapsedTime();
      float minutes = glm::floor(t / 60.f);
      float seconds = glm::floor(fmodf(t, 60.f));
      std::stringstream ss;
      ss << FontManager::makeColorCode(color)
        << "Time " << minutes << ":" << (seconds < 10 ? "0" : "") << seconds;
      return ss.str();
  });
  Renderer::get()->resetCameraRotation();
}

void GameController::onDestroy() {
  release_mouse();
  Renderer::get()->setEntityOverlayRenderer(Renderer::EntityOverlayRenderer());
  getUI()->clearWidgets();
  glDeleteTextures(1, &visTex_);
  
}

glm::vec4 GameController::getTeamColor(const id_t tid) const {
  glm::vec4 tcolor;
  id_t pid = STARTING_PID;
  while (true) {
    const Player *p = Game::get()->getPlayer(pid);
    if (p->getTeamID() == tid) {
      tcolor = glm::vec4(p->getColor(), 1);
      break;
    }
    pid++;
  }
  return tcolor;
}

void GameController::renderExtra(float dt) {
  renderDragRect(leftDrag_, leftStart_, lastMousePos_, dt);
  renderHighlights(highlights_, dt);

  if (!action_.name.empty()) {
    const GameEntity *e = Game::get()->getEntity(action_.render_id);
    invariant(e, "Unable to find action owner");

    float t = Renderer::get()->getGameTime();
    glm::vec3 owner_pos = e->getPosition(t) + glm::vec3(0, 0, 0.1f);
    glm::mat4 transform = glm::scale(
        glm::translate(glm::mat4(1.f), owner_pos),
        glm::vec3(2 * action_.range));

    renderCircleColor(transform, glm::vec4(0, 0, 1, 1.5));

    if (action_.radius > 0.f) {
      glm::vec3 pos = action_.targeting == UIAction::TargetingType::NONE
        ? owner_pos
        : Renderer::get()->screenToTerrain(lastMousePos_);
      glm::mat4 transform = glm::scale(
          glm::translate(glm::mat4(1.f), pos),
          glm::vec3(2 * action_.radius));
      renderCircleColor(transform, glm::vec4(0, 0, 1, 1.0));
    }
  }

  // TODO(zack): bit of hack here
  if (renderNavMesh_) {
    renderNavMesh(*Game::get()->getMap()->getNavMesh(),
        glm::scale(glm::translate(glm::mat4(1.f), glm::vec3(0, 0, 0.15f)), glm::vec3(0.8)),
        glm::vec4(0.6, 0.6, 0.2, 0.75f));
  }

  // Get vp infomation
  std::vector<VPInfo> vp_infos;
  for (auto it : Renderer::get()->getEntities()) {
    if (it.second->hasProperty(GameEntity::P_ACTOR)) {
      auto ui_info = ((GameEntity *)it.second)->getUIInfo();
      if (ui_info.extra.isMember("vp_status")) {
        auto vp_status = ui_info.extra["vp_status"];
        auto owner_json = must_have_idx(vp_status, "owner");
        auto capper_json = must_have_idx(vp_status, "capper");
        float cap_fact = ui_info.capture[1] ?
          glm::clamp(ui_info.capture[0] / ui_info.capture[1], 0.f, 1.f) :
          0.f;
        VPInfo vp_info = {
          owner_json.isNull() ? NO_PLAYER : toID(owner_json),
          capper_json.isNull() ? NO_PLAYER : toID(capper_json),
          cap_fact,
        };
        vp_infos.push_back(vp_info);
      }
    }
  }

  //TODO(connor): maybe make these a function elsewhere
  // render scorebar
  int t1score = Game::get()->getVictoryPoints(STARTING_TID);
  int t2score = Game::get()->getVictoryPoints(STARTING_TID + 1);
  int totalscore = fltParam("global.pointsToWin");
  float factor =
    (float)(totalscore - t2score) / (2 * totalscore - t1score - t2score);
  Shader *shader = ResourceManager::get()->getShader("scorebar");
  shader->makeActive();
  glm::vec2 center = uiVec2Param("ui.widgets.scorebar.center");
  glm::vec2 size = uiVec2Param("ui.widgets.scorebar.dim");
  shader->uniform1f("factor", factor);

  // get team colors
  glm::vec4 t1color = getTeamColor(STARTING_TID);
  glm::vec4 t2color = getTeamColor(STARTING_TID + 1);
  glm::vec2 vp_count(0);
  for (auto vp_info : vp_infos) {
    if (!vp_info.owner) {
      continue;
    }
    auto *player = Game::get()->getPlayer(vp_info.owner);
    if (player->getTeamID() == STARTING_TID) vp_count[0] += 1;
    if (player->getTeamID() == STARTING_TID + 1) vp_count[1] += 1;
  }
  shader->uniform2f("vp_count", vp_count);
  shader->uniform4f("color1", t1color);
  shader->uniform4f("color2", t2color);
  shader->uniform4f("texcoord", glm::vec4(0, 0, 1, 1));
  GLuint tex = ResourceManager::get()->getTexture("lightning1");
  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, tex);
  shader->uniform1i("tex", 0);
  float t = Renderer::get()->getRenderTime();
  shader->uniform1f("t", t);

  drawShaderCenter(center, size);

  // render VP controller indicators
  int vpi = 0;
  for (auto vp_info : vp_infos) {
    glm::vec2 widget_center = uiVec2Param("ui.widgets.vpindicators.center");
    glm::vec2 size = vec2Param("ui.widgets.vpindicators.dim");
    glm::vec4 ownerColor = vec4Param("ui.widgets.vpindicators.bgcolor");
    glm::vec4 capColor = vec4Param("ui.widgets.vpindicators.bgcolor");

    glm::vec2 center = widget_center +
      glm::vec2((vpi - (int)(vp_infos.size() / 2)) * size.x * 1.5, 0);
    ++vpi;

    id_t pid = vp_info.owner;
    if (pid != NO_PLAYER) {
      ownerColor = glm::vec4(Game::get()->getPlayer(pid)->getColor(), 1.0);
    }

    int danger = 0;
    id_t cap_pid = vp_info.capper;
    if (cap_pid != NO_PLAYER) {
      capColor = glm::vec4(Game::get()->getPlayer(cap_pid)->getColor(), 1.0);
      // Danger if being capped away from local player
      danger = pid == player_->getPlayerID();
    }

    float capture = vp_info.cap_fact;

    Shader *shader = ResourceManager::get()->getShader("vp_indicator");
    shader->makeActive();
    shader->uniform1f("capture", capture);
    shader->uniform1i("danger", danger);
    shader->uniform4f("player_color", ownerColor);
    shader->uniform4f("cap_color", capColor);
    shader->uniform4f("texcoord", glm::vec4(0, 0, 1, 1));
    shader->uniform1f("t", t);
    drawShaderCenter(center, size);
  }

  renderCursor(getCursorTexture());
}

std::string GameController::getCursorTexture() const {
  std::string texname = "cursor_normal";
  if (!action_.name.empty()) {
    texname = "cursor_action";
  }
  return texname;
}

void GameController::updateVisibility(float t) {

  ENTER_GAMESCRIPT(gameScript_);
  auto js_controller = getJSController();
  auto js_entities = v8::Array::New();
  for (auto &pair : Renderer::get()->getEntities()) {
    auto *entity = pair.second;
    if (!entity->hasProperty(GameEntity::P_GAMEENTITY)) {
      continue;
    }
    auto *game_entity = (GameEntity *)entity;
    game_entity->setVisible(
        game_entity->isVisibleTo(t, player_->getPlayerID()));

    // TODO(zack): this needs to be kept in sync with JS and is brittle
    if (game_entity->getPlayerID() == player_->getPlayerID()) {
      auto js_ent = v8::Object::New();
      js_ent->Set(v8::String::New("sight"), v8::Number::New(game_entity->getSight()));
      js_ent->Set(v8::String::New("pos"), vec2ToJS(game_entity->getPosition2(t)));
      js_entities->Set(js_entities->Length(), js_ent);
    }
  }

  auto js_update_func = v8::Handle<v8::Function>::Cast(
    js_controller->Get(v8::String::New("update")));
  const int argc = 1;
  v8::Handle<v8::Value> argv[] = { js_entities };
  js_update_func->Call(js_controller, argc, argv);


  size_t len = visDim_.x * visDim_.y;
  uint8_t *data = new uint8_t[len];
  for (auto i = 0; i < len; i++) {
    data[i] = 0;
  }
  glBindTexture(GL_TEXTURE_2D, visTex_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, visDim_.x, visDim_.y, 0, GL_ALPHA, GL_UNSIGNED_BYTE, visData_);
  delete[] data;
}

void GameController::frameUpdate(float dt) {
  float t = Renderer::get()->getGameTime();
  updateVisibility(t);
  // Remove done highlights
  for (size_t i = 0; i < highlights_.size(); ) {
    if (highlights_[i].remaining <= 0.f) {
      std::swap(highlights_[i], highlights_[highlights_.size() - 1]);
      highlights_.pop_back();
    } else {
      i++;
    }
  }

  auto it = entityHighlights_.begin();
  while (it != entityHighlights_.end()) {
    auto cur = (entityHighlights_[it->first] -= dt);
    if (cur <= 0.f) {
      it = entityHighlights_.erase(it);
    } else {
      it++;
    }
  }

  auto mouse_state = getMouseState();
  int x = mouse_state.screenpos.x;
  int y = mouse_state.screenpos.y;
  const glm::vec2 &res = Renderer::get()->getResolution();
  const int CAMERA_PAN_THRESHOLD = std::max(
    intParam("local.camera.panthresh"),
    1);  // to prevent division by zero

  // Mouse overrides keyboard
  glm::vec2 dir = cameraPanDir_;

  if (x <= CAMERA_PAN_THRESHOLD) {
    dir.x = 2 * (x - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  } else if (x >= res.x - CAMERA_PAN_THRESHOLD) {
    dir.x = -2 * ((res.x - x) - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  }
  if (y <= CAMERA_PAN_THRESHOLD) {
    dir.y = -2 * (y - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  } else if (y >= res.y - CAMERA_PAN_THRESHOLD) {
    dir.y = 2 * ((res.y - y) - CAMERA_PAN_THRESHOLD) / CAMERA_PAN_THRESHOLD;
  }
  Renderer::get()->zoomCamera(zoom_);

  // Don't allow screen translations during rotation
  if (!alt_) {
    const float CAMERA_PAN_SPEED = fltParam("local.camera.panspeed");
    glm::vec3 delta = CAMERA_PAN_SPEED * dt * glm::vec3(dir.x, dir.y, 0.f);
    Renderer::get()->updateCamera(delta);
  }

  // Deselect dead entities
  std::set<std::string> newsel;
  for (auto game_id : player_->getSelection()) {
    const GameEntity *e = Game::get()->getEntity(game_id);
    if (e && e->getPlayerID(t) == player_->getPlayerID()) {
      newsel.insert(game_id);
    }
  }
  if (!action_.name.empty() && !newsel.count(action_.owner_id)) {
    action_.name.clear();
  }
  player_->setSelection(newsel);

  if (leftDragMinimap_) {
    minimapUpdateCamera(mouse_state.screenpos);
  }
}

void GameController::quitEvent()
{
  // Send the quit game event
  PlayerAction action;
  action["type"] = ActionTypes::LEAVE_GAME;
  Game::get()->addAction(player_->getPlayerID(), action);
}

void GameController::mouseDown(const glm::vec2 &screenCoord, int button) {
  if (alt_) {
    return;
  }
  Json::Value order;
  std::set<std::string> newSelect = player_->getSelection();
  const float t = Renderer::get()->getGameTime();

  glm::vec3 loc = Renderer::get()->screenToTerrain(screenCoord);
  if (isinf(loc.x) || isinf(loc.y)) {
    return;
  }
  // The entity (maybe) under the cursor
  const GameEntity *entity = selectEntity(screenCoord);

  if (button == MouseButton::LEFT) {
    if (!order_.empty()) {
      order["type"] = order_;
      order["entity"] = toJson(player_->getSelection());
      order["target"] = toJson(loc);
      if (entity && entity->getTeamID(t) != player_->getTeamID()) {
        order["target_id"] = entity->getGameID();
      }
      order_.clear();
    } else if (!action_.name.empty()) {
      if (!newSelect.count(action_.owner_id)) {
        return;
      }
      std::set<std::string> ids(&action_.owner_id, (&action_.owner_id)+1);
      if (action_.targeting == UIAction::TargetingType::NONE) {
        order["type"] = OrderTypes::ACTION;
        order["entity"] = toJson(ids);
        order["action"] = action_.name;
      } else if (action_.targeting == UIAction::TargetingType::LOCATION) {
        order["type"] = OrderTypes::ACTION;
        order["entity"] = toJson(ids);
        order["action"] = action_.name;
        order["target"] = toJson(glm::vec2(loc));
      } else if (action_.targeting == UIAction::TargetingType::ENEMY) {
        if (!entity
            || !entity->hasProperty(GameEntity::P_TARGETABLE)
            || entity->getTeamID(t) == NO_TEAM
            || entity->getTeamID(t) == player_->getTeamID()) {
          return;
        }
        order["type"] = OrderTypes::ACTION;
        order["entity"] = toJson(ids);
        order["action"] = action_.name;
        order["target_id"] = entity->getGameID();
        highlightEntity(entity->getID());
      } else if (action_.targeting == UIAction::TargetingType::ALLY) {
        if (!entity
            || !entity->hasProperty(GameEntity::P_TARGETABLE)
            || entity->getTeamID(t) == NO_TEAM
            || entity->getTeamID(t) != player_->getTeamID()) {
          return;
        }
        order["type"] = OrderTypes::ACTION;
        order["entity"] = toJson(ids);
        order["action"] = action_.name;
        order["target_id"] = entity->getGameID();
        highlightEntity(entity->getID());
      } else if (action_.targeting == UIAction::TargetingType::PATHABLE) {
        if (Game::get()->getMap()->getNavMesh()->isPathable(glm::vec2(loc))) {
          order["type"] = OrderTypes::ACTION;
          order["entity"] = toJson(ids);
          order["action"] = action_.name;
          order["target"] = toJson(glm::vec2(loc));
        }
      } else {
        invariant_violation("Unsupported targeting type");
      }
      action_.name.clear();
    // If no order, then adjust selection
    } else {
      leftStart_ = screenCoord;
      leftDrag_ = true;
      // If no shift held, then deselect (shift just adds)
      if (!shift_) {
        newSelect.clear();
      }
      // If there is an entity and its ours, select
      if (entity && entity->getPlayerID(t) == player_->getPlayerID()) {
        newSelect.insert(entity->getGameID());
      }
    }
  } else if (button == MouseButton::RIGHT) {
    order = handleRightClick(entity, loc);
  } else if (button == MouseButton::WHEEL_UP) {
    Renderer::get()->zoomCamera(-fltParam("local.mouseZoomSpeed"));
  } else if (button == MouseButton::WHEEL_DOWN) {
    Renderer::get()->zoomCamera(fltParam("local.mouseZoomSpeed"));
  }

  // Mutate
  player_->setSelection(newSelect);
  attemptIssueOrder(order);
}

void GameController::attemptIssueOrder(Json::Value order) {
  if (order.isMember("type")) {
    PlayerAction action;
    action["type"] = ActionTypes::ORDER;
    action["order"] = order;
    Game::get()->addAction(player_->getPlayerID(), action);
  }
}

Json::Value GameController::handleRightClick(
    const GameEntity *entity,
    const glm::vec3 &loc) {
  Json::Value order;
  const float t = Renderer::get()->getGameTime();
  // TODO(connor) make right click actions on minimap
  // If there is an order, it is canceled by right click
  if (!order_.empty() || !action_.name.empty()) {
    order_.clear();
    action_.name.clear();
  } else {
    // Otherwise default to some right click actions
    // If right clicked on enemy unit (and we have a selection)
    // go attack them
    if (entity && entity->getTeamID(t) != player_->getTeamID()
        && !player_->getSelection().empty()) {
      // Visual feedback
      highlightEntity(entity->getID());

      // Queue up action
      if (entity->hasProperty(GameEntity::P_CAPPABLE)) {
        order["type"] = OrderTypes::CAPTURE;
      } else {
        order["type"] = OrderTypes::ATTACK;
      }
      order["entity"] = toJson(player_->getSelection());
      order["target_id"] = entity->getGameID();
    // If we have a selection, and they didn't click on the current
    // selection, move them to target
    } else if (!player_->getSelection().empty()
        && (!entity || !player_->getSelection().count(entity->getGameID()))) {
      //loc.x/y wil be -/+HUGE_VAL if outside map bounds
      if (loc.x != HUGE_VAL && loc.y != HUGE_VAL && loc.x != -HUGE_VAL && loc.y != -HUGE_VAL) {
        // Visual feedback
        highlight(glm::vec2(loc.x, loc.y));

        // Queue up action
        order["type"] = OrderTypes::MOVE;
        order["entity"] = toJson(player_->getSelection());
        order["target"] = toJson(loc);
      }
    }
  }
  return order;
}

void GameController::mouseUp(const glm::vec2 &screenCoord, int button) {
  if (button == MouseButton::LEFT) {
    if (leftDrag_ &&
        glm::distance(leftStart_, screenCoord) > fltParam("hud.minDragDistance")) {
      auto selected_entities = selectEntities(
        leftStart_,
        screenCoord,
        player_->getPlayerID());

      std::set<std::string> new_selection;
      if (shift_) {
        new_selection = player_->getSelection();
      }
      for (auto entity : selected_entities) {
        new_selection.insert(entity->getGameID());
      }
      player_->setSelection(std::move(new_selection));
    }

    leftDrag_ = false;
    leftDragMinimap_ = false;
  }
  // nop
}

void GameController::mouseMotion(const glm::vec2 &screenCoord) {
  // Rotate camera while alt is held
  if (alt_) {
    glm::vec2 delta = screenCoord - lastMousePos_;
    delta *= fltParam("local.cameraRotSpeed");
    Renderer::get()->rotateCamera(delta);
  }
  lastMousePos_ = screenCoord;
}

void GameController::keyPress(const KeyEvent &ev) {
  float t = Renderer::get()->getGameTime();
  int key = ev.key;
  // TODO(zack) watch out for pausing here
  // Actions available in all player states:
  if (key == INPUT_KEY_F10) {
    PlayerAction action;
    action["type"] = ActionTypes::LEAVE_GAME;
    Game::get()->addAction(player_->getPlayerID(), action);
  // Camera panning
  } else if (key == INPUT_KEY_UP) {
    if (alt_) {
      zoom_ = -fltParam("local.keyZoomSpeed");
    } else {
      cameraPanDir_.y = 1.f;
    }
  // Control group binding and recalling
  } else if (isControlGroupHotkey(key)) {
    if (ctrl_) {
      player_->addSavedSelection(key, player_->getSelection());
    } else {
      auto saved_selection = player_->getSavedSelection(key);
      // center camera for double tapping
      if (!saved_selection.empty()) {
        if (saved_selection == player_->getSelection()) {
          auto game_id = *(saved_selection.begin());
          auto entity = Game::get()->getEntity(game_id);
          glm::vec3 pos = entity->getPosition(t);
          Renderer::get()->setCameraLookAt(pos);
        }
      }
      player_->setSelection(saved_selection);
    }
  } else if (key == INPUT_KEY_DOWN) {
    if (alt_) {
      zoom_ = fltParam("local.keyZoomSpeed");
    } else {
      cameraPanDir_.y = -1.f;
    }
  } else if (key == INPUT_KEY_RIGHT) {
    cameraPanDir_.x = 1.f;
  } else if (key == INPUT_KEY_LEFT) {
    cameraPanDir_.x = -1.f;
  } else if (state_ == PlayerState::DEFAULT) {
    if (key == INPUT_KEY_RETURN) {
      std::string prefix = (shift_) ? "/all " : "";
      ((CommandWidget *)getUI()->getWidget("ui.widgets.chat"))
        ->captureText(prefix);
    } else if (key == INPUT_KEY_ESC) {
      // ESC clears out current states
      if (!order_.empty() || !action_.name.empty()) {
        order_.clear();
        action_.name.clear();
      } else {
        player_->setSelection(std::set<std::string>());
      }
    } else if (key == INPUT_KEY_LEFT_SHIFT || key == INPUT_KEY_RIGHT_SHIFT) {
      shift_ = true;
    } else if (key == INPUT_KEY_LEFT_CTRL || key == INPUT_KEY_RIGHT_CTRL) {
      ctrl_ = true;
    } else if (key == INPUT_KEY_LEFT_ALT || key == INPUT_KEY_RIGHT_ALT) {
      alt_ = true;
    } else if (key == INPUT_KEY_N) {
      renderNavMesh_ = !renderNavMesh_;
    } else if (key == INPUT_KEY_BACKSPACE) {
      Renderer::get()->resetCameraRotation();
    } else if (!player_->getSelection().empty()) {
      // Handle unit commands
      // Order types
      if (key == INPUT_KEY_A) {
        order_ = OrderTypes::ATTACK;
      } else if (key == INPUT_KEY_M) {
        order_ = OrderTypes::MOVE;
      } else if (key == INPUT_KEY_X) {
        Json::Value order;
        order["type"] = OrderTypes::RETREAT;
        order["entity"] = toJson(player_->getSelection());
        PlayerAction action;
        action["type"] = ActionTypes::ORDER;
        action["order"] = order;
        Game::get()->addAction(player_->getPlayerID(), action);
      } else if (key == INPUT_KEY_H) {
        Json::Value order;
        order["type"] = OrderTypes::HOLD_POSITION;
        order["entity"] = toJson(player_->getSelection());
        PlayerAction action;
        action["type"] = ActionTypes::ORDER;
        action["order"] = order;
        Game::get()->addAction(player_->getPlayerID(), action);
      } else if (key == INPUT_KEY_S) {
        Json::Value order;
        order["type"] = OrderTypes::STOP;
        order["entity"] = toJson(player_->getSelection());
        PlayerAction action;
        action["type"] = ActionTypes::ORDER;
        action["order"] = order;
        Game::get()->addAction(player_->getPlayerID(), action);
      } else {
        auto sel = player_->getSelection().begin();
        auto actor = Game::get()->getEntity(*sel);
        auto actions = actor->getActions();
        for (auto &action : actions) {
          if (action.hotkey && action.hotkey == tolower(key)) {
            handleUIAction(action);
            break;
          }
        }
      }
    }
  }
}

void GameController::keyRelease(const KeyEvent &ev) {
  int key = ev.key;
  if (key == INPUT_KEY_RIGHT || key == INPUT_KEY_LEFT) {
    cameraPanDir_.x = 0.f;
  } else if (key == INPUT_KEY_UP || key == INPUT_KEY_DOWN) {
    cameraPanDir_.y = 0.f;
    zoom_ = 0.f;
  } else if (key == INPUT_KEY_LEFT_SHIFT || key == INPUT_KEY_RIGHT_SHIFT) {
    shift_ = false;
  } else if (key == INPUT_KEY_LEFT_CTRL || key == INPUT_KEY_RIGHT_CTRL) {
    ctrl_ = false;
  } else if (key == INPUT_KEY_LEFT_ALT || key == INPUT_KEY_RIGHT_ALT) {
    alt_ = false;
  }
}

void GameController::minimapUpdateCamera(const glm::vec2 &screenCoord) {
  auto minimapWidget = (MinimapWidget *)getUI()->getWidget("ui.widgets.minimap");
  const glm::vec2 minimapPos = minimapWidget->getCenter();
  const glm::vec2 minimapDim = minimapWidget->getSize();
  glm::vec2 mapCoord = screenCoord;
  // [-minimapDim/2, minimapDim/2]
  mapCoord -= minimapPos;
  // [-mapSize/2, mapSize/2]
  mapCoord *= Renderer::get()->getMapSize() / minimapDim;
  mapCoord.y *= -1;
  glm::vec3 finalPos(mapCoord, Renderer::get()->getCameraPos().z);
  Renderer::get()->setCameraLookAt(finalPos);
}

GameEntity * GameController::selectEntity(const glm::vec2 &screenCoord) const {
  float t = Renderer::get()->getGameTime();
  glm::vec3 origin, dir;
  std::tie(origin, dir) = Renderer::get()->screenToRay(screenCoord);
  return (GameEntity *)Renderer::get()->castRay(
      origin,
      dir,
      [=](const ModelEntity *e) {
        if (!e->hasProperty(GameEntity::P_ACTOR)) {
          return false;
        }
        return ((GameEntity *)e)->isVisibleTo(t, player_->getPlayerID());
      });
}

std::set<GameEntity *> GameController::selectEntities(
    const glm::vec2 &start,
    const glm::vec2 &end,
    id_t pid) const {
  assertPid(pid);
  // returnedEntities is a subset of those the user boxed, depending on some 
  // selection criteria
  std::set<GameEntity *> boxedEntities;

  const float t = Renderer::get()->getGameTime();
  glm::vec2 terrainStart = Renderer::get()->screenToTerrain(start).xy;
  glm::vec2 terrainEnd = Renderer::get()->screenToTerrain(end).xy;
  
  //Bound selection rect by map bounds
  glm::vec2 mapSize = Game::get()->getMap()->getSize()/2.f;
  terrainStart = glm::clamp(terrainStart, -mapSize, mapSize);
  terrainEnd = glm::clamp(terrainEnd, -mapSize, mapSize);
  
  // Defines the rectangle
  glm::vec2 center = (terrainStart + terrainEnd) / 2.f;
  glm::vec2 size = glm::abs(terrainEnd - terrainStart);
  Rect dragRect(center, size, 0.f);
  bool onlySelectUnits = false;

  for (const auto &pair : Renderer::get()->getEntities()) {
    auto e = pair.second;
    // Must be an actor owned by the passed player
    if (!e->hasProperty(GameEntity::P_ACTOR) && e->isVisible()) {
      continue;
    }
    auto ge = (GameEntity *) e;
    if (ge->getPlayerID(t) == pid
        && boxInBox(dragRect, ge->getRect(t))) {
      boxedEntities.insert(ge);
      if (ge->hasProperty(GameEntity::P_UNIT)) {
        onlySelectUnits = true;
      }
    }
  }
  std::set<GameEntity *> ret;
  for (const auto &e : boxedEntities) {
    if (!onlySelectUnits || e->hasProperty(GameEntity::P_UNIT)) {
      ret.insert(e);
    }
  }
  return ret;
}

void GameController::highlight(const glm::vec2 &mapCoord) {
  MapHighlight hl;
  hl.pos = mapCoord;
  hl.remaining = fltParam("ui.highlight.duration");
  highlights_.push_back(hl);
}

void GameController::highlightEntity(id_t eid) {
  entityHighlights_[eid] = fltParam("ui.highlight.duration");
}

void renderHealthBar(
    const glm::vec2 &center,
    const glm::vec2 &size,
    const std::vector<GameEntity::UIPart> &parts,
    const GameEntity *actor,
    const Player *player) {
  const float t = Renderer::get()->getGameTime();
  float current_health = 0.f;
  float total_health = 0.f;
  for (auto part : parts) {
    current_health += glm::max(0.f, part.health[0]);
    total_health += part.health[1];
  }

  glm::vec2 bottom_left = center - size / 2.f;

  if (actor->getTeamID(t) != player->getTeamID()) {
    auto bgcolor = vec4Param("hud.actor_health.bg_color");
    auto color = vec4Param("hud.actor_health.enemy_color");
    float factor = glm::clamp(current_health / total_health, 0.f, 1.f);

    drawRect(bottom_left, size, bgcolor);
    drawRect(bottom_left, glm::vec2(size.x * factor, size.y), color);
    return;
  }

  float s = 0.f;
  bool first = true;
  int i = 0;
  for (auto part : parts) {
    float health = glm::max(0.f, part.health[0]);
    float max_health = part.health[1];
    float bar_size = glm::clamp(max_health / total_health, 0.f, 1.f);
    glm::vec2 p = bottom_left + glm::vec2(size.x * s / total_health, 0.f);

    glm::vec2 total_size = glm::vec2(bar_size * size.x, size.y);
    glm::vec2 total_center = p + total_size / 2.f;

    glm::vec2 cur_size = glm::vec2(
        total_size.x * health / max_health,
        size.y);
    glm::vec2 cur_center = p + cur_size / 2.f;

    s += max_health;

    glm::vec4 bgcolor = health > 0
      ? vec4Param("hud.actor_health.bg_color")
      : vec4Param("hud.actor_health.disabled_bg_color");
    glm::vec4 healthBarColor;
    if (actor->getPlayerID(t) == player->getPlayerID()) {
      healthBarColor = vec4Param("hud.actor_health.local_color");
    } else {
      healthBarColor = vec4Param("hud.actor_health.team_color");
    }
    float timeSinceDamage = Clock::secondsSince(actor->getLastTookDamage(i++));
    // TODO flash the specific bar, not the entire thingy
    if (timeSinceDamage < fltParam("hud.actor_health.flash_duration")) {
      healthBarColor = vec4Param("hud.actor_health.flash_color");
    }
    drawRectCenter(total_center, total_size, bgcolor);
    // Green on top for current health
    drawRectCenter(cur_center, cur_size, healthBarColor);
    if (!first) {
      // Draw separator
      drawLine(
          glm::vec2(p.x, p.y),
          glm::vec2(p.x, p.y + size.y),
          vec4Param("hud.actor_health.separator_color"));
    }
    first = false;
  }
}

void renderEntity(
    const LocalPlayer *localPlayer,
    const std::map<id_t, float>& entityHighlights,
    const ModelEntity *e,
    float t) {
  if (!e->hasProperty(GameEntity::P_ACTOR) || !e->isVisible()) {
    return;
  }
  auto game_entity = (const GameEntity *)e;
  auto pos = e->getPosition(t);
  auto transform = glm::translate(glm::mat4(1.f), pos);
  record_section("renderActorInfo");
  // TODO(zack): only render for actors currently on screen/visible
  auto entitySize = e->getSize();
  auto circleTransform = glm::scale(
      glm::translate(
        transform,
        glm::vec3(0, 0, 0.1)),
      glm::vec3(0.25f + sqrt(entitySize.x * entitySize.y)));
  if (localPlayer->isSelected(game_entity->getGameID())) {
    // A bit of a hack here...
    renderCircleColor(
        circleTransform,
        glm::vec4(vec3Param("colors.selected"), 1.f),
        fltParam("ui.highlight.thickness"));
  } else if (entityHighlights.find(e->getID()) != entityHighlights.end()) {
    // A bit of a hack here...
    renderCircleColor(
        circleTransform,
        glm::vec4(vec3Param("colors.targeted"), 1.f),
        fltParam("ui.highlight.thickness"));
  }

  glDisable(GL_DEPTH_TEST);
  glm::vec3 placardPos(0.f, 0.f, e->getHeight() + 0.50f);
  auto ndc = getProjectionStack().current() * getViewStack().current()
    * transform * glm::vec4(placardPos, 1.f);
  ndc /= ndc.w;
  auto resolution = Renderer::get()->getResolution();
  auto coord = (glm::vec2(ndc.x, -ndc.y) / 2.f + 0.5f) * resolution;
  auto actor = (const GameEntity *)e;
  const auto ui_info = actor->getUIInfo();

  // Hotkey
  if (actor->getPlayerID(t) == localPlayer->getPlayerID()
      && ui_info.hotkey >= '0' && ui_info.hotkey <= '9') {
    std::string s;
    s.append(FontManager::get()->makeColorCode(glm::vec3(1,1,1)));
    s.push_back(ui_info.hotkey);
    FontManager::get()->drawString(
        s,
        coord + vec2Param("hud.actor_hotkey.pos"),
        fltParam("hud.actor_hotkey.font_size"));
  }

  // Cap status
  if (ui_info.capture[1]) {
    float capFact = glm::max(ui_info.capture[0] / ui_info.capture[1], 0.f);
    glm::vec2 size = vec2Param("hud.actor_cap.dim");
    glm::vec2 pos = coord - vec2Param("hud.actor_cap.pos");
    // Black underneath
    drawRectCenter(pos, size, glm::vec4(0, 0, 0, 1));
    pos.x -= size.x * (1.f - capFact) / 2.f;
    size.x *= capFact;
    const glm::vec4 cap_color = vec4Param("hud.actor_cap.color");
    drawRectCenter(pos, size, cap_color);
  }

  if (!ui_info.parts.empty()) {
    glm::vec2 center = coord - vec2Param("hud.actor_health.pos");
    glm::vec2 size = vec2Param("hud.actor_health.dim");
    renderHealthBar(center, size, ui_info.parts, actor, localPlayer);
  }

  if (ui_info.mana[1]) {
    // Display the mana bar
    glm::vec4 manaBarColor = vec4Param("hud.actor_mana.color");

    float manaFact = glm::max(
        0.f,
        ui_info.mana[0] / ui_info.mana[1]);
    glm::vec2 size = vec2Param("hud.actor_mana.dim");
    glm::vec2 pos = coord - vec2Param("hud.actor_mana.pos");
    // black underneath for max mana
    drawRectCenter(pos, size, glm::vec4(0, 0, 0, 1));
    // mana color on top
    pos.x -= size.x * (1.f - manaFact) / 2.f;
    size.x *= manaFact;
    drawRectCenter(pos, size, manaBarColor);
  }

  if (ui_info.retreat) {
    glm::vec2 size = vec2Param("hud.actor_retreat.dim");
    glm::vec2 pos = coord - vec2Param("hud.actor_retreat.pos");
    std::string texname = strParam("hud.actor_retreat.texture");
    auto tex = ResourceManager::get()->getTexture(texname);
    drawTextureCenter(pos, size, tex);
  }

  glEnable(GL_DEPTH_TEST);
  // Render path if selected
  if (localPlayer->isSelected(actor->getGameID())) {
    const auto &pathQueue = ui_info.path;
    // TODO(zack): remove z hack
    const auto zhack = glm::vec3(0, 0, 0.05f);
    auto prev = pos + zhack;
    for (auto target : pathQueue) {
      target += zhack;
      renderLineColor(prev, target, glm::vec4(1.f));
      prev = target;
    }
  }
}

void GameController::updateMapShader(Shader *shader) const {
  shader->uniform1i("texture", 0);

  glActiveTexture(GL_TEXTURE0);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, visTex_);
}

void GameController::setVisibilityData(void *data, size_t len, const glm::ivec2 &dim) {
  invariant(visData_ == nullptr, "cannot set visibility data twice");
  visData_ = data;
  visDataLength_ = len;
  visDim_ = dim;
}

void GameController::handleUIAction(const UIAction &action) {
  if (action.state != UIAction::ENABLED) {
    return;
  }
  if (action.targeting == UIAction::TargetingType::NONE) {
    PlayerAction player_action;
    player_action["type"] = ActionTypes::ORDER;
    Json::Value order;
    order["type"] = OrderTypes::ACTION;
    std::set<std::string> ids(&action.owner_id, (&action.owner_id)+1);
    order["entity"] = toJson(ids);
    order["action"] = action.name;
    player_action["order"] = order;
    Game::get()->addAction(player_->getPlayerID(), player_action);
    // No extra params
  } else {
    action_ = action;
  }
}
};  // rts
