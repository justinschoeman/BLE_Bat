
#include <ArduinoBLE.h>

class bleUART {
public:
  bleUART(const char * addr_, const char * svcUUID_, const char * rdUUID_, const char * wrUUID_) 
    : addr(String(addr_)), svcUUID(String(svcUUID_)), rdUUID(String(rdUUID_)), wrUUID(String(wrUUID_)) {
      found = false;
      connected = false;
  }

  ~bleUART(void) {
    close();
  }

  void close(void) {
    if(!found) return;
    if(connected) {
      device.disconnect();
    }
    connected = false;
  }

  bool open(void) {
    if(!device.connect()) {
      Serial.print("Failed to connect to: ");
      Serial.println(addr);
      return false;
    }
    Serial.println("Connected...");
    connected = true;
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

  bool tryDevice(BLEDevice dev_) {
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
      return true;
    }
    // copy device
    device = dev_;
    found = true;
    return true;
  }

  bool run(void) {
    // sanity check
    if(!found) {
      Serial.println("Attempt to run UART which was not advertised!");
      return false;      
    }
    // connect
    if(!connected) {
      Serial.println("try connect...");
      if(!open()) {
        Serial.print("Failed to open device: ");
        Serial.println(device.address());
        return false;
      }
    }

    //Serial.print("Run: ");
    //Serial.println(device.address());
    return true;
  }

  int read(uint8_t * buf, int len) {
    // check for read data?
    if(!rdCHAR.valueUpdated()) return 0;
    int i = rdCHAR.readValue(buf, len);
    if(i == 0) return -1;
    return i;
  }

  int write(uint8_t * buf, int len) {
    if(!wrCHAR.writeValue(buf, len)) return -1;
    int i = wrCHAR.valueLength();
    Serial.print("Wrote: ");
    Serial.println(i);
    return i;
  }


  bool isFound(void) { return found; }
  bool isConnected(void) { return connected; }

  const String &myId(void) {
    return addr;
  }

private:
  const String addr;
  String svcUUID;
  String rdUUID;
  String wrUUID;
  BLEDevice device;
  BLECharacteristic rdCHAR;
  BLECharacteristic wrCHAR;
  bool connected;
  bool found;
};

bleUART myBLE("2b:80:03:b4:76:0a", "6e400000-b5a3-f393-e0a9-e50e24dcca9e", "6e400001-b5a3-f393-e0a9-e50e24dcca9e", "6e400002-b5a3-f393-e0a9-e50e24dcca9e");

/*
2b:80:03:b4:78:03 '2531200220811107' 
21:29:09.393 -> Test: 2b:80:03:b4:71:d9
21:29:09.393 -> Not my device...
21:29:09.393 -> Found unhandled: 2b:80:03:b4:71:d9 '2531200220826074' 
21:29:09.491 -> Test: 2b:80:03:b4:76:21
21:29:09.491 -> Not my device...
21:29:09.491 -> Found unhandled: 2b:80:03:b4:76:21 '2531200220811218' 
*/

class batVestwoods {
public:
  batVestwoods(bleUART & uart_) : uart(&uart_) {
    runstate = 0;
  }

  int run(void) {
    if(!uart->run()) {
      Serial.println("Error in uart->run()");
      return false;
    }
    // check for read....
    int i = uart->read(buf, sizeof(buf));
    if(i < 0) {
      Serial.println("Read failed!");
      uart->close();
      return false;
    }
    if(i != 0) {
      Serial.print("Read: ");
      Serial.println(i);
      for(int j = 0; j < i; j++) {
        Serial.print(buf[j], HEX);
        Serial.print(" ");
      }
      Serial.println();
      if(runstate) {
        Serial.println("process response");
        runstate = 0;
      } else {
        Serial.println("Unsollicited message - ignore.");
      }
    }
    if(runstate) {
      // still in runstate? check rx timeout!
    }
    return true;
  }

  bool busy(void) {
    return runstate ? 1 : 0;
  }

  int poll(void) {
    if(runstate) {
      Serial.println("POLL IN BUSY STATE! IGNORE");
      return true;
    }
    uint8_t txbuf[] = {0x7a, 0, 5, 0, 0, 1, 12, 229, 0xa7};
    int i = uart->write(txbuf, sizeof(txbuf));
    Serial.print("Wrote: ");
    Serial.println(i);
    if(i != sizeof(txbuf)) {
      Serial.println("write failed!");
      uart->close();
      return false;
    }
    runstate = 1;
    return true;
  }

  const String &myId(void) {
    return uart->myId();
  }

private:
  bleUART * uart;
  int runstate; // 0 = idle, 1 = waiting for rx
  uint8_t buf[256];

};

batVestwoods myBAT(myBLE);

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
  if(myBLE.isFound()) {
    Serial.println("all devices found - advancing to run state");
    BLE.stopScan();
    runstate = 1;
    runtime = millis();
    return;
  }
  // discover more...
  while(BLEDevice peripheral = BLE.available()) {
    // discovered a peripheral, print out address, local name, and advertised service
    if(myBLE.tryDevice(peripheral)) continue;
    Serial.print("Found unhandled: ");
    Serial.print(peripheral.address());
    Serial.print(" '");
    Serial.print(peripheral.localName());
    Serial.print("' ");
    Serial.print(peripheral.advertisedServiceUuid());
    Serial.println();
  }
  return;
}

// main loop
void loop() {
  // first wait until we discover all devices
  if(runstate == 0) {
    loop_discover();
    return;
  }
  // normal run - run battery engines
  if(!myBAT.run()) {
    Serial.print("Error running battery ");
    Serial.println(myBAT.myId());
  }
  if(millis() - runtime > 1000UL) {
    Serial.println("POLL!");
    if(!myBAT.poll()) {
      Serial.println("Poll failed!");
    }
    runtime += 1000UL;
  }
}
