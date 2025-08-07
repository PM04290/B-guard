var gateway = `ws://${window.location.hostname}/ws`;
var websocket;
var client;

function onLoad(event) {
	initWebSocket();
	$('#formwifi').submit(function( event ) {
		event.preventDefault();
		var posting = $.post(event.currentTarget.action, $(this).serialize() );
		posting.done(function( data ) {
			$("#resultwifi").fadeTo(100, 1);
			window.setTimeout(function() {$("#resultwifi").fadeTo(500, 0)}, 2000);
			//console.log(data);
		});
	});

	document.querySelectorAll("details").forEach(function (item) {
		item.addEventListener("click", handleClickOnDetails);
	});
}

function loadSelect() {

  $('input[type="number"][id^="mon_"]').each(function () {
    const $input = $(this);
    $input.on('input', function () {
      const id = $input.attr('id');
      clearTimeout(debounceDelays[id]);
      debounceDelays[id] = setTimeout(() => handleFinalChange($input), 2000);
    });
    $input.on('blur', function () {
      const id = $input.attr('id');
      clearTimeout(debounceDelays[id]);
      handleFinalChange($input);
    });
  });
}

const debounceDelays = {};
function handleFinalChange($input) {
	const value = $input.val();
	const id = $input.attr('id');
	console.log("input change", id, ":", value);
	const [m,d,c] = id.split("_");
	websocket.send("inputmon;"+d+";"+c+";"+value);
}

function handleClickOnDetails() {
  // close all details
  let detailsOpened = document.querySelectorAll("details[open]");

  for (const item of detailsOpened) {
    // keep open only details clicked
    if (this != item) {
      item.removeAttribute("open");
    }
  }
}

function initWebSocket() {
	//console.log('Trying to open a WebSocket connection...');
	websocket = new WebSocket(gateway);
	websocket.onopen    = WSonOpen;
	websocket.onclose   = WSonClose;
	websocket.onmessage = WSonMessage;
}

function WSonOpen(event) {
	//console.log('Connection opened');
}

function WSonClose(event) {
	//console.log('Connection closed');
}

function WSonMessage(event) {
	var cmd = "";
//console.log(event.data);
	JSON.parse(event.data, (key, value) => {
		console.log(cmd + ":" + key + '=' + value);
		if (key === "cmd") {
			cmd = value;
		} else
		if (key !== "" && typeof value !== 'object') {
			if (cmd === "html") {
				$(key).html( value );
			} else
			if (cmd === "replace") {
				$(key).replaceWith( value );
			} else
			if (cmd === "value") {
				const $obj = $(key);
				if ($obj[0].nodeName === 'INPUT') {
					$(key).val( value ).change();
				} else {
					$(key).html( value ).change();
				}
			} else
			if (cmd === "checked") {
				$(key).prop( "checked", value ).trigger('change');;
			} else
			if (cmd === "log") {
				console.log( '[' + new Date().toLocaleTimeString('fr-FR', { hour12: false }) + '] ' + value );
			} else
			if (cmd === "formnotify") {
				$('#conf_dev > mark:first').remove();
				if ($("#"+key).length === 0)
				{
					$("#conf_dev").append( value );
					loadSelect();
				}
			} else
			if (cmd === "childnotify") {
				if ($("#"+key).length)
				  $("#"+key).html(value);
				else {
					let arr = key.split(/[_]+/);
					let d = parseInt(arr[2]);
					$("<div id='"+key+"' class='row'>"+value+"<div>").insertBefore("#footer_"+(d));
				}
				loadSelect();
			} else
			if (cmd === "monitornotify") {
				if ($("#"+key).length === 0)
				{
					$("#monitor_dev").append( value );
					loadSelect();
				}
			} else
			if (cmd === "monitornotifychild") {
				if ($("#"+key).length > 0)
				{
					$("#"+key).append( value );
					loadSelect();
				}
			}
		} else {
			if (cmd === "loadselect") {
				loadSelect();
			}
		}
	});
}

function changeOut1(elt) {
	websocket.send("out1;"+(elt.checked?"1":"0"));
}

function sendPairing() {
	websocket.send("pairing");
}

function pairingActive(elt) {
	$("#floatinfo").fadeTo(100, elt.checked?1:0);
	websocket.send("pairingactive;"+(elt.checked?"1":"0"));
}

function changeAdr(oldadr) {
	let newadr = $("input[id='newadr_"+oldadr+"']").val();
	if (newadr === "") {
		alert("Address required");
		return;
	}
	$("#conf_dev").children().each(function () {
		let adr = $(this).attr('id').split('_')[1];
        if (adr == newadr) {
			alert("This address is already used");
			return;
		}
	});
	websocket.send("newadr;"+oldadr+";"+newadr);
}

function endPairing(d) {
	websocket.send("endpairing;"+d);
}

function postdev(event) {
	event.preventDefault();
	var posting = $.post(event.currentTarget.action, $(event.currentTarget).serialize() );
	posting.done(function( data ) {
		$("#toast").fadeTo(100, 1);
		window.setTimeout(function() {$("#toast").fadeTo(500, 0)}, 2000);
		//console.log(data);
	});
}

function btnMonitor(d,c) {
	websocket.send("btnmon;"+d+";"+c);
}