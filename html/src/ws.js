"use strict";

// class WsClient
var WsClient = function()
{
	var ws;
	var url = "ws://127.0.0.1";
	var connected = false;

	this.connect = function ()
	{
		if ("WebSocket" in window)
			ws = new WebSocket(url);
		else if ("MozWebSocket" in window)
			ws = new MozWebSocket(url);

		ws.onopen = function(e) { connected = true; doConnect.value="已連線"; doConnect.disabled = true; };
		ws.onerror = function(e) { alert("連線發生錯誤! 請確認伺服器程式(sailing.exe)已開啟"); console.log(e); };
		ws.onclose = function(e) { connected = false; doConnect.value="重新連線"; doConnect.disabled = false; alert("伺服器已斷線! 請重新連線"); console.log(e.code+(e.reason != "" ? ","+e.reason : ""));};

		ws.onmessage = function(e)
		{
			console.log(e.data);
			var cmd = e.data.split('=');
			if (cmd.length == 2)
				switch (cmd[0].trim())
				{
					case "startdate": // TODO
						break;
					case "orig":
						orig.setLatLng(cmd[1].match(/\S+/g));
						break;
					case "dest":
						dest.setLatLng(cmd[1].match(/\S+/g));
						break;
				}
			else
			{
				cmd = e.data.split(' ');
				switch (cmd[0].trim())
				{
					case "kml":
						omnivore.kml(cmd[1].trim()).addTo(map);
						break;
					case "json":
						var v = JSON.parse(e.data.slice(e.data.indexOf(cmd[0]) + cmd[0].length));

						// Check if new voyage
						var newv = true;
						for (var i=0; i<voyage.length; i++) if (voyage[i].name == v.name) newv = false;
						if (newv)
						{
							voyage.push(v);
							parseVoyage(voyage[voyage.length-1]);
						} else alert("此參數設定已計算過!");
						break;
				}
			}
		};
	};

	this.disconnect = function() { ws.close(); };
	this.send = function(msg) { ws.send(msg); };
}

function parseVoyage(v)
{
	v.layerGroup = L.layerGroup()
	v.circleMarker = [];
	var points = [];
	for (var i = 0; i < v.path.length; i++)
	{
		points.push(v.path[i].curr);
		v.circleMarker.push(L.circleMarker(points[i], {radius: 2, color: "red", fillOpacity: 0.9, stroke: false}).addTo(map));
		v.layerGroup.addLayer(v.circleMarker[i]);
	}
	v.polyline = L.polyline(points, {color: "red", lineJoin:"round"})
	v.layerGroup.addLayer(v.polyline);
	v.layerGroup.addTo(map);
	outputlist.innerHTML +=
	"<div class='outputitem'>" +
	"<input type='checkbox'>顯示 <input type='button' value='刪除'> </br>" +
	"起點: " + v.orig.lat + ", " + v.orig.lng + "</br>" +
	"資料: " + v.mode + "</br>" +
	"時間: " + 
	v.startdate.year + "/" + (v.startdate.month<10?"0":"") + v.startdate.month + "/" + (v.startdate.day<10?"0":"") + v.startdate.day + " " + v.startdate.hour + "hr" +
	" - " +
	v.enddate.year + "/" + (v.enddate.month<10?"0":"") + v.enddate.month + "/" + (v.enddate.day<10?"0":"") + v.enddate.day + " " + v.enddate.hour + "hr" +
	"</br>" +
	"天數: " + v.days + "</br>" +
	"模式: " + v.mode + "</br>" +
	"風帆高度: " + v.altitude + "m</br>" +
	"張帆極限風速: " + v.windlimit + "m/s</br>" +
	"每日張帆時數: " + v.sailopenhours + "hr</br>" +
	//v.name +
	"</div>";
}
