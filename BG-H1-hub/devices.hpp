void notifyDeviceForm(uint8_t);
void notifyDeviceHeader(uint8_t);
void notifyDevicePairing(uint8_t);
void notifyChildLine(uint8_t, uint8_t);
void notifyDeviceMonitor(uint8_t);
void notifyState(uint8_t, uint8_t, String );
void MQTTcallback(char* topic, byte* payload, unsigned int length);

enum EntityCategory {
  CategoryAuto = 0,
  CategoryConfig,
  CategoryDiagnostic
};

class CEntity {
  public:
    explicit CEntity(uint8_t adr, const char* devName, JsonVariant JSconf)
    {
      _devAdr = adr;
      _devName = devName;
      _nextEntity = nullptr;
      _JSconf = JSconf;
      _decim = 0;
      _state = nullptr;
      _tsState = 0;
      //DEBUGf("NEW   entity %s\n", getLabel());
    }
    ~CEntity()
    {
      DEBUGf("Delete entity %s\n", getLabel());
      // TODO cleanConfig();
    }
    CEntity* Next()
    {
      return _nextEntity;
    }
    void setNext(CEntity* ent)
    {
      _nextEntity = ent;
    }
    void setAttr(String attr, String newValue, uint8_t idx)
    {
      // if name changed, HA configuration will be erased
      // it need to be reconfigured after
      if (attr == "label" && strcmp(newValue.c_str(), _JSconf["label"].as<const char*>()) != 0)
      {
        // TODO cleanConfig();
      }
      if (isValidInt(newValue))
      {
        _JSconf[attr] = newValue.toInt();
      } else if (isValidFloat(newValue))
      {
        _JSconf[attr] = serialized(String(newValue.toFloat(), 3));
      } else
      {
        if (idx) {
          String tmp = _JSconf[attr];
          _JSconf[attr] = tmp + "," + newValue;
        } else {
          _JSconf[attr] = newValue;
        }
      }
    }
    uint8_t getId()
    {
      return _JSconf["id"].as<int>();
    }
    const char* getLabel()
    {
      return _JSconf["label"].as<const char*>();
    }
    rl_element_t getElementType()
    {
      return (rl_element_t)_JSconf["sensortype"].as<int>();
    }
    rl_data_t getDataType()
    {
      return (rl_data_t)_JSconf["datatype"].as<int>();
    }
    const char* getUnit()
    {
      if (_JSconf["unit"].is<const char*>())
        return _JSconf["unit"].as<const char*>();
      else
        return "";
    }
    const char* getClass()
    {
      if (_JSconf["class"].is<const char*>())
        return _JSconf["class"].as<const char*>();
      else
        return "";
    }
    EntityCategory getCategory()
    {
      EntityCategory ec = EntityCategory::CategoryAuto;
      if (_JSconf["category"].is<int>()) {
        ec = (EntityCategory)(_JSconf["category"].as<int>() % 3);
      }
      return ec;
    }
    int32_t getMini()
    {
      int32_t mini(LONG_MIN);
      if (_JSconf["min"].is<int>())
        mini = _JSconf["min"].as<int>();
      return mini;
    }
    int32_t getMaxi()
    {
      int32_t maxi(LONG_MAX);
      if (_JSconf["max"].is<int>())
        maxi = _JSconf["max"].as<int>();
      return maxi;
    }
    float getCoefA()
    {
      float coefA = 1.;
      if (_JSconf["coefa"].is<float>())
        coefA = _JSconf["coefa"];
      return coefA;
    }
    float getCoefB()
    {
      float coefB = 0.;
      if (_JSconf["coefb"].is<float>())
        coefB = _JSconf["coefb"];
      return coefB;
    }
    const char* getSelectOptions()
    {
      return _JSconf["options"].as<const char*>();
    }
    int32_t getNumberMin()
    {
      int32_t imin(LONG_MIN);
      if (_JSconf["imin"].is<int>())
        imin = _JSconf["imin"].as<int>();
      return imin;
    }
    int32_t getNumberMax()
    {
      int32_t imax(LONG_MAX);
      if (_JSconf["imax"].is<int>())
        imax = _JSconf["imax"].as<int>();
      return imax;
    }
    int16_t getNumberDiv()
    {
      int32_t idiv(1);
      if (_JSconf["idiv"].is<int>())
        idiv = _JSconf["idiv"].as<int>();
      return idiv;
    }
    void setNumberConfig(int32_t mini, int32_t maxi, int16_t divider)
    {
      _JSconf["imin"] = mini;
      _JSconf["imax"] = maxi;
      _JSconf["idiv"] = divider;
      DEBUGf("   %d : mM=%d %d %d\n", _JSconf["id"].as<int>(), mini, maxi, divider);
    }
    bool setState(uint32_t ts, int32_t newState)
    {
      if (ts < _tsState) return false;
      if (!_state)
      {
        _state = new int32_t(newState);
      }
      _decim = 0;
      *reinterpret_cast<int32_t*>(_state) = newState;
      notifyState( _devAdr, getId(), String(newState) );
      _tsState = ts;
      return true;
    }
    bool setState(uint32_t ts, bool newState)
    {
      if (ts < _tsState) return false;
      if (!_state)
      {
        _state = new bool(newState);
      }
      _decim = 0;
      *reinterpret_cast<bool*>(_state) = newState;
      if (getElementType() == E_BINARYSENSOR)
      {
        notifyState( _devAdr, getId(), newState ? "on" : "off" );
      } else
      {
        notifyState( _devAdr, getId(), String(newState));
      }
      _tsState = ts;
      return true;
    }
    bool setState(uint32_t ts, float newState, uint8_t divider)
    {
      if (ts < _tsState) return false;
      if (!_state)
      {
        _state = new float(newState);
      }
      _decim = 0;
      while (divider > 1) {
        _decim++;
        divider = divider / 10;
      }
      *reinterpret_cast<float*>(_state) = newState;
      notifyState( _devAdr, getId(), String(newState, (int)_decim) );
      _tsState = ts;
      return true;
    }
    bool setState(uint32_t ts, char* newState)
    {
      if (ts < _tsState) return false;
      if (!_state)
      {
        _state = malloc(MAX_PACKET_DATA_LEN + 1);
      }
      _decim = 0;
      strcpy(reinterpret_cast<char*>(_state), newState);
      notifyState( _devAdr, getId(), String(newState) );
      _tsState = ts;
      return true;
    }
    String getState()
    {
      if (_state) {
        rl_element_t et = getElementType();
        bool b = *reinterpret_cast<bool*>(_state);
        int32_t v = *reinterpret_cast<int32_t*>(_state);
        float f = *reinterpret_cast<float*>(_state);
        char* s = reinterpret_cast<char*>(_state);
        switch (et) {
          case E_BINARYSENSOR:
          case E_SWITCH:
          case E_LIGHT:
            return String(b ? "on" : "off");
            break;
          case E_NUMERICSENSOR:
          case E_INPUTNUMBER:
            if (getDataType() == D_NUM)
              return String(v);
            else
              return String(f, (int)_decim);
            break;
          case E_SELECT:
          case E_COVER:
          case E_TEXTSENSOR:
            return String(s);
            break;
          case E_TAG:
            return String(v, HEX);
            break;
        }
      }
      return "?";
    }
    void setDeviceAddress(uint8_t address)
    {
      _devAdr = address;
    }
    String getHAtopic()
    {
      String topic = String(HAConfigRoot) + "/";
      switch (getElementType())
      {
        case E_BINARYSENSOR:
          topic += "binary_sensor";
          break;
        case E_NUMERICSENSOR:
          topic += "sensor";
          break;
        case E_SWITCH:
          topic += "switch";
          break;
        case E_LIGHT:
          topic += "light";
          break;
        case E_COVER:
          topic += "cover";
          break;
        case E_FAN:
          topic += "fan";
          break;
        case E_HVAC:
          topic += "hvac";
          break;
        case E_SELECT:
          topic += "select";
          break;
        case E_TRIGGER:
          topic += "device_automation";
          break;
        case E_EVENT:
          topic += "event";
          break;
        case E_TAG:
          topic += "tag";
          // TODO
          break;
        case E_TEXTSENSOR:
          topic += "sensor";
          break;
        case E_INPUTNUMBER:
          topic += "number";
          break;
        case E_CUSTOM:
          break;
        case E_DATE:
          break;
        case E_TIME:
          break;
        case E_DATETIME:
          break;
        case E_BUTTON:
          topic += "button";
          break;
        case E_CONFIG:
          break;
        default:
          break;
      }
      if (getElementType() == E_TRIGGER)
      {
        topic += "/" + String(HAuniqueId) + "/action_single_" + strHAuid() + "/config";
      } else {
        topic += "/" + String(HAuniqueId) + "/" + strHAuid() + "/config";
      }
      return topic;
    }
    void onMessageMQTT(char* topic, char* payload)
    {
      String command = strHAprefix() + "/" + String(HACommandTopic);
      //DEBUGf("%s\n%s\n", topic, command.c_str());
      if (String(topic) == command)
      {
        switch (getElementType())
        {
          case E_SWITCH:
            if (strcmp(payload, "ON") == 0)
            {
              MLiotComm.publishSwitch(_devAdr, uid, getId(), 1);
            }
            if (strcmp(payload, "OFF") == 0)
            {
              MLiotComm.publishSwitch(_devAdr, uid, getId(), 0);
            }
            break;
          case E_LIGHT:
            break;
          case E_COVER:
            MLiotComm.publishText(_devAdr, uid, getId(), payload);
            break;
          case E_SELECT:
            MLiotComm.publishText(_devAdr, uid, getId(), payload);
            break;
          case E_EVENT:
            break;
          case E_INPUTNUMBER:
            MLiotComm.publishFloat(_devAdr, uid, getId(), String(payload).toFloat() * getNumberDiv(), getNumberDiv());
            break;
          case E_BUTTON:
            MLiotComm.publishText(_devAdr, uid, getId(), "!");
            break;
        }
      }
    }
    void publishConfig()
    {
      rl_element_t elType = getElementType();
      JsonDocument docJSon;
      JsonObject Jconfig = docJSon.to<JsonObject>();
      JsonObject device = Jconfig["device"].to<JsonObject>();

      String devUID = String(_devName);
      devUID.replace(" ", "_");

      device[HAIdentifiers] = String(AP_ssid) + "_" + devUID;
      device[HAManufacturer] = "M&L";
      device[HAName] = String(_devName);
      device[HAViaDevice] = "hub_" + String(HAuniqueId);

      Jconfig[HAName] = String(getLabel());
      Jconfig[HAUniqueID] = String(HAuniqueId) + "_" + strHAuid();
      if (elType != E_TRIGGER)
      {
        Jconfig[HAStateTopic] = strHAprefix() + "/" + String(HAStateTopic);
      }
      switch (elType)
      {
        case E_BINARYSENSOR:
          if (strlen(getClass())) {
            Jconfig[HADeviceClass] = getClass();
          }
          break;
        case E_NUMERICSENSOR:
          if (strlen(getClass())) {
            Jconfig[HADeviceClass] = getClass();
          }
          if (strlen(getUnit())) {
            Jconfig[HAUnitOfMeasurement] = getUnit();
          }
          if (strcmp(getClass(), "energy") == 0)
          {
            Jconfig["state_class"] = "total_increasing";
          }
          break;
        case E_SWITCH:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_LIGHT:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_COVER:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          Jconfig[HAStateOpen] = "open";
          Jconfig[HAStateOpening] = "opening";
          Jconfig[HAStateClosed] = "closed";
          Jconfig[HAStateClosing] = "closing";
          Jconfig[HAStateStopped] = "stopped";
          Jconfig[HAPayloadOpen] = "OPEN";
          Jconfig[HAPayloadClose] = "CLOSE";
          Jconfig[HAPayloadStop] = "STOP";
          break;
        case E_FAN:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_HVAC:
          break;
        case E_SELECT:
          if (strlen(getSelectOptions())) {
            char tmp[20] = {0};
            const char* p = getSelectOptions();
            uint8_t n = 0;
            while (*p) {
              if (*p == ',') {
                Jconfig["options"].add(tmp);
                n = 0;
              } else {
                tmp[n++] = *p;
              }
              p++;
            }
            Jconfig["options"].add(tmp);
          }
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_TRIGGER:
          Jconfig["atype"] = "trigger";
          Jconfig["payload"] = String(getLabel()); //"single";
          Jconfig["subtype"] = String(getLabel()); //"single";
          Jconfig["topic"] = strHAprefix() + "/action";
          Jconfig["type"] = "action";
          break;
        case E_EVENT:
          break;
        case E_TAG:
          // TODO
          break;
        case E_TEXTSENSOR:
          break;
        case E_INPUTNUMBER:
          Jconfig["min"] = serialized(String((float)getNumberMin() / getNumberDiv(), 3));
          Jconfig["max"] = serialized(String((float)getNumberMax() / getNumberDiv(), 3));
          Jconfig["step"] = serialized(String(1. / getNumberDiv(), 1));
          if (strlen(getUnit())) {
            Jconfig[HAUnitOfMeasurement] = getUnit();
          }
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_CUSTOM:
          break;
        case E_DATE:
          break;
        case E_TIME:
          break;
        case E_DATETIME:
          break;
        case E_BUTTON:
          Jconfig[HACommandTopic] = strHAprefix() + "/" + String(HACommandTopic);
          break;
        case E_CONFIG:
          DEBUGf("E_CONFIG not available %s\n", getLabel());
          break;
        default:
          DEBUGf("Incorrect sensorType %s\n", getLabel());
          break;
      }
      if (getCategory() == CategoryDiagnostic)
      {
        Jconfig[HAEntityCategory] = "diagnostic";
      }
      if (getCategory() == CategoryConfig)
      {
        Jconfig[HAEntityCategory] = "config";
      }
      String topic = getHAtopic();
      //DEBUGln(topic);

      String payload;
      size_t Lres = serializeJson(docJSon, payload);
      //DEBUGln(payload);
      mqtt.beginPublish(topic.c_str(), Lres, true);
      mqtt.write((const uint8_t*)(payload.c_str()), Lres);
      mqtt.endPublish();
    }
    void publishState()
    {
      String topic = "";
      String payload = "";
      bool retained = true;
      rl_element_t elType = getElementType();
      rl_data_t elData = getDataType();

      topic = strHAprefix();
      (elType == E_TRIGGER) ? topic += "/action" : topic += "/" + String(HAStateTopic);

      switch (elType)
      {
        case E_BINARYSENSOR:
          payload = *reinterpret_cast<bool*>(_state) > 0 ? HAStateOn : HAStateOff;
          break;
        case E_NUMERICSENSOR:
          if (elData == D_FLOAT)
          {
            payload = String(*reinterpret_cast<float*>(_state));
          } else {
            payload = String(*reinterpret_cast<int32_t*>(_state));
          }
          break;
        case E_SWITCH:
          payload = *reinterpret_cast<bool*>(_state) > 0 ? HAStateOn : HAStateOff;
          break;
        case E_LIGHT:
          break;
        case E_COVER:
          payload = String(reinterpret_cast<char*>(_state));
          break;
        case E_FAN:
          break;
        case E_HVAC:
          break;
        case E_SELECT:
          payload = String(reinterpret_cast<char*>(_state));
          break;
        case E_TRIGGER:
          payload = String(getLabel());
          retained = false;
          break;
        case E_EVENT:
          payload = "press";
          break;
        case E_TAG:
          break;
        case E_TEXTSENSOR:
          payload = String(reinterpret_cast<char*>(_state));
          break;
        case E_INPUTNUMBER:
          if (elData == D_FLOAT)
          {
            payload = String(*reinterpret_cast<float*>(_state));
          } else {
            payload = String(*reinterpret_cast<int32_t*>(_state));
          }
          break;
        case E_CUSTOM:
          break;
        case E_DATE:
          break;
        case E_TIME:
          break;
        case E_DATETIME:
          break;
        case E_BUTTON:
          break;
      }
      if (payload != "")
      {
        DEBUGf("Publish : %s\n", payload.c_str());
        mqtt.beginPublish(topic.c_str(), payload.length(), retained);
        mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
        mqtt.endPublish();
      }
    }
    String strHAuid()
    {
      String devUID = String(_devName);
      devUID.replace(" ", "_");
      String eltUID = String(getLabel());
      eltUID.replace(" ", "_");
      return devUID + "_" + eltUID;
    }
    String strHAprefix()
    {
      return String(HAlora2ha) + "/" + String(HAuniqueId) + "/" + strHAuid();
    }

  private:
    uint8_t _devAdr;
    const char* _devName;
    CEntity* _nextEntity;
    JsonVariant _JSconf;
    void* _state; // typed pointer : uint32_t, bool, float, char[]
    uint8_t _decim;
    uint32_t _tsState;
};

class CDevice {
  public:
    explicit CDevice(JsonVariant JSconf)
    {
      _nextDevice = nullptr;
      _JSconf = JSconf;
      _pairing = false;
      _firstEntity = nullptr;
      //DEBUGf("NEW device %s\n", getName());
      // walk childs array
      if (JSconf["childs"].is<JsonArray>())
      {
        JsonVariant entities = JSconf["childs"];
        for (JsonVariant entityItem : entities.as<JsonArray>())
        {
          if (!entityItem.isNull())
          {
            newEntity(entityItem);
          }
        }
      }
    }
    ~CDevice()
    {
      DEBUGf("Delete device %s\n", getName());
      while (_firstEntity)
      {
        CEntity* ent = _firstEntity;
        _firstEntity = ent->Next();
        delete ent;
      }
    }
    CDevice* Next() {
      return _nextDevice;
    }
    void setNext(CDevice* dev)
    {
      _nextDevice = dev;
    }
    uint8_t getAddress()
    {
      return _JSconf["address"].as<int>();
    }
    void setAddress(uint8_t oldAdr, uint8_t newAdr)
    {
      rl_configParam_t cnfp;
      memset(&cnfp, 0, sizeof(cnfp));
      cnfp.childID = 0; // 0 for device param
      cnfp.pInt = newAdr;
      // Send new address to Device
      MLiotComm.publishConfig(oldAdr, uid, (rl_configs_t*)&cnfp, C_PARAM);
      _JSconf["address"] = newAdr;
    }
    const char* getName()
    {
      return _JSconf["name"].as<const char*>();
    }
    const char* getModel()
    {
      return _JSconf["model"].as<const char*>();
    }
    void setModel(const char* model)
    {
      _JSconf["model"] = String(model);
    }
    JsonVariant addEmptyEntity()
    {
      return _JSconf["childs"].add<JsonObject>();
    }
    CEntity* newEntity(JsonVariant JSconf)
    {
      CEntity* newent = new CEntity(getAddress(), getName(), JSconf);
      if (_firstEntity == nullptr)
      {
        _firstEntity = newent;
      } else
      {
        CEntity* ent = _firstEntity;
        while (ent)
        {
          if (ent->Next() == nullptr)
          {
            ent->setNext(newent);
            return newent;
          }
          ent = ent->Next();
        }
      }
      return newent;
    }
    CEntity* getEntityById(uint8_t id)
    {
      CEntity* ent = _firstEntity;
      while (ent)
      {
        if (ent->getId() == id)
        {
          return ent;
        }
        ent = ent->Next();
      }
      return nullptr;
    }
    CEntity* walkEntity(CEntity* ent)
    {
      if (ent) {
        return ent->Next();
      }
      return _firstEntity;
    }
    bool getPairing()
    {
      return _pairing;
    }
    void setPairing(bool pairing)
    {
      _pairing = pairing;
    }
    void onMessageMQTT(char* topic, char* payload)
    {
      CEntity* ent = nullptr;
      while ((ent = walkEntity(ent)))
      {
        ent->onMessageMQTT(topic, payload);
      }
    }
    void publishConfig()
    {
      String payload;
      String topic;
      size_t Lres;
      DEBUGf("Publish config %s\n", getName());
      // HUB configuration
      JsonDocument docJSon;
      JsonObject Jconfig = docJSon.to<JsonObject>();
      JsonObject device = Jconfig["device"].to<JsonObject>();
      String devUID = String(getName());
      devUID.replace(" ", "_");
      device[HAIdentifiers] = String(AP_ssid) + "_" + devUID;
      device[HAManufacturer] = "M&L";
      device[HAName] = String(getName());
      device[HAModel] = String(getModel());
      device[HAViaDevice] = "hub_" + String(HAuniqueId);

      Jconfig[HAUniqueID] = String(HAuniqueId) + "_" + devUID + "_linkquality";
      Jconfig[HAName] = "Link quality";
      Jconfig[HAStateTopic] = String(HAlora2ha) + "/" + String(HAuniqueId) + "/" + devUID + "/linkquality/" + String(HAState);
      Jconfig[HAEntityCategory] = "diagnostic";
      Jconfig[HAUnitOfMeasurement] = "lqi";
      Jconfig[HAIcon] = "mdi:signal";

      topic = String(HAConfigRoot) + "/" + String(HAComponentSensor) + "/" + String(HAuniqueId) + "/" + devUID + "_linkquality/config";

      Lres = serializeJson(docJSon, payload);
      mqtt.beginPublish(topic.c_str(), Lres, true);
      mqtt.write((const uint8_t*)(payload.c_str()), Lres);
      mqtt.endPublish();
      //
      CEntity* ent = nullptr;
      while ((ent = walkEntity(ent)))
      {
        ent->publishConfig();
      }
    }
  private:
    CDevice* _nextDevice;
    JsonVariant _JSconf;
    CEntity* _firstEntity;
    bool _pairing;
};

class CHub {
  public:
    explicit CHub()
    {
      _deviceFirst = nullptr;
      _oldMQTTconnected = false;
      saveRecorderNeeded = false;
    }
    CDevice* newDevice(JsonVariant JSconf)
    {
      CDevice* newdev = new CDevice(JSconf);
      if (_deviceFirst == nullptr)
      {
        _deviceFirst = newdev;
      } else
      {
        CDevice* dev = _deviceFirst;
        while (dev)
        {
          if (dev->Next() == nullptr)
          {
            dev->setNext(newdev);
            return newdev;
          }
          dev = dev->Next();
        }
      }
      return newdev;
    }
    void delDevice(uint8_t address)
    {
      CDevice* dev = getDeviceByAddress(address);
      if (dev)
      {
        if (dev == _deviceFirst)
        {
          _deviceFirst = dev->Next();
        } else
        {
          CDevice* wDev = _deviceFirst;
          while (wDev && (wDev->Next() != dev))
          {
            wDev = wDev->Next();
          }
          if (wDev)
          {
            wDev->setNext(dev->Next());
          }
        }
        delete dev;
        //
        JsonArray devs = _configJson["dev"];
        for (JsonArray::iterator it = devs.begin(); it != devs.end(); ++it)
        {
          if ((*it)["address"] == address)
          {
            devs.remove(it);
          }
        }
      }
      saveConfig();
    }
    CDevice* getDeviceByAddress(uint8_t address)
    {
      CDevice* dev = _deviceFirst;
      while (dev)
      {
        if (dev->getAddress() == address)
        {
          return dev;
        }
        dev = dev->Next();
      }
      return nullptr;
    }
    CEntity* getEntityById(uint8_t address, uint8_t id)
    {
      CDevice* dev = getDeviceByAddress(address);
      if (dev)
      {
        return dev->getEntityById(id);
      }
      return nullptr;
    }
    CDevice* walkDevice(CDevice* dev)
    {
      if (dev) {
        return dev->Next();
      }
      return _deviceFirst;
    }
    bool endPairing(uint8_t address)
    {
      CDevice* dev = getDeviceByAddress(address);
      if (dev)
      {
        rl_configParam_t cnfp;
        memset(&cnfp, 0, sizeof(cnfp));
        DEBUGf("Send pairing done for %d\n", address);
        MLiotComm.publishConfig(address, uid, (rl_configs_t*)&cnfp, C_END);
        dev->setPairing(false);
        return true;
      }
      DEBUGf("Device %d not found\n", address);
      return false;
    }
    void loraToState(rl_packet_t* cp)
    {
      char txt[MAX_PACKET_DATA_LEN + 1];
      char key[8];
      DateTime dtRec = DateTime(rtc.year(), rtc.month(), rtc.day(), rtc.hour(), 0, 0);
      uint32_t ts = dtRec.unixtime();
      CEntity* ent = getEntityById(cp->senderID, cp->childID);
      if (ent)
      {
        sprintf(key, "%02d_%02d", cp->senderID, cp->childID);
        rl_element_t et = ent->getElementType();
        DEBUGf("state %d:%d(%d) = %d/%d\n", cp->senderID, cp->childID, (int)et, cp->data.num.value, cp->data.num.divider);
        switch (et) {
          case E_BINARYSENSOR:
          case E_SWITCH:
          case E_LIGHT:
            ent->setState(ts, (bool)(cp->data.num.value > 0));
            break;
          case E_NUMERICSENSOR:
          case E_INPUTNUMBER:
            if (ent->getDataType() == D_NUM)
            {
              ent->setState(ts, cp->data.num.value);
            } else
            {
              ent->setState(ts, (float)cp->data.num.value / (float)cp->data.num.divider, cp->data.num.divider);
            }
            storeToRecorder(ts, cp->senderID, cp->childID, (float)cp->data.num.value / (float)cp->data.num.divider);
            break;
          case E_TAG:
            ent->setState(ts, (int32_t)cp->data.tag.tagL);
            break;
          case E_COVER:
          case E_SELECT:
          case E_TEXTSENSOR:
            strncpy(txt, cp->data.text, MAX_PACKET_DATA_LEN);
            txt[MAX_PACKET_DATA_LEN] = 0;
            ent->setState(ts, txt);
            break;
          case E_TRIGGER:
            // TODO : Action on Hub ?
            break;
        }
        //
        ent->publishState();
      }
    }
    void pairingConfig(rl_packet_t* cp)
    {
      CDevice* dev = getDeviceByAddress(cp->senderID);
      DEBUGf("getDeviceByAddress for %d : %d\n", cp->senderID, (long)dev);
      CEntity* ent = getEntityById(cp->senderID, cp->data.configs.base.childID);
      rl_conf_t cnfIdx = (rl_conf_t)(cp->sensordataType & 0x07);
      if (cnfIdx == C_BASE)
      { // Basic configuration
        rl_configBase_t* cnfb = (rl_configBase_t*)&cp->data.configs.base;
        rl_element_t st = (rl_element_t)cnfb->deviceType;
        rl_data_t dt = (rl_data_t)cnfb->dataType;
        DEBUGf("Conf B %d %s\n", cp->senderID, cnfb->name);
        if (cnfb->childID == RL_ID_CONFIG)
        { // process Device config
          if (dev) {
            // remove existing device
            delDevice(cp->senderID);
            dev = nullptr;
          }
          JsonVariant deviceItem = _configJson["dev"].as<JsonArray>().add<JsonVariant>();
          deviceItem["address"] = cp->senderID;
          deviceItem["name"] = String(cnfb->name);
          deviceItem["model"] = "?";
          //
          dev = newDevice(deviceItem);
          if (dev) {
            DEBUGf("Added Device %d %s\n", cp->senderID, cnfb->name);
            dev->setPairing(true);
            notifyDeviceForm(cp->senderID);

      String Jres;
      size_t Lres = serializeJson(_configJson, Jres);
      DEBUGln(Jres);
          } else
          {
            DEBUGf("** Error adding new Device %d\n", cp->senderID);
            return;
          }
        } else
        { // process Entity config
          if (!dev)
          {
            DEBUGf("** Error, unable to find Device %d for Child %d\n", cp->senderID, cnfb->childID);
            return;
          }
          ent = dev->getEntityById(cnfb->childID);
          size_t srcMax = sizeof(cnfb->name);
          char cName[srcMax + 1];
          strncpy(cName, cnfb->name, srcMax);
          cName[srcMax] = 0;
          if (ent)
          { // modify Child config
            ent->setAttr("label", cName, 0);
          } else
          { // add new Child
            JsonVariant entityItem = dev->addEmptyEntity();
            entityItem["id"] = cnfb->childID;
            entityItem["label"] = String(cName);
            entityItem["sensortype"] = (int)st;
            entityItem["datatype"] = (int)dt;
            entityItem["unit"] = "";

            ent = dev->newEntity(entityItem);
            if (!ent)
            {
              DEBUGf("Error adding new Entity %d on Device %d\n", cnfb->childID, cp->senderID);
              return;
            }
          }
        }
      }
      if (cnfIdx == C_UNIT && dev && ent)
      {
        rl_configText_t* cnft = (rl_configText_t*)&cp->data.configs.text;
        uint8_t childID = cnft->childID;
        size_t srcMax = sizeof(cnft->text);
        char txt[srcMax + 1];
        strncpy(txt, cnft->text, srcMax);
        txt[srcMax] = 0;
        DEBUGf("Conf U %d %s\n", childID, txt);
        ent->setAttr("unit", txt, 0);
      }
      if (cnfIdx == C_OPTS && dev)
      {
        rl_configText_t* cnft = (rl_configText_t*)&cp->data.configs.text;
        uint8_t childID = cnft->childID;
        //uint8_t index = cnft->index;
        size_t srcMax = sizeof(cnft->text);
        char txt[srcMax + 1];
        strncpy(txt, cnft->text, srcMax);
        txt[srcMax] = 0;
        if (childID == RL_ID_CONFIG)
        {
          DEBUGf("Conf O %d %s\n", cp->senderID, txt);
          dev->setModel(txt);
        } else {
          DEBUGf("Conf O %d %s\n", childID, txt);
          if (ent)
          {
            ent->setAttr("options", txt, cnft->index);
          }
        }
      }
      if (cnfIdx == C_NUMS && dev && ent) {
        rl_configNums_t* cnfn = (rl_configNums_t*)&cp->data.configs.nums;
        uint8_t childID = cnfn->childID;
        DEBUGf("Conf N %d %d %d %d\n", childID, cnfn->divider, cnfn->mini, cnfn->maxi);
        if (ent) {
          if (cnfn->divider == 0) cnfn->divider = 1;
          ent->setNumberConfig(cnfn->mini, cnfn->maxi, cnfn->divider);
        }
      }
      if (cnfIdx == C_END && dev)
      {
        uint8_t address = dev->getAddress();
        if (cp->data.configs.base.childID == RL_ID_CONFIG)
        {
          DEBUGf("Conf END %d\n", address);
          notifyDeviceHeader(address);
          notifyDevicePairing(address);
        } else {
          uint8_t id = cp->data.configs.base.childID;
          DEBUGf("Conf END %d %d\n", address, id);
          notifyChildLine(address, id);
        }
      }
    }
    bool isMQTT()
    {
      return strlen(mqtt_host);
    }
    void MQTTconnect()
    {
      if (isMQTT())
      {
        DEBUGf("MQTT start connect(%s,%s,%s)\n", AP_ssid, mqtt_user, mqtt_pass);
        mqtt.setServer(mqtt_host, mqtt_port);
        mqtt.setCallback(MQTTcallback);
        mqtt.setBufferSize(1024);
        if (mqtt.connect(AP_ssid, mqtt_user, mqtt_pass))
        {
          DEBUGf("MQTT connect\n");
        } else {
          DEBUGf("MQTT NOT connected to %s:%d [%d]\n", mqtt_host, mqtt_port, mqtt.state());
        }
      }
    }
    bool MQTTreconnect()
    {
      if (mqtt.connect(AP_ssid, mqtt_user, mqtt_pass))
      {
        DEBUGf("MQTT reconnect\n");
        return true;
      } else {
        DEBUGf("MQTT fail to reconnected to %s:%d [%d]\n", mqtt_host, mqtt_port, mqtt.state());
      }
      return false;
    }
    void onMessageMQTT(char* topic, char* payload)
    {
      CDevice* dev = nullptr;
      while ((dev = walkDevice(dev)))
      {
        dev->onMessageMQTT(topic, payload);
      }
    }
    void processMQTT()
    {
      if (isMQTT())
      {
        static uint32_t lastTimeRetry = 0;
        if (mqtt.connected())
        {
          if (mqtt.loop())
          {
            if (!_oldMQTTconnected)
            {
              DEBUGln("MQTT connected");
              _oldMQTTconnected = true;
              publishConfig();
            }
            if (needPublishOnline) {
              publishOnline();
            }
          }
        } else {
          if (_oldMQTTconnected)
          {
            DEBUGln("MQTT disconnected");
            _oldMQTTconnected = false;
            needPublishOnline = true;
          }
          uint32_t now = millis();
          if (now - lastTimeRetry > 5000)
          {
            lastTimeRetry = now;
            if (MQTTreconnect())
            {
              lastTimeRetry = 0;
            }
          }
        }
      }
    }
    void publishOnline()
    {
      String topic = String(HAlora2ha) + "/" + String(HAuniqueId) + "/hub/availability/" + String(HAState);
      String payload = "ON";
      DEBUGln("Publish Online");
      mqtt.beginPublish(topic.c_str(), payload.length(), false);
      mqtt.write((const uint8_t*)(payload.c_str()), payload.length());
      mqtt.endPublish();
      needPublishOnline = false;
    }
    void publishConfig()
    {
      publishConfigAvail();
      // Devices configuration
      CDevice* dev = nullptr;
      while ((dev = walkDevice(dev)))
      {
        dev->publishConfig();
      }
      String command = String(HAlora2ha) + "/" + String(HAuniqueId) + "/#";
      DEBUGf("mqtt subscribe : %s\n", command.c_str());
      mqtt.subscribe(command.c_str());
    }
    void publishConfigAvail()
    {
      String payload;
      String topic;
      size_t Lres;
      // HUB configuration
      JsonDocument docJSon;
      JsonObject Jconfig = docJSon.to<JsonObject>();
      JsonObject device = Jconfig["device"].to<JsonObject>();
      device = fillPublishConfigDevice(device);

      Jconfig[HAObjectID] =  String(AP_ssid) + "_hub_connection_state";
      Jconfig[HAUniqueID] =  String(HAuniqueId) + "_hub_connection_state";
      Jconfig[HAName] = "Connection state";
      Jconfig[HADeviceClass] = "connectivity";
      Jconfig[HAEntityCategory] = "diagnostic";
      Jconfig[HAStateTopic] = String(HAlora2ha) + "/" + String(HAuniqueId) + "/hub/availability/" + String(HAState);
      Jconfig[HAExpireAfter] =  (Watchdog * 60) + 10; // 10min 10s, must publish state every 10 min (not retained)
      topic = String(HAConfigRoot) + "/" + String(HAComponentBinarySensor) + "/" + String(HAuniqueId) + "_HUB/connection_state/config";

      Lres = serializeJson(docJSon, payload);

      mqtt.beginPublish(topic.c_str(), Lres, true);
      mqtt.write((const uint8_t*)(payload.c_str()), Lres);
      mqtt.endPublish();
    }
    JsonObject fillPublishConfigDevice(JsonObject device)
    {
      device[HAIdentifiers] = "hub_" + String(HAuniqueId);
      device[HAManufacturer] = "M&L";
      device[HAName] = String(AP_ssid) + " HUB";
      device[HASwVersion] = VERSION;
#if defined(ARDUINO_LOLIN_S2_MINI)
      device[HAModel] = "MLH1";
#elif defined(ARDUINO_ESP32_POE_ISO)
      device[HAModel] = "MLH2";
#elif defined(ARDUINO_WT32_ETH01)
      device[HAModel] = "MLH3";
#elif defined(ARDUINO_D1_MINI32)
      device[HAModel] = "MLH4";
#elif defined(ARDUINO_TTGO_LoRa32_v21new)
      device[HAModel] = "MLH5";
#else
      device[HAModel] = "Custom";
#endif
      return device;
    }
    bool loadConfig()
    {
      DEBUGln("Loading config file");
      File file = SPIFFS.open("/config.json");
      if (!file || file.isDirectory())
      {
        DEBUGln("** Failed to open config file");
        return false;
      }
      DeserializationError error = deserializeJson(_configJson, file);
      file.close();
      if (error)
      {
        DEBUGln("failed to deserialize config file, new one");
        _configJson["dev"].to<JsonArray>();
      }
      // walk device array
      JsonVariant devices = _configJson["dev"];
      if (devices.is<JsonArray>())
      {
        for (JsonVariant deviceItem : devices.as<JsonArray>())
        {
          if (!deviceItem.isNull())
          {
            newDevice(deviceItem);
          }
        }
      }
      DEBUGln("Config file loaded");
      return true;
    }
    void saveConfig()
    {
      String Jres;
      size_t Lres = serializeJson(_configJson, Jres);
      DEBUGln(Jres);
      File file = SPIFFS.open("/config.json", "w");
      if (file)
      {
        file.write((byte*)Jres.c_str(), Lres);
        file.close();
        DEBUGln("Config file is saved");
      } else {
        DEBUGln("** Error saving Config file");
      }
    }
    void loadRecorder()
    {
      char fname[32];
      String Jres;
      CEntity* ent;

      int y = rtc.year();
      int m = rtc.month();
      sprintf(fname, "/rec%d%02d.json", y, m);
      DEBUGf("Load recorder %s\n", fname);
      //
      _recorderJson.clear();
      JsonArray dataArray;
      File file = SPIFFS.open(fname);
      if (file)
      {
        DeserializationError error = deserializeJson(_recorderJson, file);
        file.close();
        if (error)
        {
          DEBUGf("failed to deserialize recorder file %s\n", fname);
          dataArray = _recorderJson.to<JsonArray>();
        } else {
          dataArray = _recorderJson.as<JsonArray>();
        }
      } else {
        // try mounth-1
        m--;
        if (m == 0) {
          y--;
          m = 12;
        }
        sprintf(fname, "/rec%d%02d.json", y, m);
        File file = SPIFFS.open(fname);
        if (file)
        {
          DeserializationError error = deserializeJson(_recorderJson, file);
          file.close();
          if (error)
          {
            DEBUGf("failed to deserialize recorder file %s\n", fname);
            dataArray = _recorderJson.to<JsonArray>();
          } else {
            dataArray = _recorderJson.as<JsonArray>();
          }
        } else {
          dataArray = _recorderJson.to<JsonArray>();
        }
      }
    }

    void cleanRecorder()
    {
      DateTime olddt = DateTime(rtc.year(), rtc.month(), rtc.day(), rtc.hour(), 0, 0);
      uint32_t oldts = olddt.unixtime() - (2L * 86400L); // - 2 days
      JsonArray dataArray = _recorderJson.as<JsonArray>();
      long nbRemoved = 0;
      for (long i = dataArray.size() - 1; i >= 0; i--)
      {
        if (dataArray[i]["ts"].as<unsigned long>() < oldts)
        {
          dataArray.remove(i);
          nbRemoved++;
        }
      }
      if (nbRemoved) {
        DEBUGf("%d records deleted\n", nbRemoved);
      }
    }
    void storeToRecorder(uint32_t ts, int d, int c, float f)
    {
      char key[8];
      sprintf(key, "%02d_%02d", d, c);
      //
      saveRecorderNeeded = true;
      //
      JsonArray dataArray = _recorderJson.as<JsonArray>();
      //DEBUGf("store %d : %s\n", ts, key);
      JsonObject existingEntry;
      for (JsonObject entry : dataArray)
      {
        if (entry["ts"] == ts)
        {
          existingEntry = entry;
          break;
        }
      }
      if (existingEntry) {
        JsonObject sensors = existingEntry["dc"];
        if (!sensors) {
          sensors = existingEntry["dc"].to<JsonObject>();
        }
        sensors[key] = f;
      } else {
        JsonObject newEntry = dataArray.add<JsonObject>();
        newEntry["ts"] = ts;
        JsonObject sensors = newEntry["dc"].to<JsonObject>();
        sensors[key] = f;
      }
    }
    void saveRecorder()
    {
      char fname[32];
      String Jres;
      //
      if (!saveRecorderNeeded)
        return;
      //
      cleanRecorder();
      //
      sprintf(fname, "/rec%d%02d.json", rtc.year(), rtc.month());
      size_t Lres = serializeJson(_recorderJson, Jres);
      DEBUGln(Jres);

      File file = SPIFFS.open(fname, "w");
      if (file)
      {
        file.write((byte*)Jres.c_str(), Lres);
        file.close();
        DEBUGf("Recorder file is saved: %s\n", fname);
      } else {
        DEBUGln("** Error saving Recorder file");
      }

      saveRecorderNeeded = false;
    }
    String getPlotData(int d, int c)
    {
      char key[8];
      sprintf(key, "%02d_%02d", d, c);
      JsonDocument outJson;
      JsonArray outArray = outJson.to<JsonArray>();
      //
      JsonArray dataArray = _recorderJson.as<JsonArray>();
      for (long i = 0; i < dataArray.size(); i++)
      {
        uint32_t ts = dataArray[i]["ts"].as<unsigned long>();
        JsonObject item = dataArray[i]["dc"].as<JsonObject>();
        if (item[key].is<float>())
        {
          float f = item[key].as<float>();
          JsonObject outData = outArray.add<JsonObject>();
          outData["x"] = ts;
          outData["y"] = f;
        }
      }
      String js;
      serializeJson(outJson, js);
      DEBUGln(js);
      outJson.clear();
      return js;
    }
  private:
    CDevice* _deviceFirst;
    JsonDocument _configJson;
    JsonDocument _recorderJson;
    bool _oldMQTTconnected;
    bool saveRecorderNeeded;
};

CHub Hub;
