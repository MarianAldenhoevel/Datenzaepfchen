// *********************************************************
// Node.js-powered mockup of the Datenzaepfchen webapp
// *********************************************************

var http_port = 80;

var package_json = require("./package.json");
var morgan = require("morgan");
var express = require("express");
var body_parser = require("body-parser");
var serve_static = require("serve-static")                                           
var http = require("http");
var path = require("path");
var formidable = require("express-formidable");
var fs = require("fs");

var app = express();

var oneYear = 31557600000;
app.use(serve_static(path.join(__dirname, "../Datenzaepfchen/data"), { maxAge: oneYear }));

app.use(body_parser.json());
app.use(body_parser.urlencoded({ extended: true }));

app.use(morgan("dev"));

app.use(formidable({
  encoding: "utf-8",
  uploadDir: path.join(__dirname, "upload"),
  keepExtensions: true,
  multiples: true, // req.files to be arrays of files 
}));

app.get("/", function(req, res) {
    res.redirect("index.html");
});

var server = null;

app.use(function (req, res, next) {	
    var filename = path.basename(req.url);
    var extension = path.extname(filename);
    if (extension === '.css') {
        console.log("The file " + filename + " was requested.");
	}
    next();
});

app.post('/upload', (req, res) => {
  // console.log(req.files);
  
  for (var prop in req.files) {
    if (req.files.hasOwnProperty(prop)) {
        file = req.files[prop];
		// console.log(file);
		
		var src = file.path;
		var dest = path.join(__dirname, "upload", file.name);
	
		console.log("uploaded: " + dest);
	
		fs.rename(src, dest, function(err) {
			if (err) {
				console.log('ERROR: ' + err);
				res.json(err);
			}
		});
	}
  }
  
  /*
  var result = {
	"error": "Just kidding",
	"initialPreview": [],
	"initialPreviewConfig": [],
	"initialPreviewThumbTags": [],
	"append": true
  }
  */
  
  res.json({});
});

server = http.createServer(app).listen(http_port, function () {
	console.log("");
	console.log("Datenzaepfchen http server listening on port " + http_port + ".");
});
