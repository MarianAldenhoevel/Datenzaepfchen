// JQuery handler for document ready. Wire controls and fetch the first number.
$(document).ready(function() {
	console.log("ready()");
	
	// Setup file uploader
	// http://plugins.krajee.com/file-input
	$("#upload").fileinput({
		uploadUrl: "/upload",
		language: "de",
		theme: "explorer",
		showCaption: true,
		showPreview: true,
		showCancel: true
	});
});