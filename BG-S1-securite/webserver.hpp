#pragma once
#include <HTTPClient.h>  // native library
#include <ESPmDNS.h>     // native library
#include <Update.h>      // native library
#include "include_html.hpp"  // inside file

enum WMode {
  Mode_WifiSTA = 0,
  Mode_WifiAP,
  Mode_Ethernet
};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
size_t content_len;

String getHTML_DeviceHeader(uint8_t idx)
{
  String blocD = html_device_header;
  CDevice* dev = Router.getDeviceByAddress(idx);
  blocD.replace("#D#", String(idx));
  if (dev)
  {
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%CNFNAME%", String(dev->getName()));
    blocD.replace("%CNFMODEL%", String(dev->getModel()));
  } else {
    blocD.replace("%CNFADDRESS%", "");
    blocD.replace("%CNFNAME%", "");
    blocD.replace("%CNFMODEL%", "");
  }
  //
  return blocD;
}

String getHTML_DevicePairing(uint8_t idx)
{
  String blocD = "";
  CDevice* dev = Router.getDeviceByAddress(idx);
  if (dev && dev->getPairing())
  {
    int newIdx = idx + 1;
    CDevice* newdev = Router.getDeviceByAddress(newIdx);
    while (newdev) {
      newIdx++;
      newdev = Router.getDeviceByAddress(newIdx);
    }
    blocD = html_device_pairing;
    blocD.replace("#D#", String(idx));
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%CNFNEWADR%", String(newIdx));
    //
  }
  //
  return blocD;
}

String getHTML_DeviceForm(uint8_t id)
{
  String blocD = html_device_form;
  CDevice* dev = Router.getDeviceByAddress(id);
  blocD.replace("#D#", String(id));
  if (dev)
  {
    blocD.replace("%CNFMODEL%", dev->getModel());
    blocD.replace("%CNFNAME%", dev->getName());
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%GENHEADER%", getHTML_DeviceHeader(id));
    blocD.replace("%GENPAIRING%", getHTML_DevicePairing(id));
  }
  return blocD;
}

String getHTMLforChildLine(uint8_t address, uint8_t id)
{
  String blocC = html_child_line;
  String kcnf;
  CDevice* dev = Router.getDeviceByAddress(address);
  if (dev)
  {
    CEntity* ent = dev->getEntityById(id);
    blocC.replace("#D#", String(address));
    blocC.replace("#C#", String(id));
    if (ent)
    {
      blocC.replace("%CNFC_ID%", String(ent->getId()));
      blocC.replace("%CNFC_DTYPE%", String(ent->getDataType()));
      blocC.replace("%CNFC_LABEL%", String(ent->getLabel()));
      blocC.replace("%CNFC_LABELVALID%", strlen(ent->getLabel()) == 0 ? "aria-invalid='true'" : String(ent->getLabel()));
      const char* elementtype[] = {"Binary sensor", "Numeric sensor", "Switch", "Light", "Cover", "Fan", "HVac", "Select", "Trigger", "Event", "Tag", "Text", "Input", "Custom", "Date", "Time", "Datetime", "Button"};
      blocC.replace("%CNFC_STYPE_INT%", String((int)ent->getElementType()));
      blocC.replace("%CNFC_STYPE_STR%", elementtype[(int)ent->getElementType()]);
      switch (ent->getElementType())
      {
        case E_BINARYSENSOR:
          blocC.replace("%CNFC_UNIT%", "");
          blocC.replace("%CNFC_OPT%", "");
          blocC.replace("%CNFC_IMIN%", "");
          blocC.replace("%CNFC_IMAX%", "");
          blocC.replace("%CNFC_IDIV%", "");
          break;
        case E_NUMERICSENSOR:
          blocC.replace("%CNFC_UNIT%", String(ent->getUnit()));
          blocC.replace("%CNFC_OPT%", "");
          blocC.replace("%CNFC_IMIN%", "");
          blocC.replace("%CNFC_IMAX%", "");
          blocC.replace("%CNFC_IDIV%", "");
          break;
        case E_SELECT:
          blocC.replace("%CNFC_UNIT%", "");
          blocC.replace("%CNFC_OPT%", String(ent->getSelectOptions()));
          blocC.replace("%CNFC_IMIN%", "");
          blocC.replace("%CNFC_IMAX%", "");
          blocC.replace("%CNFC_IDIV%", "");
          break;
        case E_INPUTNUMBER:
          blocC.replace("%CNFC_UNIT%", String(ent->getUnit()));
          blocC.replace("%CNFC_OPT%", "");
          blocC.replace("%CNFC_IMIN%", String(ent->getNumberMin()));
          blocC.replace("%CNFC_IMAX%", String(ent->getNumberMax()));
          blocC.replace("%CNFC_IDIV%", String(ent->getNumberDiv()));
          break;
        default:
          blocC.replace("%CNFC_UNIT%", "");
          blocC.replace("%CNFC_OPT%", "");
          blocC.replace("%CNFC_IMIN%", "");
          blocC.replace("%CNFC_IMAX%", "");
          blocC.replace("%CNFC_IDIV%", "");
      }
    } else
    {
      blocC.replace("%CNFC_ID%", "1");
      blocC.replace("%CNFC_LABEL%", "");
      blocC.replace("%CNFC_LABELVALID%", "aria-invalid='true'");
      blocC.replace("%CNFC_STYPE_INT%", "");
      blocC.replace("%CNFC_STYPE_STR%", "");
      blocC.replace("%CNFC_UNIT%", "");
      blocC.replace("%CNFC_OPT%", "");
      blocC.replace("%CNFC_IMIN%", "");
      blocC.replace("%CNFC_IMAX%", "");
    }
  }
  return blocC;
}

String getHTML_DeviceMonitor(uint8_t id)
{
  String blocD = html_monitor_dev;
  CDevice* dev = Router.getDeviceByAddress(id);
  blocD.replace("#D#", String(id));
  if (dev)
  {
    blocD.replace("%CNFNAME%", dev->getName());
  }
  return blocD;
}

String getHTML_ChildMonitor(uint8_t address, uint8_t id)
{
  String blocC = "";
  String kcnf;
  CDevice* dev = Router.getDeviceByAddress(address);
  if (dev)
  {
    CEntity* ent = dev->getEntityById(id);
    if (ent)
    {
      blocC = "<div class='col-#N cell'>#HEADER#BODY#FOOTER</div>";
      int n = 1;
      String eltHeader = ent->getLabel();
      String eltBody = "";
      String eltFooter = "";
      switch (ent->getElementType())
      {
        case E_BINARYSENSOR:
          eltFooter = "<b id='mon_#D#_#C#'>" + ent->getState() + "</b>";
          break;
        case E_NUMERICSENSOR:
          eltFooter = "<b id='mon_#D#_#C#'>" + ent->getState() + "</b>#UNIT";
          break;
        case E_SWITCH:
          eltFooter = "<input type='checkbox' role='switch' id='mon_#D#_#C#'>";
          break;
        case E_SELECT:
          eltFooter = "<select id='mon_#D#_#C#'></select>";
          break;
        case E_TEXTSENSOR:
          eltFooter = "<b id='mon_#D#_#C#'>" + ent->getState() + "</b>";
          break;
        case E_INPUTNUMBER:
          if (strlen(ent->getUnit())) {
            eltHeader += " #UNIT";
          }
          eltFooter = "<input type='number' id='mon_#D#_#C#' min='1' max='50' step='0.1' value='" + ent->getState() + "'>";
          break;
        case E_BUTTON:
          blocC.replace("#HEADER", "");
          blocC.replace("#FOOTER", "");
          eltBody = "<br><input type='button' value='" + eltHeader + "' onClick='btnMonitor(#D#,#C#)'>";
          break;
        default:
          eltFooter = "????";
      }
      n = (eltHeader.length() < 8) ? 1 : 2;
      blocC.replace("#HEADER", eltHeader != "" ? "<header>" + eltHeader + "</header>" : "");
      blocC.replace("#BODY", eltBody);
      blocC.replace("#FOOTER", eltFooter != "" ? "<footer>" + eltFooter + "</footer>" : "");
      blocC.replace("#UNIT", strlen(ent->getUnit()) ? " (" + String(ent->getUnit()) + ")" : "");
      blocC.replace("#N", String(n));
      blocC.replace("#D#", String(address));
      blocC.replace("#C#", String(id));
    }
  }
  return blocC;
}

void sendConfigDevices()
{
  String blocD = "";
  CDevice* dev = nullptr;
  while ((dev = Router.walkDevice(dev)))
  {
    blocD += getHTML_DeviceForm(dev->getAddress());
  }
  if (blocD == "") {
    blocD = "<mark class=\"row\">📌 To add a new device, you must active pairing below, and press the CFG button when powering ON the module.</mark>";
  }
  docJson.clear();
  docJson["cmd"] = "html";
  docJson["#conf_dev"] = blocD;
  String js;
  serializeJson(docJson, js);
  ws.textAll(js);
}

void sendConfigChilds()
{
  CDevice* dev = nullptr;
  while ((dev = Router.walkDevice(dev)))
  {
    CEntity* ent = nullptr;
    while ((ent = dev->walkEntity(ent)))
    {
      uint8_t address = dev->getAddress();
      uint8_t id = ent->getId();
      docJson.clear();
      docJson["cmd"] = "childnotify";
      docJson["conf_child_" + String(address) + "_" + String(id)] = getHTMLforChildLine(address, id);
      String js;
      serializeJson(docJson, js);
      ws.textAll(js);
    }
  }
}

void sendMonitorDevices()
{
  String blocD = "";
  CDevice* dev = nullptr;
  while ((dev = Router.walkDevice(dev)))
  {
    blocD += getHTML_DeviceMonitor(dev->getAddress());
  }
  docJson.clear();
  docJson["cmd"] = "html";
  docJson["#monitor_dev"] = blocD;
  String js;
  serializeJson(docJson, js);
  ws.textAll(js);
}

void sendMonitorChilds()
{
  CDevice* dev = nullptr;
  while ((dev = Router.walkDevice(dev)))
  {
    CEntity* ent = nullptr;
    while ((ent = dev->walkEntity(ent)))
    {
      uint8_t address = dev->getAddress();
      uint8_t id = ent->getId();
      docJson.clear();
      docJson["cmd"] = "monitornotifychild";
      docJson["mon_entity_" + String(address)] = getHTML_ChildMonitor(address, id);
      String js;
      serializeJson(docJson, js);
      ws.textAll(js);
    }
  }
}

void sendLocalData()
{
  String js = "";
  docJson.clear();
  docJson["cmd"] = "value";
  docJson["#vbat"] = vbat->getFloat();
  docJson["#sin1"] = BinAlarm1->getValue() > 0 ? "alarme" : "normal";
  docJson["#sin2"] = BinAlarm2->getValue() > 0 ? "alarme" : "normal";
  docJson["#lumi"] = Lumi->getFloat();
  serializeJson(docJson, js);
  ws.textAll(js);
  //
  docJson.clear();
  docJson["cmd"] = "checked";
  docJson["#out1"] = RelayOut1->getBool();
  serializeJson(docJson, js);
  ws.textAll(js);

  CDevice* dev = nullptr;
  while ((dev = Router.walkDevice(dev)))
  {
    CEntity* ent = nullptr;
    while ((ent = dev->walkEntity(ent)))
    {
      rl_element_t et = ent->getElementType();
      if (et != E_BUTTON)
      {
        uint8_t address = dev->getAddress();
        uint8_t id = ent->getId();
        notifyState( address, id, ent->getState());
      }
    }
  }
}

void notifyDeviceMonitor(uint8_t d)
{
  if (ws.count() > 0)
  {
    docJson.clear();
    docJson["cmd"] = "monitornotify";
    docJson["mon_data_" + String(d)] = getHTML_DeviceMonitor(d);
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
  }
}

void notifyChildMonitor(uint8_t d, uint8_t c)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "monitornotifychild";
    docJson["mon_entity_" + String(d) ] = getHTML_ChildMonitor(d, c);
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
  }
}

void notifyDeviceForm(uint8_t d)
{
  if (ws.count() > 0)
  {
    docJson.clear();
    docJson["cmd"] = "formnotify";
    docJson["form_" + String(d)] = getHTML_DeviceForm(d);
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
  }
}

void notifyDeviceHeader(uint8_t d)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "html";
    docJson["#header_" + String(d)] = getHTML_DeviceHeader(d);
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
  }
}

void notifyDevicePairing(uint8_t d)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "html";
    docJson["#pairing_" + String(d)] = getHTML_DevicePairing(d);
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
  }
}

void notifyChildLine(uint8_t d, uint8_t c)
{
  if (ws.availableForWriteAll())
  {
    docJson.clear();
    docJson["cmd"] = "childnotify";
    docJson["conf_child_" + String(d) + "_" + String(c)] = getHTMLforChildLine(d, c);
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
  }
}

void notifyState(uint8_t d, uint8_t c, String val)
{
  if (ws.count() > 0)
  {
    docJson.clear();
    docJson["cmd"] = "value";
    docJson["#mon_" + String(d) + "_" + String(c)] = val;
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
  }
}

void notifyAllConfig()
{
  if (ws.count() > 0)
  {
    sendConfigDevices();
    sendConfigChilds();
    sendMonitorDevices();
    sendMonitorChilds();
    sendLocalData();
    ws.textAll("{\"cmd\":\"loadselect\"}");
  }
}

uint8_t notifyRelay(int32_t data)
{
  if (ws.count() > 0)
  {
    docJson.clear();
    docJson["cmd"] = "checked";
    docJson["#out1"] = data == 1 ? true : false;
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
    return true;
  }
  return false;
}

uint8_t notifyLumMin(int32_t data)
{
  if (ws.count() > 0)
  {
    docJson.clear();
    docJson["cmd"] = "value";
    docJson["#cnflummin"] = data;
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
    return true;
  }
  return false;
}

uint8_t notifyOutMode(int32_t data)
{
  if (ws.count() > 0)
  {
    docJson.clear();
    docJson["cmd"] = "value";
    docJson["#cnfoutmode"] = String(RegOut1->getText());
    String js = "";
    serializeJson(docJson, js);
    ws.textAll(js);
    return true;
  }
  return false;
}


void handleWsMessage(void *arg, uint8_t *data, size_t len)
{
  String js;
  DEBUGln("handleWsMessage");
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    DEBUGln((char*)data);
    String cmd = getValue((char*)data, ';', 0);
    String p1 = getValue((char*)data, ';', 1);
    String p2 = getValue((char*)data, ';', 2);
    String p3 = getValue((char*)data, ';', 3);
    if (cmd == "out1")
    {
      RelayOut1->setBool(p1 == "1");
    }
    if (cmd == "deldev")
    {
      Router.delDevice(p1.toInt());
      docJson.clear();
      docJson["cmd"] = "remove";
      docJson["#form_"] = p1;
      serializeJson(docJson, js);
      ws.textAll(js);
      docJson.clear();
      docJson["cmd"] = "remove";
      docJson["#mon_data_"] = p1;
      serializeJson(docJson, js);
      ws.textAll(js);
    }
    if (cmd == "pairing")
    {
      pairToHubPending = true;
      uint8_t h = hubid;
      hubid = RL_ID_BROADCAST;
      deviceManager.publishConfigElements(F("BG secure"), F("BG-S1"));
    }
    if (cmd == "pairingactive")
    {
      pairModulePending = p1 == "1";
    }
    if (cmd == "newadr")
    {
      uint8_t oldAdr = p1.toInt();
      uint8_t newAdr = p2.toInt();
      DEBUGf("New address for device %d = %d\n", oldAdr, newAdr);
      CDevice* dev = Router.getDeviceByAddress(oldAdr);
      if (dev && (oldAdr != newAdr))
      {
        docJson.clear();
        docJson["cmd"] = "remove";
        docJson["#form_"] = String(oldAdr);
        serializeJson(docJson, js);
        ws.textAll(js);
        //
        docJson.clear();
        docJson["cmd"] = "remove";
        docJson["#mon_data_"] = String(oldAdr);
        serializeJson(docJson, js);
        ws.textAll(js);
        //
        dev->setAddress(oldAdr, newAdr);
        //
        notifyDeviceForm(newAdr);
        notifyDeviceHeader(newAdr);
        notifyDevicePairing(newAdr);
        CEntity* ent = nullptr;
        while ((ent = dev->walkEntity(ent)))
        {
          uint8_t id = ent->getId();
          docJson.clear();
          docJson["cmd"] = "childnotify";
          docJson["conf_child_" + String(newAdr) + "_" + String(id)] = getHTMLforChildLine(newAdr, id);
          serializeJson(docJson, js);
          ws.textAll(js);
        }
        notifyDeviceMonitor(newAdr);
        ent = nullptr;
        while ((ent = dev->walkEntity(ent)))
        {
          uint8_t id = ent->getId();
          docJson.clear();
          docJson["cmd"] = "monitornotifychild";
          docJson["mon_entity_" + String(newAdr)] = getHTML_ChildMonitor(newAdr, id);
          String js;
          serializeJson(docJson, js);
          ws.textAll(js);
        }
      }
    }
    if (cmd == "endpairing")
    {
      uint8_t address = p1.toInt();
      DEBUGf("Stop pairing for device %d\n", address);
      if (Router.endPairing(address))
      {
        notifyDevicePairing(address);
        //
        notifyDeviceMonitor(address);
        CDevice* dev = Router.getDeviceByAddress(address);
        if (dev)
        {
          CEntity* ent = nullptr;
          while ((ent = dev->walkEntity(ent)))
          {
            uint8_t id = ent->getId();
            notifyChildMonitor(address, id);
          }
        }
      }
    }
    if (cmd == "btnmon")
    {
      CEntity* ent = Router.getEntityById(p1.toInt(), p2.toInt());
      if (ent) {
        MLiotComm.publishText(p1.toInt(), hubid, ent->getId(), "!");
      }
    }
    if (cmd == "inputmon")
    {
      CEntity* ent = Router.getEntityById(p1.toInt(), p2.toInt());
      if (ent) {
        float f = p3.toFloat() * ent->getNumberDiv();
        MLiotComm.publishFloat(p1.toInt(), hubid, ent->getId(), f, ent->getNumberDiv());
      }
    }
  }
}

void WiFiEvent(WiFiEvent_t event)
{
  switch (event)
  {
    case ARDUINO_EVENT_WIFI_READY:
      DEBUGln("WiFi ready");
      break;
    case ARDUINO_EVENT_WIFI_SCAN_DONE:
      DEBUGln("WiFi Completed scan");
      break;
    case ARDUINO_EVENT_WIFI_STA_START:
      DEBUGln("STA started");
      break;
    case ARDUINO_EVENT_WIFI_STA_STOP:
      DEBUGln("STA stopped");
      break;
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      DEBUGln("STA Connected");
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      DEBUGln("STA Disconnected");
      break;
    case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
      DEBUGln("STA Authmode change");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      DEBUG("STA IP address: ");
      DEBUGln(WiFi.localIP());
      break;
    case ARDUINO_EVENT_WIFI_STA_LOST_IP:
      DEBUGln("STA Lost IP");
      break;
    case ARDUINO_EVENT_WPS_ER_SUCCESS:
      DEBUGln("WPS succeeded in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_FAILED:
      DEBUGln("WPS failed in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_TIMEOUT:
      DEBUGln("WPS timeout in enrollee mode");
      break;
    case ARDUINO_EVENT_WPS_ER_PIN:
      DEBUGln("WPS pin code in enrollee mode");
      break;
    case ARDUINO_EVENT_WIFI_AP_START:
      DEBUGln("AP started");
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      DEBUGln("AP  stopped");
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      DEBUGln("AP Client connected");
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      DEBUGln("AP Client disconnected");
      break;
    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      DEBUGln("AP Assigned IP address to client");
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      DEBUGln("AP Received probe request");
      break;
    case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
      DEBUGln("AP IPv6 is preferred");
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
      DEBUGln("STA IPv6 is preferred");
      break;
    default:
      DEBUGf("[WiFi-event] %d \n", event);
      break;
  }
}

void onWsEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:
      DEBUGf("WebSocket client #%u/%d connected from %s\n", client->id(), server->count(), client->remoteIP().toString().c_str());
      notifyAllConfig();
      break;
    case WS_EVT_DISCONNECT:
      DEBUGf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWsMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      DEBUGf("Erreur WebSocket client #%u: %s\n", client->id(), (char*)data);
      break;
  }
}

uint8_t getWMODE()
{
  uint8_t wmode = Mode_WifiSTA;
  if (digitalRead(PIN_WMODE) == LOW || strlen(Wifi_ssid) == 0) {
    wmode = Mode_WifiAP;
    DEBUGf("WMode : Wifi AP\n");
  } else {
    DEBUGf("WMode : Wifi STA\n");
  }
  return wmode;
}

void startWifiSTA()
{
  DEBUG("MAC : ");
  DEBUGln(WiFi.macAddress());
  WiFi.mode(WIFI_STA);
  WiFi.begin(Wifi_ssid, Wifi_pass);
  int tentativeWiFi = 0;
  // Attente de la connexion au réseau WiFi / Wait for connection
  while ( WiFi.status() != WL_CONNECTED && tentativeWiFi < 20)
  {
    delay( 500 ); DEBUG( "." );
    tentativeWiFi++;
  }
}

void startWifiAP()
{
  DEBUGln("set Wifi AP mode.");
  // Mode AP
  WiFi.mode(WIFI_AP);
  WiFi.softAP(AP_ssid, AP_pass, 1, true);
  // Default IP Address is 192.168.4.1
  // if you want to change uncomment below
  // softAPConfig (local_ip, gateway, subnet)

  DEBUGf("AP WIFI : %s\n", AP_ssid);
  DEBUG("AP IP Address: "); DEBUGln(WiFi.softAPIP());
}

void initNetwork()
{
  byte mac[6];
  WiFi.onEvent(WiFiEvent);

  pinMode(PIN_WMODE, INPUT_PULLUP); // Wifi STA by default (Short cut to GND to have AP)
  WiFi.macAddress(mac);

  switch (getWMODE()) {
    case Mode_WifiSTA:
      startWifiSTA();
      break;
    case Mode_WifiAP:
      startWifiAP();
      break;
  }

  if (MDNS.begin(AP_ssid))
  {
    MDNS.addService("http", "tcp", 80);
    DEBUGf("MDNS on %s\n", AP_ssid);
  }
}

void onIndexRequest(AsyncWebServerRequest *request)
{
  AsyncResponseStream *response = request->beginResponseStream("text/html");
  File f = SPIFFS.open("/index.html", "r");
  if (f)
  {
    String html;
    while (f.available()) {
      html = f.readStringUntil('\n') + '\n';
      if (html.indexOf('%') > 0)
      {
        html.replace("%CNFCODE%", String(AP_ssid[strlen(AP_ssid) - 1]));
        html.replace("%CNFFREQ%", String(RadioFreq));
        html.replace("%CNFDIST%", String(RadioRange));
        html.replace("%CNFHUBID%", String(hubid));
        html.replace("%CNFROUTE%", routeID == 1 ? "checked" : "");
        html.replace("%LORAOK%", loraOK ? "" : "<i style='color:#FF0000'>Error</i>");
        html.replace("%VERSION%", VERSION);
        //
        html.replace("%CNFOUTMODE_MANU%", RegOut1->getValue() != 1 ? "selected" : "");
        html.replace("%CNFOUTMODE_AUTO%", RegOut1->getValue() == 1 ? "selected" : "");
        html.replace("%CNFLUMMIN%", String((long)LumMin->getValue()));
      }
      response->print(html);
    }
    f.close();
  }
  request->send(response);
}

void onConfigRequest(AsyncWebServerRequest * request)
{
  DEBUGln("web set config");
  /* for debug
    for (int i = 0; i < request->params(); i++)
    {
    AsyncWebParameter* p = request->getParam(i);
    DEBUGf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  */
  if (request->hasParam("cnfdev", true))
  {
    // ! ! ! POST data must contain ONLY 1 Device
    JsonDocument docJSon;
    //JsonObject Jconfig = docJSon.to<JsonObject>();
    int params = request->params();
    CDevice* dev = nullptr;
    CEntity* ent = nullptr;
    for (int i = 0; i < params; i++)
    {
      AsyncWebParameter* p = request->getParam(i);
      String str = p->name();
      if (str == "cnfdev") continue;
      // exemple : dev_0_address
      //      or : dev_1_child_2_label
      String valdev = getValue(str, '_', 0);
      String keydev = getValue(str, '_', 1);
      String attrdev = getValue(str, '_', 2);
      String keychild = getValue(str, '_', 3);
      String attrchild = getValue(str, '_', 4);
      String sval(p->value().c_str());

      DEBUGf("[%s][%s][%s][%s][%s] = %s\n", valdev.c_str(), keydev.c_str(), attrdev.c_str(), keychild.c_str(), attrchild.c_str(), sval.c_str());
      if (valdev == "dev" && keydev.toInt() > 0)
      {
        dev = Router.getDeviceByAddress(keydev.toInt());
        if (dev) {
          if (attrdev == "childs") {
            ent = Router.getEntityById(keydev.toInt(), keychild.toInt());
            if (ent) {
              ent->setAttr(attrchild, sval, 0);
            }
          }
        }
      }
    }
    //
    Router.saveConfig();
  }
  if (request->hasParam("cnfcode", true))
  {
    uid = 100 + request->getParam("cnfcode", true)->value().toInt();
    AP_ssid[strlen(AP_ssid) - 1] = request->getParam("cnfcode", true)->value().c_str()[0];
    if (request->hasParam("cnffreq", true))
    {
      RadioFreq = request->getParam("cnffreq", true)->value().toInt();
    }
    if (request->hasParam("cnfdist", true))
    {
      RadioRange = request->getParam("cnfdist", true)->value().toInt();
    }
    if (request->hasParam("cnfhubid", true))
    {
      hubid = request->getParam("cnfhubid", true)->value().toInt();
    }
    routeID = 0;
    if (request->hasParam("cnfroute", true))
    {
      routeID = strcmp(request->getParam("cnfroute", true)->value().c_str(), "on") == 0;
    }

    // Relay automation
    if (request->hasParam("cnfoutmode", true))
    {
      strcmp(request->getParam("cnfoutmode", true)->value().c_str(), "auto") == 0 ? RegOut1->setValue(1) : RegOut1->setValue(0);
    }
    if (request->hasParam("cnflummin", true))
    {
      LumMin->setFloat(request->getParam("cnflummin", true)->value().toInt());
    }

    DEBUGln(uid);
    DEBUGln(AP_ssid);
    DEBUGln(RadioFreq);
    DEBUGln(hubid);
    DEBUGln(routeID);

    EEPROM.writeChar(EEPROM_DATA_UID, AP_ssid[strlen(AP_ssid) - 1]);
    EEPROM.writeUShort(EEPROM_DATA_FREQ, RadioFreq);
    EEPROM.writeByte(EEPROM_DATA_RANGE, RadioRange);
    EEPROM.writeByte(EEPROM_DATA_HUBID, hubid);
    EEPROM.writeByte(EEPROM_DATA_ROUTEID, routeID);
    EEPROM.commit();
  }
  request->send(200, "text/plain", "OK");
}

void handleDoUpdate(AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    DEBUGln("Update start");
    content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    int cmd = (filename.indexOf("spiffs") > -1) ? U_SPIFFS : U_FLASH;
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, cmd)) {
      Update.printError(Serial);
    }
  }
  if (Update.write(data, len) != len)
  {
    Update.printError(Serial);
  }
  if (final)
  {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true))
    {
      Update.printError(Serial);
    } else
    {
      DEBUGln("Update complete");
      delay(100);
      yield();
      delay(100);
      ESP.restart();
    }
  }
}

void handleDoFile(AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final)
{
  if (!index)
  {
    if (filename.endsWith(".png"))
    {
      request->_tempFile = SPIFFS.open("/i/" + filename, "w");
    } else if (filename.endsWith(".css"))
    {
      request->_tempFile = SPIFFS.open("/css/" + filename, "w");
    } else if (filename.endsWith(".js"))
    {
      request->_tempFile = SPIFFS.open("/js/" + filename, "w");
    } else if (filename.endsWith(".hex"))
    {
      request->_tempFile = SPIFFS.open("/hex/" + filename, "w");
    } else
    {
      request->_tempFile = SPIFFS.open("/" + filename, "w");
    }
  }
  if (len)
  {
    request->_tempFile.write(data, len);
  }
  if (final)
  {
    request->_tempFile.close();
    request->redirect("/");
  }
}

void updateProgress(size_t prg, size_t sz)
{
  DEBUGf("Progress: %d%%\n", (prg * 100) / content_len);
}

void initWeb()
{
  server.serveStatic("/fonts", SPIFFS, "/fonts");
  server.serveStatic("/css", SPIFFS, "/css");
  server.serveStatic("/js", SPIFFS, "/js");
  server.serveStatic("/i", SPIFFS, "/i");
  server.serveStatic("/config.json", SPIFFS, "/config.json");
  server.on("/", HTTP_GET, onIndexRequest);
  server.on("/doconfig", HTTP_POST, onConfigRequest);
  server.on("/doupdate", HTTP_POST, [](AsyncWebServerRequest * request) {},
  [](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final) {
    handleDoUpdate(request, filename, index, data, len, final);
  });
  server.on("/dofile", HTTP_POST, [](AsyncWebServerRequest * request) {
    request->send(200);
  },
  [](AsyncWebServerRequest * request, const String & filename, size_t index, uint8_t *data, size_t len, bool final) {
    handleDoFile(request, filename, index, data, len, final);
  });
  server.on("/restart", HTTP_GET, [] (AsyncWebServerRequest * request)
  {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "10");
    response->addHeader("Location", "/");
    request->send(response);
    delay(500);
    ESP.restart();
  });
  //
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  //
  server.begin();


  Update.onProgress(updateProgress);
  DEBUGln("HTTP server started");
}
