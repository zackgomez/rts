#include "rts/GameScript.h"
#include <v8.h>
#include "rts/Game.h"

using namespace v8;

namespace rts {

static Handle<Value> jsSendMessage(const Arguments &args) {
  if (args.Length() < 2) return Undefined();
  HandleScope scope(args.GetIsolate());

  Handle<Integer> to = Handle<Integer>::Cast(args[0]);
  Handle<Object> msg = Handle<Object>::Cast(args[1]);

  return Undefined();
}

static Handle<Value> entityGetHealth(
    Local<String> property,
    const AccessorInfo &info) {
  // TODO
  return Integer::New(0);
}

static void entitySetHealth(
    Local<String> property,
    Local<Value> value,
    const AccessorInfo &info) {
  // TODO
}

GameScript::~GameScript() {
  entityTemplate_.Dispose();
  context_.Dispose(Isolate::GetCurrent());
}

void GameScript::init() {
  isolate_ = Isolate::GetCurrent();
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

};  // rts
