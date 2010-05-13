# imgdb

ImgDB addon

## Building & installation

    node-waf configure
    node-waf build
    sudo node-waf install

# API

## imgdb

### loadSync(filename)

Load all databases in `filename` in a synchronous manner.

### saveSync(filename)

Save all databases to `filename` in a synchronous manner.

### loadedDbIds() -> Array

List loaded database ids (an array of integers).


## imgdb.DB

### new DB(Number id)

New database object with database identifier `id`.

### DB.prototype.loadSync(filename)

Load database from `filename`.

### DB.prototype.saveSync(filename)

Save database to `filename`.

### DB.prototype.reset()

Reset the database, effectively removing everything.

> **Warning:** Calling this method removes all data in the current database.




## License

TODO
