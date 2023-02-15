#ifndef _BAT_BLE_MANAGER_H_
#define _BAT_BLE_MANAGER_H_

#include "bleUART.h"

class batBLEManager {
public:
  batBLEManager(bleUART ** uarts_, int count_, unsigned long connect_time_) :
    uarts(uarts_), 
    count(count_),
    connect_time(connect_time_),
    mode(0)
    {}
  
  // -1 = error, 0 = run some more, 1 = done
  int run(void) {
    // short circuit for 'done' mode
    if(mode == 2) return 1;
    if(mode == 0) {
      // init
      Serial.println("INIT BLE!");
      if(!BLE.begin()) {
        Serial.println("BLE start failed!");
        return -1;
      }
      // start scan
      Serial.println("Scanning...");
      BLE.scan();
      discovery_start = millis();
      mode = 1;
      return 0;
    }
    if(mode == 1) {
      // timeout?
      if(millis() - discovery_start > connect_time) {
        Serial.println("BLE CONNECT TIMEOUT!");
        return -1;
      }
      // discovery
      while(BLEDevice dev = BLE.available()) {
        Serial.print("Discovered device: ");
        Serial.print(dev.address());
        Serial.print(" (");
        Serial.print(dev.localName());
        Serial.println(")");
        for(int i = 0; i < count; i++) {
          if(uarts[i]->discoverDevice(dev)) {
            Serial.println("FOUND!");
            break;
          }
        }
      }
      // complete?
      for(int i = 0; i < count; i++) {
        if(!uarts[i]->isDiscovered()) {
          // not yet complete...
          return 0;
        }
      }
      // all discovered...
      Serial.println("ALL DEVICES DISCOVERED!");
      BLE.stopScan();
      mode = 2;
      return 1;
    }
    Serial.print("INVALID MODE: ");
    Serial.println(mode);
    return -1;
  }

private:
  bleUART ** uarts;
  int count;
  unsigned long connect_time;
  int mode; // 0 = init, 1 = scanning, 2 = done
  unsigned long discovery_start;
};

#endif