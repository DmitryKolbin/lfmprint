var lfm = require("lfmprint");

function callback(err, fp, duration) {
  if (err) 
     console.error(err);
  else {
    lfm.getInfo(fp, duration, "2bfed60da64b96c16ea77adbf5fe1a82", function(err, metadata) {
      if (err) {
        console.error(err)
      } else {
        console.log(metadata)
      }
    })
  }
}

lfm.fetchFingerprint("1.mp3", callback)
