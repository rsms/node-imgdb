var sys = require('sys'), fs = require('fs');
require.paths.unshift(__dirname+'/build/default');
var r = sys.inspect, puts = sys.puts;
function p(k, v) {
  if (arguments.length === 1)
    sys.puts(r(k))
  else
    sys.puts(k+' => '+r(v))
}

var imgdb = require('imgdb');

p('imgdb', imgdb)
p('imgdb.DB', imgdb.DB)
p('imgdb.DB.prototype', imgdb.DB.prototype)
imgdb.loadSync('db.db')
p('imgdb.loadedDbIds()', imgdb.loadedDbIds())

var db = new imgdb.DB(1)
p('db.length', db.length)


imgdb.saveSync('db.db');
p('imgdb.loadedDbIds()', imgdb.loadedDbIds())
