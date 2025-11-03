#pragma once
#include <HTTPClient.h>  // native library
#include <ESPmDNS.h>     // native library
#include <Update.h>      // native library
#include <WiFiClientSecure.h>
#include <queue>

enum WMode {
  Mode_WifiSTA = 0,
  Mode_WifiAP,
  Mode_Ethernet
};

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
size_t content_len;

std::queue<String> messageQueueWS;

const char* cert = \
                   "-----BEGIN CERTIFICATE-----\n" \
                   "MIIFBzCCA++gAwIBAgISBmabN/EA2coFNJTXYIHOodylMA0GCSqGSIb3DQEBCwUA\n" \
                   "MDMxCzAJBgNVBAYTAlVTMRYwFAYDVQQKEw1MZXQncyBFbmNyeXB0MQwwCgYDVQQD\n" \
                   "EwNSMTIwHhcNMjUwOTEzMDMzNTQzWhcNMjUxMjEyMDMzNTQyWjAgMR4wHAYDVQQD\n" \
                   "ExVzbXNhcGkuZnJlZS1tb2JpbGUuZnIwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAw\n" \
                   "ggEKAoIBAQDigxROQ2HDrBKd5pIJvM0JcsX0w+Fvl1oxol9Ar9Gi1rWQ56Zx4NPz\n" \
                   "IbbmgXLMnhtDqG9hq0QLLUQPIi0z+Fs/GHmWk/N8lAVGtRf18GBDUI/4ZQsyvCiC\n" \
                   "zUkSEhJZrGiKitPxUbuScC/kSFLf6NmPp4NOoSOf2pB3HSrFGnyPewIymx5eVa92\n" \
                   "p34do7YKFUXKZAYD6ND2unyor92RCJE/zmVsTJz8lLWLIqVCchJ7T8FcklqrL0Kl\n" \
                   "VbvWJ4/8QD11h0QNcimzSGNoJSTv/rGZ9t8gm4JmY1D2pk1kQ8s2V/8RKL5fHARQ\n" \
                   "Fj8GNPD1ydkEcTbpYMsRpRIq6riEuqGBAgMBAAGjggImMIICIjAOBgNVHQ8BAf8E\n" \
                   "BAMCBaAwHQYDVR0lBBYwFAYIKwYBBQUHAwEGCCsGAQUFBwMCMAwGA1UdEwEB/wQC\n" \
                   "MAAwHQYDVR0OBBYEFNrosUnMTK9sQ+BEDl7ikATk8em+MB8GA1UdIwQYMBaAFAC1\n" \
                   "KfItjm8x6JtMrXg++tzpDNHSMDMGCCsGAQUFBwEBBCcwJTAjBggrBgEFBQcwAoYX\n" \
                   "aHR0cDovL3IxMi5pLmxlbmNyLm9yZy8wIAYDVR0RBBkwF4IVc21zYXBpLmZyZWUt\n" \
                   "bW9iaWxlLmZyMBMGA1UdIAQMMAowCAYGZ4EMAQIBMC4GA1UdHwQnMCUwI6AhoB+G\n" \
                   "HWh0dHA6Ly9yMTIuYy5sZW5jci5vcmcvMzguY3JsMIIBBQYKKwYBBAHWeQIEAgSB\n" \
                   "9gSB8wDxAHcAEvFONL1TckyEBhnDjz96E/jntWKHiJxtMAWE6+WGJjoAAAGZQVox\n" \
                   "oQAABAMASDBGAiEAqFC+xnffogWHCSWxR7HxiQYo4Mn+tLNEFtoEH4pKjDICIQCN\n" \
                   "JYiffKnTTpNQfehdzjfsdo9H9HcloEKxmLZD+zAv9gB2AKRCxQZJYGFUjw/U6pz7\n" \
                   "ei0mRU2HqX8v30VZ9idPOoRUAAABmUFaOWYAAAQDAEcwRQIgcv2ry/vELRDUeokq\n" \
                   "RD0ZWraeZdFRcw1J/PJ+gcK5pu8CIQCwJbag8FGJxNNp9aX8hiXPw9zdMTgquZuB\n" \
                   "PejMTwnqcjANBgkqhkiG9w0BAQsFAAOCAQEAXOyKxIkSHsAYAXBLcReiNjeqWEwu\n" \
                   "FgjJ+rzFiNiqCLzxy+oU3aAfFdXOVoQ4CNKsBR05B6FHhM9ajwpWVwcPk01l4VJP\n" \
                   "EgBe2K/7pouztPbhFkRF/WuxE4216CHxXAn1L3qgo4d+0iVIana2Dispwm6E9WdP\n" \
                   "Xa5NdAPvVTJntGSf6Q0v6/YwMPTq02lPJwYq8fH84pRfY0AP8QJUCLsXIckMixti\n" \
                   "IRPT2qdToIxJcP622GxZtf9eUlfB7QrvDSJFCDCW5cBkQXF6BJ4OPWyfgTZtATTw\n" \
                   "FwC9fpxEG4KDw17v9G/6uJoMbfzlxEvtiGinT5ZELIvV8zdokMF/bA6SWQ==\n" \
                   "-----END CERTIFICATE-----\n";

String urlencode(String str)
{
  String encodedString = "";
  char c;
  char code0;
  char code1;
  char code2;
  for (int i = 0; i < str.length(); i++) {
    c = str.charAt(i);
    if (c == ' ') {
      encodedString += '+';
    } else if (isalnum(c)) {
      encodedString += c;
    } else {
      code1 = (c & 0xf) + '0';
      if ((c & 0xf) > 9) {
        code1 = (c & 0xf) - 10 + 'A';
      }
      c = (c >> 4) & 0xf;
      code0 = c + '0';
      if (c > 9) {
        code0 = c - 10 + 'A';
      }
      code2 = '\0';
      encodedString += '%';
      encodedString += code0;
      encodedString += code1;
      //encodedString+=code2;
    }
    yield();
  }
  return encodedString;
}

void sendFreeSMS(const char* message)
{
  WiFiClientSecure client;
  client.setInsecure(); // Ignorer la vérification du certificat SSL (plus simple pour test)
  //client.setCACert(cert);
  if (!client.connect("smsapi.free-mobile.fr", 80))
  {
    DEBUGln("Connexion échouée !");
    return;
  }

  String url = "/sendmsg?user=" + String(smsUser) + "&pass=" + String(smsPass) + "&msg=" + urlencode(message);
  DEBUGf("Envoi vers : % s\n", url.c_str());

  client.println(String("GET ") + url + " HTTP / 1.1");
  client.println("Host : smsapi.free-mobile.fr");
  client.println("Connection : close");
  client.println();

  while (client.connected())
  {
    String line = client.readStringUntil('\n');
    DEBUGln(line);
    if (line == "\r") break;
  }
  DEBUGln("SMS envoyé !");
}

void sendWS(String txt)
{
  if (ws.count() > 0)
  {
    messageQueueWS.push(txt);
  }
}

void processMessagesWS()
{
  if (!messageQueueWS.empty())
  {
    if ((ws.count() > 0) && ws.availableForWriteAll())
    {
      String msg = messageQueueWS.front();
      ws.textAll(msg);
      messageQueueWS.pop();
    }
  }
}

String getFromModl(String tag)
{
  bool found = false;
  String bloc = "";
  File f = SPIFFS.open("/index.modl", "r");
  if (f)
  {
    String html;
    while (f.available())
    {
      html = f.readStringUntil('\n') + '\n';
      html.trim();
      int i = html.indexOf("{§");
      if (i >= 0)
      {
        html.replace("{§", "");
        i = html.indexOf(tag);
        if (i >= 0)
        {
          html.replace(tag, "");
          found = true;
        }
      }
      if (found && html != "")
      {
        i = html.indexOf("§}");
        if (i >= 0)
        {
          html.replace("§}", "");
          bloc += html;
          break;
        } else
        {
          bloc += html;
        }
      }
    }
    close(f);
  }
  return bloc;
}

String getHTML_DeviceHeader(uint8_t address)
{
  String blocD = getFromModl("DEV_HEAD");
  CDevice* dev = Hub.getDeviceByAddress(address);
  blocD.replace("#D#", String(address));
  if (dev)
  {
    blocD.replace("%CNFNAME%", String(dev->getName()));
    blocD.replace("%CNFMODEL%", String(dev->getModel()));
  } else {
    blocD.replace("%CNFNAME%", "");
    blocD.replace("%CNFMODEL%", "");
  }
  //
  return blocD;
}

String getHTML_DevicePairing(uint8_t idx)
{
  String blocD = "";
  CDevice* dev = Hub.getDeviceByAddress(idx);
  if (dev && dev->getPairing())
  {
    int newIdx = idx + 1;
    CDevice* newdev = Hub.getDeviceByAddress(newIdx);
    while (newdev) {
      newIdx++;
      newdev = Hub.getDeviceByAddress(newIdx);
    }
    blocD = getFromModl("DEV_PAIRING");
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
  String blocD = getFromModl("DEV_FORM");
  CDevice* dev = Hub.getDeviceByAddress(id);
  if (dev)
  {
    blocD.replace("%CNFMODEL%", dev->getModel());
    blocD.replace("%CNFNAME%", dev->getName());
    blocD.replace("%CNFADDRESS%", String(dev->getAddress()));
    blocD.replace("%GENHEADER%", getHTML_DeviceHeader(id));
    blocD.replace("%GENPAIRING%", getHTML_DevicePairing(id));
  }
  blocD.replace("#D#", String(id));
  return blocD;
}

String getHTMLforChildLine(uint8_t address, uint8_t id)
{
  const char html_child_line_class[] = "<input type='text' name='dev_#D#_childs_#C#_class' list='classlist#T#' value='%CNFC_CLASS%'>";
  const char html_child_line_categ[] = "<select name='dev_#D#_childs_#C#_category'><option value='0' %CNF_CAT0_DS%>Auto</option><option value='1' %CNF_CAT1_DS%>Configuration</option><option value='2' %CNF_CAT2_DS%>Diagnostic</option></select>";
  const char html_child_line_min[] = "<input type='text' name='dev_#D#_childs_#C#_min' value='%CNFC_MIN%'>";
  const char html_child_line_max[] = "<input type='text' name='dev_#D#_childs_#C#_max' value='%CNFC_MAX%'>";
  const char html_child_line_coefa[] = "<input type='text' name='dev_#D#_childs_#C#_coefa' value='%CNFC_COEFA%'>";
  const char html_child_line_coefb[] = "<input type='text' name='dev_#D#_childs_#C#_coefb' value='%CNFC_COEFB%'>";

  String blocC = getFromModl("CHILD_LINE");
  String kcnf;
  CDevice* dev = Hub.getDeviceByAddress(address);
  if (dev)
  {
    CEntity* ent = dev->getEntityById(id);
    if (ent)
    {
      blocC.replace("%CNFC_ID%", String(ent->getId()));
      blocC.replace("%CNFC_LABEL%", String(ent->getLabel()));
      blocC.replace("%CNFC_LABELVALID%", strlen(ent->getLabel()) == 0 ? "aria-invalid='true'" : String(ent->getLabel()));
      EntityCategory ec = ent->getCategory();
      switch (ent->getElementType())
      {
        case E_BINARYSENSOR:
          blocC.replace("%CNFL_CLASS%", html_child_line_class);
          blocC.replace("#T#", "BS");
          blocC.replace("%CNFL_CATEG%", html_child_line_categ);
          blocC.replace("%CNFL_MIN%", "");
          blocC.replace("%CNFL_MAX%", "");
          blocC.replace("%CNFL_COEFA%", "");
          blocC.replace("%CNFL_COEFB%", "");
          //
          blocC.replace("%CNF_CAT0_DS%", ec == CategoryAuto ? "selected" : "");
          blocC.replace("%CNF_CAT1_DS%", "disabled");
          blocC.replace("%CNF_CAT2_DS%", ec == CategoryDiagnostic ? "selected" : "");
          break;
        case E_NUMERICSENSOR:
          blocC.replace("%CNFL_CLASS%", html_child_line_class);
          blocC.replace("#T#", "NS");
          blocC.replace("%CNFL_CATEG%", html_child_line_categ);
          blocC.replace("%CNFL_MIN%", html_child_line_min);
          blocC.replace("%CNFL_MAX%", html_child_line_max);
          blocC.replace("%CNFL_COEFA%", html_child_line_coefa);
          blocC.replace("%CNFL_COEFB%", html_child_line_coefb);
          //
          blocC.replace("%CNF_CAT0_DS%", ec == CategoryAuto ? "selected" : "");
          blocC.replace("%CNF_CAT1_DS%", "disabled");
          blocC.replace("%CNF_CAT2_DS%", ec == CategoryDiagnostic ? "selected" : "");
          break;
        case E_SWITCH:
          blocC.replace("%CNFL_CLASS%", "");
          blocC.replace("%CNFL_CATEG%", html_child_line_categ);
          blocC.replace("%CNFL_MIN%", "");
          blocC.replace("%CNFL_MAX%", "");
          blocC.replace("%CNFL_COEFA%", "");
          blocC.replace("%CNFL_COEFB%", "");
          //
          blocC.replace("%CNF_CAT0_DS%", ec == CategoryAuto ? "selected" : "");
          blocC.replace("%CNF_CAT1_DS%", "disabled");
          blocC.replace("%CNF_CAT2_DS%", ec == CategoryDiagnostic ? "selected" : "");
          break;
        case E_SELECT:
          blocC.replace("%CNFL_CLASS%", "");
          blocC.replace("%CNFL_CATEG%", html_child_line_categ);
          blocC.replace("%CNFL_MIN%", "");
          blocC.replace("%CNFL_MAX%", "");
          blocC.replace("%CNFL_COEFA%", "");
          blocC.replace("%CNFL_COEFB%", "");
          //
          blocC.replace("%CNF_CAT0_DS%", ec == CategoryAuto ? "selected" : "");
          blocC.replace("%CNF_CAT1_DS%", ec == CategoryConfig ? "selected" : "");
          blocC.replace("%CNF_CAT2_DS%", "disabled");
          break;
        case E_INPUTNUMBER:
          blocC.replace("%CNFL_CLASS%", "");
          blocC.replace("%CNFL_CATEG%", html_child_line_categ);
          blocC.replace("%CNFL_MIN%", "");
          blocC.replace("%CNFL_MAX%", "");
          blocC.replace("%CNFL_COEFA%", "");
          blocC.replace("%CNFL_COEFB%", "");
          //
          blocC.replace("%CNF_CAT0_DS%", ec == CategoryAuto ? "selected" : "");
          blocC.replace("%CNF_CAT1_DS%", ec == CategoryConfig ? "selected" : "");
          blocC.replace("%CNF_CAT2_DS%", "disabled");
          break;
        case E_BUTTON:
          blocC.replace("%CNFL_CLASS%", "");
          blocC.replace("%CNFL_CATEG%", html_child_line_categ);
          blocC.replace("%CNFL_MIN%", "");
          blocC.replace("%CNFL_MAX%", "");
          blocC.replace("%CNFL_COEFA%", "");
          blocC.replace("%CNFL_COEFB%", "");
          //
          blocC.replace("%CNF_CAT0_DS%", ec == CategoryAuto ? "selected" : "");
          blocC.replace("%CNF_CAT1_DS%", ec == CategoryConfig ? "selected" : "");
          blocC.replace("%CNF_CAT2_DS%", "disabled");
          break;
      }
      blocC.replace("%CNFC_CLASS%", String(ent->getClass()));
      blocC.replace("%CNFC_MIN%", ent->getMini() == LONG_MIN ? "" : String(ent->getMini()));
      blocC.replace("%CNFC_MAX%", ent->getMaxi() == LONG_MAX ? "" : String(ent->getMaxi()));
      blocC.replace("%CNFC_COEFA%", String(ent->getCoefA()));
      blocC.replace("%CNFC_COEFB%", String(ent->getCoefB()));
    }
  }
  blocC.replace("#D#", String(address));
  blocC.replace("#C#", String(id));
  return blocC;
}
/*
  String getHTMLForChildModal(uint8_t address, uint8_t id)
  {
  String blocC = getFromModl("CHILD_MODAL");
  String kcnf;
  CDevice* dev = Hub.getDeviceByAddress(address);
  if (dev)
  {
    CEntity* ent = dev->getEntityById(id);
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
        case E_SWITCH:
          blocC.replace("%CNFC_UNIT%", "");
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
  blocC.replace("#D#", String(address));
  blocC.replace("#C#", String(id));
  return blocC;
  }
*/
String getHTML_DeviceMonitor(uint8_t id)
{
  String blocD = getFromModl("DEV_MONITOR");
  CDevice* dev = Hub.getDeviceByAddress(id);
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
  CDevice* dev = Hub.getDeviceByAddress(address);
  if (dev)
  {
    CEntity* ent = dev->getEntityById(id);
    if (ent)
    {
      blocC = "<div class='cell'>#HEADER##BODY##FOOTER#</div>"; // modify : class='col-#N# cell'
      float step = 1.;
      int n = 1;
      String eltHeader = ent->getLabel();
      String eltBody = "";
      String eltFooter = "";
      String sTmp = "";
      int idx;
      switch (ent->getElementType())
      {
        case E_BINARYSENSOR:
          eltFooter = "<b id='mon_#D#_#C#'>" + ent->getState() + "</b>";
          break;
        case E_NUMERICSENSOR:
          eltFooter = "<a id='mon_#D#_#C#' data-target='modalplot' onClick='displayPlot(event,#D#,#C#)'>" + ent->getState() + "</a>#UNIT#";
          break;
        case E_SWITCH:
          eltFooter = "<input type='checkbox' role='switch' id='mon_#D#_#C#' ";
          if (ent->getState() == "1") eltBody += "checked";
          eltFooter += " onclick='switchMonitor(this,#D#,#C#)'>";
          break;
        case E_SELECT:
          sTmp = ent->getSelectOptions();
          DEBUGln(sTmp);
          idx = 0;
          eltFooter = "<select id='mon_#D#_#C#' onchange='selectMonitor(this,#D#,#C#)'>";
          while (getValue(sTmp, ',', idx) != "")
          {
            eltFooter += "<option value='" + String(idx) + "'>" + getValue(sTmp, ',', idx) + "</option>";
            idx++;
          }
          eltFooter += "</select>";
          break;
        case E_TEXTSENSOR:
          eltFooter = "<b id='mon_#D#_#C#'>" + ent->getState() + "</b>";
          break;
        case E_INPUTNUMBER:
          if (strlen(ent->getUnit())) {
            eltHeader += " #UNIT#";
          }
          step = 1. / ent->getNumberDiv();
          eltFooter = "<input type='number' id='mon_#D#_#C#' min='"
                    + String(ent->getNumberMin()) + "' max='"
                    + String(ent->getNumberMax()) + "' step='" + String(step) + "' value='" + ent->getState() + "'>";
          break;
        case E_BUTTON:
          blocC.replace("#HEADER#", "");
          eltBody = "<br><input type='button' value='" + eltHeader + "' onClick='buttonMonitor(#D#,#C#)'>";
          break;
        default:
          eltBody = "????";
      }
      n = (eltHeader.length() < 8) ? 1 : 2;
      blocC.replace("#HEADER#", eltHeader != "" ? "<header>" + eltHeader + "</header>" : "");
      blocC.replace("#BODY#", eltBody != "" ? "<body>" + eltBody + "</body>" : "");
      blocC.replace("#FOOTER#", eltFooter != "" ? "<footer>" + eltFooter + "</footer>" : "");
      blocC.replace("#UNIT#", strlen(ent->getUnit()) ? " (" + String(ent->getUnit()) + ")" : "");
      blocC.replace("#N#", String(n));
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
  while ((dev = Hub.walkDevice(dev)))
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
  sendWS(js);
}

void sendConfigChilds()
{
  CDevice* dev = nullptr;
  while ((dev = Hub.walkDevice(dev)))
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
      sendWS(js);
    }
  }
}

void sendMonitorDevices()
{
  String blocD = "";
  CDevice* dev = nullptr;
  while ((dev = Hub.walkDevice(dev)))
  {
    blocD += getHTML_DeviceMonitor(dev->getAddress());
  }
  if (blocD == "") {
    blocD = "<mark class=\"row\">📌 add device to see monitor data.</mark>";
  }
  docJson.clear();
  docJson["cmd"] = "html";
  docJson["#monitor_dev"] = blocD;
  String js;
  serializeJson(docJson, js);
  sendWS(js);
}

void sendMonitorChilds()
{
  CDevice* dev = nullptr;
  while ((dev = Hub.walkDevice(dev)))
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
      sendWS(js);
    }
  }
}

void sendLocalData()
{
  String js = "";
  CDevice* dev = nullptr;
  while ((dev = Hub.walkDevice(dev)))
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
  docJson.clear();
  docJson["cmd"] = "monitornotify";
  docJson["mon_data_" + String(d)] = getHTML_DeviceMonitor(d);
  String js = "";
  serializeJson(docJson, js);
  sendWS(js);
}

void notifyChildMonitor(uint8_t d, uint8_t c)
{
  docJson.clear();
  docJson["cmd"] = "monitornotifychild";
  docJson["mon_entity_" + String(d) ] = getHTML_ChildMonitor(d, c);
  String js = "";
  serializeJson(docJson, js);
  sendWS(js);
}

void notifyDeviceForm(uint8_t d)
{
  docJson.clear();
  docJson["cmd"] = "formnotify";
  docJson["form_" + String(d)] = getHTML_DeviceForm(d);
  String js = "";
  serializeJson(docJson, js);
  sendWS(js);
}

void notifyDeviceHeader(uint8_t d)
{
  docJson.clear();
  docJson["cmd"] = "html";
  docJson["#header_" + String(d)] = getHTML_DeviceHeader(d);
  String js = "";
  serializeJson(docJson, js);
  sendWS(js);
}

void notifyDevicePairing(uint8_t d)
{
  docJson.clear();
  docJson["cmd"] = "html";
  docJson["#pairing_" + String(d)] = getHTML_DevicePairing(d);
  String js = "";
  serializeJson(docJson, js);
  sendWS(js);
}

void notifyChildLine(uint8_t d, uint8_t c)
{
  docJson.clear();
  docJson["cmd"] = "childnotify";
  docJson["conf_child_" + String(d) + "_" + String(c)] = getHTMLforChildLine(d, c);
  String js = "";
  serializeJson(docJson, js);
  sendWS(js);
}

void notifyState(uint8_t d, uint8_t c, String val)
{
  if (ws.count() > 0)
  {
    CDevice* dev = Hub.getDeviceByAddress(d);
    if (dev)
    {
      CEntity* ent = dev->getEntityById(c);
      if (ent)
      {
        docJson.clear();
        switch (ent->getElementType())
        {
          case E_BINARYSENSOR:
            docJson["cmd"] = "value";
            docJson["#mon_" + String(d) + "_" + String(c)] = val;
            break;
          case E_NUMERICSENSOR:
            docJson["cmd"] = "value";
            docJson["#mon_" + String(d) + "_" + String(c)] = val;
            break;
          case E_SWITCH:
            docJson["cmd"] = "checked";
            docJson["#mon_" + String(d) + "_" + String(c)] = val;
            break;
          case E_COVER:
            docJson["cmd"] = "value";
            docJson["#mon_" + String(d) + "_" + String(c)] = "TODO";
            break;
          case E_SELECT:
            docJson["cmd"] = "selected";
            docJson["#mon_" + String(d) + "_" + String(c)] = val;
            break;
          case E_TEXTSENSOR:
            docJson["cmd"] = "checked";
            docJson["#mon_" + String(d) + "_" + String(c)] = val;
            break;
          case E_INPUTNUMBER:
            docJson["cmd"] = "value";
            docJson["#mon_" + String(d) + "_" + String(c)] = val;
            break;
          case E_BUTTON:
            docJson["cmd"] = "checked";
            docJson["#mon_" + String(d) + "_" + String(c)] = val;
            break;
        }
        String js = "";
        serializeJson(docJson, js);
        sendWS(js);
      }
    }
  }
}

void notifyDateTime()
{
  if (ws.count() > 0)
  {
    char buf[25];
    sprintf(buf, "%02d:%02d", rtc.hour(), rtc.minute());
    docJson.clear();
    docJson["cmd"] = "html";
    docJson["#datetime"] = String(buf);;
    String js = "";
    serializeJson(docJson, js);
    sendWS(js);
  }
}

void notifyAllConfig()
{
  sendConfigDevices();
  sendConfigChilds();
  sendMonitorDevices();
  sendMonitorChilds();
  sendLocalData();
  notifyDateTime();
  sendWS("{\"cmd\":\"loadselect\"}");

}

void handleWsMessage(void *arg, uint8_t *data, size_t len)
{
  String js;
  AwsFrameInfo *info = (AwsFrameInfo*)arg;
  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
  {
    data[len] = 0;
    DEBUGln((char*)data);
    String cmd = getValue((char*)data, ';', 0);
    String p1 = getValue((char*)data, ';', 1);
    String p2 = getValue((char*)data, ';', 2);
    String p3 = getValue((char*)data, ';', 3);
    if (cmd == "deldev")
    {
      Hub.delDevice(p1.toInt());
      docJson.clear();
      docJson["cmd"] = "remove";
      docJson["#form_"] = p1;
      serializeJson(docJson, js);
      sendWS(js);
      docJson.clear();
      docJson["cmd"] = "remove";
      docJson["#mon_data_"] = p1;
      serializeJson(docJson, js);
      sendWS(js);
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
      CDevice* dev = Hub.getDeviceByAddress(oldAdr);
      if (dev && (oldAdr != newAdr))
      {
        docJson.clear();
        docJson["cmd"] = "remove";
        docJson["#form_"] = String(oldAdr);
        serializeJson(docJson, js);
        sendWS(js);
        //
        docJson.clear();
        docJson["cmd"] = "remove";
        docJson["#mon_data_"] = String(oldAdr);
        serializeJson(docJson, js);
        sendWS(js);
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
          sendWS(js);
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
          sendWS(js);
        }
      }
    }
    if (cmd == "endpairing")
    {
      uint8_t address = p1.toInt();
      DEBUGf("Stop pairing for device %d\n", address);
      if (Hub.endPairing(address))
      {
        notifyDevicePairing(address);
        //
        notifyDeviceMonitor(address);
        CDevice* dev = Hub.getDeviceByAddress(address);
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
    if (cmd == "buttonmon")
    {
      CEntity* ent = Hub.getEntityById(p1.toInt(), p2.toInt());
      if (ent) {
        MLiotComm.publishText(p1.toInt(), uid, ent->getId(), "!");
      }
    }
    if (cmd == "inputmon")
    {
      CEntity* ent = Hub.getEntityById(p1.toInt(), p2.toInt());
      if (ent) {
        float f = p3.toFloat() * ent->getNumberDiv();
        MLiotComm.publishFloat(p1.toInt(), uid, ent->getId(), f, ent->getNumberDiv());
      }
    }
    if (cmd == "switchmon")
    {
      CEntity* ent = Hub.getEntityById(p1.toInt(), p2.toInt());
      if (ent) {
        MLiotComm.publishSwitch(p1.toInt(), uid, ent->getId(), p3.toInt());
      }
    }
    if (cmd == "selectmon")
    {
      CEntity* ent = Hub.getEntityById(p1.toInt(), p2.toInt());
      if (ent) {
        MLiotComm.publishText(p1.toInt(), uid, ent->getId(), p3.c_str());
      }
    }
    if (cmd == "plot")
    {
      docJson.clear();
      docJson["cmd"] = "plotdata";
      docJson["datas"] = Hub.getPlotData(p1.toInt(), p2.toInt());
      String js;
      serializeJson(docJson, js);
      sendWS(js);
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
      Hub.MQTTconnect();
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
    DEBUG( "." ); delay( 500 );
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
  byteArrayToStr(HAuniqueId, mac, 6);

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
    DEBUGf("\nMDNS on %s\n", AP_ssid);
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
        html.replace("%LORAOK%", loraOK ? "" : "<i style='color:#FF0000'>Error</i>");
        html.replace("%VERSION%", VERSION);
        //
        html.replace("%WIFISSID%", Wifi_ssid);
        html.replace("%WIFIPASS%", Wifi_pass);
        html.replace("%MQTTHOST%", mqtt_host);
        html.replace("%MQTTUSER%", mqtt_user);
        html.replace("%MQTTPASS%", mqtt_pass);
        html.replace("%TZ%", datetimeTZ);
        html.replace("%NTP%", datetimeNTP);
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
      // exemple : dev_20_address
      //      or : dev_10_child_2_label
      String valdev = getValue(str, '_', 0);
      String keydev = getValue(str, '_', 1);
      String attrdev = getValue(str, '_', 2);
      String keychild = getValue(str, '_', 3);
      String attrchild = getValue(str, '_', 4);
      String sval(p->value().c_str());

      DEBUGf("[%s][%s][%s][%s][%s] = %s\n", valdev.c_str(), keydev.c_str(), attrdev.c_str(), keychild.c_str(), attrchild.c_str(), sval.c_str());
      if (valdev == "dev" && keydev.toInt() > 0)
      {
        dev = Hub.getDeviceByAddress(keydev.toInt());
        if (dev) {
          if (attrdev == "childs") {
            ent = Hub.getEntityById(keydev.toInt(), keychild.toInt());
            if (ent) {
              ent->setAttr(attrchild, sval, 0);
            }
          }
        }
      }
    }
    //
    Hub.saveConfig();
  }
  if (request->hasParam("cnfcode", true))
  {
    uid = request->getParam("cnfcode", true)->value().toInt();
    AP_ssid[strlen(AP_ssid) - 1] = request->getParam("cnfcode", true)->value().c_str()[0];
    EEPROM.writeChar(EEPROM_DATA_UID, AP_ssid[strlen(AP_ssid) - 1]);
    if (request->hasParam("cnffreq", true))
    {
      RadioFreq = request->getParam("cnffreq", true)->value().toInt();
      EEPROM.writeUShort(EEPROM_DATA_FREQ, RadioFreq);
    }
    if (request->hasParam("cnfdist", true))
    {
      RadioRange = request->getParam("cnfdist", true)->value().toInt();
      EEPROM.writeByte(EEPROM_DATA_RANGE, RadioRange);
    }
    // Wifi
    if (request->hasParam("wifissid", true))
    {
      strcpy(Wifi_ssid, request->getParam("wifissid", true)->value().c_str() );
      EEPROM.writeString(EEPROM_TEXT_OFFSET, Wifi_ssid);
    }
    if (request->hasParam("wifipass", true))
    {
      strcpy(Wifi_pass, request->getParam("wifipass", true)->value().c_str() );
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 1), Wifi_pass);
    }
    if (request->hasParam("mqtthost", true))
    {
      strcpy(mqtt_host, request->getParam("mqtthost", true)->value().c_str() );
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 2), mqtt_host);
    }
    if (request->hasParam("mqttuser", true))
    {
      strcpy(mqtt_user, request->getParam("mqttuser", true)->value().c_str() );
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 3), mqtt_user);
    }
    if (request->hasParam("mqtt_pass", true))
    {
      strcpy(mqtt_pass, request->getParam("mqttpass", true)->value().c_str() );
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 4), mqtt_pass);
    }
    if (request->hasParam("tz", true))
    {
      strcpy(datetimeTZ, request->getParam("tz", true)->value().c_str() );
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 5), datetimeTZ);
    }
    if (request->hasParam("ntp", true))
    {
      strcpy(datetimeNTP, request->getParam("ntp", true)->value().c_str() );
      EEPROM.writeString(EEPROM_TEXT_OFFSET + (EEPROM_TEXT_SIZE * 6), datetimeNTP);
    }

    EEPROM.commit();

    DEBUGln(uid);
    DEBUGln(AP_ssid);
    DEBUGln(RadioFreq);
    DEBUGln(Wifi_ssid);
    DEBUGln(Wifi_pass);
    DEBUGln(mqtt_host);
    DEBUGln(mqtt_user);
    DEBUGln(mqtt_pass);
    DEBUGln(datetimeTZ);
    DEBUGln(datetimeNTP);

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
      Update.printError(DSerial);
    }
  }
  if (Update.write(data, len) != len)
  {
    Update.printError(DSerial);
  }
  if (final)
  {
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true))
    {
      Update.printError(DSerial);
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
  server.serveStatic("/rec202510.json", SPIFFS, "/rec202510.json");
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
