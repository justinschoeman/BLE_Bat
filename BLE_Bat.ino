
#include <ArduinoBLE.h>
#include "bleUART.h"
#include "batVestwoods.h"
#include "bat.h"

// Bluetooth UARTS
//bleUART myBLE("2b:80:03:b4:76:0a", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e");
bleUART myBLEs[] = {
  bleUART("2b:80:03:b4:71:d9", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  bleUART("2b:80:03:b4:76:0a", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  bleUART("2b:80:03:b4:76:21", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  bleUART("2b:80:03:b4:78:03", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e")
};

/*
2b:80:03:b4:78:03 '2531200220811107' 
21:29:09.393 -> Test: 2b:80:03:b4:71:d9
21:29:09.393 -> Not my device...
21:29:09.393 -> Found unhandled: 2b:80:03:b4:71:d9 '2531200220826074' 

40:d6:3c:00:08:30 (DL-40D63C000830)
*/

// battery states
bat myBATs[] = {
  bat(4),
  bat(4),
  bat(4),
  bat(4)
};

//batVestwoods myBAT(myBLE);
batVestwoods * myBMSs[] = {
  new batVestwoods(&myBLEs[0]),
  new batVestwoods(&myBLEs[1]),
  new batVestwoods(&myBLEs[2]),
  new batVestwoods(&myBLEs[3])
};
int batCount = sizeof(myBMSs) / sizeof(myBMSs[0]);
int batNum;

int runstate; // 0 = discovery, 1 = run
unsigned long runtime;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  if(!BLE.begin()) {
    Serial.println("BLE start failed!");
    while (1);
  }
  // start scan
  Serial.println("Scanning...");
  BLE.scan();
  runstate = 0;
}

// initial loop - device discovery
void loop_discover(void) {
  // discover more...
  while(BLEDevice peripheral = BLE.available()) {
    // discovered a peripheral, print out address, local name, and advertised service
    bool found = false;
    for(auto &i: myBLEs) {
      if(i.discoverDevice(peripheral)) {
        found = true;
        break;
      }
    }
    if(found) {
      // found one - check if we have all?
      for(auto &i: myBLEs) {
        Serial.print("discover test: ");
        Serial.println(i.myId());
        if(!i.isDiscovered()) {
          Serial.println("not found...");
          found = false;
          break;
        }
      }
      if(found) {
        Serial.println("all devices found - advancing to run state");
        BLE.stopScan();
        runstate = 1;
        runtime = millis();
        batNum = -1;
        return;
      }
      continue;
    }
    Serial.print("Found unhandled: ");
    Serial.print(peripheral.address());
    Serial.print(" (");
    Serial.print(peripheral.localName());
    Serial.println(")");
  }
  return;
}

bool isPolled;
// main loop
void loop() {
  // first wait until we discover all devices
  if(runstate == 0) {
    loop_discover();
    return;
  }
  // normal run - run battery engines
  for(int i = 0; i < batCount; i++) {
    if(myBMSs[i]->run() < 0) {
      Serial.print("Error running battery ");
      Serial.println(myBMSs[i]->myId());
    }
  }
#if 1
  // parallel poll
  // waiting for poll
  if(millis() - runtime < 1000UL) return;
  // advance poll timer
  runtime += 1000UL;
  // scan all batteries
  for(int i = 0; i < batCount; i++) {
    if(i) delay(50);
    if(!myBMSs[i]->poll()) {
      Serial.print("Poll failed: ");
      Serial.println(myBMSs[i]->myId());
    }
    // test
    //break;
  }
  Serial.println("polls sent");
#else
  // sequential poll
  if(batNum < 0) {
    // waiting for poll
    if(millis() - runtime < 1000UL) return;
    // timer expired - start poll and schedule next...
    batNum = 0;
    isPolled = 0;
    runtime += 1000UL;
  }
  if(!isPolled) {
    Serial.print("POLL: ");
    Serial.println(batNum);
    if(!myBMSs[batNum].poll()) {
      Serial.print("Poll failed: ");
      Serial.println(myBMSs[batNum].myId());
      // fall through and advance battery
    } else {
      Serial.println("poll sent");
      isPolled = true;
    }
  }
  if(isPolled) {
    // busy polling
    // fixme - timeout? or trust the battery driver?
    if(myBMSs[batNum].isBusy()) return;    
  }
  Serial.println("poll done");
  batNum++;
  isPolled = false;  
  if(batNum >= batCount) {
    Serial.println("poll complete");
    batNum = -1;
    if(millis() - runtime >= 1000UL) {
      Serial.println("WARNING - POLL OVERRUN!");
    }
  }
#endif
}
