// JQuery handler for document ready. Wire controls and fetch the first number.
$(document).ready(function() {
	console.log("ready()");
	
	$("#btnLicense").click(function(ev) {
		location.href = "/license.html";
	});
	
	$("#btnUpload").click(function(ev) {
		location.href = "/upload.html";
	});
});