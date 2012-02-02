var lfmprint = require("lfmprint");
var lfm = new lfmprint.lfmprint();

function callback(err, fp) {
  if (err) 
     console.error(err);
  else {
    console.log(fp);
    
  }
}

lfm.fetchFingerprint("1.mp3", callback)
