#ifndef TURBO_NET_SOCKET_H
#define TURBO_NET_SOCKET_H

#include <nan.h>

using namespace v8;

class TurboNetSocket : public Nan::ObjectWrap {
public:
  Nan::Persistent<Object> context;

  Nan::Callback *on_connection;
  Nan::Callback *on_connect;

  uv_tcp_t handle;
  uv_connect_t connect_handle;

  static void Init ();
  static Local<Value> NewInstance ();

  TurboNetSocket ();
  ~TurboNetSocket ();

private:
  static NAN_METHOD(New);
  static NAN_METHOD(Context);

  static NAN_METHOD(Connect);
  static NAN_METHOD(Listen);
  static NAN_METHOD(Address);

  static NAN_METHOD(OnConnect);
  static NAN_METHOD(OnConnection);
};

#endif
