buzzedIn = false;


var exampleSocket = new WebSocket('ws://502a0f92.ngrok.io', 'echo-protocol');

exampleSocket.onopen = function (event) {
  //exampleSocket.send("connection with username "+username); 
};
exampleSocket.onmessage = function (event) {
//alert(event.data);
  if(event.data == "resetBuzzer"){
	  resetBuzzer();
  }
  
  
};

$(document).ready(function(){
	//$("#bodyDiv").css("width",(2 * window.innerWidth) + "px");
	//$(".smallDiv").css("width",(1 * window.innerWidth) + "px");
	buzzedIn = false;
  //$("#buzzer").click(function(){
	
  //});
});

function buzz(){
	
	if(!buzzedIn){
		
		
		exampleSocket.send("buzz:28023180:"+playerName);
		//setTimeout(function(){reset();},1000);
	}
}
function buzzIn(){
	$("#buzzer").css("backgroundColor","green");
	$("#buzzer").text("Buzzed in");
	$("title").text("You buzzed in!");
	
}
function resetBuzzer(){
	buzzedIn = false;
	$("#buzzer").css("backgroundColor","red");
	$("#buzzer").text("Buzz!");
	$("title").text("Buzzer");
}

$( window ).resize(function() {
	//$("#bodyDiv").css("width",(2 *window.innerWidth) + "px");
	//$(".smallDiv").css("width",(1 * window.innerWidth) + "px");
});