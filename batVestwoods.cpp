#include "batVestwoods.h"

// -1 = error, 0 = run again, 1 = OK
int batVestwoods::run(void) {
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
      if (buf[rxo] == 0x7a) {
        Serial.println("offset packet starts with start sentinel - discard old data and use this...");
        memmove(buf, &buf[rxo], i);
        rxo = 0;
      }
    } else {
      // entire frame
      if (buf[0] != 0x7a) {
        Serial.println("Invalid start sentinel - discard data!");
        break;
      }
    }
    // check end sentinel
    i += rxo;  // total buffer length
    if (i < 8) {
      Serial.println("too short - wait for more data");
      rxo = i;
      break;
    }
    int len = buf[2];
    if (i < len + 4) {
      Serial.println("too short for length - wait for more data");
      rxo = i;
      break;
    }
    if (buf[1 + len + 2] != 0xa7) {
      // no end sentinel?
      Serial.println("No end sentinel - wait for more data...");
      rxo = i;
      break;
    }
    //Serial.println("process response");
    if (!handleRx(rxo + i)) {
      Serial.println("Bad response!");
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

// fixme... -1 on error, 0 on still busy, 1 on poll complete
int batVestwoods::poll(void) {
  if (runstate) {
    Serial.println("POLL IN BUSY STATE! IGNORE");
    return true;
  }
  uint8_t txbuf[] = { 0x7a, 0, 5, 0, 0, 1, 12, 229, 0xa7 };
  int i = uart->write(txbuf, sizeof(txbuf));
  //Serial.print("Wrote: ");
  //Serial.println(i);
  if (i != sizeof(txbuf)) {
    Serial.println("write failed!");
    errors++;
    return false;
  }
  runstate = 1;
  rxo = 0;  // start with a clean ex buffer...
  errors = 0;
  runtime = millis();
  return true;
}

uint16_t batVestwoods::calcCRC(uint8_t* b, int len) {
  if (!b || len <= 0) return 0;
  uint16_t crc = 0xFFFF;
  for (int i = 0; i < len; i++) {
    crc ^= b[i];
    for (int j = 0; j < 8; j++) {
      if (crc & 1) {
        crc = (crc >> 1) ^ 40961;
      } else {
        crc = crc >> 1;
      }
    }
  }
  return crc & 0xFFFF;
}

uint8_t batVestwoods::get8(void) {
  return buf[bo++];
}

uint16_t batVestwoods::get16(void) {
  uint16_t r = buf[bo++];
  r <<= 8;
  return r | buf[bo++];
}

bool batVestwoods::handleRx(int len) {
  if (len < 8) {
    Serial.println("too short!");
    return false;
  }
  // crc
  uint16_t msgCRC = buf[len - 3];
  msgCRC <<= 8;
  msgCRC |= buf[len - 2];
  uint16_t dataCRC = calcCRC(buf + 1, len - 4);
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
  // start sentinel
  int i = get8();
  // 1 = unknwon - probably high byte of length
  i = get8();
  // 2 = length
  i = get8();
  if (len != i + 4) {
    Serial.print("INVALID LENGTH (should be): ");
    Serial.println(len);
    Serial.print("Length: ");
    Serial.println(i);
    return false;
  }
  // 4 = unknown - probably address
  i = get8();
  // 5,6 = command
  i = get16();
  Serial.print("command: ");
  Serial.println(i, HEX);
  if (i == 0x0001) return do0001();
  Serial.println("BAD COMMAND!");
  return false;
}

bool batVestwoods::do0001(void) {
  // 0 - onlineStatus
  int i = get8();
  Serial.print("onlineStatus ");
  Serial.println(i);
  // 1 - batteriesSeriesNumber
  i = get8();
  Serial.print("batteriesSeriesNumber ");
  Serial.println(i);
  for (int j = 0; j < i; j++) {
    // 2, 3 - cellVoltage (mV)
    uint16_t k = get16();
    Serial.print("cellVoltage (mV) ");
    Serial.println(k);
  }
  // 4 - maxCellNumber
  i = get8();
  Serial.print("maxCellNumber ");
  Serial.println(i);
  // 5,6 - maxCellVoltage
  i = get16();
  Serial.print("maxCellVoltage ");
  Serial.println(i);
  // 7 - minCellNumber
  i = get8();
  Serial.print("minCellNumber ");
  Serial.println(i);
  // 8,9 - minCellVoltage
  i = get16();
  Serial.print("minCellVoltage ");
  Serial.println(i);
  // 10,11 - totalCurrent ( x / 100 - 300 )
  i = get16();
  float f = i;
  f = (f / 100.0) - 300.0;
  Serial.print("totalCurrent ");
  Serial.println(f);
  // 12, 13 - soc (x / 100)
  i = get16();
  f = i;
  f /= 100.0;
  Serial.print("soc ");
  Serial.println(f);
  // 14, 15 - soh (x / 100)
  i = get16();
  f = i;
  f /= 100.0;
  Serial.print("soh ");
  Serial.println(f);
  // 16, 17 - actualCapacity (x / 100)
  i = get16();
  f = i;
  f /= 100.0;
  Serial.print("actualCapacity ");
  Serial.println(f);
  // 18, 19 - surplusCapacity (x / 100)
  i = get16();
  f = i;
  f /= 100.0;
  Serial.print("surplusCapacity ");
  Serial.println(f);
  // 20, 21 - nominalCapacity (x / 100)
  i = get16();
  f = i;
  f /= 100.0;
  Serial.print("nominalCapacity ");
  Serial.println(f);
  // 22 - batteriesTemperatureNumber
  i = get8();
  Serial.print("batteriesTemperatureNumber ");
  Serial.println(i);
  for (int j = 0; j < i; j++) {
    // 23, 24 - cellTemperature (x - 50)
    int k = get16();
    k -= 50;
    Serial.print("cellTemperature ");
    Serial.println(k);
  }
  // 25, 26 - environmentalTemperature
  i = get16();
  i -= 50;
  Serial.print("environmentalTemperature ");
  Serial.println(i);
  // 25, 26 -pcbTemperature
  i = get16();
  i -= 50;
  Serial.print("pcbTemperature ");
  Serial.println(i);
  // 27 - maxTemperatureCellNumber
  i = get8();
  Serial.print("maxTemperatureCellNumber ");
  Serial.println(i);
  // 28 - maxTemperatureCellValue
  i = get8();
  i -= 50;
  Serial.print("maxTemperatureCellValue ");
  Serial.println(i);
  // 29 - minTemperatureCellNumber
  i = get8();
  Serial.print("minTemperatureCellNumber ");
  Serial.println(i);
  // 30 - minTemperatureCellValue
  i = get8();
  i -= 50;
  Serial.print("minTemperatureCellValue ");
  Serial.println(i);
  // 31 bmsFault1
  i = get8();
  Serial.print("bmsFault1 ");
  Serial.println(i, HEX);
  // 32 bmsFault2
  i = get8();
  Serial.print("bmsFault2 ");
  Serial.println(i, HEX);
  // 33 bmsAlert1
  i = get8();
  Serial.print("bmsAlert1 ");
  Serial.println(i, HEX);
  // 34 bmsAlert2
  i = get8();
  Serial.print("bmsAlert2 ");
  Serial.println(i);
  // 35 bmsAlert3
  i = get8();
  Serial.print("bmsAlert3 ");
  Serial.println(i, HEX);
  // 36 bmsAlert4
  i = get8();
  Serial.print("bmsAlert4 ");
  Serial.println(i, HEX);
  // 37, 38 u.cycleIndex
  i = get16();
  Serial.print("u.cycleIndex ");
  Serial.println(i);
  // 39, 40 totalVoltage ( x / 100)
  i = get16();
  f = i;
  f /= 100.0;
  Serial.print("u.totalVoltage ");
  Serial.println(f);
  // 41 bmsStatus
  i = get8();
  Serial.print("bmsStatus ");
  Serial.println(i, HEX);
  // 42, 43 - totalChargingCapacity
  //i = get16();
  //Serial.print("totalChargingCapacity ");
  //Serial.println(i);
  // 44, 45 - totalDischargeCapacity
  //i = get16();
  //Serial.print("totalDischargeCapacity ");
  //Serial.println(i);
  // 46, 47 - totalRechargeTime
  //i = get16();
  //Serial.print("totalRechargeTime ");
  //Serial.println(i);
  // 48, 49 - totaldischargeTime
  //i = get16();
  //Serial.print("totaldischargeTime ");
  //Serial.println(i);
  // 50 - batteryType
  //i = get8();
  //Serial.print("batteryType ");
  //Serial.println(i);
  return true;
}