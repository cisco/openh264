UIATarget.onAlert = function onAlert(alert){

    UIALogger.logMessage("In Alert!");
	title = alert.name();
	if (title && title.indexOf("Microphone") !== -1) {
		UIALogger.logMessage("Alert with title '" + title + "' encountered!");
		var buttons = alert.buttons();
		var buttonCount = buttons.length;

		if (buttonCount > 0) {
			var acceptButton = buttons[buttonCount - 1];
			acceptButton.tap(); // last button is accept
		}
	return true; //forbid the default cancel processing
    }
    return false; //using the default cancel processing
}
var target = UIATarget.localTarget();
target.delay(6000);
