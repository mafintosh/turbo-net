#include <nan.h>
#include <uv.h>
#include "socket.h"
#include "stream.h"

using namespace v8;

NAN_METHOD(TurboNet) {
  info.GetReturnValue().Set(TurboNetSocket::NewInstance());
}

NAN_MODULE_INIT(InitAll) {
  TurboNetSocket::Init();
  TurboNetStream::Init();
  Nan::Set(target, Nan::New<String>("create").ToLocalChecked(), Nan::GetFunction(Nan::New<FunctionTemplate>(TurboNet)).ToLocalChecked());
}

NODE_MODULE(turbonet, InitAll)
