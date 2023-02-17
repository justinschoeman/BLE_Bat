#include "bleUART.h"
#include "config.h"

bool bleUART::discoverDevice(BLEDevice dev_) {
  Serial.print("Test: ");
  Serial.println(dev_.address());
  if(dev_.address() != addr) {
    Serial.println("Not my device...");
    return false;
  }
  Serial.print("My device: ");
  Serial.println(dev_.localName());
  if(found) {
    Serial.println("Already found...");
  } else {
    // copy device
    device = dev_;
    found = true;
  }
  return true;
}

bool bleUART::open(void) {
  if(!found) {
    Serial.println("Not discovered! Can't open!");
    return false;
  }

  // connect
  if(!device.connect()) {
    Serial.print("Failed to connect to: ");
    Serial.println(addr);
    // tell run not to try reconnect for this time....
    reconnect_time = millis();
    return false;
  }
  Serial.println("Connected...");
  connected = true;
  reconnect_time = 0;
  connect_time = millis();
  connect_time_fuzz = connect_time & 0x400;
  connect_time_fuzz *= 100; // semi random time up to ~100s

  // get service
  if(!device.discoverService(svcUUID.c_str())) {
    Serial.print("Could not find service ");
    Serial.print(svcUUID);
    Serial.print(" for device ");
    Serial.println(addr);
    close();
    return false;
  }
  Serial.println("Found service");

  // get read characteristic
  rdCHAR = device.characteristic(rdUUID.c_str());
  if(!rdCHAR) {
    Serial.print("Could not find read characteristic ");
    Serial.print(rdUUID);
    Serial.print(" for device ");
    Serial.println(addr);
    close();
    return false;
  }
  Serial.println("Found read characteristic");
  if(!rdCHAR.canSubscribe()) {
    Serial.print("Read characteristic ");
    Serial.print(rdUUID);
    Serial.print(" not subscribable for device ");
    Serial.println(addr);
    close();
    return false;
  }
  if(!rdCHAR.subscribe()) {
    Serial.print("Subscribe failed for read characteristic ");
    Serial.print(rdUUID);
    Serial.print(" for device ");
    Serial.println(addr);
    close();
    return false;
  }
  Serial.println("Subscribed");

  // get write characteristic
  wrCHAR = device.characteristic(wrUUID.c_str());
  if(!wrCHAR) {
    Serial.print("Could not find write characteristic ");
    Serial.print(wrUUID);
    Serial.print(" for device ");
    Serial.println(addr);
    close();
    return false;
  }
  Serial.println("Found write characteristic");
  if(!wrCHAR.canWrite()) {
    Serial.print("Write characteristic ");
    Serial.print(wrUUID);
    Serial.print(" not writable for device ");
    Serial.println(addr);
    close();
    return false;
  }
  return true;
}

void bleUART::close(void) {
  if(!found) return;
  if(connected) {
    device.disconnect();
  }
  connected = false;
}

int bleUART::run(void) {
  // sanity check
  if(!found) {
    Serial.println("Attempt to run UART which was not discovered?");
    return -1;
  }
  // connect
  if(!connected) {
    // disconnect lockout - not actually an error, but do nothing...
    if(reconnect_time && millis() - reconnect_time < 500UL) return 0;
    Serial.print("try connect ");
    Serial.println(myId());
    if(!open()) {
      Serial.print("Failed to open device: ");
      Serial.println(myId());
      return -1;
    }
  }
  // test for restart time
  if(millis() - connect_time > BAT_CFG_BLE_MAX_CONNECT_TIME + connect_time_fuzz) {
    Serial.print("CONNECTED TOO LONG - RESTART: ");
    Serial.println(myId());
    close();
    return 0;
  }
  return 1;
}

int bleUART::read(uint8_t * buf, int len) {
  if(!connected) return -1;
  // check for read data?
  if(!rdCHAR.valueUpdated()) return 0;
  int i = rdCHAR.readValue(buf, len);
  if(i == 0) {
    Serial.print("Read failed: ");
    Serial.println(myId());
    //close();
    return -1;
  }
  return i;
}

int bleUART::write(uint8_t * buf, int len) {
  if(!connected) return -1;
  if(!wrCHAR.writeValue(buf, len)) {
    Serial.print("Write failed: ");
    Serial.println(myId());
    //close();
    return -1;
  }
  int i = wrCHAR.valueLength();
  //Serial.print("Wrote: ");
  //Serial.println(i);
  return i;
}
