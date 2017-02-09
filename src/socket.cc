#include "socket.h"
#include "stream.h"
#include "macros.h"

static Nan::Persistent<FunctionTemplate> turbo_net_socket_constructor;

static void on_uv_connect (uv_connect_t* req, int status) {
  Nan::HandleScope scope;

  // TODO: error handle
  assert(status >= 0);

  TurboNetSocket *self = (TurboNetSocket *) req->handle->data;

  Local<Value> stream_wrap = TurboNetStream::NewInstance();
  TurboNetStream *stream_unwrap = Nan::ObjectWrap::Unwrap<TurboNetStream>(stream_wrap->ToObject());

  stream_unwrap->handle_ptr = req->handle;
  stream_unwrap->handle_ptr->data = stream_unwrap;

  Local<Value> argv[] = {
    stream_wrap
  };

  TURBO_NET_CALL(self, on_connect, 1, argv)
}

static void on_uv_connection (uv_stream_t* server, int status) {
  Nan::HandleScope scope;
  TurboNetSocket *self = (TurboNetSocket *) server->data;

  // TODO: error handle
  assert(status >= 0);

  if (!self->on_connection) return;

  Local<Value> stream_wrap = TurboNetStream::NewInstance();
  TurboNetStream *stream_unwrap = Nan::ObjectWrap::Unwrap<TurboNetStream>(stream_wrap->ToObject());

  uv_stream_t *client = &(stream_unwrap->handle);

  // TODO: instead of asserting call an on_error function and clean up the stream_wrap using delete?
  assert(uv_tcp_init(uv_default_loop(), (uv_tcp_t *) client) == 0);

  client->data = stream_unwrap;

  uv_accept(server, client);

  Local<Value> argv[] = {
    stream_wrap
  };

  TURBO_NET_CALL(self, on_connection, 1, argv)
}

TurboNetSocket::TurboNetSocket () {
  if (uv_tcp_init(uv_default_loop(), &handle)) {
    Nan::ThrowError("Could not create TCP handle");
    return;
  }

  handle.data = this;
  on_connection = NULL;
  on_connect = NULL;
}

TurboNetSocket::~TurboNetSocket () {
  if (on_connection != NULL) delete on_connection;
  if (on_connect != NULL) delete on_connect;

  context.Reset();
}

NAN_METHOD(TurboNetSocket::New) {
  TurboNetSocket* obj = new TurboNetSocket();
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(TurboNetSocket::Context) {
  TurboNetSocket *self = Nan::ObjectWrap::Unwrap<TurboNetSocket>(info.This());
  Local<Object> context = info[0]->ToObject();
  self->context.Reset(context);
}

NAN_METHOD(TurboNetSocket::Connect) {
  TurboNetSocket *self = Nan::ObjectWrap::Unwrap<TurboNetSocket>(info.This());
  struct sockaddr_in addr;
  Nan::Utf8String ip(info[1]);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(info[0]->Uint32Value());
  inet_aton(*ip, (struct in_addr *) &addr.sin_addr.s_addr);

  uv_tcp_connect(&(self->connect_handle), &(self->handle), (const struct sockaddr *) &addr, on_uv_connect);
}

NAN_METHOD(TurboNetSocket::Listen) {
  TurboNetSocket *self = Nan::ObjectWrap::Unwrap<TurboNetSocket>(info.This());
  struct sockaddr_in addr;
  Nan::Utf8String ip(info[1]);

  addr.sin_family = AF_INET;
  addr.sin_port = htons(info[0]->Uint32Value());
  inet_aton(*ip, (struct in_addr *) &addr.sin_addr.s_addr);

  uv_tcp_bind(&(self->handle), (const struct sockaddr *) &addr, 0);
  uv_listen((uv_stream_t *) &(self->handle), 5, on_uv_connection); // TODO: research backlog
}

NAN_METHOD(TurboNetSocket::Address) {
  Nan::EscapableHandleScope scope;

  TurboNetSocket *self = Nan::ObjectWrap::Unwrap<TurboNetSocket>(info.This());
  uv_tcp_t *handle = &(self->handle);

  TURBO_NET_RETURN_ADDRESS(handle, uv_tcp_getsockname)
}

TURBO_NET_LISTENER(TurboNetSocket, TurboNetSocket::OnConnection, self->on_connection);
TURBO_NET_LISTENER(TurboNetSocket, TurboNetSocket::OnConnect, self->on_connect);

void TurboNetSocket::Init () {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(TurboNetSocket::New);
  turbo_net_socket_constructor.Reset(tpl);
  tpl->SetClassName(Nan::New("TurboNetSocket").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "context", TurboNetSocket::Context);
  Nan::SetPrototypeMethod(tpl, "onconnection", TurboNetSocket::OnConnection);
  Nan::SetPrototypeMethod(tpl, "onconnect", TurboNetSocket::OnConnect);
  Nan::SetPrototypeMethod(tpl, "connect", TurboNetSocket::Connect);
  Nan::SetPrototypeMethod(tpl, "listen", TurboNetSocket::Listen);
  Nan::SetPrototypeMethod(tpl, "address", TurboNetSocket::Address);
}

Local<Value> TurboNetSocket::NewInstance () {
  Nan::EscapableHandleScope scope;

  Local<Object> instance;

  Local<FunctionTemplate> constructorHandle = Nan::New<FunctionTemplate>(turbo_net_socket_constructor);
  instance = Nan::NewInstance(constructorHandle->GetFunction()).ToLocalChecked();

  return scope.Escape(instance);
}
