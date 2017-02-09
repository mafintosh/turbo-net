#ifndef TURBO_NET_MACROS_H
#define TURBO_NET_MACROS_H


#define TURBO_NET_LISTENER(Type, OnEvent, on_event) \
  NAN_METHOD(OnEvent) { \
    Type *self = Nan::ObjectWrap::Unwrap<Type>(info.This()); \
    on_event = new Nan::Callback(info[0].As<Function>()); \
  }

#define TURBO_NET_CALL(self, method, argc, argv) \
  assert(self->method != NULL); \
  Local<Object> ctx = Nan::New(self->context); \
  self->method->Call(ctx, argc, argv);

#define TURBO_NET_RETURN_ADDRESS(handle, getname) \
  Local<Object> result = Nan::New<Object>(); \
  \
  int err; \
  struct sockaddr_in name; \
  int name_len = sizeof(name); \
  \
  err = getname(handle, (struct sockaddr *) &name, &name_len); \
  if (err) { \
    Nan::ThrowError("Could not get address"); \
    return; \
  } \
  \
  int port = ntohs(name.sin_port); \
  char ip[17]; \
  \
  uv_ip4_name(&name, ip, 17); \
  \
  Nan::Set(result, Nan::New<String>("address").ToLocalChecked(), Nan::New<String>(ip).ToLocalChecked()); \
  Nan::Set(result, Nan::New<String>("port").ToLocalChecked(), Nan::New<Number>(port)); \
  \
  info.GetReturnValue().Set(scope.Escape(result));

#define TURBO_NET_ON_ERROR(self, on_error, status) \
  Local<Value> argv[] = { \
    Nan::New<String>(uv_err_name(status)).ToLocalChecked() \
  }; \
  TURBO_NET_CALL(self, on_error, 1, argv)

#endif
