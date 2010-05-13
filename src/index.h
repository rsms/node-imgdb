#ifndef IMGDB_INDEX_H_
#define IMGDB_INDEX_H_

#include <node.h>
#include <node_object_wrap.h>
#include <v8.h>

class DB : node::ObjectWrap {
public:
  static void Initialize(v8::Handle<v8::Object> target);

protected:
  static v8::Handle<v8::Value> New(const v8::Arguments& args);
  static v8::Handle<v8::Value> LoadSync(const v8::Arguments& args);
  static v8::Handle<v8::Value> SaveSync(const v8::Arguments& args);
  static v8::Handle<v8::Value> Reset(const v8::Arguments& args);
  // images
  static v8::Handle<v8::Value> Push(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> LengthGetter(v8::Local<v8::String> property,
    const v8::AccessorInfo& info);

  DB(int id);
  ~DB() { }

private:
  int id_;
};

#endif // IMGDB_INDEX_H_
