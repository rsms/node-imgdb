var sys = require('sys'),
    fs = require('fs'),
    imgdb = require('./imgdb');

var r = sys.inspect, puts = sys.puts;
function p(k, v) {
  if (arguments.length === 1) sys.puts(r(k))
  else sys.puts(k+' => '+r(v))
}

//p('imgdb', imgdb)
//p('imgdb.DB', imgdb.DB)
//p('imgdb.DB.prototype', imgdb.DB.prototype)
//p('imgdb.loadedDbIds()', imgdb.loadedDbIds())

var db = new imgdb.DB(1)
try { db.loadSync('db.db') }catch(e){}
p('imgdb.loadedDbIds()', imgdb.loadedDbIds())
p('db.length', db.length)

// add image from file
var imageFile = '/Users/rasmus/similar-images/low-winter-sun.jpg';
db.addImageFromFile(imageFile, function(err, id) {
  if (err) return sys.error(err.stack || err);
  p('addImageFromFile completed', id);
  //p('db.addKeyword('+id+', "foo")', db.addKeyword(id, 'foo'));
  p('db.remove('+id+')', db.remove(id));
})

// add image from URL
var imageURL = 'http://www.google.com/intl/en_ALL/images/srpr/logo1w.png';
db.addImageFromURL(imageURL, function(err, id) {
  if (err) return sys.error(err.stack || err);
  p('addImageFromURL completed', id);
  p('db.query('+id+')', db.query(1))
  //p('db.remove('+id+')', db.remove(id));
})

// add image from file in a synchronous manner
var id = db.addImageFromFileOrURLSync(imageFile);
p('addImageFromFileOrURLSync', id)
p('db.length', db.length)

p('db.contains('+id+')', db.contains(id))

p('imgdb.loadedDbIds()', imgdb.loadedDbIds())

p('db.addKeyword(0, "foo")', db.addKeyword(0, 'foo'));
p("db.addKeywords(0, ['foo', 'bar', 'baz'])", db.addKeywords(0, ['foo', 'bar', 'baz']));

p('db.ids', db.ids)

p('db.queryByKeywords(["foo"])', db.queryByKeywords(["foo"]))

db.saveSync('db.db');
