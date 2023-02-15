/*
TODO:

restart ble channels after x hours (+ random fuzz)
x reboot processor if battery stale by more than y
wdt

*/

#include "bleUART.h"
#include "batBLEManager.h"
#include "batVestwoods.h"
#include "batBMSManager.h"
#include "bat.h"
#include "batBalancer.h"
#include "config.h"

// Bluetooth UARTS
bleUART* myBLEs[] = {
  new bleUART("2b:80:03:b4:71:d9", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("2b:80:03:b4:76:0a", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("2b:80:03:b4:76:21", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("2b:80:03:b4:78:03", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e")
};
int bleCount = sizeof(myBLEs) / sizeof(myBLEs[0]);
// manager for device discovery
batBLEManager myBLEMan(myBLEs, bleCount, BAT_CFG_CONNECT_TIMEOUT);

// battery states
batBat* myBATs[] = {
  // 12v batteries
  new batBat(4),
  new batBat(4),
  new batBat(4),
  new batBat(4)
  // 12v series bank
  // daly battery
  // parallel bank
};
int batCount = sizeof(myBATs) / sizeof(myBATs[0]);

// balancer
balUnit myUnits[] = {
  balUnit(myBATs[0], 32),
  balUnit(myBATs[1], 33),
  balUnit(myBATs[2], 25),
  balUnit(myBATs[3], 26)
};
int balCount = sizeof(myUnits) / sizeof(myUnits[0]);
balBank myBank(myUnits, balCount, true);

// BMS drivers
batBMS* myBMSs[] = {
  new batVestwoods(myBLEs[0], myBATs[0]),
  new batVestwoods(myBLEs[1], myBATs[1]),
  new batVestwoods(myBLEs[2], myBATs[2]),
  new batVestwoods(myBLEs[3], myBATs[3])
};
int bmsCount = sizeof(myBMSs) / sizeof(myBMSs[0]);
int bmsNum;
// manager for group of bms
batBMSManager myBMSMan(myBMSs, bmsCount, BAT_CFG_POLL_TIME);

void setup() {
  Serial.begin(115200);
  while (!Serial) {}
  myBank.init(); // get balancer pins into sane state ASAP
}

// main loop
void loop() {
  // run BLE manager to discover all required devices
  int i = myBLEMan.run();
  if(i < 0) {
    Serial.println("ERROR STARTING BLE OR CONNECTING DEVICES - TRY REBOOT!");
    esp_restart();
    while(1) {}
  }
  if(i == 0) return; // still discovering BLE devices
  // fall through when all devices discovered...

  // normal run - run bms engines
  myBMSMan.run();

  // if any battery data is waaayyyy too old? reboot
  for(int i = 0; i < batCount ; i++) {
    if(millis() - myBATs[i]->updateMillis > BAT_CFG_STATE_TIMEOUT) {
      Serial.println("BATTERY STATE WAY TOO OLD - TRY REBOOT!");
      esp_restart();
      while(1) {}
    }
  }

  // now run balancer
  myBank.run();
}
