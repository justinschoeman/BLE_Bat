#ifndef _BAT_BALLANCERSW_H_
#define _BAT_BALLANCERSW_H_

/*
 * 
 * Ugly hack to switch an active balancer on/off
 * 
 * keeping 4 pin control to reuse hardware
 * 
 */

#include "bat.h"

// if data is older than this, consider battery data stale and unreliable
#define BAL_STALE   15000UL
// minimum charge current (+ve for charge) before balancing starts
#define BAL_MIN_CURRENT 0.0f
// minimum voltage for a single cell before balancing starts
#define BAL_START_VOLTAGE 3.5f
// minimum voltage for a single cell before balancing stops
#define BAL_STOP_VOLTAGE 3.45f

class balUnit {
public:
  balUnit(batBat * bat_, int pin_) :
    bat(bat_),
    pin(pin_),
    active(false)
    {}

private:
  batBat * bat;
  int pin;
  bool active;
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

  // call once in setup
  // initialise balancer and pins
  // call early in setup to make sure balance relays are off!
  void init(void) {
    for(int i = 0; i < numUnits; i++) {
      pinMode(units[i].pin, OUTPUT);
      pinOff(i);
    }
    // cycle relays for test/verification
    for(int i = 0; i < numUnits; i++) {
      pinOn(i);
    }
    delay(1000);
    for(int i = 0; i < numUnits; i++) {
      pinOff(i);
      units[i].active = false;
    }
    // relay test
    //esp_restart();
    //while(1) {}
  }

  // arduino loop style function - call repeatedly, and this handles all logic internally
  void run(void) {
    // first scan all units for updates and compare to our last time
    // just use the sum, as even if it wraps, it should give a unique state for all batteries...
    unsigned long testms = 0;
    for(int i = 0; i < numUnits; i++) testms += units[i].bat->updateMillis;
    if(testms == lastTime) return;
    lastTime = testms;
    Serial.println("BALANCER RUN!");
    unsigned long oldest = 0;
    float max_voltage = 0.0f;
    float min_current = 999.0f;
    for(int i = 0; i < numUnits; i++) {
      if(units[i].bat->updateMillis) {
        unsigned long age = millis() - units[i].bat->updateMillis;
        if(age > oldest) oldest = age;
      } else {
        oldest = BAL_STALE * 2; // force stale, if not yet updated
      }
      if(units[i].bat->current < min_current) min_current = units[i].bat->current;
      if(units[i].bat->maxCellVoltage > max_voltage) max_voltage = units[i].bat->maxCellVoltage;
    }
    Serial.print("Age: ");
    Serial.println(oldest);
    Serial.print("Max V: ");
    Serial.println(max_voltage);
    Serial.print("min I: ");
    Serial.println(min_current);
    // 1 - check for stale data, and pause balancing if data is too old
    if(oldest > BAL_STALE) {
      Serial.println("DATA STALE - STOPPING BALANCER!");
      stopAll();
      return;
    }
    // 2 - check for start conditions
    // min current
    if(-min_current < BAL_MIN_CURRENT) {
      Serial.println("CURRENT TOO LOW - STOPPING BALANCER!");
      stopAll();
      return;
    }
    // all active states are the same - just check first
    if(units[0].active) {
      // active - evaluate stop voltage
      if(max_voltage < BAL_STOP_VOLTAGE) {
        Serial.println("VOLTAGE TOO LOW - STOPPING BALANCER!");
        stopAll();
      }
    } else {
      // not active - evaluate start voltage
      if(max_voltage > BAL_START_VOLTAGE) {
        Serial.println("VOLTAGE HIGH - START BALANCER!");
        startAll();
      }
    }
  }

private:
  // turn a pin on
  void pinOn(int unit) {
    // as a sanity check, and to allow calling from init, always write the pin, even if we think state is already correct.
    if(invert) {
      digitalWrite(units[unit].pin, LOW);
    } else {
      digitalWrite(units[unit].pin, HIGH);
    }
    if(units[unit].active) return;
    units[unit].active = true;
  }

  // turn a pin off
  void pinOff(int unit) {
    if(invert) {
      digitalWrite(units[unit].pin, HIGH);
    } else {
      digitalWrite(units[unit].pin, LOW);
    }
    if(!units[unit].active) return;
    units[unit].active = false;
  }

  // turn off all pins and return to inactive state
  void stopAll(void) {
    for(int i = 0; i < numUnits; i++) {
      pinOff(i);
    }
  }

  // turn on all pins and aet active state
  void startAll(void) {
    for(int i = 0; i < numUnits; i++) {
      pinOn(i);
    }
  }

  balUnit * units;
  int numUnits;
  bool invert;
  unsigned long lastTime;
};

#endif
