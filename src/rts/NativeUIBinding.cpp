#include "rts/NativeUIBinding.h"
#include "common/util.h"

using namespace v8;

static Handle<Value> setVisibilityBuffer(const Arguments &args) {
  HandleScope handle_scope;
  invariant(
    args.Length() == 2,
    "void setVisibilityBuffer(vec2 dim, ArrayBuffer data)");

  return Undefined();
};

Handle<Value> getNativeUIBinding() {
  HandleScope handle_scope;
  auto binding = Object::New();
  binding->Set(
    String::New("setVisibilityBuffer"),
    FunctionTemplate::New(setVisibilityBuffer)->GetFunction());

  return handle_scope.Close(binding);
}