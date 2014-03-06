var BTCMD_GetVersion = {
	reqCode: 'G',
	reqIndex: false,
	reqParams: [],
	rspCode: 'G',
	rspParams: [
		"responseCode",
		"btVersion",
		"build",
		"protocol",
		"schema",
		"metric"
	]
};

var BTCMD_SetAlarm = {
	reqCode: 'V',
	reqIndex: false,
	reqParams: ["alarmStatus"],
	rspCode: 'e',
	rspParams: [
		"responseCode",
		"alarmStatus"
	]
};

var BTCMD_GetProgram = {
	reqCode: '@',
	reqIndex: true,
	reqParams: [],
	rspCode: '@',
	rspParams: [
		"responseCode",
		"name",
		"batchVolume",
		"grainWeight",
		"mashRatio",
		"mashDoughIn_Temperature",
		"mashDoughIn_Minutes",
		"mashAcid_Temperature",
		"mashAcid_Minutes",
		"mashProtein_Temperature",
		"mashProtein_Minutes",
		"mashSacch_Temperature",
		"mashSacch_Minutes",
		"mashSacch2_Temperature",
		"mashSacch2_Minutes",
		"mashMashOut_Temperature",
		"mashMashOut_Minutes",
		"spargeTemperature",
		"hltTemperature",
		"boilMinutes",
		"pitchTemperature",
		"boilAdditions",
		"strikeHeatSource",
		"calcStrikeTemperature",
		"firstStepTemperature",
		"calcPreboilVolume",
		"calcStrikeVolume",
		"calcSpargeVolume",
		"calcGrainVolume",
		"calcGrainLiquorLoss"
	]
};

var BTCMD_GetStatus = {
	reqCode: 'a',
	reqIndex: false,
	reqParams: [],
	rspCode: 'a',
	rspParams: [
		"responseCode",
		"alarmStatus",
		"autoValveStatus",
		"profileStatus",
		"outputStatus",
		"HLT_Setpoint",
		"HLT_Temperature",
		"HLT_HeatPower",
		"HLT_TargetVolume",
		"HLT_Volume",
		"HLT_FlowRate",
		"Mash_Setpoint",
		"Mash_Temperature",
		"Mash_HeatPower",
		"Mash_TargetVolume",
		"Mash_Volume",
		"Mash_FlowRate",
		"Kettle_Setpoint",
		"Kettle_Temperature",
		"Kettle_HeatPower",
		"Kettle_TargetVolume",
		"Kettle_Volume",
		"Kettle_FlowRate",
		"Mash_TimerValue",
		"Mash_TimerStatus",
		"Boil_TimerValue",
		"Boil_TimerStatus",
		"Boil_ControlState",
		"ProgramThread1_Step",
		"ProgramThread1_Recipe",
		"ProgramThread2_Step",
		"ProgramThread2_Recipe",
	]
};


function brewTrollerExecCommand(cmdObj, index, params, host, user, pwd, callback){
	var command = cmdObj.reqCode;
	if (cmdObj.reqIndex) {
		command += index;
	}
	command += brewtrollerParseRequestParameters(cmdObj, params);
	$.ajax({
		url: "http://" + host + "/btnic.cgi",
		beforeSend: function (xhr) {
           if(user != null) {
                xhr.withCredentials = true;
                var tok = user + ':' + password;
                var hash = btoa(tok);
                xhr.setRequestHeader("Authorization", "Basic " + hash);
           }
		},
		data: command,
		dataType: "json",
		success:function(result){
			var object = brewTrollerParseResponse(result, cmdObj);
			callback(object);
		}
	});
}

function brewtrollerParseRequestParameters(cmdObj, params) {
	var parameters = [];
	if (cmdObj.reqParams.length) {
		for (var i = 0; i < cmdObj.reqParams.length; i++) {
			parameters[i] = params[cmdObj.reqParams[i]];
		}
	}
	return (parameters.length ? "&" + parameters.join('&') : "");
}

function brewTrollerParseResponse(result, cmdObject)
{
	if (result[0][0] != cmdObject.rspCode) {
		var errorText;
		switch (result[0]) {
			case '!':
				errorText = "Bad Command";
				break;
			case '#':
				errorText = "Bad Parameter";
				break;
			case '$':
				errorText = "Bad Index";
				break;
			default:
				errorText = "Unknown response: " + result[0];
				break;
		}
		throw new Error("BrewTroller response error: " + errorText);
	}
	var returnObject;
	returnObject = {};
	for (var i = 0; i < cmdObject.rspParams.length; i++){
		returnObject[cmdObject.rspParams[i]] = result[i];
	}
	return returnObject;
};
