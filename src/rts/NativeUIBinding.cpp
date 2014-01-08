#include "rts/NativeUIBinding.h"
#include <v8.h>
#include "common/util.h"
#include "common/Logger.h"
#include "rts/GameScript.h"
#include "rts/GameController.h"
#include "rts/Renderer.h"

using namespace v8;
using namespace rts;

static void setVisibilityBuffer(const FunctionCallbackInfo<Value> &args) {
  HandleScope handle_scope(args.GetIsolate());
  invariant(
    args.Length() == 2,
    "void setVisibilityBuffer(vec2 dim, ArrayBuffer data)");

  auto dim = jsToIVec2(Handle<Array>::Cast(args[0]));
  auto data = Handle<ArrayBuffer>::Cast(args[1]);
  invariant(
    !data->IsExternal(),
    "cannot set visibility to externalized buffer");

  auto contents = data->Externalize();
  auto game_controller = (GameController *)Renderer::get()->getController();
  game_controller->setVisibilityData(contents.Data(), contents.ByteLength(), dim);

  args.GetReturnValue().SetUndefined();
}

Handle<Object> getNativeUIBinding() {
  HandleScope handle_scope(Isolate::GetCurrent());
  auto binding = Object::New();
  binding->Set(
    String::New("setVisibilityBuffer"),
    FunctionTemplate::New(setVisibilityBuffer)->GetFunction());

  return handle_scope.Close(binding);
}