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
    derateErrorCount(0)
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
  }

/*

void derate(void) {
  uint16_t delta;
  uint32_t t32;

  // always start with some sane defaults
  // for sanity sake, always keep the lowest meaningful charge voltage, and ramp up as required...
  // this  could potentially overflow
  t32 = CELL_MAX_V - bat_maxv;
  t32 *= (uint32_t)CELL_CNT;  // set the target voltage = current voltage + whatever is required to make the max cell full (spread over all cells)
  // inverter is too gentle close to voltage limits - lets make it slightly more aggressive
  t32 *= (uint32_t)2;
  derate_set_trg_v(t32);
  
  // default to normal max current
  trg_chg_i = BAT_CHG_I*100;

  // HIGH VOLTAGE LOGIC
  if(bat_maxv > CELL_MAX_V) {
    // over - shut off charging...
    Serial.println("OVERVOLTAGE!");
    trg_chg_i = 0;
    derate_set_trg_v(0);
    // do not do this - implausible values can cause inverter to ignore us!
    //trg_chg_v -= (bat_maxv - BAT_MAX_V) * 200; // retard by 200mV for each mV above target...
  } else if(bat_maxv > CELL_DERATE_V) {
    Serial.println("HIGH VOLTAGE!");
    // derate voltage - done globally above as the default
    t32 = CELL_MAX_V - bat_maxv;
    t32 *= trg_chg_i;
    t32 /= (uint32_t)(CELL_MAX_V - CELL_DERATE_V); // can never remeber C rules - just make 100% sure this doesn't demote somewhere
    trg_chg_i = t32;
  }
  
  // test - hard limit current in closing phases...
  if(bat_maxv > 3500) trg_chg_i = trg_chg_i / 10; // max 15A

  // ramp functions
  if(trg_chg_i < bat_chg_i) {
    // going down? rapid derate
    delta = bat_chg_i - trg_chg_i;
    delta /= 2;
    if(delta == 0) delta = 1;
    bat_chg_i -= delta;
  } else if(trg_chg_i > bat_chg_i) {
    // going up? very slow
    delta = trg_chg_i / 100; //100s ramp up total
    if(delta < 1) delta = 1;
    bat_chg_i += delta;
    if(bat_chg_i > trg_chg_i) bat_chg_i = trg_chg_i;
  }

  if(trg_chg_v < bat_chg_v) {
    // going down? rapid derate
    delta = bat_chg_v - trg_chg_v;
    delta /= 2;
    if(delta == 0) delta = 1;
    bat_chg_v -= delta;
  } else if(trg_chg_v > bat_chg_v) {
    // going up? very slow
    bat_chg_v += 10; // 100 s per volt... 10s per unit that the inverter can notice
    if(bat_chg_v > trg_chg_v) bat_chg_v = trg_chg_v;
  }
  Serial.print("bat_chg_v: ");
  Serial.println(bat_chg_v);
  Serial.print("trg_chg_v: ");
  Serial.println(trg_chg_v);
  Serial.print("bat_chg_i: ");
  Serial.println(bat_chg_i);
  Serial.print("trg_chg_i: ");
  Serial.println(trg_chg_i);

  // LOW VOLTAGE LOGIC

  // default to normal maximum
  bat_dis_i = BAT_DIS_I * 100U;
  
  if(bat_minv <= CELL_MIN_V) {
    // any cell <= minimum? shut off discharge
    Serial.println("LOW VOLTAGE LIMIT!");
    bat_dis_i = 0;
    bat_soc = 0;
    bat_soh = 0;
    lv_lockout = 1;
    lv_lockout_ms = millis();
  } else {
    // otherwise wait for lockout interval then release
    if(lv_lockout) {
      Serial.println("LOW VOLTAGE LOCKOUT");
      bat_dis_i = 0;
      bat_soc = 0;
      bat_soh = 0;
      if(millis() - lv_lockout_ms >= LV_LOCKOUT_MS) {
        lv_lockout = 0;        
      }
    }
  }
  */

private:

  // test if we are ready to switch to float mode
  // old code was very relaxed
  // in this version, let's make sure all cells are nearly full and well balanced before going to float.
  // FIXME - do this once every X cycles
  bool testFloat(void) {
    if(bat->minCellVoltage < cfgCellFloatV) return false; // at least one cell is too low
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

  // set/limit discharge current
  void setDischargeI(float i) {
    if(i > bat->nomDischargeCurrent) i = bat->nomDischargeCurrent;
    bat->dischargeCurrent = i;
  }

  batBat * bat;
  unsigned long lastMillis;
  bool bulk; // bulk/float mode
  unsigned long bulkMillis;
  int derateErrorCount;

  // fixme - config should be configurable...
  const float cfgCellMaxV = 3.65f; // maximum cell voltage
  const float cfgCellMinV = 2.5f; // minimum cell voltage
  const float cfgCellDerateV = cfgCellMaxV - 0.2f; // maximum cell voltage before we derate
  const float cfgCellFloatV = 3.6f; // target cell voltage in float mode
  const float cfgCellFloatTargetV = 3.4f; // target charge voltage in float mode
  const float cfgCellFloatEndV = 3.35f; // end float before this voltage
  const unsigned long cfgLvLockoutMillis = (5UL * 60UL * 1000UL); // min low voltage recovery time before we restore dischare caps
  const unsigned long cfgFloatMillis = (30UL * 60UL * 1000UL); // must be in float conditions for at least this long before activating float mode
};

#endif