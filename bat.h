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
  batBat(int num) {
    if(num < 1) num = 1; // sanity check
    numCells = num;
    cells = new batCell[num];
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
  }

  ~batBat(void) {
    delete cells;
  }

  // cell data - not used in pylontech can protocol, but collect for now anyway
  int numCells;
  batCell * cells;

  // nominal charge/discharge parameters
  // populate these if we are not manually derating, so we can detect when the battery derates itself
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
  float current;
  float temperature;
  // need these for derating
  float minCellVoltage;
  float maxCellVoltage;
  // not strictly required, but useful info - cell numbers
  int minCellVoltageNumber;
  int maxCellVoltageNumber;
  // so we can keep track of changes...
  unsigned long updateMillis;

  // debug
  void dump(void) {
    Serial.print("nomChargeCurrent ");
    Serial.println(nomChargeCurrent);
    Serial.print("nomChargeVoltage ");
    Serial.println(nomChargeVoltage);
    Serial.print("nomDischargeCurrent ");
    Serial.println(nomDischargeCurrent);
    Serial.print("chargeCurrent ");
    Serial.println(chargeCurrent);
    Serial.print("chargeVoltage ");
    Serial.println(chargeVoltage);
    Serial.print("dischargeCurrent ");
    Serial.println(dischargeCurrent);
    Serial.print("soh ");
    Serial.println(soh);
    Serial.print("soc ");
    Serial.println(soc);
    Serial.print("voltage ");
    Serial.println(voltage);
    Serial.print("current ");
    Serial.println(current);
    Serial.print("temperature ");
    Serial.println(temperature);
    for(int i = 0; i < numCells; i++) {
      Serial.print("Cell ");
      Serial.print(i);
      Serial.print(" : ");
      Serial.print(cells[i].voltage);
      Serial.print("V ");
      Serial.print(cells[i].temperature);
      Serial.print("C");
      if(i == minCellVoltageNumber) {
        Serial.print(" min ");
        Serial.print(minCellVoltage);
      }
      if(i == maxCellVoltageNumber) {
        Serial.print(" max ");
        Serial.print(maxCellVoltage);
      }
      Serial.println();
    }
    //float minCellVoltage;
    //float maxCellVoltage;
    //int minCellVoltageNumber;
    //int maxCellVoltageNumber;
  }
};

#endif
