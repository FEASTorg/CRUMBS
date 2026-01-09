# TODO

## Upcoming Features / Tasks

- [ ] reserved addresses for standard functions? (think thru the use cases and implecations)

- [ ] chaining responses to enable larger data queries (would use reserved addresses maybe like a data response is always x00 or x01 and then we can chain x02, x03, etc for larger data sets up to a certain reserved limit like x0F or something and after that is ones we can use to attach to functions handler command callbacks). for example we can reserve like 0x01 as the command id for "data response" and then the next byte is the "data chunk id" so if we have a large data set to return we can break it up into chunks and send them one at a time with the chunk id incrementing each time until all data is sent. this would require some way to signal the end of the data set as well.

- [ ] do full review of library to check over and see if its ready to increment version to 0.10.0

## Next Phase Tasks

- [ ] gateway service to convert controller function handlers to web api calls (think thru the best way to do this, maybe a simple rest api that maps to function handlers on the controller side? get work from the prelim planning to flesh this out more)

## Post Dependent Tasks

- [ ] update dcmt and rlht firmwares to use latest crumbs [thru platformio] once mvp is done

## Backlog

- [ ] Back fill the CHANGELOG to the best of our ability to at least display the progression of the project with rough version numbers and dates.
