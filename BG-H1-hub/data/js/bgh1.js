var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

var canvas;
var ctx;
var toPlot = {d: 0, c: 0, data: []};

function onLoad(event) {
	canvas = document.getElementById('plot');
	ctx = canvas.getContext('2d');

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
	classdata["binary_sensor"].forEach(function (arrayItem) {
		$("#classlistBS").append($('<option>', {value:arrayItem}));
	});
	classdata["sensor"].forEach(function (arrayItem) {
		for (const property in arrayItem) {
			$("#classlistNS").append($('<option>', {value:property}));
		}
	});

	document.querySelectorAll("details").forEach(function (item) {
		item.addEventListener("click", handleClickOnDetails);
	});
	window.addEventListener('resize', resizeCanvas);
	resizeCanvas();
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
	$("#datetime").css('color', 'black');
}

function WSonClose(event) {
	//console.log('Connection closed');
	$("#datetime").css('color', 'red');
}

function WSonMessage(event) {
	var cmd = "";
//console.log(event.data);
	JSON.parse(event.data, (key, value) => {
//		console.log(cmd + ":" + key + '=' + value);
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
			if (cmd === "remove") {
				$(key+value).remove();
				$("#toast").fadeTo(100, 1);
				window.setTimeout(function() {$("#toast").fadeTo(500, 0)}, 2000);
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
				$(key).prop( "checked", value==="1"?true:false ).trigger('change');;
			} else
			if (cmd === "selected") {
				const opt = $("select"+key+" option:contains('"+value+"')");
				opt.prop( "selected", true ).trigger('change');;
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
			} else
			if (cmd === "plotdata") {
				toPlot.data = JSON.parse(value);
				toPlot.data.sort((a, b) => a.x - b.x);
				drawChart();
			}
		} else {
			if (cmd === "loadselect") {
				loadSelect();
			}
		}
	});
}

function pairingActive(elt) {
	$("#floatinfo").fadeTo(100, elt.checked?1:0);
	websocket.send("pairingactive;"+(elt.checked?"1":"0"));
}

function deldev(elt, adr) {
	if (confirm("Would you like to delete the Device ?") == true) {
		websocket.send("deldev;"+adr);
	}
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

function buttonMonitor(d,c) {
	websocket.send("buttonmon;"+d+";"+c);
}

function switchMonitor(elt, d, c) {
	websocket.send("switchmon;"+d+";"+c+";"+(elt.checked?"1":"0"));
}

function selectMonitor(elt, d, c) {
	websocket.send("selectmon;"+d+";"+c+";"+$(elt).find('option:selected').text());
}

function displayPlot(event, d, c) {
	toPlot.d = d;
	toPlot.c = c;
	toPlot.data = [];
	websocket.send("plot;"+d+";"+c);
	toggleModal(event);
}

function resizeCanvas() {
  let container = canvas.parentElement;
  canvas.width = container.clientWidth;
  canvas.height = container.clientHeight;
  drawChart();
}

function drawChart() {
	let container = canvas.parentElement;
	canvas.width = container.clientWidth;
	canvas.height = container.clientHeight;

	const width = canvas.width;
	const height = canvas.height;
	
	ctx.clearRect(0, 0, width, height);

	if (!toPlot.data || toPlot.data.length === 0) {
		ctx.fillStyle = '#666';
		ctx.font = '16px Arial';
		ctx.textAlign = 'center';
		ctx.fillText('No data', width/2, height/2);
		return;
	}

	const padding = Math.max(40, width * 0.08);
	const chartWidth = width - 2 * padding;
	const chartHeight = height - 2 * padding;

	const xValues = toPlot.data.map(d => d.x);
	const yValues = toPlot.data.map(d => d.y);
	let minX = Math.min(...xValues);
	let maxX = Math.max(...xValues);
	let minY = Math.min(...yValues);
	let maxY = Math.max(...yValues);

	let yRange = maxY - minY;
	if (toPlot.data.length === 1)
		yRange = minY;
	if (yRange === 0) {
		minY = 0;
		yRange = maxY;
	}
	const stp = Math.pow(10, Math.floor(Math.log10(yRange)));
	let yMin = Math.floor(minY / stp) * stp;
	let yMax = Math.ceil(maxY / stp) * stp;

	function toCanvasX(x) {
		if (toPlot.data.length > 1)
			return padding + ((x - minX) / (maxX - minX)) * chartWidth;
		else
			return padding;
	}

	function toCanvasY(y) {
		return height - padding - ((y - yMin) / (yMax - yMin)) * chartHeight;
	}

	const fontSize = Math.max(10, Math.min(14, width * 0.018));

	// border
	ctx.strokeStyle = '#333';
	ctx.lineWidth = Math.max(1.5, width * 0.002);
	ctx.beginPath();
	ctx.moveTo(padding, padding);
	ctx.lineTo(padding, height - padding);
	ctx.lineTo(width - padding, height - padding);
	ctx.stroke();

	const nbzoneY = 4;

	// title
	let origin = $("#mon_"+toPlot.d+"_"+toPlot.c);
	let title = origin.closest("div").children("header").text();
	let unit = origin[0].nextSibling.nodeValue.trim();
	ctx.fillStyle = '#333';
	ctx.font = `bold ${fontSize + 4}px Arial`;
	ctx.textAlign = 'center';
	ctx.fillText(title + " " + unit, width / 2, fontSize + 15);
	ctx.font = `${fontSize}px Arial`;
	
	if (toPlot.data.length > 1) {
		// horizontal lines
		ctx.strokeStyle = '#e0e0e0';
		ctx.lineWidth = Math.max(0.5, width * 0.001);
		for (let i = 0; i <= nbzoneY; i++) {
			const y = padding + (chartHeight / nbzoneY) * i;
			ctx.beginPath();
			ctx.moveTo(padding, y);
			ctx.lineTo(width - padding, y);
			ctx.stroke();
		}
		// Y label
		ctx.fillStyle = '#666';
		ctx.textAlign = 'right';
		for (let i = 0; i <= nbzoneY; i++) {
			const value = yMax - ((yMax - yMin) / nbzoneY) * i;
			const y = padding + (chartHeight / nbzoneY) * i;
			ctx.fillText(value.toFixed(1), padding - 10, y + 4);
		}
		// graph
		ctx.strokeStyle = '#007bff';
		ctx.lineWidth = Math.max(2, width * 0.003);
		ctx.beginPath();
		toPlot.data.forEach((point, i) => {
			const x = toCanvasX(point.x);
			const y = toCanvasY(point.y);
			if (i === 0) {
				ctx.moveTo(x, y);
			} else {
				ctx.lineTo(x, y);
			}
		});
		ctx.stroke();
		// points on graph
		const pointRadius = Math.max(3, width * 0.005);
		ctx.fillStyle = '#007bff';
		toPlot.data.forEach(point => {
			const x = toCanvasX(point.x);
			const y = toCanvasY(point.y);
			ctx.beginPath();
			ctx.arc(x, y, pointRadius, 0, Math.PI * 2);
			ctx.fill();
		});
	}
	// X label
	let label = new Date(toPlot.data[0].x*1000).toLocaleString(navigator.languages, { timeZone: 'UTC', month: "2-digit", day: "numeric", hour: "2-digit" });
	let x = toCanvasX(toPlot.data[0].x);

	ctx.textAlign = 'left';
	ctx.fillText(label, x, height - padding + fontSize + 8);
	let skip = x + Math.ceil(ctx.measureText(label).width*1.05);

	if (toPlot.data.length > 1) {
		ctx.textAlign = 'center';
		for (let i = 1; i < toPlot.data.length; i++) {
			label = new Date(toPlot.data[i].x*1000).toLocaleString(navigator.languages, { timeZone: 'UTC', hour: "2-digit" });
			const x = toCanvasX(toPlot.data[i].x);
			if (x > skip) {
				ctx.fillText(label, x, height - padding + fontSize + 8);
				skip = x + Math.ceil(ctx.measureText(label).width*1.05) / 2;
			}
		}
	}
	// Y max
	ctx.strokeStyle = '#20e020';
	ctx.lineWidth = Math.max(0.5, width * 0.001);
	ctx.setLineDash([10, 5]);
	ctx.beginPath();
	ctx.moveTo(padding, toCanvasY(maxY));
	ctx.lineTo(width - padding, toCanvasY(maxY));
	ctx.stroke();
	ctx.setLineDash([]);
	ctx.fillStyle = '#333';
	ctx.textAlign = 'left';
	ctx.fillText(maxY.toFixed(1), padding + 5, toCanvasY(maxY) + fontSize + 4);

}

var classdata = {
	"binary_sensor":["battery","battery_charging","carbon_monoxyde","cold","connectivity","door","garage_door","gas","heat","light","lock","moisture","motion","moving","occupency","opening","plug","power","presense","problem","running","safety","smoke","sound","tamper","update","vibration","window"],
	"sensor":[
	{"apparent_power":["VA"]},
	{"aqi":[]},
	{"atmospheric_pressure":["cbar","bar","hPA","inHg","kPa","mbar","Pa","psi"]},
	{"battery":["%"]},
	{"carbon_dioxyde":["ppm"]},
	{"carbon_monoxyde":["ppm"]},
	{"current":["A","mA"]},
	{"data_rate":["bit/s","kbit/s","Mbit/s","Gbit/s","B/s","kB/s","MB/s","GB/s"]},
	{"data_size":["bit","kbit","Mbit","Gbit","B","kB","MB","GB","TB"]},
	{"date":[]},
	{"distance":["km","m","cm","mm","mi","yd","in"]},
	{"duration":["d","h","min","sec"]},
	{"energy":["Wh","kWh","MWh"]},
	{"energy_storage":["Wh","kWh","MWh"]},
	{"frequency":["Hz","kHz","MHz","GHz"]},
	{"gas":["m³","ft³","CCF"]},
	{"humidity":["%"]},
	{"illuminance":["lx"]},
	{"irradiance":["W/m²","BTU/(h.ft²)"]},
	{"moisture":["%"]},
	{"monetary":["€","$"]},
	{"nitrogen_dioxyde":["µg/m³"]},
	{"nitrogen_monoxyde":["µg/m³"]},
	{"nitrous_oxyde":["µg/m³"]},
	{"ozone":["µg/m³"]},
	{"ph":[""]},
	{"pm1":["µg/m³"]},{"pm10":["µg/m³"]},{"pm25":["µg/m³"]},
	{"power":["W","kW"]},
	{"power_factor":["%"]},
	{"precipitation":["cm","in","mm"]},
	{"precipitation_intensity":["in/d","in/h","mm/d","mm/h"]},
	{"pressure":["Pa","kPa","hPa","bar","cbar","mbar","mmHg","inHg","psi"]},
	{"reactive_power":["var"]},
	{"signal_strength":["dB","dBm"]},
	{"sound_pressure":["dB","dBA"]},
	{"speed":["ft/s","in/d","in/h","km/h","kn","m/s","mph","mm/d"]},
	{"sulphur_dioxyde":["µg/m³"]},
	{"temperature":["°C","°F"]},
	{"timestamp":[]},
	{"volatile_organic_compounds":["µg/m³"]},
	{"volatile_organic_compounds_parts":["ppm","ppb"]},
	{"voltage":["V","mV"]},
	{"volume":["L","mL","gal","fl.oz.","m³","ft³","CCF"]},
	{"volume_flow_rate":["m³/h", "ft³/min", "L/min", "gal/min"]},
	{"volume_storage":["L","mL","gal","fl.oz.","m³","ft³","CCF"]},
	{"water":["L","gal","m³","ft³","CCF"]},
	{"weight":["kg","g","mg","µg","oz","lb","st"]},
	{"wind_speed":["ft/s","km/h","kn","m/s","mph"]},
]
};