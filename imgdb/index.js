var binding = require('./binding'),
    fs = require('fs'),
    http = require('http'),
    Buffer = require('buffer').Buffer;

var URL_RE = /^https?:\/\//;
var DB = binding.DB;

Object.keys(binding).forEach(function(k){ exports[k] = binding[k]; });

function _addImageFromBuffer(self, buffer, callback) {
  try {
    var id = self.addImageFromData(buffer);
    callback(null, id);
  } catch (e) {
    if (callback) callback(e);
  }
}

function _setImageFromBuffer(self, id, buffer, callback) {
  try {
    self.setImageFromData(id, buffer);
    callback(null);
  } catch (e) {
    if (callback) callback(e);
  }
}

function _queryImageBuffer(self, argc, buffer, limit, fast, sketch, callback) {
  var v;
  try {
    if (argc === 2) {
      v = self.queryByImageData(buffer);
    } else if (argc === 3) {
      v = self.queryByImageData(buffer, limit);
    } else if (argc === 4) {
      v = self.queryByImageData(buffer, limit, fast);
    } else {
      v = self.queryByImageData(buffer, limit, fast, sketch);
    }
    callback(null, v);
  } catch (e) {
    callback(e);
  }
}

function _httpCatBuffer(url, callback) {
  http.cat(url, 'binary', function(err, str){
    if (err) return callback(err);
    var buffer = new Buffer(str.length);
    buffer.binaryWrite(str, 0);
    callback(null, buffer);
  });
}

// addImageFromFile(filename[, callback(err, newId)])
DB.prototype.addImageFromFile = function(filename, callback) {
  var self = this;
  fs.readFile(filename, function(err, buffer){
    if (err) return callback && callback(err);
    _addImageFromBuffer(self, buffer, callback);
  });
}

// setImageFromFile(id, filename[, callback(err)])
DB.prototype.setImageFromFile = function(id, filename, callback) {
  var self = this;
  fs.readFile(filename, function(err, buffer){
    if (err) return callback && callback(err);
    _setImageFromBuffer(self, id, buffer, callback);
  });
}

// addImageFromURL(url[, callback(err, newId)])
DB.prototype.addImageFromURL = function(url, callback) {
  var self = this;
  _httpCatBuffer(url, function(err, buffer){
    if (err) return callback && callback(err);
    _addImageFromBuffer(self, buffer, callback);
  });
}

// setImageFromURL(id, url[, callback(err)])
DB.prototype.setImageFromURL = function(id, url, callback) {
  var self = this;
  _httpCatBuffer(url, function(err, buffer){
    if (err) return callback && callback(err);
    _setImageFromBuffer(self, id, buffer, callback);
  });
}

// addImageFromFileOrURL(filename|url[, callback(err, newImageId)])
DB.prototype.addImageFromFileOrURL = function(source, callback) {
  // primarily exists to match the sync version in binding (which is a combo
  // because of the underlying ImageMagick API used for sync methods).
  if (source.match(URL_RE)) {
    this.addImageFromURL(source, callback);
  } else {
    this.addImageFromFile(source, callback);
  }
}

function _queryByImageFileOrURL(fun, source, limit, fast, sketch, callback) {
  var self = this;
  callback = arguments[arguments.length-1];
  if (typeof callback !== 'function')
    throw new TypeError("last argument must be a function");
  fun(source, function(err, buffer){
    if (err) return callback(err);
    _queryImageBuffer(self, arguments.length, buffer, limit, fast, sketch, callback);
  });
}

DB.prototype.queryByImageFile = function(filename, limit, fast, sketch, callback) {
  _queryByImageFileOrURL.apply(this,
    [fs.readFile].concat(Array.prototype.slice.call(arguments)));
}

DB.prototype.queryByImageURL = function(url, limit, fast, sketch, callback) {
  _queryByImageFileOrURL.apply(this,
    [_httpCatBuffer].concat(Array.prototype.slice.call(arguments)));
}
