void notifyDeviceForm(uint8_t);
void notifyDeviceHeader(uint8_t);
void notifyDevicePairing(uint8_t);
void notifyChildLine(uint8_t, uint8_t);
void notifyState( uint8_t, uint8_t, String );

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
    const char* getClass()
    {
      return _JSconf["class"].as<const char*>();
    }
    const char* getUnit()
    {
      return _JSconf["unit"].as<const char*>();
    }
    EntityCategory getCategory()
    {
      EntityCategory ec = EntityCategory::CategoryAuto;
      if (_JSconf["category"].is<int>()) {
        ec = (EntityCategory)_JSconf["category"].as<int>();
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
    bool setState(int32_t newState)
    {
      if (!_state)
      {
        _state = new int32_t(newState);
      }
      _decim = 0;
      *reinterpret_cast<int32_t*>(_state) = newState;
      notifyState( _devAdr, getId(), String(newState) );
      return true;
    }
    bool setState(bool newState)
    {
      return setState((int32_t)newState);
    }
    bool setState(float newState, uint8_t divider)
    {
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
      return true;
    }
    bool setState(char* newState)
    {
      if (!_state)
      {
        _state = malloc(MAX_PACKET_DATA_LEN + 1);
      }
      _decim = 0;
      strcpy(reinterpret_cast<char*>(_state), newState);
      notifyState( _devAdr, getId(), String(newState) );
      return true;
    }
    void setDeviceAddress(uint8_t address)
    {
      _devAdr = address;
    }
    String getState()
    {
      if (_state) {
        rl_element_t et = getElementType();
        int32_t v = *reinterpret_cast<int32_t*>(_state);
        float f = *reinterpret_cast<float*>(_state);
        char* s = reinterpret_cast<char*>(_state);
        switch (et) {
          case E_BINARYSENSOR:
          case E_SWITCH:
            return String(v > 0);
            break;
          case E_NUMERICSENSOR:
          case E_INPUTNUMBER:
            return String(f, (int)_decim);
            break;
          case E_SELECT:
          case E_TEXTSENSOR:
            return String(s);
            break;
        }
      }
      return "?";
    }

  private:
    uint8_t _devAdr;
    const char* _devName;
    CEntity* _nextEntity;
    JsonVariant _JSconf;
    void* _state; // typed pointer : uint32_t, float, char[]
    uint8_t _decim;
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
      MLiotComm.publishConfig(oldAdr, hubid, (rl_configs_t*)&cnfp, C_PARAM);
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
  private:
    CDevice* _nextDevice;
    JsonVariant _JSconf;
    CEntity* _firstEntity;
    bool _pairing;
};

class CRouter {
  public:
    explicit CRouter()
    {
      _deviceFirst = nullptr;
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
        JsonArray devs = _docJson["dev"];
        for (JsonArray::iterator it = devs.begin(); it != devs.end(); ++it)
        {
          if ((*it)["address"] == address) {
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
        MLiotComm.publishConfig(address, hubid, (rl_configs_t*)&cnfp, C_END);
        dev->setPairing(false);
        return true;
      }
      return false;
    }
    void displayState(rl_packet_t* cp)
    {
      CEntity* ent = getEntityById(cp->senderID, cp->childID);
      char txt[MAX_PACKET_DATA_LEN + 1];
      if (ent)
      {
        rl_element_t et = ent->getElementType();
        DEBUGf("state %d:%d(%d) = %d/%d\n", cp->senderID, cp->childID, (int)et, cp->data.num.value, cp->data.num.divider);
        switch (et) {
          case E_BINARYSENSOR:
          case E_SWITCH:
            ent->setState(cp->data.num.value > 0);
            break;
          case E_NUMERICSENSOR:
          case E_INPUTNUMBER:
            ent->setState( (float)cp->data.num.value / (float)cp->data.num.divider, cp->data.num.divider);
            break;
          case E_SELECT:
          case E_TEXTSENSOR:
            strncpy(txt, cp->data.text, MAX_PACKET_DATA_LEN);
            txt[MAX_PACKET_DATA_LEN] = 0;
            ent->setState( txt );
            break;
        }
      }
    }
    void pairingConfig(rl_packet_t* cp)
    {
      CDevice* dev = getDeviceByAddress(cp->senderID);
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
          JsonVariant deviceItem = _docJson["dev"].as<JsonArray>().add<JsonVariant>();
          deviceItem["address"] = cp->senderID;
          deviceItem["name"] = String(cnfb->name);
          deviceItem["model"] = "?";
          //
          dev = newDevice(deviceItem);
          if (dev) {
            DEBUGf("Added Device %d %s\n", cp->senderID, cnfb->name);
            dev->setPairing(true);
            notifyDeviceForm(cp->senderID);
            // TODO monitor
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
    bool loadConfig()
    {
      DEBUGln("Loading config file");
      File file = SPIFFS.open("/config.json");
      if (!file || file.isDirectory())
      {
        DEBUGln("** Failed to open config file");
        return false;
      }
      DeserializationError error = deserializeJson(_docJson, file);
      file.close();
      if (error)
      {
        DEBUGln("failed to deserialize config file");
        return false;
      }
      // walk device array
      JsonVariant devices = _docJson["dev"];
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
      size_t Lres = serializeJson(_docJson, Jres);
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

  private:
    CDevice* _deviceFirst;
    JsonDocument _docJson;
};

CRouter Router;
