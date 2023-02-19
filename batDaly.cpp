#include "batDaly.h"

// RX
// A5 1 90 8 2 F 0 0 75 38 2 93 91
//  | |  | | | | | |  |  | |  |  |
//  | |  | | | | | |  |  | |  |  - CRC
//  | |  | | | | | |  |  | |  ---- data 7
//  | |  | | | | | |  |  | ------- data 6
//  | |  | | | | | |  |  --------- data 5
//  | |  | | | | | |  ------------ data 4
//  | |  | | | | | --------------- data 3
//  | |  | | | | ----------------- data 2
//  | |  | | | ------------------- data 1
//  | |  | | --------------------- data 0
//  | |  | ----------------------- nr of data bytes (8 for all frames we are interested in)
//  | |  ------------------------- original command
//  | ---------------------------- device address (always 1?)
//  ------------------------------ start sentinel
// in theory all frames will be fixed 13 bytes long

void batDaly::logTag(const char *tag) {
  Serial.print(myId());
  Serial.print(" : ");
  Serial.print(tag);
  Serial.print(" = ");
}

void batDaly::log(const char *tag, int value, int fmt) {
  //return;  
  logTag(tag);
  Serial.println(value, fmt);
}

void batDaly::log(const char *tag, double value, int fmt) {
  //return;
  logTag(tag);
  Serial.println(value, fmt);
}

// -1 = error, 0 = run again, 1 = OK
int batDaly::run(void) {
  if (errors > 5) {
    Serial.println("TOO MANY ERRORS - RESTART UART");
    uart->close();
    errors = 0;
  }
  int i = uart->run();
  if (i < 0) {
    Serial.println("Error in uart->run()");
    errors++;
    return -1;
  }
  if (i == 0) {
    errors = 0;
    return 0;
  }
  // check for read....
  i = uart->read(buf + rxo, sizeof(buf) - rxo);
  if (i < 0) {
    Serial.println("Read failed!");
    errors++;
    return -1;
  }
  while (i != 0) {  // fake loop so we can break when needed
    Serial.print("Read ");
    Serial.print(myId());
    Serial.print(": ");
    Serial.println(i);
    for (int j = 0; j < i; j++) {
      Serial.print(buf[rxo + j], HEX);
      Serial.print(" ");
    }
    Serial.println();
    if (!runstate) {
      Serial.println("Unsollicited message - ignore.");
      break;
    }
    // check basic framing...
    if (rxo) {
      // receiving additional data on a partial buffer
      if (buf[rxo] == 0xa5) {
        Serial.println("offset packet starts with start sentinel - discard old data and use this...");
        memmove(buf, &buf[rxo], i);
        rxo = 0;
      }
    } else {
      // entire frame
      if (buf[0] != 0xa5) {
        Serial.println("Invalid start sentinel - discard data!");
        break;
      }
    }
    // check end sentinel
    i += rxo;  // total buffer length
    if(i < 4) {
      Serial.println("too short - wait for more data");
      rxo = i;
      break;
    }
    int len = buf[3];
    if(len != 8) {
      Serial.print("Invalid data length: ");
      Serial.println(len);
      errors++;
      runstate = 0;
      return -1;
    }
    if (i < len + 5) {
      Serial.println("too short for length - wait for more data");
      rxo = i;
      break;
    }
    //Serial.println("process response");
    if (!handleRx(rxo + i)) {
      Serial.println("Bad response!");
      errors++;
      runstate = 0;
      return -1;
    }
    errors = 0;
    runstate = 0;
    break;  // end fake loop
  }
  if (runstate) {
    // still in runstate? check rx timeout!
    if (millis() - runtime > 500UL) {
      Serial.println("READ TIMEOUT!");
      errors++;
      runstate = 0;
      return -1;
    }
  }
  return 1;
}

// return true if a poll message was successfully sent
bool batDaly::poll(void) {
  if (runstate) {
    Serial.println("POLL IN BUSY STATE! IGNORE");
    return true;
  }
  if(!sendCommand(0x90)) {
    Serial.println("send 0x90 failed!");
    errors++;
    return false;
  }
  runstate = 1;
  return true;
}

uint8_t batDaly::calcCRC(uint8_t* b, int len) {
  uint8_t crc = 0;
  for (int i = 0; i < len; i++) {
    crc += b[i];
  }
  return crc;
}

bool batDaly::sendCommand(uint8_t cmd) {
  //uint8_t txbuf[] = { 0xA5, 0x40, 0x90, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7D };
  txbuf[2] = cmd;
  txbuf[12] = calcCRC(txbuf, 12);
  Serial.print("TX: ");
  for(int i = 0; i < sizeof(txbuf); i++) {
    Serial.print(txbuf[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  int i = uart->write(txbuf, sizeof(txbuf));
  if (i != sizeof(txbuf)) {
    Serial.println("write failed!");
    errors++;
    return false;
  }
  rxo = 0;  // start with a clean rx buffer...
  errors = 0;
  runtime = millis();
  return true;
}

uint8_t batDaly::get8(void) {
  return buf[bo++];
}

uint16_t batDaly::get16(void) {
  uint16_t r = buf[bo++];
  r <<= 8;
  return r | buf[bo++];
}

bool batDaly::handleRx(int len) {
  if (len < 8) {
    Serial.println("too short!");
    return false;
  }
  // crc
  uint8_t msgCRC = buf[len - 1];
  uint8_t dataCRC = calcCRC(buf, len - 1);
  if (msgCRC != dataCRC) {
    Serial.println("BAD CRC!");
    Serial.print("msgCRC: ");
    Serial.println(msgCRC, HEX);
    Serial.print("dataCRC: ");
    Serial.println(dataCRC, HEX);
    return false;
  }
  // now start scanning from the top...
  bo = 0;
  // 0 = start sentinel (already checked)
  int i = get8();
  // 1 = device address
  i = get8();
  if(i != dev_addr) {
    Serial.print("INVALID DEVICE ADDRESS: ");
    Serial.println(i);
    Serial.print("SHOUND BE: ");
    Serial.println(dev_addr);
    return false;
  }
  // 2 = command
  uint8_t cmd = get8();
  Serial.print("COMMAND: ");
  Serial.println(cmd, HEX);
  // 3 = data length
  i = get8();
  if(i != 8) {
    Serial.print("INVALID LENGTH (should be): ");
    Serial.println(8);
    Serial.print("Length: ");
    Serial.println(i);
    return false;
  }
  if(cmd == 0x90) return do90();
  Serial.println("BAD COMMAND!");
  return false;
}

bool batDaly::do90(void) {
  int i;
  float f;
  // 0..1 = voltage
  f = get16();
  f *= 0.1f;
  log("voltage", f);
  bat->voltage = f;
  // 2..3 = acquisition?
  f = get16();
  f *= 0.1f;
  log("acquisition?", f);
  // 4..5 = current
  f = get16();
  f -= 30000.0f;
  f *= 0.1f;
  log("current", f);
  bat->current = f;
  // 6..7 = soc
  f = get16();
  f *= 0.1f;
  log("soc", f);
  bat->soc = f;
  //bat->updateMillis = millis();
  // test
  //Serial.println(myId());
  //bat->dump();
  return true;
}
