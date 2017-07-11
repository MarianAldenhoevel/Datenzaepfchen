var doneFiles = null;

// JQuery handler for document ready. Wire controls and fetch the first number.
$(document).ready(function() {
	console.log("ready()");
	
	$("#btnDownload").click(function(ev) {
		location.href = "/";
	});
	
	// Setup file uploader
	// http://plugins.krajee.com/file-input
	var input = $("#upload");
	input.fileinput({
		uploadUrl: "/upload",
		language: "de",
		theme: "explorer",
		uploadAsync: false,
		showCaption: true,
		showPreview: true,
		showCancel: true,
		showUpload: false,
		showRemove: false,
		minFileCount: 1,
		maxFileCount: 5
	}).on("filebatchselected", function(event, files) {
		console.log("onFilebatchselected");
		
		// get a list of all successfully uploaded files from previous batch.
		doneFiles = input.fileinput("getFrames", ".file-preview-success");
	
		// trigger new uploads.
		input.fileinput("upload");
	}).on("filebatchuploadsuccess", function(event, data, previewId, index) {
        // This event is triggered only for ajax uploads and after a successful synchronous batch upload. 
		console.log("onFilebatchuploadsuccess");
		
		// Hide and remove all the files from the last but one batch. Leaving the
		// ones from the current batch.
		if (doneFiles) {
			doneFiles.each(function() {
				$(this).fadeOut("slow", function() { 
					$(this).remove();
				});
			});
			doneFiles = null;
		};
	});
});