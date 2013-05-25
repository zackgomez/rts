#include "rts/GameScript.h"
#include <v8.h>
#include "common/util.h"
#include "rts/Actor.h"
#include "rts/Game.h"
#include "rts/Renderer.h"

using namespace v8;

namespace rts {

static Handle<Value> jsSendMessage(const Arguments &args) {
  if (args.Length() < 2) return Undefined();
  HandleScope scope(args.GetIsolate());

  //Handle<Integer> to = Handle<Integer>::Cast(args[0]);
  //Handle<Object> msg = Handle<Object>::Cast(args[1]);

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

static Handle<Value> jsGetNearbyEntities(const Arguments &args) {
  if (args.Length() < 3) return Undefined();
  HandleScope scope(args.GetIsolate());

  id_t eid = args[0]->IntegerValue();
  float radius = args[1]->NumberValue();
  Handle<Function> callback = Handle<Function>::Cast(args[2]);

  auto *e = Game::get()->getEntity(eid);
  if (!e) return Undefined();
  auto pos = e->getPosition();

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
      String::New("GetNearbyEntities"),
      FunctionTemplate::New(jsGetNearbyEntities));
  global->Set(
      String::New("Log"),
      FunctionTemplate::New(jsLog));

  context_.Reset(isolate_, Context::New(isolate_, nullptr, global));
  Context::Scope context_scope(isolate_, context_);

  entityTemplate_ =
      Persistent<ObjectTemplate>::New(isolate_, ObjectTemplate::New());
  entityTemplate_->SetInternalFieldCount(1);
  entityTemplate_->Set(String::New("getHealth"), FunctionTemplate::New(entityGetHealth));
  entityTemplate_->Set(String::New("getID"), FunctionTemplate::New(entityGetID));
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
