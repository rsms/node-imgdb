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
  static v8::Handle<v8::Value> AddImageFromFileOrURLSync(const v8::Arguments& args);
  static v8::Handle<v8::Value> SetImageFromFileOrURLSync(const v8::Arguments& args);
  static v8::Handle<v8::Value> AddImageFromData(const v8::Arguments& args);
  static v8::Handle<v8::Value> SetImageFromData(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetImageDimensions(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetImageAvgL(const v8::Arguments& args);
  static v8::Handle<v8::Value> Contains(const v8::Arguments& args);
  static v8::Handle<v8::Value> Remove(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> Query(const v8::Arguments& args);
  static v8::Handle<v8::Value> QueryByImageData(const v8::Arguments& args);
  static v8::Handle<v8::Value> QueryByKeywords(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> AddKeyword(const v8::Arguments& args);
  static v8::Handle<v8::Value> AddKeywordHash(const v8::Arguments& args);
  static v8::Handle<v8::Value> AddKeywords(const v8::Arguments& args);
  static v8::Handle<v8::Value> GetKeywordHashes(const v8::Arguments& args);
  
  static v8::Handle<v8::Value> LengthGetter(v8::Local<v8::String> property,
    const v8::AccessorInfo& info);
  static v8::Handle<v8::Value> IdsGetter(v8::Local<v8::String> property,
    const v8::AccessorInfo& info);

  DB(int id);
  ~DB() { }

private:
  int id_;
};

#endif // IMGDB_INDEX_H_
