var fs = require('fs');
var Buffer = require('buffer');
var http = require('http');
var httpServerPort = 80;
var Server = require('http_server').Server;
var server = undefined;


function http_handler(req, res) {
  var reqBody = '';
  var url = req.url;

  req.on('data', function (chunk) {
    reqBody += chunk;
  });

  var endHandler = function () {
    var isCloseServer = false;
    var resBody = '';
    var errorMsg = '';
    var message = "";
    var messageResCode = 200;
    if (req.method == 'GET') {
      if (url.indexOf('/command/') >= 0) {

        if (url == '/command/deleteAll') {
          if (fs.existsSync('/mnt/total_size.log')) fs.unlinkSync('/mnt/total_size.log');
          if (fs.existsSync('/mnt/segment_utilization.log'))
            fs.unlinkSync('/mnt/segment_utilization.log');
          if (fs.existsSync('/mnt/time.log')) fs.unlinkSync('/mnt/time.log');
          if (fs.existsSync('/mnt/object_lifespan.log')) fs.unlinkSync('/mnt/object_lifespan.log');
          if (fs.existsSync('/mnt/object_allocation.log'))
            fs.unlinkSync('/mnt/object_allocation.log');
          message = "Delete all the logs successfully!";
        } else if (url == '/command/reboot') {
          isCloseServer = true;
          message = "Reboot!";
        }
      } else {
        var isHtml = false;
        var filePath = undefined;
        if (url.indexOf('.html') >= 0 || url.indexOf('.htm') >= 0) {
          isHtml = true;
        }
        if (url == '/') {
          // index.html
          filePath = '/rom/index.html';
          isHtml = true;
        } else if (isHtml) {
          filePath = '/rom' + url;  // html in rom
        } else {
          filePath = '/mnt' + url;  // others in mnt
        }
        if (!fs.existsSync(filePath)) {
          message = "Not found file: " + filePath;
          messageResCode = 404;
        } else {
          resBody = fs.readFileSync(filePath).toString();
          var host = req.headers.Host;
          if (isHtml && (host !== undefined)) {
            resBody = resBody.replace(/IPADDR/gi, host);
          }
          res.writeHead(200, { 'Content-Length': resBody.length });
        }
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
      message = "Install finished!";
    } else {
      message = "Not allowed URL: " + filePath;
      messageResCode = 403;
    }

    if (message.length > 0) {
      resBody = fs.readFileSync('/rom/message.html').toString();
      resBody = resBody.replace(/MESSAGE/gi, message);
      res.writeHead(messageResCode, { 'Content-Length': resBody.length });
      console.log(message);
    }

    if (resBody.length > 0) {
      res.write(resBody);
    }
    res.end(function () {
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
  server.listen(httpServerPort, function () {
    console.log('\n\nIoT.js Launcher: installer server starts: port=' + httpServerPort);
  });
}
app_main();