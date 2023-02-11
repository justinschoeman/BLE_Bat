
#include <ArduinoBLE.h>
#include "bleUART.h"
#include "batVestwoods.h"
#include "bat.h"
#include "batBalancer.h"

// Bluetooth UARTS
//bleUART myBLE("2b:80:03:b4:76:0a", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e");
bleUART* myBLEs[] = {
  new bleUART("2b:80:03:b4:71:d9", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("2b:80:03:b4:76:0a", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("2b:80:03:b4:76:21", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e"),
  new bleUART("2b:80:03:b4:78:03", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e")
};

/*
2b:80:03:b4:78:03 '2531200220811107' 
21:29:09.393 -> Test: 2b:80:03:b4:71:d9
21:29:09.393 -> Not my device...
21:29:09.393 -> Found unhandled: 2b:80:03:b4:71:d9 '2531200220826074' 

40:d6:3c:00:08:30 (DL-40D63C000830)
*/

// battery states
batBat* myBATs[] = {
  new batBat(4),
  new batBat(4),
  new batBat(4),
  new batBat(4)
};
int batCount = sizeof(myBATs) / sizeof(myBATs[0]);

balUnit myUnits[] = {
  balUnit(myBATs[0], 32),
  balUnit(myBATs[1], 33),
  balUnit(myBATs[2], 25),
  balUnit(myBATs[3], 26)
};
int balCount = sizeof(myUnits) / sizeof(myUnits[0]);

balBank myBank(myUnits, balCount, true);

//batVestwoods myBAT(myBLE);
batVestwoods* myBMSs[] = {
  new batVestwoods(myBLEs[0], myBATs[0]),
  new batVestwoods(myBLEs[1], myBATs[1]),
  new batVestwoods(myBLEs[2], myBATs[2]),
  new batVestwoods(myBLEs[3], myBATs[3])
};
int bmsCount = sizeof(myBMSs) / sizeof(myBMSs[0]);
int bmsNum;

int runstate;  // 0 = discovery, 1 = run
unsigned long runtime;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  myBank.init(); // get balancer pins into sane state ASAP
  if (!BLE.begin()) {
    Serial.println("BLE start failed!");
    while (1)
      ;
  }
  // start scan
  Serial.println("Scanning...");
  BLE.scan();
  runstate = 0;
}

// initial loop - device discovery
void loop_discover(void) {
  // discover more...
  while (BLEDevice peripheral = BLE.available()) {
    // discovered a peripheral, print out address, local name, and advertised service
    bool found = false;
    for (auto i : myBLEs) {
      if (i->discoverDevice(peripheral)) {
        found = true;
        break;
      }
    }
    if (found) {
      // found one - check if we have all?
      for (auto i : myBLEs) {
        Serial.print("discover test: ");
        Serial.println(i->myId());
        if (!i->isDiscovered()) {
          Serial.println("not found...");
          found = false;
          break;
        }
      }
      if (found) {
        Serial.println("all devices found - advancing to run state");
        BLE.stopScan();
        runstate = 1;
        runtime = millis();
        bmsNum = -1;
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
  if (runstate == 0) {
    loop_discover();
    return;
  }
  // normal run - run battery engines
  for (int i = 0; i < bmsCount; i++) {
    if (myBMSs[i]->run() < 0) {
      Serial.print("Error running battery ");
      Serial.println(myBMSs[i]->myId());
    }
  }
  // now run balancer
  myBank.run();  
#if 1
  // parallel poll
  // waiting for poll
  if (millis() - runtime < 1000UL) return;
  // advance poll timer
  runtime += 1000UL;
  // scan all batteries
  for (int i = 0; i < bmsCount; i++) {
    if (i) delay(50);
    if (!myBMSs[i]->poll()) {
      Serial.print("Poll failed: ");
      Serial.println(myBMSs[i]->myId());
    }
    // test
    //break;
  }
  Serial.println("polls sent");
#else
  // sequential poll
  if (bmsNum < 0) {
    // waiting for poll
    if (millis() - runtime < 1000UL) return;
    // timer expired - start poll and schedule next...
    bmsNum = 0;
    isPolled = 0;
    runtime += 1000UL;
  }
  if (!isPolled) {
    Serial.print("POLL: ");
    Serial.println(bmsNum);
    if (!myBMSs[bmsNum].poll()) {
      Serial.print("Poll failed: ");
      Serial.println(myBMSs[bmsNum].myId());
      // fall through and advance battery
    } else {
      Serial.println("poll sent");
      isPolled = true;
    }
  }
  if (isPolled) {
    // busy polling
    // fixme - timeout? or trust the battery driver?
    if (myBMSs[bmsNum].isBusy()) return;
  }
  Serial.println("poll done");
  bmsNum++;
  isPolled = false;
  if (bmsNum >= bmsCount) {
    Serial.println("poll complete");
    bmsNum = -1;
    if (millis() - runtime >= 1000UL) {
      Serial.println("WARNING - POLL OVERRUN!");
    }
  }
#endif
}

/*
16:27:58.301 -> 7A 0 67 0 0 1 1 4 D 47 D 2B D 3D D 3E 1 D 47 2 D 2B 75 30 27 10 27 10 4E 20 4E 20 4E 20 2 0 49 0 49 0 4B 0 4B 1 49 1 49 0 0 0 0 0 0 0 0 5 4A 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 0 3 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1C FD A7 
16:27:58.333 -> BAD CRC!
16:27:58.333 -> msgCRC: 1CFD
16:27:58.333 -> dataCRC: 7832
1*/