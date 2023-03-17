#ifndef _OUT_PYLON_CAH_H_
#define _OUT_PYLON_CAH_H_

//https://github.com/coryjfowler/MCP_CAN_lib
#include <mcp_can.h>

/*
  accepts an initialised MCP_CAN object, as it may need to be shared between modules!

  parent must init can:
  CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ);
  CAN.setMode(MCP_NORMAL); 

*/


class outPylonCAN {
public:
  outPylonCAN(MCP_CAN & can_, batBat * bat_) : can(can_), bat(bat_) {
  //outPylonCAN(batBat * bat_) : bat(bat_) {
    isinit = false;
  }

  void run(void) {
    // init can data
    if(!isinit) {
      // set up safe initial output
      // should be safe to charge at bac current up to nominal voltage (at best ~50%)
      chargeVoltage = bat->nomVoltage;
      chargeCurrent = bat->nomChargeCurrent;
      // set up first run...
      outMsg = 0;
      lastRunMS = millis() - 1000UL; // force immediate...
      isinit = true;
      return;
    }

    // run outputs
    if(outMsg) {
      if(millis() - lastOutMS < 5UL) return; // wait at least 5ms...
      switch(outMsg) {
        case 0x351:
          do_351();
          outMsg = 0x355;
          return;
        case 0x355:
          do_355();
          outMsg = 0x356;
          return;
        case 0x356:
          do_356();
          outMsg = 0x359;
          return;
        case 0x359:
          do_359();
          outMsg = 0x35c;
          return;
        case 0x35c:
          do_35c();
          outMsg = 0x35e;
          return;
        case 0x35e:
          do_35e();
          outMsg = 0;
          return;
        default:
          Serial.println("INVALID OUTMSG: ");
          Serial.println(outMsg);
          esp_restart();
          while(1) {}
      }
      lastOutMS = millis();
    }
    if(millis() - lastRunMS < 1000UL) return; // every second...
    lastRunMS = millis();
    outMsg = 0x351;
    lastOutMS = millis() - 5UL;
    Serial.println("CAN RUN!");
  }

private:
  void bufClear(void) {
    memset(buf, 0, sizeof(buf));
    bufi = 0;
  }

  void bufPutU8(uint8_t v) {
    if((bufi + 1) > sizeof(buf)) {
      Serial.println("bufPutU8 with invalid bufi: ");
      Serial.println(bufi);
      esp_restart();
      while(1) {}
    }
    buf[bufi] = v;
    bufi++;
  }

  void bufPutU16(uint16_t v) {
    if((bufi + 2) > sizeof(buf)) {
      Serial.println("bufPutU16 with invalid bufi: ");
      Serial.println(bufi);
      esp_restart();
      while(1) {}
    }
    buf[bufi] = v & 0xff;
    bufi++;
    buf[bufi] = v >> 8;
    bufi++;
  }

  void bufPutS16(int16_t v) {
    if((bufi + 2) > sizeof(buf)) {
      Serial.println("bufPutS16 with invalid bufi: ");
      Serial.println(bufi);
      esp_restart();
      while(1) {}
    }
    buf[bufi] = v & 0xff;
    bufi++;
    buf[bufi] = v >> 8;
    bufi++;
  }

  void bufSend(uint16_t cmd) {
    if(bufi <= 0 || bufi > sizeof(buf)) {
      Serial.println("bufSend with invalid bufi: ");
      Serial.println(bufi);
      esp_restart();
      while(1) {}
    }
    Serial.print("CAN SEND ");
    Serial.print(bufi);
    Serial.print(" BYTES - CMD ");
    Serial.print(cmd, HEX);
    Serial.print(":");
    for(int i = 0; i < bufi; i++) {
      Serial.print(" ");
      Serial.print(buf[i]);
    }
    Serial.println();
    can.sendMsgBuf(cmd, 0, bufi, buf);
  }

  void do_351(void) {
    // use ramp functions from old code
    // derate algo and parallel algo can result in rapid changes...
    // inverter does not like this - use ramp algorithm to slowly change to target.

    if(bat->chargeCurrent < 0.0f) {
      // do battery data available! revert to default...
      chargeCurrent = bat->nomChargeCurrent;
      chargeVoltage = bat->nomVoltage;
    } else if(bat->chargeCurrent == 0.0f) {
      // stop charge = instant!
      chargeCurrent = 0.0f;
      chargeVoltage = bat->chargeVoltage;
      if(chargeVoltage < 0.0f) chargeVoltage = bat->nomVoltage;
    } else {
      // charge current
      if(bat->chargeCurrent < chargeCurrent) {
        // going down? rapid derate
        float delta = chargeCurrent - bat->chargeCurrent;
        delta /= 2.0f;
        chargeCurrent -= delta;
      } else if(bat->chargeCurrent > chargeCurrent) {
        // going up? very slow
        float delta = bat->chargeCurrent / 100.0f; //100s ramp up total
        chargeCurrent += delta;
        if(chargeCurrent > bat->chargeCurrent) chargeCurrent = bat->chargeCurrent;
      }

      // charge voltage
      float targ = bat->chargeVoltage;
      if(targ < 0.0f) targ = bat->nomVoltage;  // should never be possible, but put in a safe default anyway...
      if(targ < chargeVoltage) {
        // going down? rapid derate
        float delta = chargeVoltage - targ;
        delta /= 2.0f;
        chargeVoltage -= delta;
      } else if(targ > chargeVoltage) {
        // going up? very slow
        chargeVoltage += 0.02f; // 50 s per volt... 5s per unit that the inverter can notice
        if(chargeVoltage > targ) chargeVoltage = targ;
      }
    }

    // serial logging...
    Serial.println("<<<<<");
    Serial.print("Out_chargeVoltage = ");
    Serial.println(chargeVoltage);
    Serial.print("Out_chargeCurrent = ");
    Serial.println(chargeCurrent);
    Serial.println(">>>>>");

    bufClear();
    float f = chargeVoltage * 10.0f;
    Serial.println(f);
    bufPutU16(f); // Battery charge voltage
    f = chargeCurrent * 10.0f;
    Serial.println(f);
    bufPutS16(f); // Charge current limit
    f = bat->dischargeCurrent * 10.0f;
    if(f < 0.0f) f= bat->nomDischargeCurrent; // sanity check?
    Serial.println(f);
    bufPutS16(f); // Discharge current limit
    bufSend(0x351); // 6
  }
  
  void do_355(void) {
    bufClear();
    float f = bat->soc;
    if(f < 0.0f) f = 50.0f; // sanity check
    if(chargeCurrent == 0.0f) f = 100.0f; // seems this may help stop charge?
    Serial.println(f);
    bufPutU16(f); // SOC of single module or average value of system
    f = bat->soh;
    if(f < 0.0f) f = 100.0f;
    bufPutU16(f); // SOH of single module or average value of system
    bufSend(0x355); //, 4, mbuf);
  }
  
  void do_356(void) {
    bufClear();
    float f = bat->voltage * 100.0f;
    if(f < 0.0f) f = bat->nomVoltage * 100.0f; // sane default until battery updates...
    Serial.println(f);
    bufPutS16(f); // Voltage of single module or average module voltage of system
    f = bat->current * 10.0f;
    Serial.println(f);
    bufPutS16(f); // Module or system total current
    f = bat->temperature * 10.0f;
    Serial.println(f);
    bufPutS16(f); // Average cell temperature
    bufSend(0x356); //, 6, mbuf);
  }
  
  void do_359(void) {
    bufClear();
    bufPutU8(2); // protection
    bufPutU8(0); // protection
    bufPutU8(0); // alarm
    bufPutU8(0); // alarm
    bufPutU8(1); // module count
    bufPutU8('P'); // P
    bufPutU8('N'); // N
    bufSend(0x359); //, 7, mbuf);
  }
  
  void do_35c(void) {
    bufClear();
    uint16_t v = 0;
    if(chargeCurrent > 0.0f) v |= 0x80; // Charge enable
    if(bat->dischargeCurrent > 0.0f) v |= 0x40; // Discharge enable
    // b5 = Request force charge I*
    // b4 = Request force charge II*
    // b3 = Request full charge**
    bufPutU16(v); // Request flag 
    bufSend(0x35C); //, 2, mbuf);
  }
  
  void do_35e(void) {
    bufClear();
    bufPutU8('P'); // Manufacturer Name
    bufPutU8('Y');
    bufPutU8('L');
    bufPutU8('O');
    bufPutU8('N');
    bufPutU8(' ');
    bufPutU8(' ');
    bufPutU8(' ');
    bufSend(0x35E); //, 8, mbuf);
  }

  batBat * bat;
  MCP_CAN & can;
  bool isinit;
  int outMsg;
  unsigned long lastRunMS;
  unsigned long lastOutMS;
  float chargeVoltage;
  float chargeCurrent;
  unsigned long lastMS;
  uint8_t buf[8];
  int bufi;
};

#if 0

#define CAN_CS 10
MCP_CAN CAN(CAN_CS); //set CS pin for can controlelr

void can_setup() {
  if(CAN.begin(MCP_ANY, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    Serial.println("MCP2515 Initialized Successfully!");
  } else {
    Serial.println("Error Initializing MCP2515...");
    while(1); // fixme - error handler
  }
  CAN.setMode(MCP_NORMAL); 
}
#endif

#endif
