#include <esp_task_wdt.h>
#include "bleUART.h"
#include "batBLEManager.h"
#include "batVestwoods.h"
#include "batDaly.h"
#include "batBMSManager.h"
#include "bat.h"
#include "batBalancer.h"
#include "batBank.h"
#include "batDerate.h"
#include "config.h"

#define BUZZER_PIN 27

#if 1
// my real bank

// Bluetooth UARTS
bleUART* myBLEs[] = {
  new bleUART("2b:80:03:b4:71:d9", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("2b:80:03:b4:76:0a", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("2b:80:03:b4:76:21", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("2b:80:03:b4:78:03", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("40:d6:3c:00:08:30", "fff0", "fff1", "fff2") // no idea why the lib only finds these by the short handles...
};
int bleCount = sizeof(myBLEs) / sizeof(myBLEs[0]);
// manager for device discovery
batBLEManager myBLEMan(myBLEs, bleCount, BAT_CFG_CONNECT_TIMEOUT);

// battery states
batBat* myBATs[] = {
  // 12v batteries
  new batBat(4), // 0
  new batBat(4), // 1
  new batBat(4), // 2
  new batBat(4), // 3
  // daly battery
  new batBat(16), // 4
  // 12v series bank
  new batBat(16), // 5
  // parallel bank
  new batBat(16) // 6
};
int batCount = sizeof(myBATs) / sizeof(myBATs[0]);
batBank myVestBank(myBATs, 4, myBATs[5], true);
batBank myAllBank(&myBATs[4], 2, myBATs[6], false);

batDerate myDalyDerate(myBATs[4]);
batDerate myVestDerate(myBATs[5]);

// balancer
balUnit myUnits[] = {
  balUnit(myBATs[0], 32),
  balUnit(myBATs[1], 33),
  balUnit(myBATs[2], 25),
  balUnit(myBATs[3], 26)
};
int balCount = sizeof(myUnits) / sizeof(myUnits[0]);
balBank myBal(myUnits, balCount, true);

// BMS drivers
batBMS* myBMSs[] = {
  new batDaly(myBLEs[4], myBATs[4]),
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

  // get balancer pins into sane state ASAP
  myBal.init();

  // bleep to indicate reboot
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  delay(1000);
  digitalWrite(BUZZER_PIN, HIGH);

  // initialise WDT next - expect some BLE tasks to take up to 5s, so let set it to 10...
  esp_task_wdt_init(10, true);
  esp_task_wdt_add(NULL);

  // initialise any other nominal battery state
  for(int i = 0; i < 4; i++) {
    myBATs[i]->nomVoltage = 12.8f;
    myBATs[i]->nomAH = 200.0f;
    myBATs[i]->nomChargeCurrent = 100.0f;
    myBATs[i]->nomChargeVoltage = 14.6f;
    myBATs[i]->nomDischargeCurrent = 100.0f;
  }
  myBATs[4]->nomVoltage = 51.2f;
  myBATs[4]->nomAH = 360.0f;
  myBATs[4]->nomChargeCurrent = 150.0f;
  myBATs[4]->nomChargeVoltage = 58.4f;
  myBATs[4]->nomDischargeCurrent = 200.0f;

  // initialise banks
  myVestBank.init();
  myAllBank.init();
}

// main loop
void loop() {
  esp_task_wdt_reset();
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

// disable until all bats implemented
  // if any battery data is waaayyyy too old? reboot
  for(int i = 0; i < batCount ; i++) {
    if(millis() - myBATs[i]->updateMillis > BAT_CFG_STATE_TIMEOUT) {
      Serial.println("BATTERY STATE WAY TOO OLD - TRY REBOOT!");
      esp_restart();
      while(1) {}
    }
  }

  // now run balancer
  myBal.run();

  // and battery banks
  // 4 battery vestwoods:
  myVestBank.run();

  // derate
  myDalyDerate.run();
  myVestDerate.run();
  // parallel bank
  myAllBank.run();
  // can output
}

#endif

#if 0
// daly test
// Bluetooth UARTS
bleUART* myBLEs[] = {
  new bleUART("40:d6:3c:00:08:30", "fff0", "fff1", "fff2") // no idea why the lib only finds these by the short handles...
};
int bleCount = sizeof(myBLEs) / sizeof(myBLEs[0]);
// manager for device discovery
batBLEManager myBLEMan(myBLEs, bleCount, BAT_CFG_CONNECT_TIMEOUT);

// battery states
batBat* myBATs[] = {
  // daly battery
  new batBat(16)
};
int batCount = sizeof(myBATs) / sizeof(myBATs[0]);

batDerate myDalyDerate(myBATs[0]);

// BMS drivers
batBMS* myBMSs[] = {
  new batDaly(myBLEs[0], myBATs[0])
};
int bmsCount = sizeof(myBMSs) / sizeof(myBMSs[0]);
int bmsNum;
// manager for group of bms
batBMSManager myBMSMan(myBMSs, bmsCount, BAT_CFG_POLL_TIME);

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  // bleep to indicate reboot
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  delay(1000);
  digitalWrite(BUZZER_PIN, HIGH);

  // initialise WDT next - expect some BLE tasks to take up to 5s, so let set it to 10...
  esp_task_wdt_init(10, true);
  esp_task_wdt_add(NULL);

  myBATs[0]->nomVoltage = 51.2f;
  myBATs[0]->nomAH = 360.0f;
  myBATs[0]->nomChargeCurrent = 150.0f;
  myBATs[0]->nomChargeVoltage = 58.4f;
  myBATs[0]->nomDischargeCurrent = 200.0f;
}

// main loop
void loop() {
  esp_task_wdt_reset();
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
  // now run derate algos  
  myDalyDerate.run();
}

#endif