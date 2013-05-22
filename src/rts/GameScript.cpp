#include "rts/GameScript.h"
#include <v8.h>
#include "rts/Actor.h"
#include "rts/Game.h"

using namespace v8;

namespace rts {

static Handle<Value> jsSendMessage(const Arguments &args) {
  if (args.Length() < 2) return Undefined();
  HandleScope scope(args.GetIsolate());

  //Handle<Integer> to = Handle<Integer>::Cast(args[0]);
  //Handle<Object> msg = Handle<Object>::Cast(args[1]);

  return Undefined();
}

static Handle<Value> entityGetHealth(
    Local<String> property,
    const AccessorInfo &info) {
  Local<Object> self = info.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  Actor *actor = static_cast<Actor *>(wrap->Value());
  return Integer::New(actor->getHealth());
}

static void entitySetHealth(
    Local<String> property,
    Local<Value> value,
    const AccessorInfo &info) {
  if (!value->IsNumber())  return;

  Local<Object> self = info.Holder();
  Local<External> wrap = Local<External>::Cast(self->GetInternalField(0));
  Actor *actor = static_cast<Actor *>(wrap->Value());
  actor->setHealth(value->NumberValue());
}

GameScript::~GameScript() {
  {
    Locker lock(isolate_);
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
  Locker lock(isolate_);
  isolate_->Enter();

  HandleScope handle_scope(isolate_);
  
  Handle<ObjectTemplate> global = ObjectTemplate::New();

  global->Set(
      String::New("SendMessage"),
      FunctionTemplate::New(jsSendMessage));

  context_.Reset(isolate_, Context::New(isolate_, nullptr, global));

  Context::Scope context_scope(isolate_, context_);

  entityTemplate_ =
      Persistent<ObjectTemplate>::New(isolate_, ObjectTemplate::New());
  entityTemplate_->SetInternalFieldCount(1);
  entityTemplate_->SetAccessor(
      String::New("health"), entityGetHealth, entitySetHealth);
  // TODO(zack) the rest of the methods here)
}

void GameScript::wrapEntity(GameEntity *e) {
  Locker lock(isolate_);
  HandleScope handle_scope(isolate_);
  Context::Scope context_scope(isolate_, context_);

  Persistent<Object> wrapper = Persistent<Object>::New(
      isolate_, entityTemplate_->NewInstance());
  wrapper->SetInternalField(0, External::New(e));

  scriptObjects_[e] = wrapper;
}

void GameScript::destroyEntity(GameEntity *e) {
  Locker lock(isolate_);
  HandleScope handle_scope(isolate_);
  Context::Scope context_scope(isolate_, context_);

  auto it = scriptObjects_.find(e);
  invariant(it != scriptObjects_.end(), "destroying entity without wrapper");
  it->second.Dispose();
  scriptObjects_.erase(it);
}

};  // rts
