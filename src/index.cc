#include "imgdb.h"
#include "index.h"

using namespace v8;
using namespace node;

#define STRPTR(obj) (*String::Utf8Value((obj)->ToString()))
#define STRSIZ(obj) ((obj)->ToString()->Utf8Length())

static Persistent<String> id_symbol;
static Persistent<String> ids_symbol;
static Persistent<String> length_symbol;

DB::DB(int id) : ObjectWrap() {
  id_ = id;
  if (!isValidDB(id_))
    initDbase(id_);
}


Handle<Value> DB::New(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsNumber())
    return ThrowException(Exception::Error(String::New("Bad argument")));
  DB *p = new DB(args[0]->IntegerValue());
  p->Wrap(args.Holder());
  return args.This();
}

// ----------------------------------------------------------------------------
// DB.prototype functions

Handle<Value> DB::LoadSync(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::Error(String::New("Bad argument")));
  DB *self = ObjectWrap::Unwrap<DB>(args.Holder());
  if (!loaddb(self->id_, STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}


Handle<Value> DB::SaveSync(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::Error(String::New("Bad argument")));
  DB *self = ObjectWrap::Unwrap<DB>(args.Holder());
  if (!savedb(self->id_, STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}


Handle<Value> DB::Reset(const Arguments& args) {
  HandleScope scope;
  DB *self = ObjectWrap::Unwrap<DB>(args.Holder());
  if (!resetdb(self->id_))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}


Handle<Value> DB::Push(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::Error(String::New("Bad argument")));
  DB *self = ObjectWrap::Unwrap<DB>(args.Holder());
  if (!savedb(self->id_, STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}


Handle<Value> DB::LengthGetter(Local<String> property, const AccessorInfo& info) {
  HandleScope scope;
  DB *self = ObjectWrap::Unwrap<DB>(info.This()); assert(self);
  Local<Integer> v = Integer::New(getImgCount(self->id_));
  return scope.Close(v);
}

// ----------------------------------------------------------------------------
// module functions

static Handle<Value> LoadedDbIds(const Arguments& /*args*/){
  HandleScope scope;
  std::vector<int> v = getDBList();
  size_t len = v.size();
  Local<Array> a = Array::New(len);
  for (size_t i = 0; i < len; ++i) {
    a->Set(Integer::New(i), Integer::New(v[i]));
  }
  return scope.Close(a);
}

// load all databases in file arg0
static Handle<Value> LoadSync(const Arguments& args){
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::Error(String::New("Bad argument")));
  if (!loadalldbs(STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}

// load all databases to file arg0
static Handle<Value> SaveSync(const Arguments& args){
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::Error(String::New("Bad argument")));
  if (!savealldbs(STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}

// ----------------------------------------------------------------------------
// setup

void DB::Initialize(Handle<Object> target) {
  HandleScope scope;

  Local<FunctionTemplate> t = FunctionTemplate::New(DB::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(String::NewSymbol("DB"));
  
  // Class-method:
  //NODE_SET_METHOD(t, "list", List);

  NODE_SET_PROTOTYPE_METHOD(t, "loadSync", DB::SaveSync);
  NODE_SET_PROTOTYPE_METHOD(t, "saveSync", DB::SaveSync);
  NODE_SET_PROTOTYPE_METHOD(t, "reset", DB::SaveSync);
  // images
  NODE_SET_PROTOTYPE_METHOD(t, "push", DB::Push);
  
  // DB.prototype properties
  t->InstanceTemplate()->SetAccessor(length_symbol, LengthGetter);

  target->Set(String::NewSymbol("DB"), t->GetFunction());
}

extern "C" void init (Handle<Object> target) {
  HandleScope scope;
  Local<Object> exports;
  
  id_symbol = NODE_PSYMBOL("id");
  length_symbol = NODE_PSYMBOL("length");
  ids_symbol = NODE_PSYMBOL("ids");

  DB::Initialize(target);

  NODE_SET_METHOD(target, "loadedDbIds", LoadedDbIds);
  NODE_SET_METHOD(target, "loadSync", LoadSync);
  NODE_SET_METHOD(target, "saveSync", SaveSync);

  target->Set(String::NewSymbol("version"), String::New("0.1"));
}
