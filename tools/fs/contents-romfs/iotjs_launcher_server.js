var fs = require('fs');
var Buffer = require('buffer');
var http = require('http');
var httpServerPort = 80;
var Server = require('http_server').Server;
var server = undefined;


function http_handler(req, res) {
  var reqBody = '';
  var url = req.url;

  req.on('data', function(chunk) {
    reqBody += chunk;
  });

  var endHandler = function() {
    var isCloseServer = false;
    var resBody = '';
    var errorMsg = '';
    if (req.method == 'GET') {
      var isHtml = false;
      var filePath = undefined;
      if (url.indexOf('.html') >= 0 || url.indexOf('.htm') >= 0) {
        isHtml = true;
      }
      if (url == '/') {
        // index.html
        filePath = '/rom/index.html';
        isHtml = true;
      } else if (url == '/deleteAll') {
        if (fs.existsSync('/mnt/total_size.log')) fs.unlinkSync('/mnt/total_size.log');
        if (fs.existsSync('/mnt/segment_utilization.log')) fs.unlinkSync('/mnt/segment_utilization.log');
        if (fs.existsSync('/mnt/time.log')) fs.unlinkSync('/mnt/time.log');
        if (fs.existsSync('/mnt/object_lifespan.log')) fs.unlinkSync('/mnt/object_lifespan.log');
        if (fs.existsSync('/mnt/object_allocation.log')) fs.unlinkSync('/mnt/object_allocation.log');
        resBody = 'Delete all the logs successfully!';
        res.writeHead(200, {'Content-Length': resBody.length});
      } else if (isHtml) {
        filePath = '/rom' + url;  // html in rom
      } else {
        filePath = '/mnt' + url;  // others in mnt
      }
      if (!fs.existsSync(filePath)) {
        resBody = 'Not found file: ' + filePath;
        res.writeHead(404, {'Content-Length': resBody.length});
      } else {
        resBody = fs.readFileSync(filePath).toString();
        var host = req.headers.Host;
        if (isHtml && (host !== undefined)) {
          resBody = resBody.replace(/IPADDR/gi, host);
        }
        res.writeHead(200, {'Content-Length': resBody.length});
      }
    } else if (req.method == 'POST') {
      var filePath = '/mnt/index.js';
      var fd = fs.openSync(filePath, 'w');

      var parsingState = 'ContentBody';
      var reqBodyLines = reqBody.split(/\r?\n/);
      for (var i in reqBodyLines) {
        var line = reqBodyLines[i];
        // console.log("[" + parsingState + "] " + line);
        if (parsingState == 'ContentBody') {
          if (line.indexOf('------') >= 0) {
            parsingState = 'ContentHeader';
          }
        } else if (parsingState == 'ContentHeader') {
          if (line.indexOf('Content-Disposition') >= 0 && line.indexOf('filename') >= 0) {
            parsingState = 'JSHeader';
          } else if (line.length == 0) {
            parsingState = 'ContentBody';
          }
        } else if (parsingState == 'JSHeader') {
          if (line.length == 0) {
            parsingState = 'JSBody';
          }
        } else if (parsingState == 'JSBody') {
          if (line.indexOf('------') >= 0) {
            parsingState = 'ContentHeader';
          } else {
            // console.log("Write: " + line);
            line += '\n';
            var lineBuffer = new Buffer(line);
            fs.writeSync(fd, lineBuffer, 0, lineBuffer.length);
          }
        }
      }
      fs.closeSync(fd);

      resBody = fs.readFileSync('/rom/install_finished.html').toString();
      resBody = resBody.replace(/IPADDR/gi, host);
      res.writeHead(200, {'Content-Length': resBody.length});
    } else {
      resBody = 'Not allowed URL: ' + filePath;
      res.writeHead(403, {'Content-Length': resBody.length});
    }

    if (url == '/close.html') {
      isCloseServer = true;
    }

    if (resBody.length > 0) {
      res.write(resBody);
    }
    res.end(function() {
      if (isCloseServer) {
        console.log('close the server... reboot this device...');
        server.close();
        console.reboot();
      }
    });
  };

  req.on('end', endHandler);
}

function app_main() {
  server = http.createServer(http_handler);
  server.listen(httpServerPort, function() {
    console.log('\n\nIoT.js Launcher: installer server starts: port=' + httpServerPort);
  });
}
app_main();