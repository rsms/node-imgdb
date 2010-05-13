# imgdb

Image fingerprinting.

## Building & installation

    node-waf configure
    node-waf build
    sudo node-waf install

### Requirements

- [Nodejs](http://nodejs.org/) >= v0.1.94
- [ImageMagick (libMagick++)](http://www.imagemagick.org/Magick++/)

### Example

    var db = new imgdb.DB(1)
    db.addImageFromFileOrURLSync('http://some.site/kittens.jpg')
    db.addImageFromFile('mothercat.jpg', function(err, id) {
      if (err) return sys.error(err.stack || err)
      sys.puts('added image with id ' + id)
      var result = db.query(0)
      sys.puts('images similar to image '+id+': '+sys.inspect(result))
    })
    db.saveSync('db.db');

# API

## imgdb

### imgdb.loadSync(filename)

Load all databases in `filename` in a synchronous manner.

### imgdb.saveSync(filename)

Save all databases to `filename` in a synchronous manner.

### imgdb.loadedDbIds() -> Array

List loaded database ids (an array of integers).

### imgdb.keywordHash(keyword) -> int

Calculate the 32-bit integer hash value for `keyword` string. This is the hashing algorithm used for keywords.

---

## imgdb.DB

### new DB(id)

New database object with database identifier `id` (an integer).

### DB.prototype.length -> int

Number of images in the database.

### DB.prototype.ids -> [id, ..]

Array of ids for all images in the database.

### DB.prototype.loadSync(filename)

Load database from `filename`.

### DB.prototype.saveSync(filename)

Save database to `filename`.

### DB.prototype.reset()

Reset the database, effectively removing everything.
**Warning:** Calling this method removes all data in the current database.

---

*Section: Adding images*

### DB.prototype.addImageFromFile(filename[, callback(err, id)])

Add an image from `filename`.

### DB.prototype.setImageFromFile(id, filename[, callback(err)])

Like `DB.prototype.addImageFromFile`, but replaces the image if `id` already exists, or creates a new image with explicit `id`.

### DB.prototype.addImageFromURL(url[, callback(err, id)])

Add an image from http or https `url`.

### DB.prototype.setImageFromURL(id, url[, callback(err)])

Like `DB.prototype.addImageFromURL`, but replaces the image if `id` already exists, or creates a new image with explicit `id`.

### DB.prototype.addImageFromFileOrURL(filename|url[, callback(err, id)])

Add an image from `filename` or http(s) `url`. *Exists primarily to match the sync version which naturally have this combo of url/filename, because of the underlying ImageMagick API.*

### DB.prototype.addImageFromFileOrURLSync(filename|url) -> id

Add image from `filename` or http(s) `url`. Faster than the asynchronous version (direct I/O handled by ImageMagick), but is blocking.

### DB.prototype.setImageFromFileOrURLSync(id, filename|url)

Like `DB.prototype.addImageFromFileOrURLSync`, but replaces the image if `id` already exists, or creates a new image with explicit `id`.

### DB.prototype.addImageFromData(buffer[, offset[, length]]) -> id

Add image from `buffer` (a node Buffer instane), optionally starting at `offset` in buffer, optionally reading up to `length` bytes. `offset` defaults to `0` and `length` defaults to the length of `buffer`.

The `buffer` can contain any image data readable by ImageMagick.

### DB.prototype.setImageFromData(id, buffer[, offset[, length]])

Like `DB.prototype.addImageFromData`, but replaces the image if `id` already exists, or creates a new image with explicit `id`.

### DB.prototype.remove(id) -> bool

Remove image with `id` from the database. Returns true if `id` was in -- and removed from -- the database.

---

*Section: Querying images and reading image metadata*

### DB.prototype.contains(id) -> bool

Check if image with `id` is contained within the database.

### DB.prototype.getImageDimensions(id) -> {width:int, height:int}

Get dimensions of image with `id`. `width` and `height` is `0` (zero) if there is no image with `id`.

### DB.prototype.getImageAvgL(id) -> [y, i, q]

Get average luminance (number between [0-1]) for each component. Y is grey scale, I is the in-phase (blue/red position) and Q is the quadrature (lilac/green position). Learn more from the [YIQ article on Wikipedia](http://en.wikipedia.org/wiki/YIQ).

This is one of two sets of data used for image fingerprinting. When performing a "fast" query, it's only the average luminance which is used for comparison.

### DB.prototype.query(id[, limit=25[, fast=false[, keywords]]) -> [{id:int, score:float}, ..]

Return a list of image ids and scores for images similar to image denoted by `id`. Sorted descending on score.

- **limit** -- Return no more than `limit` number of ids.

- **fast** -- If `true`, only the average luminance is looked at. Otherwise the geometry and content of the image is also weigthed (default).

- **keywords** -- If an array of keywords is provided, only images associated with *any* of the keywords (keywords are `OR`-ed) will be considered. If `keywords` is `true`, only images associated with *any* of the keywords in the image denoted by `id` are considered.

### DB.prototype.queryByImageData(data[, limit=25[, fast=false[, sketch=false]]]) -> [{id:int, score:float}, ..]

Return a list of image ids and scores for images similar to image `data`, sorted descending on score. The `data` must be a node Buffer instance and can contain any image data readable by ImageMagick.

- **limit** -- Return no more than `limit` number of ids.

- **fast** -- If `true`, only the average luminance is looked at. Otherwise the geometry and content of the image is also weigthed (default).

- **sketch** -- If `true` a special set of HAAR weights are used, optimal for "sketched" images.


### DB.prototype.queryByImageFile(filename[, limit[, fast[, sketch]]], callback(err, [{id:int, score:float}, ..]))

Like `DB.prototype.queryByImageData` but reads the data from `filename` and returns the image ids and scores in the `callback`.

### DB.prototype.queryByImageURL(url[, limit[, fast[, sketch]]], callback(err, [{id:int, score:float}, ..]))

Like `DB.prototype.queryByImageData` but reads the data from `url` (http or https) and returns the image ids and scores in the `callback`.

### DB.prototype.queryByKeywords(keywords[, limit=25[, andJoin=false]]) -> [id, ..]

Return a list of image ids where each image contains one of or all of `keywords`.

- **limit** -- Return no more than `limit` number of ids.

- **andJoin** -- If `true` *all* of the keywords must exist in an image for it to be a match ("AND"-logic). If `false` only one of the keyword need to be associated with an image for it to be considered a match (default).

---

*Section: Dealing with keywords*

### DB.prototype.addKeyword(id, keyword) -> hash|false

Associate `keyword` (a string) with image `id`. Returns `false` if image `id` was already associated with the `keyword`, otherwise the int hash value of the keyword is returned.

### DB.prototype.addKeywordHash(id, hash) -> true|false

Associate keyword with `hash` (an integer) with image `id`. Returns `false` if image `id` already associated with the keyword, otherwise `true` is returned.

You can use `imgdb.keywordHash(string) -> int` to calculate the hash for a keyword or simply use `DB.prototype.addKeyword(id, keyword) -> hash|false` instead, which takes a string rather than a hash value.

### DB.prototype.addKeywords(id, keywords) -> [hash, ..]

Associate `keywords` (an array of strings) with image `id`. Returns an array of hash values for `keywords` (in the same order as `keywords` was input).

### DB.prototype.getKeywordHashes(id) -> [hash, ..]

Return a list of keyword hash values associated with image `id`.

**Note:** Since only the hash values of keywords are stored there is no way to retrieve the actual (original) string values of the keywords. This can however be modelled on top of imgdb, for instance by storing `hash=keyword` in an external database.

---

## License

TODO
