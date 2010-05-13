#include "imgdb.h"
#include "index.h"

#include <node_buffer.h>
#include <Magick++.h>

using namespace v8;
using namespace node;

#define STRPTR(obj) (*String::Utf8Value((obj)->ToString()))
#define STRSIZ(obj) ((obj)->ToString()->Utf8Length())

#define VDOUBLE(obj) ((obj)->NumberValue())
#define VINT32(obj) ((obj)->Int32Value())
#define VINT64(obj) ((obj)->IntegerValue())

// null or undefined
#define VNOU(obj) ((obj)->IsNull() || (obj)->IsUndefined())

static uint _strHash(const char *str) {
	char ch;
	unsigned int h = 0;
	while ((ch = *str++) != 0) {
		h ^= h >> 27;
		h <<= 5;
		h ^= ch;
	}
	return h;
}


DB::DB(int id) : ObjectWrap() {
  id_ = id;
  if (!isValidDB(id_))
    initDbase(id_);
}


Handle<Value> DB::New(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsNumber())
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a number")));
  DB *p = new DB(args[0]->IntegerValue());
  p->Wrap(args.This());
  return args.This();
}

// ----------------------------------------------------------------------------
// DB.prototype functions

Handle<Value> DB::LoadSync(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a string")));
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  if (!loaddb(self->id_, STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}


Handle<Value> DB::SaveSync(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a string")));
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  if (!savedb(self->id_, STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}


Handle<Value> DB::Reset(const Arguments& args) {
  HandleScope scope;
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  if (!resetdb(self->id_))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}


Handle<Value> DB::Contains(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    return ThrowException(Exception::TypeError(
      String::New("first and only argument must be a number")));
  }
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  bool y = isImageOnDB(self->id_, VINT64(args[0]));
  return scope.Close(Boolean::New(y));
}


Handle<Value> DB::Remove(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    return ThrowException(Exception::TypeError(
      String::New("first and only argument must be a number")));
  }
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  bool y = !!removeID(self->id_, VINT64(args[0]));
  return scope.Close(Boolean::New(y));
}

static Local<Array> _packageScoreList(const std::vector<double> &v,
                              const Local<Value> &id,
                              bool fast)
{
  size_t len = v.size();
  Local<Array> a = Array::New(len);
  if (len > 1) {
    size_t x = 0;
    for (ssize_t i = len-2; i > -1; i -= 2) {
      Local<Integer> tid = Integer::New(v[i]);
      if (tid == id) continue;
      double score = v[i+1];
      if (fast) {
        /*if (score > 0.0)
          score = 1.0/score;
        score = 1.0 - score;*/
      } else {
        score = (-score) / 38.0f;
        if (score < 0.0) score = 0.0;
        else if (score > 1.0) score = 1.0;
      }
      Local<Object> o = Object::New();
      o->Set(String::New("id"), Integer::New(v[i]));
      o->Set(String::New("score"), Number::New(score));
      
      a->Set(Integer::New(x++), o);
    }
  }
  return a;
}


// query(id[, limit[, fast[, keywords]]]) -> [id, ..]
Handle<Value> DB::Query(const Arguments& args) {
  HandleScope scope;
  try {
  if (args.Length() < 1 || !args[0]->IsNumber()) {
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a number")));
  }
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  unsigned int numres = 25;
  bool fast = false;
  int keywords = 0; // 1=auto/extract, 2=explicit
  int kwJoinType = 0; // 0=OR, 1=AND

  if (args.Length() > 1) {
    if (!args[1]->IsNumber()) {
      return ThrowException(Exception::TypeError(
        String::New("second argument must be a number")));
    }
    numres = VINT32(args[1]);
    if (args.Length() > 2) {
      if (!args[2]->IsBoolean()) return ThrowException(Exception::TypeError(
        String::New("third argument must be a boolean")));
      fast = args[2]->BooleanValue();
      if (args.Length() > 3) {
        if (args[3]->IsBoolean()) {
          if (args[3]->BooleanValue())
            keywords = 1;
        } else if (args[3]->IsArray()) {
          keywords = 2;
        } else {
          return ThrowException(Exception::TypeError(
            String::New("fourth argument must be a boolean or an array")));
        }
      }
    }
  }
  
  long int maxnum = getImgCount(self->id_);
  
  // compensate for not returning ourself
  numres++;
  if (numres > maxnum) numres = maxnum;
  
  std::vector<double> v;
  
  if (keywords) {
    std::vector<int> hv;
    
    if (keywords == 1) {
      hv = getKeywordsImg(self->id_, VINT64(args[0]));
    } else if (keywords == 2) {
      Local<Array> a = Local<Array>::Cast(args[3]);
      uint32_t len = a->Length();
      hv = std::vector<int>(len);
      for (uint32_t i = 0; i<len; ++i) {
        hv[i] = _strHash(STRPTR(a->Get(i)));
      }
    }
    if (fast) {
      // not yet implemented
      //v = queryImgIDFastKeywords(self->id_, VINT64(args[0]), numres, kwJoinType, hv);
      return ThrowException(Exception::Error(
        String::New("not yet implemented: fast keyword-aided query")));
    } else {
      v = queryImgIDKeywords(self->id_, VINT64(args[0]), numres, kwJoinType, hv);
    }
  } else {
    if (fast) {
      v = queryImgIDFast(self->id_, VINT64(args[0]), numres);
    } else {
      v = queryImgID(self->id_, VINT64(args[0]), numres);
    }
  }
  
  Local<Array> a = _packageScoreList(v, args[0], fast);
  return scope.Close(a);
  
  } catch (std::string e) {
    return ThrowException(Exception::Error(
      String::Concat(String::New("ImgDBError: "), String::New(e.c_str()))
    ));
  }
}


// queryByImageData(data[, limit=25[, fast=false[, sketch=false]]]) -> [id, ..]
Handle<Value> DB::QueryByImageData(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1 || !Buffer::HasInstance(args[0])) {
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a buffer")));
  }
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  Buffer *buffer = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
  unsigned int numres = 25;
  bool fast = false;
  int sketch = 0;

  if (args.Length() > 1) {
    if (!args[1]->IsNumber()) {
      return ThrowException(Exception::TypeError(
        String::New("second argument must be a number")));
    }
    numres = VINT32(args[1]);
    if (args.Length() > 2) {
      if (!args[2]->IsBoolean()) {
        return ThrowException(Exception::TypeError(
          String::New("third argument must be a boolean")));
      }
      fast = args[2]->BooleanValue();
      if (args.Length() > 3) {
        if (!args[3]->IsBoolean()) {
          return ThrowException(Exception::TypeError(
            String::New("fourth argument must be a boolean")));
        }
        sketch = args[3]->BooleanValue() ? 1 : 0;
      }
    }
  }

  numres++; // <- compensate for not returning ourself
  long int maxnum = getImgCount(self->id_);
  if (numres > maxnum) numres = maxnum;

  Magick::Blob blob((const void*)buffer->data(), buffer->length());
  Magick::Image image(blob);
  SigStruct *nsig = analyzeImage(&image);

  std::vector<double> v;

  if (fast) {
    v = queryImgDataFast(self->id_, nsig->sig1, nsig->sig2, nsig->sig3, nsig->avgl,
      numres, sketch);
  } else {
    v = queryImgData(self->id_, nsig->sig1, nsig->sig2, nsig->sig3, nsig->avgl,
      numres, sketch);
  }

  delete nsig;

  Local<Array> a = _packageScoreList(v, args[0], fast);
  return scope.Close(a);
}


// queryByKeywords(keywords[, numres[, andJoin]])
Handle<Value> DB::QueryByKeywords(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1 || !args[0]->IsArray()) {
    return ThrowException(Exception::TypeError(
      String::New("first argument must be an array")));
  }
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  unsigned int numres = 25;
  int kwJoinType = 0; // 0=OR, 1=AND

  if (args.Length() > 1) {
    if (!args[1]->IsNumber()) {
      return ThrowException(Exception::TypeError(
        String::New("second argument must be a number")));
    }
    numres = VINT32(args[1]);
    if (args.Length() > 2) {
      if (!args[2]->IsBoolean()) return ThrowException(Exception::TypeError(
        String::New("third argument must be a boolean")));
      kwJoinType = args[2]->BooleanValue() ? 1 : 0;
    }
  }

  long int maxnum = getImgCount(self->id_);
  if (numres > maxnum) numres = maxnum;

  Local<Array> a = Local<Array>::Cast(args[0]);
  uint32_t len = a->Length();
  std::vector<int> hv(len);
  for (uint32_t i = 0; i<len; ++i) {
    hv[i] = _strHash(STRPTR(a->Get(i)));
  }
  
  std::vector<long int> v;
  v = getAllImgsByKeywords(self->id_, numres, kwJoinType, hv);
  
  len = v.size();
  Local<Array> a2 = Array::New(len);
  for (uint32_t i = 0; i<len; ++i) {
    a2->Set(i, Integer::New(v[i]));
  }
  return scope.Close(a2);
}


Handle<Value> DB::SetImageFromFileOrURLSync(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 2) return ThrowException(Exception::TypeError(
    String::New("takes exactly 2 arguments")));
  if (args[0]->IsNumber()) return ThrowException(Exception::TypeError(
    String::New("first argument must be a number")));
  if (args[1]->IsString()) return ThrowException(Exception::TypeError(
    String::New("second argument must be a string")));
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  long int id = VINT64(args[0]);
  if (!addImage(self->id_, id, STRPTR(args[1])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  Local<Integer> v = Integer::New(id);
  return scope.Close(v);
}


Handle<Value> DB::AddImageFromFileOrURLSync(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a string")));
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  long int id = getImgCount(self->id_);
  if (!addImage(self->id_, id, STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  Local<Integer> v = Integer::New(id);
  return scope.Close(v);
}

// addImageFromData(buffer[, offset[, length]])
Handle<Value> DB::AddImageFromData(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 1 || !Buffer::HasInstance(args[0])) {
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a buffer")));
  }
  
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  Buffer *buffer = ObjectWrap::Unwrap<Buffer>(args[0]->ToObject());
  size_t off = 0;
  size_t len = buffer->length();
  
  if (args.Length() > 1) {
    if (!args[1]->IsNumber()) {
      return ThrowException(Exception::TypeError(
        String::New("second argument must be a number")));
    }
    off = args[1]->Int32Value();
    if (off >= buffer->length()) {
      return ThrowException(Exception::Error(
        String::New("offset is out of bounds")));
    }
    
    if (args.Length() > 2) {
      if (!args[2]->IsNumber()) {
        return ThrowException(Exception::TypeError(
          String::New("third argument must be a number")));
      }
      len = args[2]->Int32Value();
      if (off + len > buffer->length()) {
        return ThrowException(Exception::Error(
              String::New("length extends beyond buffer")));
      }
    }
  }

  long int id = getImgCount(self->id_);

  int rc = addImageBlob(self->id_, id,
    (const void *)((char*)buffer->data() + off), len);
  if (!rc)
    return ThrowException(Exception::Error(String::New("ImgDBError")));

  Local<Integer> v = Integer::New(id);
  return scope.Close(v);
}


// setImageFromData(id, buffer[, offset[, length]])
Handle<Value> DB::SetImageFromData(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 2) return ThrowException(Exception::TypeError(
    String::New("takes at least 2 arguments")));

  if (!args[0]->IsNumber()) return ThrowException(Exception::TypeError(
    String::New("first argument must be a number")));

  if (!Buffer::HasInstance(args[1])) return ThrowException(Exception::TypeError(
    String::New("second argument must be a buffer")));
  
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  long int id = VINT64(args[0]);
  Buffer *buffer = ObjectWrap::Unwrap<Buffer>(args[1]->ToObject());
  size_t off = 0;
  size_t len = buffer->length();
  
  if (args.Length() > 2) {
    if (!args[2]->IsNumber()) return ThrowException(Exception::TypeError(
      String::New("third argument must be a number")));
    off = args[2]->Int32Value();
    if (off >= buffer->length()) return ThrowException(Exception::Error(
      String::New("offset is out of bounds")));

    if (args.Length() > 3) {
      if (!args[3]->IsNumber()) return ThrowException(Exception::TypeError(
        String::New("fourth argument must be a number")));
      len = args[3]->Int32Value();
      if (off + len > buffer->length()) return ThrowException(Exception::Error(
        String::New("length extends beyond buffer")));
    }
  }

  int rc = addImageBlob(self->id_, id,
    (const void *)((char*)buffer->data() + off), len);
  if (!rc)
    return ThrowException(Exception::Error(String::New("ImgDBError")));

  Local<Integer> v = Integer::New(id);
  return scope.Close(v);
}


// getImageDimensions(id) -> {width:int, height:int}
Handle<Value> DB::GetImageDimensions(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    return ThrowException(Exception::TypeError(
      String::New("first and only argument must be a number")));
  }
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  int w = 0, h = 0;
  getImageWidthAndHeight(self->id_, VINT64(args[0]), &w, &h);
  Local<Object> o = Object::New();
  o->Set(String::NewSymbol("width"), Integer::New(w));
  o->Set(String::NewSymbol("height"), Integer::New(h));
  return scope.Close(o);
}


// getImageAvgL(id) -> [Y, I, Q]
Handle<Value> DB::GetImageAvgL(const Arguments& args) {
  // avgl is the average luminance of Y, I and Q (YIQ color space)
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    return ThrowException(Exception::TypeError(
      String::New("first and only argument must be a number")));
  }
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  std::vector<double> v = getImageAvgl(self->id_, VINT64(args[0]));
  size_t len = v.size();
  Local<Array> a = Array::New(len);
  for (uint32_t i = 0; i<len; ++i) {
    a->Set(i, Number::New(v[i]));
  }
  return scope.Close(a);
}


// addKeyword(id, keyword) -> hash|false
Handle<Value> DB::AddKeyword(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 2) return ThrowException(Exception::TypeError(
    String::New("takes at least 2 arguments")));
  if (!args[0]->IsNumber()) return ThrowException(Exception::TypeError(
    String::New("first argument must be a number")));
  if (!args[1]->IsString()) return ThrowException(Exception::TypeError(
    String::New("second argument must be a string")));
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  uint32_t hash = _strHash(STRPTR(args[1]));
  bool added = addKeywordImg(self->id_, VINT32(args[0]), hash);
  if (added) {
    return scope.Close(Integer::New(hash));
  } else {
    return scope.Close(Boolean::New(false));
  }
}


// addKeywordHash(id, keywordHash) -> added
Handle<Value> DB::AddKeywordHash(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 2) return ThrowException(Exception::TypeError(
    String::New("takes at least 2 arguments")));
  if (!args[0]->IsNumber()) return ThrowException(Exception::TypeError(
    String::New("first argument must be a number")));
  if (!args[1]->IsNumber()) return ThrowException(Exception::TypeError(
    String::New("second argument must be an integer")));
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  bool added = addKeywordImg(self->id_, VINT32(args[0]), VINT32(args[1]));
  return scope.Close(Boolean::New(added));
}


// addKeywords(id, keywords) -> [hash, ..]
Handle<Value> DB::AddKeywords(const Arguments& args) {
  HandleScope scope;
  if (args.Length() < 2) return ThrowException(Exception::TypeError(
    String::New("takes at least 2 arguments")));
  if (!args[0]->IsNumber()) return ThrowException(Exception::TypeError(
    String::New("first argument must be a number")));
  if (!args[1]->IsArray()) return ThrowException(Exception::TypeError(
    String::New("second argument must be an array")));
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  Local<Array> a = Local<Array>::Cast(args[1]);
  uint32_t len = a->Length();
  Local<Array> ha = Array::New(len);
  std::vector<int> v(len);
  for (uint32_t i = 0; i<len; ++i) {
    int h = _strHash(STRPTR(a->Get(i)));
    v[i] = h;
    ha->Set(i, Number::New(h));
  }
  addKeywordsImg(self->id_, VINT32(args[0]), v);
  return scope.Close(ha);
}


Handle<Value> DB::GetKeywordHashes(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    return ThrowException(Exception::TypeError(
      String::New("first and only argument must be a number")));
  }
  DB *self = ObjectWrap::Unwrap<DB>(args.This());
  std::vector<int> v = getKeywordsImg(self->id_, VINT64(args[0]));
  size_t len = v.size();
  Local<Array> a = Array::New(len);
  for (size_t i = 0; i < len; ++i) {
    a->Set(Integer::New(i), Integer::New(v[i]));
  }
  return scope.Close(a);
}


Handle<Value> DB::LengthGetter(Local<String> /*property*/, const AccessorInfo& info) {
  HandleScope scope;
  DB *self = ObjectWrap::Unwrap<DB>(info.This()); assert(self);
  Local<Integer> v = Integer::New(getImgCount(self->id_));
  return scope.Close(v);
}


Handle<Value> DB::IdsGetter(Local<String> /*property*/, const AccessorInfo& info) {
  HandleScope scope;
  DB *self = ObjectWrap::Unwrap<DB>(info.This()); assert(self);
  std::vector<long int> v = getImgIdList(self->id_);
  size_t len = v.size();
  Local<Array> a = Array::New(len);
  for (size_t i = 0; i < len; ++i) {
    a->Set(Integer::New(i), Integer::New(v[i]));
  }
  return scope.Close(a);
}

// ----------------------------------------------------------------------------
// module functions

Handle<Value> Remove(const Arguments& args) {
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsNumber()) {
    return ThrowException(Exception::TypeError(
      String::New("first and only argument must be a number")));
  }
  if (!removedb(VINT32(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}

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
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a string")));
  if (!loadalldbs(STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}

// load all databases to file arg0
static Handle<Value> SaveSync(const Arguments& args){
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString())
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a string")));
  if (!savealldbs(STRPTR(args[0])))
    return ThrowException(Exception::Error(String::New("ImgDBError")));
  return Undefined();
}

// return the 32bit integer hash for keyword arg0
static Handle<Value> KeywordHash(const Arguments& args){
  HandleScope scope;
  if (args.Length() != 1 || !args[0]->IsString()) {
    return ThrowException(Exception::TypeError(
      String::New("first argument must be a string")));
  }
  return scope.Close(Integer::New(_strHash(STRPTR(args[0]))));
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

  NODE_SET_PROTOTYPE_METHOD(t, "loadSync", DB::LoadSync);
  NODE_SET_PROTOTYPE_METHOD(t, "saveSync", DB::SaveSync);
  NODE_SET_PROTOTYPE_METHOD(t, "reset", DB::Reset);
  // images
  NODE_SET_PROTOTYPE_METHOD(t, "addImageFromFileOrURLSync", DB::AddImageFromFileOrURLSync);
  NODE_SET_PROTOTYPE_METHOD(t, "setImageFromFileOrURLSync", DB::SetImageFromFileOrURLSync);
  NODE_SET_PROTOTYPE_METHOD(t, "addImageFromData", DB::AddImageFromData);
  NODE_SET_PROTOTYPE_METHOD(t, "setImageFromData", DB::SetImageFromData);
  NODE_SET_PROTOTYPE_METHOD(t, "getImageDimensions", DB::GetImageDimensions);
  NODE_SET_PROTOTYPE_METHOD(t, "getImageAvgL", DB::GetImageAvgL);
  NODE_SET_PROTOTYPE_METHOD(t, "contains", DB::Contains);
  NODE_SET_PROTOTYPE_METHOD(t, "remove", DB::Remove);
  NODE_SET_PROTOTYPE_METHOD(t, "query", DB::Query);
  NODE_SET_PROTOTYPE_METHOD(t, "queryByImageData", DB::QueryByImageData);
  NODE_SET_PROTOTYPE_METHOD(t, "queryByKeywords", DB::QueryByKeywords);
  NODE_SET_PROTOTYPE_METHOD(t, "addKeyword", DB::AddKeyword);
  NODE_SET_PROTOTYPE_METHOD(t, "addKeywordHash", DB::AddKeywordHash);
  NODE_SET_PROTOTYPE_METHOD(t, "addKeywords", DB::AddKeywords);
  NODE_SET_PROTOTYPE_METHOD(t, "getKeywordHashes", DB::GetKeywordHashes);

  // DB.prototype properties
  t->InstanceTemplate()->SetAccessor(String::NewSymbol("length"), LengthGetter);
  t->InstanceTemplate()->SetAccessor(String::NewSymbol("ids"), IdsGetter);

  target->Set(String::NewSymbol("DB"), t->GetFunction());
}

extern "C" void init (Handle<Object> target) {
  HandleScope scope;
  Local<Object> exports;

  DB::Initialize(target);

  NODE_SET_METHOD(target, "loadedDbIds", LoadedDbIds);
  NODE_SET_METHOD(target, "loadSync", LoadSync);
  NODE_SET_METHOD(target, "saveSync", SaveSync);
  NODE_SET_METHOD(target, "remove", Remove);
  NODE_SET_METHOD(target, "keywordHash", KeywordHash);

  target->Set(String::NewSymbol("version"), String::New("0.1"));
}
