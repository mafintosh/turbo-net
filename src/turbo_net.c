#include <uv.h>
#include <node_api.h>
#include <napi-macros.h>
#include <stdio.h>

#ifdef _WIN32
#include <stdlib.h>
#endif

#define TURBO_NET_STREAM (uv_stream_t *) &(self->handle)

#define TURBO_NET_CALLBACK(fn, src) \
  napi_env env = self->env; \
  napi_handle_scope scope; \
  napi_open_handle_scope(env, &scope); \
  napi_value ctx; \
  napi_get_reference_value(env, self->ctx, &ctx); \
  napi_value callback; \
  napi_get_reference_value(env, fn, &callback); \
  src \
  napi_close_handle_scope(env, scope);

#define NAPI_MAKE_CALLBACK_FATAL(n, argv, result) \
  if (napi_make_callback(env, NULL, ctx, callback, n, argv, result) == napi_pending_exception) { \
    napi_get_reference_value(env, on_fatal_exception, &callback); \
    napi_value fatal_exception; \
    napi_get_and_clear_last_exception(env, &fatal_exception); \
    napi_make_callback(env, NULL, ctx, callback, 1, &fatal_exception, NULL); \
    return; \
  }

typedef struct {
  uv_tcp_t handle;
  uv_shutdown_t shutdown;
  uv_connect_t connect;
  uv_buf_t reading;
  napi_env env;
  napi_ref ctx;
  napi_ref on_alloc_connection;
  napi_ref on_connect;
  napi_ref on_write;
  napi_ref on_read;
  napi_ref on_finish;
  napi_ref on_close;
} turbo_net_tcp_t;

static napi_ref on_fatal_exception;

static void on_uv_connection (uv_stream_t* server, int status) {
  const turbo_net_tcp_t *self = server->data;

  if (status < 0) return; // ignore bad connections. TODO: bubble up?

  TURBO_NET_CALLBACK(self->on_alloc_connection,
    napi_value result;
    napi_make_callback(env, NULL, ctx, callback, 0, NULL, &result);

    NAPI_BUFFER_CAST(turbo_net_tcp_t *, client, result)
    uv_accept(server, (uv_stream_t *) &(client->handle));

    napi_get_reference_value(env, client->on_connect, &callback);
    napi_get_reference_value(env, client->ctx, &ctx);

    napi_value argv[1];
    napi_create_int32(env, 0, &(argv[0]));
    NAPI_MAKE_CALLBACK_FATAL(1, argv, NULL)
  )
}

static void on_uv_alloc (uv_handle_t *handle, size_t size, uv_buf_t *buf) {
  const turbo_net_tcp_t *self = handle->data;
  *buf = self->reading;
}

static void on_uv_read (uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf) {
  if (nread == 0) return;
  if (nread == UV_EOF) nread = 0;

  turbo_net_tcp_t *self = stream->data;

  TURBO_NET_CALLBACK(self->on_read,
    napi_value result;
    napi_value argv[1];
    napi_create_int32(env, nread, &(argv[0]));
    NAPI_MAKE_CALLBACK_FATAL(1, argv, &result)
    NAPI_BUFFER(next, result)
  )

  if (next_len) {
    self->reading = (uv_buf_t) { .base = next, .len = next_len };
  } else {
    uv_read_stop(TURBO_NET_STREAM);
  }
}

static void on_uv_write (uv_write_t *req, int status) {
  const turbo_net_tcp_t *self = req->handle->data;

  TURBO_NET_CALLBACK(self->on_write,
    napi_value argv[1];
    napi_create_int32(env, status, &(argv[0]));
    NAPI_MAKE_CALLBACK_FATAL(1, argv, NULL)
  )
}

static void on_uv_shutdown (uv_shutdown_t* req, int status) {
  const turbo_net_tcp_t *self = req->handle->data;

  TURBO_NET_CALLBACK(self->on_finish,
    napi_value argv[1];
    napi_create_int32(env, status, &(argv[0]));
    NAPI_MAKE_CALLBACK_FATAL(1, argv, NULL)
  )
}

static void on_uv_close (uv_handle_t *handle) {
  const turbo_net_tcp_t *self = handle->data;

  TURBO_NET_CALLBACK(self->on_close,
    NAPI_MAKE_CALLBACK_FATAL(0, NULL, NULL)
  )
}

static void on_uv_connect (uv_connect_t* req, int status) {
  const turbo_net_tcp_t *self = req->handle->data;

  TURBO_NET_CALLBACK(self->on_connect,
    napi_value argv[1];
    napi_create_int32(env, status, &(argv[0]));
    NAPI_MAKE_CALLBACK_FATAL(1, argv, NULL)
  )
}

NAPI_METHOD(turbo_net_tcp_init) {
  NAPI_ARGV(8)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)

  int err;
  uv_tcp_t *handle = &(self->handle);

  handle->data = self;
  self->env = env;

  NAPI_UV_THROWS(err, uv_tcp_init(uv_default_loop(), handle));

  napi_create_reference(env, argv[1], 1, &(self->ctx));
  napi_create_reference(env, argv[2], 1, &(self->on_alloc_connection));
  napi_create_reference(env, argv[3], 1, &(self->on_connect));
  napi_create_reference(env, argv[4], 1, &(self->on_write));
  napi_create_reference(env, argv[5], 1, &(self->on_read));
  napi_create_reference(env, argv[6], 1, &(self->on_finish));
  napi_create_reference(env, argv[7], 1, &(self->on_close));

  return NULL;
}

NAPI_METHOD(turbo_net_tcp_destroy) {
  NAPI_ARGV(1)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)

  napi_delete_reference(env, self->ctx);
  napi_delete_reference(env, self->on_alloc_connection);
  napi_delete_reference(env, self->on_connect);
  napi_delete_reference(env, self->on_write);
  napi_delete_reference(env, self->on_read);
  napi_delete_reference(env, self->on_finish);
  napi_delete_reference(env, self->on_close);

  return NULL;
}

NAPI_METHOD(turbo_net_tcp_listen) {
  NAPI_ARGV(3)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)
  NAPI_ARGV_UINT32(port, 1)
  NAPI_ARGV_UTF8(ip, 17, 2)

  int err;
  struct sockaddr_in addr;

  NAPI_UV_THROWS(err, uv_ip4_addr(ip, port, &addr));

  NAPI_UV_THROWS(err, uv_tcp_bind(
    &(self->handle),
    (const struct sockaddr *) &addr,
    0
  ))

  // TODO: research backlog
  NAPI_UV_THROWS(err, uv_listen(TURBO_NET_STREAM, 511, on_uv_connection));

  return NULL;
}

NAPI_METHOD(turbo_net_tcp_read) {
  NAPI_ARGV(2)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)
  NAPI_ARGV_BUFFER(buffer, 1)

  int err;
  self->reading = (uv_buf_t) { .base = buffer, .len = buffer_len };

  NAPI_UV_THROWS(err, uv_read_start(TURBO_NET_STREAM, on_uv_alloc, on_uv_read))

  return NULL;
}

NAPI_METHOD(turbo_net_tcp_write) {
  NAPI_ARGV(4)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)
  NAPI_ARGV_BUFFER_CAST(uv_write_t *, req, 1)
  NAPI_ARGV_BUFFER(buffer, 2)
  NAPI_ARGV_UINT32(len, 3)

  int err;
  const uv_buf_t buf = { .base = buffer, .len = len };

  NAPI_UV_THROWS(err, uv_write(req, TURBO_NET_STREAM, &buf, 1, on_uv_write))

  return NULL;
}

NAPI_METHOD(turbo_net_tcp_write_two) {
  NAPI_ARGV(6)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)
  NAPI_ARGV_BUFFER_CAST(uv_write_t *, req, 1)
  NAPI_ARGV_BUFFER(buffer1, 2)
  NAPI_ARGV_UINT32(len1, 3)
  NAPI_ARGV_BUFFER(buffer2, 4)
  NAPI_ARGV_UINT32(len2, 5)

  int err;
  uv_buf_t buf[2] = {
    { .base = buffer1, .len = len1 },
    { .base = buffer2, .len = len2 }
  };

  NAPI_UV_THROWS(err, uv_write(req, TURBO_NET_STREAM, buf, 2, on_uv_write))

  return NULL;
}

NAPI_METHOD(turbo_net_tcp_writev) {
  NAPI_ARGV(4)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)
  NAPI_ARGV_BUFFER_CAST(uv_write_t *, req, 1)

  int err;
  uint32_t len;
  napi_value buffers = argv[2];
  napi_value lengths = argv[3];
  napi_get_array_length(env, buffers, &len);

#ifdef _WIN32
  // no dynamic arrays on windows, so use malloc
  uv_buf_t *bufs = malloc(len * sizeof(uv_buf_t));
#else
  uv_buf_t bufs[len];
#endif

  uv_buf_t *ptr = bufs;
  napi_value element;

  for (uint32_t i = 0; i < len; i++) {
    napi_get_element(env, buffers, i, &element);
    NAPI_BUFFER(next_buf, element)
    napi_get_element(env, lengths, i, &element);
    NAPI_UINT32(next_len, element)
    *ptr++ = (uv_buf_t) { .base = next_buf, .len = next_len };
  }

  NAPI_UV_THROWS(err, uv_write(req, TURBO_NET_STREAM, bufs, len, on_uv_write))

#ifdef _WIN32
  free(bufs);
#endif

  return NULL;
}

NAPI_METHOD(turbo_net_tcp_shutdown) {
  NAPI_ARGV(1)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)

  int err;
  uv_shutdown_t *req = &(self->shutdown);
  NAPI_UV_THROWS(err, uv_shutdown(req, TURBO_NET_STREAM, on_uv_shutdown))

  return NULL;
}

NAPI_METHOD(turbo_net_tcp_close) {
  NAPI_ARGV(1)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)

  uv_close((uv_handle_t *) &(self->handle), on_uv_close);

  return NULL;
}

NAPI_METHOD(turbo_net_tcp_port) {
  NAPI_ARGV(1)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)

  int err;
  struct sockaddr_in addr;
  int addr_len = sizeof(struct sockaddr_in);

  NAPI_UV_THROWS(err, uv_tcp_getsockname(
    &(self->handle),
    (struct sockaddr *) &addr,
    &addr_len
  ))

  int port = ntohs(addr.sin_port);

  NAPI_RETURN_UINT32(port)
}

NAPI_METHOD(turbo_net_tcp_connect) {
  NAPI_ARGV(3)
  NAPI_ARGV_BUFFER_CAST(turbo_net_tcp_t *, self, 0)
  NAPI_ARGV_UINT32(port, 1)
  NAPI_ARGV_UTF8(ip, 17, 2)

  int err;
  struct sockaddr_in addr;

  NAPI_UV_THROWS(err, uv_ip4_addr(ip, port, &addr))

  NAPI_UV_THROWS(err, uv_tcp_connect(
    &(self->connect),
    &(self->handle),
    (const struct sockaddr *) &addr,
    on_uv_connect)
  )

  return NULL;
}

NAPI_METHOD(turbo_net_on_fatal_exception) {
  NAPI_ARGV(1)

  napi_create_reference(env, argv[0], 1, &on_fatal_exception);

  return NULL;
}

NAPI_INIT() {
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_init)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_destroy)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_listen)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_connect)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_port)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_write)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_write_two)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_writev)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_read)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_shutdown)
  NAPI_EXPORT_FUNCTION(turbo_net_tcp_close)
  NAPI_EXPORT_SIZEOF(turbo_net_tcp_t)
  NAPI_EXPORT_SIZEOF(uv_write_t)

  // work around for https://github.com/nodejs/node/issues/15371
  NAPI_EXPORT_FUNCTION(turbo_net_on_fatal_exception)
}
