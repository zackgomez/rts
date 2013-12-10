#include "rts/NativeUIBinding.h"
#include <v8.h>
#include "common/util.h"
#include "common/Logger.h"
#include "rts/GameScript.h"

using namespace v8;
using namespace rts;

static Handle<Value> setVisibilityBuffer(const Arguments &args) {
  HandleScope handle_scope;
  invariant(
    args.Length() == 2,
    "void setVisibilityBuffer(vec2 dim, ArrayBuffer data)");

  auto dim = jsToVec2(Handle<Array>::Cast(args[0]));
  auto data = Handle<ArrayBuffer>::Cast(args[1]);
  invariant(
    !data->IsExternal(),
    "cannot set visibility to externalized buffer");

  // TODO(zack): set on game controller

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