#ifndef _BAT_BALLANCER_H_
#define _BAT_BALLANCER_H_

#include "bat.h"

class balUnit {
public:
  balUnit(batBat * bat_, int pin_) :
    bat(bat_),
    pin(pin_),
    active(false),
    lockTime(0)
    {}

private:
  batBat * bat;
  int pin;
  bool active;
  unsigned long lockTime;
  friend class balBank;
};

class balBank {
public:
  balBank(balUnit * units_, int num, bool invert_) :
    units(units_),
    numUnits(num),
    invert(invert_),
    lastTime(0)
    {}
  
  // initialise balancer and pins
  // call early in setup to make sure balance relays are off!
  void init(void) {
    for(int i = 0; i < numUnits; i++) {
      pinMode(units[i].pin, OUTPUT);
      pin_off(i);
    }
    // cycle relays for test/verification
    for(int i = 0; i < numUnits; i++) {
      Serial.print(i);
      Serial.print(" : ");
      Serial.println(units[i].pin);
      pin_on(i);
      delay(200);
      pin_off(i);
      units[i].active = false;
      units[i].lockTime = 0;
    }
  }

  void run(void) {
    // first scan all units for updates and compare to our last time
    // just use the sum, as even if it wraps, it should give a unique state for all batteries...
    unsigned long testms = 0;
    for(int i = 0; i < numUnits; i++) testms += units[i].bat->updateMillis;
    if(testms == lastTime) return;
    lastTime = testms;
    Serial.println("BALANCER RUN!");
  }

private:
  void pin_on(int unit) {
    if(invert) {
      digitalWrite(units[unit].pin, LOW);
    } else {
      digitalWrite(units[unit].pin, HIGH);
    }
  }

  void pin_off(int unit) {
    if(invert) {
      digitalWrite(units[unit].pin, HIGH);
    } else {
      digitalWrite(units[unit].pin, LOW);
    }
  }

  balUnit * units;
  int numUnits;
  bool invert;
  unsigned long lastTime;
};

#endif
