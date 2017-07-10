// JQuery handler for document ready. Wire controls and fetch the first number.
$(document).ready(function() {
	console.log("ready()");
	
	$("#btnOk").click(function(ev) {
		location.href = "/setup.html";
	});
});
