#include "rts/GameScript.h"
#include <v8.h>
#include "common/util.h"
#include "rts/Actor.h"
#include "rts/Game.h"
#include "rts/MessageHub.h"
#include "rts/Renderer.h"

using namespace v8;

namespace rts {

static Handle<Value> jsSendMessage(const Arguments &args) {
  if (args.Length() < 1) return Undefined();
  HandleScope scope(args.GetIsolate());

  Handle<Object> msg = Handle<Object>::Cast(args[0]);
  Message json_msg;

  Handle<Array> names = msg->GetPropertyNames();
  for (int i = 0; i < names->Length(); i++) {
    Handle<Value> val = msg->Get(names->Get(i));
    const auto name = std::string(*String::AsciiValue(names->Get(i)));

    if (val->IsUint32()) {
      json_msg[name] = val->IntegerValue();
    } else if (val->IsString()) {
      json_msg[name] = *String::AsciiValue(val);
    } else if (val->IsBoolean()) {
      json_msg[name] = val->BooleanValue();
    } else if (val->IsNumber()) {
      json_msg[name] = val->NumberValue();
    } else {
      invariant_violation("Unsupported json value type for key " + name);
    }
  }

  MessageHub::get()->sendMessage(json_msg);

  return Undefined();
}

static Handle<Value> jsLog(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  for (int i = 0; i < args.Length(); i++) {
    if (i != 0) std::cout << ' ';
    std::cout << *String::AsciiValue(args[i]);
  }
  std::cout << '\n';

  return Undefined();
}

static Handle<Value> jsAddRequisition(const Arguments &args) {
  if (args.Length() < 3) return Undefined();
  HandleScope scope(args.GetIsolate());

  id_t pid = args[0]->IntegerValue();
  float amount = args[1]->NumberValue();
  id_t from_eid = args[2]->IntegerValue();
  Game::get()->addResources(pid, ResourceType::REQUISITION, amount, from_eid);

  return Undefined();
}

static Handle<Value> jsAddVPs(const Arguments &args) {
  if (args.Length() < 3) return Undefined();
  HandleScope scope(args.GetIsolate());

  id_t pid = args[0]->IntegerValue();
  float amount = args[1]->NumberValue();
  id_t from_eid = args[2]->IntegerValue();
  Game::get()->addVPs(pid, amount, from_eid);

  return Undefined();
}

static Handle<Value> entityGetNearbyEntities(const Arguments &args) {
  if (args.Length() < 2) return Undefined();
  HandleScope scope(args.GetIsolate());

  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *entity = static_cast<GameEntity *>(wrap->Value());

  float radius = args[0]->NumberValue();
  Handle<Function> callback = Handle<Function>::Cast(args[1]);

  auto pos = entity->getPosition();

  const int argc = 1;
  Renderer::get()->getNearbyEntities(pos, radius,
      [&](const GameEntity *e) -> bool {
        auto jsEntity = Game::get()->getScript()->getEntity(e->getID());
        Handle<Value> argv[1] = {jsEntity};
        auto ret = callback->Call(args.Holder(), argc, argv);
        return ret->BooleanValue();
      });

  return Undefined();
}

static Handle<Value> entityGetHealth(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  Actor *actor = static_cast<Actor *>(wrap->Value());
  return scope.Close(Integer::New(actor->getHealth()));
}

static Handle<Value> entityGetID(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  Handle<Integer> ret = Integer::New(e->getID());
  return scope.Close(ret);
}

static Handle<Value> entityGetPlayerID(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  Handle<Integer> ret = Integer::New(e->getPlayerID());
  return scope.Close(ret);
}

static Handle<Value> entityGetTeamID(const Arguments &args) {
  HandleScope scope(args.GetIsolate());
  Local<Object> self = args.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  GameEntity *e = static_cast<GameEntity *>(wrap->Value());
  Handle<Integer> ret = Integer::New(e->getTeamID());
  return scope.Close(ret);
}

GameScript::GameScript()
  : isolate_(nullptr) {
}

GameScript::~GameScript() {
  {
    Locker locker(isolate_);
    HandleScope handle_scope(isolate_);
    Context::Scope context_scope(isolate_, context_);

    entityTemplate_.Dispose();
    for (auto pair : scriptObjects_) {
      pair.second.Dispose();
    }
    context_.Dispose(isolate_);
  }
  isolate_->Exit();
  isolate_->Dispose();
}

void GameScript::init() {
  isolate_ = Isolate::New();
  Locker locker(isolate_);
  isolate_->Enter();

  HandleScope handle_scope(isolate_);

  Handle<ObjectTemplate> global = ObjectTemplate::New();

  global->Set(
      String::New("SendMessage"),
      FunctionTemplate::New(jsSendMessage));
  global->Set(
      String::New("Log"),
      FunctionTemplate::New(jsLog));
  global->Set(
      String::New("AddRequisition"),
      FunctionTemplate::New(jsAddRequisition));
  global->Set(
      String::New("AddVPs"),
      FunctionTemplate::New(jsAddVPs));

  context_.Reset(isolate_, Context::New(isolate_, nullptr, global));
  Context::Scope context_scope(isolate_, context_);

  entityTemplate_ =
      Persistent<ObjectTemplate>::New(isolate_, ObjectTemplate::New());
  entityTemplate_->SetInternalFieldCount(1);
  entityTemplate_->Set(
      String::New("getHealth"),
      FunctionTemplate::New(entityGetHealth));
  entityTemplate_->Set(
      String::New("getID"),
      FunctionTemplate::New(entityGetID));
  entityTemplate_->Set(
      String::New("getPlayerID"),
      FunctionTemplate::New(entityGetPlayerID));
  entityTemplate_->Set(
      String::New("getTeamID"),
      FunctionTemplate::New(entityGetTeamID));
  entityTemplate_->Set(
      String::New("getNearbyEntities"),
      FunctionTemplate::New(entityGetNearbyEntities));
  // TODO(zack) the rest of the methods here)
  
  loadScripts();
}

void GameScript::loadScripts() {
  // TODO(zack): HACK ALERT
  std::ifstream file("scripts/effects.js");
  if (!file) {
    throw file_exception("Unable to open script");
  }
  std::string text;
  std::getline(file, text, (char)EOF);
  file.close();

  TryCatch try_catch;
  Handle<Script> script = Script::Compile(String::New(text.c_str()));
  if (script.IsEmpty()) {
    String::AsciiValue e_str(try_catch.Exception());
    LOG(ERROR) << "Unable to compile script: " << *e_str << '\n';
  }

  Handle<Value> result = script->Run();
  if (result.IsEmpty()) {
    String::AsciiValue e_str(try_catch.Exception());
    LOG(ERROR) << "Unable to run script: " << *e_str << '\n';
  }
}

Handle<Object> GameScript::getEntity(id_t eid) {
  auto it = scriptObjects_.find(eid);
  invariant(it != scriptObjects_.end(), "no wrapper found for entity");

  return it->second;
}

Handle<Object> GameScript::getGlobal() {
  return context_->Global();
}

void GameScript::wrapEntity(GameEntity *e) {
  HandleScope handle_scope(isolate_);
  Context::Scope context_scope(isolate_, context_);

  Persistent<Object> wrapper = Persistent<Object>::New(
      isolate_, entityTemplate_->NewInstance());
  wrapper->SetInternalField(0, External::New(e));

  scriptObjects_[e->getID()] = wrapper;
}

void GameScript::destroyEntity(GameEntity *e) {
  HandleScope handle_scope(isolate_);
  Context::Scope context_scope(isolate_, context_);

  auto it = scriptObjects_.find(e->getID());
  invariant(it != scriptObjects_.end(), "destroying entity without wrapper");
  it->second.Dispose();
  scriptObjects_.erase(it);
}

};  // rts
