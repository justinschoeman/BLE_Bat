#ifndef _BAT_DERATE_H_
#define _BAT_DERATE_H_

#include "bat.h"

class batDerate {
public:
  batDerate(batBat * bat_) :
    bat(bat_),
    lastMillis(0),
    bulk(true),
    bulkMillis(0),    
    derateErrorCount(0),
    lvLockout(false)
    {}
  
  void run(void) {
    if(bat->updateMillis == lastMillis) return;
    lastMillis = bat->updateMillis;
    Serial.println("RUN DERATE!");

    // sanity check inputs
    // in older tests, batteries sometimes spit out bad data for a bit - ignore it
    if(bat->voltage < 60.0f && bat->minCellVoltage < 4.0f) {
      // total battery voltage < 60V and minimum cell voltage < 4V
      // assume OKish
      derateErrorCount = 0;
    } else {
      Serial.println("BAD DATA!");
      derateErrorCount++;
      if(derateErrorCount > 5) {
        Serial.println("TOO MUCH BAD DATA - ATTEMPT REBOOT!");
        esp_restart();
        while(1) {}
      }
      return; // do not update outputs based on this data...
    }

    // check bulk/float mode
    if(bulk) {
      // in bulk mode, test if we must go to float
      // testFloat
      if(testFloat()) {
        // we meet all the requirements (except possibly time) for float mode
        if((millis() - bulkMillis) > cfgFloatMillis) {
          Serial.println("FLOAT MODE NOW");
          bulk = false;
        }
      } else {
        // not ready for float - update timestamp for latest non-float time
        bulkMillis = millis();
      }
    } else {
      // in float mode, test if we must go to bulk
      if(bat->minCellVoltage < cfgCellFloatEndV) {
        Serial.println("FLOAT END");
        bulk = true;
        bulkMillis = millis();
      }
    }

    // check charge derating
    if(bat->maxCellVoltage > cfgCellMaxV) {
      // overvoltage - shut off charging...
      Serial.println("OVERVOLTAGE!");
      setChargeI(0.0f);
      setChargeV(bat->voltage); // go no higher
    } else {
      // not overvoltage - set sane target voltage
      // always start with some sane defaults
      // for sanity sake, always keep the lowest meaningful charge voltage, and ramp up as required...
      float t = cfgCellMaxV - bat->maxCellVoltage;
      t *= (float)bat->numCells;  // set the target voltage = current voltage + whatever is required to make the max cell full (spread over all cells)
      // inverter is too gentle close to voltage limits - lets make it slightly more aggressive
      t *= 2.0f;
      setChargeV(bat->voltage + t);

      // and set current based on derate curve
      if(bat->maxCellVoltage > cfgCellMaxDerateV) {
        // derating
        Serial.println("derating!");
        float t = cfgCellMaxV - bat->maxCellVoltage;
        t /= cfgCellMaxV - cfgCellMaxDerateV; // proportional over derate range
        setChargeI(bat->nomChargeCurrent * t);
        // this is an extra gotcha from my old production code
        // above some arbitrary hard limit, drastically reduce current...
        // test - hard limit current in closing phases...
        if(bat->maxCellVoltage > 3.5f) bat->chargeCurrent /= 10.0f; // max 15A
      } else {
        // no derate - default to nominal
        setChargeI(bat->nomChargeCurrent);
      }
    }

    // check discharge derating
    if(bat->minCellVoltage < cfgCellMinV) {
      // any cell <= minimum? shut off discharge
      Serial.println("LOW VOLTAGE LIMIT!");
      lvLockout = true;
      lvLockoutMillis = millis();
    }
    // otherwise, make sure any low voltage lockout has expired
    if(lvLockout) {
      Serial.println("LOW VOLTAGE LOCKOUT");
      bat->dischargeCurrent = 0.0f;
      bat->soc = 0.0f;
      if(millis() - lvLockoutMillis >= cfgLvLockoutMillis) {
        lvLockout = false;
      }
    } else {
      // default to nominal discharge
      bat->dischargeCurrent = bat->nomDischargeCurrent;
      if(bat->minCellVoltage < cfgCellMinDerateV) {
        // derate soc
        float t = bat->minCellVoltage - cfgCellMinV;
        float t2 = cfgCellMinDerateV - cfgCellMinV;
        t = (t * t) / (t2 * t2);
        t *= 100.0f;
        if(t < bat->soc) {
          Serial.println("LOW VOLTAGE DERATE SOC");
          bat->soc = t;
        }
      }
    }
    Serial.println("DERATE OUT:");
    Serial.print("chargeVoltage: ");
    Serial.println(bat->chargeVoltage);
    Serial.print("chargeCurrent: ");
    Serial.println(bat->chargeCurrent);
    Serial.print("dischargeCurrent: ");
    Serial.println(bat->dischargeCurrent);
    Serial.print("soc: ");
    Serial.println(bat->soc);
  }

private:

  // test if we are ready to switch to float mode
  // old code was very relaxed
  // in this version, let's make sure all cells are nearly full and well balanced before going to float.
  // FIXME - do this once every X cycles
  bool testFloat(void) {
    if(bat->minCellVoltage < cfgCellFloatStartV) return false; // at least one cell is too low
    if(bat->current > 0.0f) return false; // discharging
    if(-bat->current > bat->nomChargeCurrent * 0.03f) return false; // discharging at more than 3% of nom
    Serial.println("FLOAT OK");
    return true;
  }

  // set/limit charge voltage based on mode
  void setChargeV(float v) {
    if(bulk) {
      if(v > cfgCellMaxV * bat->numCells) v = cfgCellMaxV * bat->numCells; // clamp to normal max
    } else {
      if(v > cfgCellFloatTargetV * bat->numCells) v = cfgCellFloatTargetV * bat->numCells; // clamp to float target
    }
    bat->chargeVoltage = v;
  }

  // set/limit charge current
  void setChargeI(float i) {
    if(i > bat->nomChargeCurrent) i = bat->nomChargeCurrent;
    bat->chargeCurrent = i;
  }

  batBat * bat;
  unsigned long lastMillis;
  bool bulk; // bulk/float mode
  unsigned long bulkMillis;
  int derateErrorCount;
  bool lvLockout;
  unsigned long lvLockoutMillis;

  // fixme - config should be configurable...
  const float cfgCellMaxV = 3.65f; // maximum cell voltage
  const float cfgCellMaxDerateV = cfgCellMaxV - 0.2f; // maximum cell voltage before we derate
  const float cfgCellMinV = 2.8f; // minimum cell voltage
  const float cfgCellMinDerateV = cfgCellMinV + 0.2f; // minimum cell voltage before we derate
  const float cfgCellFloatStartV = 3.6f; // target cell voltage in float mode
  const float cfgCellFloatTargetV = 3.4f; // target charge voltage in float mode
  const float cfgCellFloatEndV = 3.35f; // end float before this voltage
  const unsigned long cfgLvLockoutMillis = (5UL * 60UL * 1000UL); // min low voltage recovery time before we restore dischare caps
  const unsigned long cfgFloatMillis = (30UL * 60UL * 1000UL); // must be in float conditions for at least this long before activating float mode
};

#endif