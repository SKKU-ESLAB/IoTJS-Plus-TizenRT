var fs = require('fs');
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
    var resBody = "";
    var errorMsg = "";
    if (req.method == 'GET') {
      var isHtml = false;
      var filePath = undefined;
      if (url.indexOf(".html") >= 0 || url.indexOf(".htm") >= 0) {
        isHtml = true;
      }
      if (url == "/") {
        // index.html
        filePath = "/rom/index.html";
        isHtml = true;
      } else if (isHtml) {
        filePath = "/rom" + url; // html in rom
      } else {
        filePath = "/mnt" + url; // others in mnt
      }
      if (!fs.existsSync(filePath)) {
        res.writeHead(404);
      } else {
        resBody = fs.readFileSync(filePath).toString();
        var host = req.headers.Host;
        if (isHtml && (host !== undefined)) {
          resBody = resBody.replace(/IPADDR/gi, host);
        }
        res.writeHead(200, {
          'Content-Length': resBody.length
        });
      }
    } else if(req.method == 'POST') {
      if(url.indexOf(".js" >= 0)) {
        var filePath = "/mnt" + url;
        fs.writeFileSync(filePOath, reqBody);
        console.log("Write to " + filePath + " success!");
      }
    } else {
      res.writeHead(403);
    }

    if (url == "/close.html") {
      isCloseServer = true;
    }

    res.write(resBody);
    res.end(function () {
      if (isCloseServer) {
        console.log("close the server");
        server.close();
      }
    });
  };

  req.on('end', endHandler);
}

function app_main() {
  server = http.createServer(http_handler);
  server.listen(httpServerPort, function () {
    console.log("Log download server starts: port=" + httpServerPort);
  });
}
app_main();
