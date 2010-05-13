/**
 * This test takes all images in `dirname`, puts them into a database and
 * renders a HTML page with query results for each image.
 *
 * This requires you to have a bunch of images in `dirname` and changing the
 * `dirname` variable to point to your directory. Also, automatically opening of
 * the generated HTML page is only supported for Mac OS X.
 */

var sys = require('sys'),
    fs = require('fs'),
    exec = require('child_process').exec,
    imgdb = require('./imgdb');

var db = new imgdb.DB(1),
    dirname = '/Users/rasmus/similar-images',
    scoreThreshold = 0.5, // what's considered "almost the same image" (adds a *)
    limit = 5,
    imagenameRE = /\.(?:jpe?g|png|gif|tif+|bmp)$/;

fs.readdir(dirname, function(err, names) {
  var idToName = {}, buf = [];

  names.filter(function(name){
    return name.substr(0,1) !== '.' && name.match(imagenameRE);
  }).forEach(function(name){
    id = db.addImageFromFileOrURLSync(dirname+'/'+name);
    idToName[id] = name;
    sys.puts(id+' => '+name);
  })
  
  buf.push('<html><head><style type="text/css">'+
    'p,td,body,li { font-family:helvetica }'+
    'td { border-top:1px solid #ccc; padding:1em; }'+
    'th { text-align:center; background:#ddd; }'+
    'hr { height:4px; background:#9df; border:none; margin:0 -999px; }'+
    '@media print {'+
      'hr { display:none; }'+
      'pbr { display:block; page-break-before:always; }'+
      '.intro { margin-bottom:1em; padding-bottom:1em; border-bottom:1pt solid #eee; }'+
      '.intro h1 { display:none; }'+
    '}'+
    '</style><body>'+
    '<div class="intro">'+
      '<h1>node-imgdb/test2.js</h1>'+
      '<p>* = match (past the threshold of '+scoreThreshold+')</p>'+
    '</div>');
  
  db.ids.forEach(function(id){
    buf.push('<hr>'+
      '<h2>'+idToName[id]+' (#'+id+')'+'</h2>'+
      '<p><img src="'+idToName[id]+'" style="max-width:256px;max-height:256px;"></p>'+
      '<table>');

    buf.push('<tr><th>Method</th>');
    for (var i=0;i<limit;++i)
      buf.push('<th>#'+(i+1)+'</th>');
    buf.push('</tr>');

    buf.push('<tr><td><b>Avg. luminance</b><br><tt>N=distance [0-inf]</tt></td>');
    
    db.query(id, limit, true).forEach(function(t){
      //if (t.score < 0.1) return;
      buf.push('<td align="center" valign="bottom"><img src="'+idToName[t.id]+
        '" style="max-width:128px;max-height:128px;"><br>'+
        Number(t.score).toFixed(4)+' '+(t.score <= scoreThreshold ? '*' : '')+
        '</td>');
    })
    buf.push('</tr>');
    
    buf.push('<tr><td><b>Features+Avg. lum.</b><br><tt>N=similarity [1-0]</tt></td>');
    db.query(id, limit).forEach(function(t){
      //if (t.score < 0.1) return;
      buf.push('<td align="center" valign="bottom"><img src="'+idToName[t.id]+
        '" style="max-width:128px;max-height:128px;"><br>'+
        Number(t.score).toFixed(4)+' '+(t.score >= scoreThreshold ? '*' : '')+
        '</td>');
    })
    buf.push('</tr>');
    
    buf.push('</table><pbr>');
  })
  
  buf.push('</body></html>');
  fs.writeFileSync(dirname+'/index.html', buf.join('\n'));
  exec("open '"+dirname+"/index.html'");
});


