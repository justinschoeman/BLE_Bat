#ifndef _BAT_BALLANCER_H_
#define _BAT_BALLANCER_H_

#include "bat.h"

// ms to ignore a cell after a balance event
#define BAL_LOCKOUT 30000UL
// if data is older than this, consider battery data stale and unreliable
#define BAL_STALE   15000UL
// minimum charge current (+ve for charge) before balancing starts
#define BAL_MIN_CURRENT -1.0f
// minimum voltage for a single cell before balancing starts
#define BAL_MIN_VOLTAGE 13.0f
// minimum difference to trigger discharge
#define BAL_DIFF 0.01f

class balUnit {
public:
  balUnit(batBat * bat_, int pin_) :
    bat(bat_),
    pin(pin_),
    active(false),
    lockTime(0)
    {}
  
  // test if the battery is locked out (recently discharged, consider voltage unreliable while recovering)
  // lockTime starts at 0, so we are also locked out for BAL_LOCKOUT after boot before balancing will start
  bool isLockout(void) {
    if(active) return false; // and active pin can not be locked out
    return millis() - lockTime > BAL_LOCKOUT ? false : true;
  }

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
      Serial.print(i);
      Serial.print(" : ");
      Serial.println(units[i].pin);
      pinOn(i);
      delay(200);
      pinOff(i);
      units[i].active = false;
      units[i].lockTime = 0;
    }
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
    float min_voltage = 999.0f;
    float min_current = 999.0f;
    float min_bal_voltage = 999.0f;
    for(int i = 0; i < numUnits; i++) {
      if(units[i].bat->updateMillis) {
        unsigned long age = millis() - units[i].bat->updateMillis;
        if(age > oldest) oldest = age;
      } else {
        oldest = BAL_STALE * 2; // force stale, if not yet updated
      }
      if(!units[i].active && !units[i].isLockout() && units[i].bat->current < min_current) min_current = units[i].bat->current;
      if(units[i].bat->voltage < min_voltage) min_voltage = units[i].bat->voltage;
      if(!units[i].active && !units[i].isLockout() && units[i].bat->voltage < min_bal_voltage) min_bal_voltage = units[i].bat->voltage;
    }
    Serial.print("Age: ");
    Serial.println(oldest);
    Serial.print("Min V: ");
    Serial.println(min_voltage);
    Serial.print("min I: ");
    Serial.println(min_current);
    Serial.print("Bal Min V: ");
    Serial.println(min_bal_voltage);
    // 1 - check for stale data, and pause balancing if data is too old
    if(oldest > BAL_STALE) {
      Serial.println("DATA STALE - STOPPING BALANCER!");
      stopAll();
      return;
    }
    // 2 - check for start conditions
    // min current
    if(min_current < BAL_MIN_CURRENT) {
      Serial.println("CURRENT TOO LOW - STOPPING BALANCER!");
      stopAll();
      return;
    }
    // min voltage
    if(min_voltage < BAL_MIN_VOLTAGE) {
      Serial.println("VOLTAGE TOO LOW - STOPPING BALANCER!");
      stopAll();
      return;
    }
    // find all cells > x above lowest voltage (not locked out)
    for(int i = 0; i < numUnits; i++) {
      if(units[i].active) {
        // active cells, we only evaluate whether or not to turn them off
        if(units[i].bat->voltage <= min_bal_voltage) {
          Serial.print("Balance end cell: ");
          Serial.println(i);
          pinOff(i);
        }
        continue;
      }
      if(units[i].isLockout()) continue; // locked out? ignore
      if(min_bal_voltage == 999.0f) continue; // no min voltage? ignore
       // not active or lockout, test for discharge difference
      float diff = units[i].bat->voltage - min_bal_voltage;
      if(diff >= BAL_DIFF) {
        Serial.print("Cell ");
        Serial.print(i);
        Serial.print(" voltage ");
        Serial.print(units[i].bat->voltage);
        Serial.print(" diff ");
        Serial.print(diff);
        Serial.println(" too high - start balance");
        pinOn(i);
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
    units[unit].bat->balancing = true;
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
    units[unit].bat->balancing = false;
    units[unit].lockTime = millis();
  }


  // turn off all pins and return to inactive state
  void stopAll(void) {
    for(int i = 0; i < numUnits; i++) {
      pinOff(i);
    }
  }

  balUnit * units;
  int numUnits;
  bool invert;
  unsigned long lastTime;
};

#endif
