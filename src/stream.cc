#include <assert.h>
#include "stream.h"
#include "macros.h"

#define TURBO_NET_STREAM_WRITE(self, buf, len) \
  assert(self->buffer_write_free > 0); \
  self->buffer_write_free--; \
  uv_write_t *wreq = &(self->buffer_write[self->buffer_write_top]); \
  TURBO_NET_STREAM_INC(self->buffer_write_top) \
  assert(uv_write(wreq, self->handle_ptr, buf, len, on_uv_write) == 0);

static Nan::Persistent<FunctionTemplate> turbo_net_stream_constructor;

static void on_uv_close (uv_handle_t* handle) {
  Nan::HandleScope scope;
  TurboNetStream *self = (TurboNetStream *) handle->data;
  TURBO_NET_CALL(self, on_close, 0, NULL)
}

static void on_uv_shutdown (uv_shutdown_t* req, int status) {
  Nan::HandleScope scope;
  TurboNetStream *self = (TurboNetStream *) req->handle->data;

  if (status < 0) {
    TURBO_NET_ON_ERROR(self, on_error, status);
    return;
  }

  TURBO_NET_CALL(self, on_finish, 0, NULL)
}

static void on_uv_write (uv_write_t* req, int status) {
  Nan::HandleScope scope;
  TurboNetStream *self = (TurboNetStream *) req->handle->data;

  if (status < 0) {
    TURBO_NET_ON_ERROR(self, on_error, status)
    return;
  }

  // assert that at least one buffer was allocated
  assert(self->buffer_write_free < TURBO_NET_STREAM_BUFFER);
  self->buffer_write_free++;

  TURBO_NET_STREAM_INC(self->buffer_write_btm)
  TURBO_NET_CALL(self, on_write, 0, NULL)
}

static void on_uv_read (uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
  Nan::HandleScope scope;
  TurboNetStream *self = (TurboNetStream *) stream->data;

  if (nread == UV_EOF) {
    TURBO_NET_CALL(self, on_end, 0, NULL)
    return;
  }

  if (nread < 0) {
    TURBO_NET_ON_ERROR(self, on_error, nread)
    return;
  }

  if (nread == 0) {
    return;
  }

  // assert that at least one buffer was allocated
  assert(self->buffer_read_free < TURBO_NET_STREAM_BUFFER);
  self->buffer_read_free++;

  TURBO_NET_STREAM_INC(self->buffer_read_btm)

  Local<Value> argv[] = {
    Nan::New((uint32_t) nread)
  };

  TURBO_NET_CALL(self, on_read, 1, argv)

  if (self->buffer_read_free == TURBO_NET_STREAM_BUFFER) {
    self->paused = 1;
    assert(uv_read_stop(self->handle_ptr) == 0);
  }
}

static void on_uv_alloc (uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
  TurboNetStream *self = (TurboNetStream *) handle->data;

  // assert that at least one buffer was allocated
  assert(self->buffer_read_free < TURBO_NET_STREAM_BUFFER);

  uv_buf_t *next = &(self->buffer_read[self->buffer_read_btm]);
  buf->base = next->base;
  buf->len = next->len;
}

TurboNetStream::TurboNetStream () {
  on_read = NULL;
  on_end = NULL;
  on_write = NULL;
  on_finish = NULL;
  on_error = NULL;
  on_close = NULL;

  paused = 1;
  handle_ptr = &handle;

  buffer_read_top = 0;
  buffer_read_btm = 0;
  buffer_read_free = TURBO_NET_STREAM_BUFFER;

  buffer_write_top = 0;
  buffer_write_btm = 0;
  buffer_write_free = TURBO_NET_STREAM_BUFFER;
}

TurboNetStream::~TurboNetStream () {
  if (on_read != NULL) delete on_read;
  if (on_end != NULL) delete on_end;
  if (on_write != NULL) delete on_write;
  if (on_finish != NULL) delete on_finish;
  if (on_error != NULL) delete on_error;
  if (on_close != NULL) delete on_close;

  context.Reset();
}

NAN_METHOD(TurboNetStream::New) {
  TurboNetStream* obj = new TurboNetStream();
  obj->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(TurboNetStream::Context) {
  TurboNetStream *self = Nan::ObjectWrap::Unwrap<TurboNetStream>(info.This());
  Local<Object> context = info[0]->ToObject();
  self->context.Reset(context);
}

NAN_METHOD(TurboNetStream::Write) {
  TurboNetStream *self = Nan::ObjectWrap::Unwrap<TurboNetStream>(info.This());
  Local<Object> buf = info[0]->ToObject();

  uv_buf_t wbuf = {
    .base = node::Buffer::Data(buf),
    .len = node::Buffer::Length(buf)
  };

  TURBO_NET_STREAM_WRITE(self, &wbuf, 1)
}

NAN_METHOD(TurboNetStream::End) {
  TurboNetStream *self = Nan::ObjectWrap::Unwrap<TurboNetStream>(info.This());
  assert(uv_shutdown(&(self->finish_handle), self->handle_ptr, on_uv_shutdown) == 0);
}

NAN_METHOD(TurboNetStream::Address) {
  Nan::EscapableHandleScope scope;
  TurboNetStream *self = Nan::ObjectWrap::Unwrap<TurboNetStream>(info.This());

  uv_tcp_t *handle = (uv_tcp_t *) self->handle_ptr;
  TURBO_NET_RETURN_ADDRESS(handle, uv_tcp_getsockname)
}

NAN_METHOD(TurboNetStream::RemoteAddress) {
  Nan::EscapableHandleScope scope;
  TurboNetStream *self = Nan::ObjectWrap::Unwrap<TurboNetStream>(info.This());

  uv_tcp_t *handle = (uv_tcp_t *) self->handle_ptr;
  TURBO_NET_RETURN_ADDRESS(handle, uv_tcp_getpeername)
}

NAN_METHOD(TurboNetStream::WriteAll) {
  TurboNetStream *self = Nan::ObjectWrap::Unwrap<TurboNetStream>(info.This());
  Local<Array> buffers = info[0].As<Array>();

  uint32_t length = buffers->Length();
  uv_buf_t wbufs[length];

  for (uint32_t i = 0; i < length; i++) {
    Local<Value> item = buffers->Get(i);

    if (!item->IsObject()) {
      Nan::ThrowError("Can only write buffers");
      return;
    }

    Local<Object> buf = item->ToObject();

    wbufs[i] = {
      .base = node::Buffer::Data(buf),
      .len = node::Buffer::Length(buf)
    };
  }

  TURBO_NET_STREAM_WRITE(self, wbufs, length)
}

NAN_METHOD(TurboNetStream::Read) {
  TurboNetStream *self = Nan::ObjectWrap::Unwrap<TurboNetStream>(info.This());
  Local<Object> buf = info[0]->ToObject();

  // assert that we have room for this buffer
  assert(self->buffer_read_free > 0);
  self->buffer_read_free--;

  uv_buf_t *rbuf = &(self->buffer_read[self->buffer_read_top]);
  TURBO_NET_STREAM_INC(self->buffer_read_top)

  rbuf->base = node::Buffer::Data(buf);
  rbuf->len = node::Buffer::Length(buf);

  if (self->paused) {
    self->paused = 0;
    assert(uv_read_start(self->handle_ptr, on_uv_alloc, on_uv_read) == 0);
  }
}

NAN_METHOD(TurboNetStream::Close) {
  TurboNetStream *self = Nan::ObjectWrap::Unwrap<TurboNetStream>(info.This());
  uv_close((uv_handle_t *) self->handle_ptr, on_uv_close);
}

TURBO_NET_LISTENER(TurboNetStream, TurboNetStream::OnRead, self->on_read);
TURBO_NET_LISTENER(TurboNetStream, TurboNetStream::OnEnd, self->on_end);
TURBO_NET_LISTENER(TurboNetStream, TurboNetStream::OnWrite, self->on_write);
TURBO_NET_LISTENER(TurboNetStream, TurboNetStream::OnFinish, self->on_finish);
TURBO_NET_LISTENER(TurboNetStream, TurboNetStream::OnError, self->on_error);
TURBO_NET_LISTENER(TurboNetStream, TurboNetStream::OnClose, self->on_close);

void TurboNetStream::Init () {
  Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(TurboNetStream::New);
  turbo_net_stream_constructor.Reset(tpl);
  tpl->SetClassName(Nan::New("TurboNetStream").ToLocalChecked());
  tpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tpl, "context", TurboNetStream::Context);
  Nan::SetPrototypeMethod(tpl, "address", TurboNetStream::Address);
  Nan::SetPrototypeMethod(tpl, "remoteAddress", TurboNetStream::RemoteAddress);
  Nan::SetPrototypeMethod(tpl, "onread", TurboNetStream::OnRead);
  Nan::SetPrototypeMethod(tpl, "onend", TurboNetStream::OnEnd);
  Nan::SetPrototypeMethod(tpl, "onwrite", TurboNetStream::OnWrite);
  Nan::SetPrototypeMethod(tpl, "onfinish", TurboNetStream::OnFinish);
  Nan::SetPrototypeMethod(tpl, "onerror", TurboNetStream::OnError);
  Nan::SetPrototypeMethod(tpl, "onclose", TurboNetStream::OnClose);
  Nan::SetPrototypeMethod(tpl, "close", TurboNetStream::Close);
  Nan::SetPrototypeMethod(tpl, "read", TurboNetStream::Read);
  Nan::SetPrototypeMethod(tpl, "write", TurboNetStream::Write);
  Nan::SetPrototypeMethod(tpl, "writeAll", TurboNetStream::WriteAll);
  Nan::SetPrototypeMethod(tpl, "end", TurboNetStream::End);
}

Local<Value> TurboNetStream::NewInstance () {
  Nan::EscapableHandleScope scope;

  Local<Object> instance;

  Local<FunctionTemplate> constructorHandle = Nan::New<FunctionTemplate>(turbo_net_stream_constructor);
  instance = Nan::NewInstance(constructorHandle->GetFunction()).ToLocalChecked();

  return scope.Escape(instance);
}
