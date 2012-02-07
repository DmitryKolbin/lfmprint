var lfm = new (require("./lfmprint.node")).lfmprint()
var request = require("request")

fetchFingerprint = function (fileName, callback) {
  lfm.fetchFingerprint(fileName, callback)
}


getInfo = function (fingerprint, duration, lfmkey, callback) {
  var binBuffer = new Buffer(fingerprint.length + 71)
  var i = binBuffer.write('--bound\r\nContent-Disposition: form-data; name=\"fpdata\"\r\n\r\n', 0, 'ascii')
  fingerprint.copy(binBuffer, i)
  i = i + fingerprint.length
  i = i + binBuffer.write('\r\n--bound--\r\n', i, 'ascii')
  request( 
    { method: "POST"
      , uri:"http://ws.audioscrobbler.com/fingerprint/query/?duration=" + duration
      , headers : {'Content-type': 'multipart/form-data; Boundary=bound', 'Accept': '*/*'}
      , body: binBuffer
      , jar : false 
    }  
    , function(err, response, body) {
      if (body.match("FOUND")) {
        var fingerprintid = 1
        request("http://ws.audioscrobbler.com/2.0/?method=track.getfingerprintmetadata&fingerprintid=" + 
              fingerprintid + "&api_key=" + lfmkey + "&format=json", function (err, response, body) {
          if (!err && response.statusCode == 200)
              callback(null, body)
          else
             callback("Error:" + response.statusCode + body, null)
        });
      } else {
        callback("metadata doesn't exist for this file")
      }
    });
}


exports.fetchFingerprint = fetchFingerprint
exports.getInfo = getInfo
