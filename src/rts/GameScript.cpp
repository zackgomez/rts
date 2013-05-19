#include "rts/GameScript.h"
#include <v8.h>
#include "rts/Game.h"

using namespace v8;

namespace rts {

static Handle<Value> jsSendMessage(const Arguments &args) {
  if (args.Length() < 1) return Undefined();
  HandleScope scope(args.GetIsolate());

  //Handle<Object> msg = Handle<Object>::Cast(args[0]);
  return Undefined();
}

GameScript::~GameScript() {
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
  // TODO add game object here
}

};  // rts
