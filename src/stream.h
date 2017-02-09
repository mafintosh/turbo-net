#ifndef TURBO_NET_STREAM_H
#define TURBO_NET_STREAM_H

#include <nan.h>

#define TURBO_NET_STREAM_BUFFER 64
#define TURBO_NET_STREAM_INC(i) (i == (TURBO_NET_STREAM_BUFFER - 1) ? i = 0 : i++);

using namespace v8;

class TurboNetStream : public Nan::ObjectWrap {
public:
  Nan::Persistent<Object> context;

  Nan::Callback *on_read;
  Nan::Callback *on_end;
  Nan::Callback *on_write;
  Nan::Callback *on_finish;
  Nan::Callback *on_error;
  Nan::Callback *on_close;

  uv_shutdown_t finish_handle;
  uv_stream_t handle;
  uv_stream_t *handle_ptr;

  uv_buf_t buffer_read[TURBO_NET_STREAM_BUFFER];
  int buffer_read_top;
  int buffer_read_btm;
  int buffer_read_free;

  uv_write_t buffer_write[TURBO_NET_STREAM_BUFFER];
  int buffer_write_top;
  int buffer_write_btm;
  int buffer_write_free;

  int paused;

  static void Init ();
  static Local<Value> NewInstance ();

  TurboNetStream ();
  ~TurboNetStream ();

private:
  static NAN_METHOD(New);
  static NAN_METHOD(Context);

  static NAN_METHOD(Address);
  static NAN_METHOD(RemoteAddress);
  static NAN_METHOD(Read);
  static NAN_METHOD(Write);
  static NAN_METHOD(End);
  static NAN_METHOD(WriteAll);
  static NAN_METHOD(Close);

  static NAN_METHOD(OnRead);
  static NAN_METHOD(OnEnd);
  static NAN_METHOD(OnWrite);
  static NAN_METHOD(OnFinish);
  static NAN_METHOD(OnError);
  static NAN_METHOD(OnClose);
};

#endif
