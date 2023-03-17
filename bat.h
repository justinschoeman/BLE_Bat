#ifndef _BAT_H_
#define _BAT_H_

/* 
  basically just a data item representing a battery
  based on the basic information required for Pylon CAN protocol
*/

class batCell {
public:
  batCell(void) {
    voltage = -1.0f;
    temperature = -1.0f;
  }
  float voltage;
  float temperature;
};

class batBat {
public:
  // number of cells
  batBat(const char * name_, int num) : name(name_) {
    if(num < 1) num = 1; // sanity check
    numCells = num;
    cells = new batCell[num];
    nomVoltage = -1.0f;
    nomAH = -1.0f;
    nomChargeCurrent = -1.0f;
    nomChargeVoltage = -1.0f;
    nomDischargeCurrent = -1.0f;
    chargeCurrent = -1.0f;
    chargeVoltage = -1.0f;
    dischargeCurrent = -1.0f;
    soh = -1.0f;
    soc = -1.0f;
    voltage = -1.0f;
    current = -1.0f;
    temperature = -1.0f;
    minCellVoltage = -1.0f;
    maxCellVoltage = -1.0f;
    minCellVoltageNumber = -1;
    maxCellVoltageNumber = -1;
    updateMillis = 0UL;
    balancing = false;
  }

  ~batBat(void) {
    delete cells;
  }

  // return a unique identifier string for logging
  String &myId(void) { return name; }

  // id/name
  String name;

  // cell data - not used in pylontech can protocol, but collect for now anyway
  int numCells;
  batCell * cells;

  // nominal charge/discharge parameters
  // populate these if we are not manually derating, so we can detect when the battery derates itself
  float nomVoltage;
  float nomAH;
  float nomChargeCurrent;
  float nomChargeVoltage;
  float nomDischargeCurrent;
  // reported charge/discharge parameters
  // if these are not set, call a derate function to populate
  float chargeCurrent;
  float chargeVoltage;
  float dischargeCurrent;
  // current state
  float soh;
  float soc;
  float voltage;
  float current; // daly - discharge is positive!
  float temperature;
  // need these for derating
  float minCellVoltage;
  float maxCellVoltage;
  // not strictly required, but useful info - cell numbers
  int minCellVoltageNumber;
  int maxCellVoltageNumber;
  // so we can keep track of changes...
  unsigned long updateMillis;
  // we need to know if this pack is balancing, as current will be unreliable...
  bool balancing;

  // debug
  // l0 = only dynamic parameters
  // l1 = include cell voltages
  // l2 = include cell temperatures
  // l3 = include nominals
  void dump(int level = 0) {
    if(level > 2) {
      Serial.print(name);
      Serial.print("_nomVoltage = ");
      Serial.println(nomVoltage);
      Serial.print(name);
      Serial.print("_nomAH = ");
      Serial.println(nomAH);
      Serial.print(name);
      Serial.print("_nomChargeCurrent = ");
      Serial.println(nomChargeCurrent);
      Serial.print(name);
      Serial.print("_nomChargeVoltage = ");
      Serial.println(nomChargeVoltage);
      Serial.print(name);
      Serial.print("_nomDischargeCurrent = ");
      Serial.println(nomDischargeCurrent);
    }
    if(chargeCurrent != -1.0f) {
      Serial.print(name);
      Serial.print("_chargeCurrent = ");
      Serial.println(chargeCurrent);
    }    
    if(chargeVoltage != -1.0f) {
      Serial.print(name);
      Serial.print("_chargeVoltage = ");
      Serial.println(chargeVoltage);
    }
    if(dischargeCurrent != -1.0f) {
      Serial.print(name);
      Serial.print("_dischargeCurrent = ");
      Serial.println(dischargeCurrent);
    }
    if(soh != -1.0f) {
      Serial.print(name);
      Serial.print("_soh = ");
      Serial.println(soh);
    }      
    if(soc != -1.0f) {
      Serial.print(name);
      Serial.print("_soc = ");
      Serial.println(soc);
    }
    if(voltage != -1.0f) {
      Serial.print(name);
      Serial.print("_voltage = ");
      Serial.println(voltage, 3);
    }
    if(current != -1.0f) {
      Serial.print(name);
      Serial.print("_current = ");
      Serial.println(current, 3);
    }
    if(temperature != -1.0f) {
      Serial.print(name);
      Serial.print("_temperature = ");
      Serial.println(temperature);
    }
    Serial.print(name);
    Serial.print("_balancing = ");
    Serial.println(balancing);
    Serial.print(name);
    Serial.print("_minCellVoltageNumber = ");
    Serial.println(minCellVoltageNumber);
    if(minCellVoltage != -1.0f) {
      Serial.print(name);
      Serial.print("_minCellVoltage = ");
      Serial.println(minCellVoltage, 3);
    }
    Serial.print(name);
    Serial.print("_maxCellVoltageNumber = ");
    Serial.println(maxCellVoltageNumber);
    if(maxCellVoltage != -1.0f) {
      Serial.print(name);
      Serial.print("_maxCellVoltage = ");
      Serial.println(maxCellVoltage, 3);
    }
    for(int i = 0; level > 0 && i < numCells; i++) {
      if(cells[i].voltage != -1.0f) {
        Serial.print(name);
        Serial.print("_CellV_");
        Serial.print(i);
        Serial.print(" = ");
        Serial.println(cells[i].voltage, 3);
      }
    }
    for(int i = 0; level > 1 && i < numCells; i++) {
      if(cells[i].temperature != -1.0f) {
        Serial.print(name);
        Serial.print("_CellT_");
        Serial.print(i);
        Serial.print(" = ");
        Serial.println(cells[i].temperature);
      }
    }
  }
};

#endif
