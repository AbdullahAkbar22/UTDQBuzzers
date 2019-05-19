touchstartX = 0;
touchstartY = 0;
touchendX = 0;
touchendY = 0;
nextGo = "right";
midtransition = false;
transitionTime = 250;
document.addEventListener("keydown", function(event) {
	keyPressed  = event.keyCode;
	if(keyPressed == 39 && nextGo == "right" && !midtransition){
		
		hideLeftPanel();
	}
	else if(keyPressed == 37 && nextGo == "left" && !midtransition){
		
		hideRightPanel();
	}
	else if(keyPressed == 32 && nextGo == "right" && !midtransition){
		buzz();
	}
	
});

function falsifyTransition(){
	midtransition = false;
}
function hideLeftPanel(){
	nextGo = "left";
	midtransition = true;
	//$("#bodyDiv").animate({left: -1 * (window.innerWidth) + "px"}, transitionTime);
	$("#bodyDiv").animate({left: "-100%"}, transitionTime);
	$("#locationBar").animate({left: "50%"}, transitionTime)
	$("#buzzer").animate({left: "-100%"}, transitionTime, function(){midtransition = false});
	;
}
function hideRightPanel(){
	midtransition = true;
	nextGo = "right";
	$("#bodyDiv").animate({left: "0%"}, transitionTime);
	$("#locationBar").animate({left: "0%"}, transitionTime)
	$("#buzzer").animate({left: "50%"}, transitionTime, function(){midtransition = false});
}

document.addEventListener('touchstart', function(event) {
	
    touchstartX = event.changedTouches[0].screenX;
	//alert("touchStart: " + touchstartX);
    touchstartY = event.changedTouches[0].screenY;
}, false);

document.addEventListener('touchend', function(event) {
    touchendX = event.changedTouches[0].screenX;
	//alert("touchEnd: " + touchendX);
    touchendY = event.changedTouches[0].screenY;
    handleGesture();
}, false); 

function handleGesture() {
	documentWidth = window.innerWidth;
	triggerWidth = (documentWidth / 7);
	//alert("start: " + touchstartX + "and end: " + touchendX + " and " + triggerWidth);
    if (touchendX <= touchstartX && (Math.abs(touchstartX - touchendX) >= triggerWidth)) {
        hideLeftPanel();
    }
    
    if (touchendX >= touchstartX && (Math.abs(touchstartX - touchendX) >= triggerWidth)) {
        hideRightPanel();
    }
    
    
}