#ifndef _BAT_BANK_H_
#define _BAT_BANK_H_

#include "bat.h"

class batBank {
public:
  batBank(batBat ** bats_, int count_, batBat * out_, bool series_) :
    bats(bats_),
    count(count_),
    out(out_),
    series(series_),
    lastTime(0)
    {}

  // call once in setup
  void init(void) {
    if(series) {
      initSeries();
    } else {
      initParallel();
    }
    out->dump();
  }

  // arduino loop style function - call repeatedly, and this handles all logic internally
  void run(void) {
    // first scan all bats for updates and compare to our last time
    // just use the sum, as even if it wraps, it should give a unique state for all batteries...
    unsigned long testms = 0;
    for(int i = 0; i < count; i++) {
      if(bats[i]->updateMillis == 0) return; // battery not initialised yet - can not run!
      testms += bats[i]->updateMillis;
    }
    if(testms == lastTime) return;
    lastTime = testms;
    if(series) {
      runSeries();
    } else {
      runParallel();
    }
    out->dump();
  }

private:
  // init banks - basically just upscale nominals, if not already set.
  void initSeries(void) {
    int i;
    
    // nomVoltage - total series voltage
    if(out->nomVoltage < 0.0f) {
      out->nomVoltage = 0.0f;
      for(i = 0; i < count; i++) {
        if(bats[i]->nomVoltage < 0.0f) break;
        out->nomVoltage += bats[i]->nomVoltage;
      }
      if(i < count) out->nomVoltage = -1.0f; // one or more source cells have no nominal spec
    }
    
    // nomAH - should be identical, but if not, lowest will limit the bank
    if(out->nomAH < 0.0f) {
      for(i = 0; i < count; i++) {
        if(bats[i]->nomAH < 0.0f) break;
        if(out->nomAH < 0.0f) {
          out->nomAH = bats[i]->nomAH;
        } else {
          if(bats[i]->nomAH < out->nomAH) out->nomAH = bats[i]->nomAH;
        }
      }
      if(i < count) out->nomAH = -1.0f; // one or more source cells have no nominal spec
    }
    
    // nomChargeCurrent - should be identical, but if not, lowest will limit the bank
    if(out->nomChargeCurrent < 0.0f) {
      for(i = 0; i < count; i++) {
        if(bats[i]->nomChargeCurrent < 0.0f) break;
        if(out->nomChargeCurrent < 0.0f) {
          out->nomChargeCurrent = bats[i]->nomChargeCurrent;
        } else {
          if(bats[i]->nomChargeCurrent < out->nomChargeCurrent) out->nomChargeCurrent = bats[i]->nomChargeCurrent;
        }
      }
      if(i < count) out->nomChargeCurrent = -1.0f; // one or more source cells have no nominal spec
    }
    
    // nomChargeVoltage  - total series voltage
    if(out->nomChargeVoltage < 0.0f) {
      out->nomChargeVoltage = 0.0f;
      for(i = 0; i < count; i++) {
        if(bats[i]->nomChargeVoltage < 0.0f) break;
        out->nomChargeVoltage += bats[i]->nomChargeVoltage;
      }
      if(i < count) out->nomChargeVoltage = -1.0f; // one or more source cells have no nominal spec
    }
    
    // nomDischargeCurrent  - should be identical, but if not, lowest will limit the bank
    if(out->nomDischargeCurrent < 0.0f) {
      for(i = 0; i < count; i++) {
        if(bats[i]->nomDischargeCurrent < 0.0f) break;
        if(out->nomDischargeCurrent < 0.0f) {
          out->nomDischargeCurrent = bats[i]->nomDischargeCurrent;
        } else {
          if(bats[i]->nomDischargeCurrent < out->nomDischargeCurrent) out->nomDischargeCurrent = bats[i]->nomDischargeCurrent;
        }
      }
      if(i < count) out->nomDischargeCurrent = -1.0f; // one or more source cells have no nominal spec
    }
  }

  void initParallel(void) {
    int i;
    
    // nomVoltage - should really not parallel banks of different voltages, but have to do something sane here. just average
    if(out->nomVoltage < 0.0f) {
      out->nomVoltage = 0.0f;
      for(i = 0; i < count; i++) {
        if(bats[i]->nomVoltage < 0.0f) break;
        out->nomVoltage += bats[i]->nomVoltage;
      }
      if(i < count) {
        out->nomVoltage = -1.0f; // one or more source cells have no nominal spec
      } else {
        out->nomVoltage /= (float)count;
      }
    }
    
    // nomAH - add
    if(out->nomAH < 0.0f) {
      out->nomAH = 0.0f;
      for(i = 0; i < count; i++) {
        if(bats[i]->nomAH < 0.0f) break;
        out->nomAH += bats[i]->nomAH;
      }
      if(i < count) out->nomAH = -1.0f; // one or more source cells have no nominal spec
    }
    
    // nomChargeCurrent - add
    if(out->nomChargeCurrent < 0.0f) {
      out->nomChargeCurrent = 0.0f;
      for(i = 0; i < count; i++) {
        if(bats[i]->nomChargeCurrent < 0.0f) break;
        out->nomChargeCurrent += bats[i]->nomChargeCurrent;
      }
      if(i < count) out->nomChargeCurrent = -1.0f; // one or more source cells have no nominal spec
    }
    
    // nomChargeVoltage  - lowest is most restrictive
    if(out->nomChargeVoltage < 0.0f) {
      for(i = 0; i < count; i++) {
        if(bats[i]->nomChargeVoltage < 0.0f) break;
        if(out->nomChargeVoltage < 0.0f) {
          out->nomChargeVoltage = bats[i]->nomChargeVoltage;
        } else {
          if(bats[i]->nomChargeVoltage < out->nomChargeVoltage) out->nomChargeVoltage = bats[i]->nomChargeVoltage;
        }
      }
      if(i < count) out->nomChargeVoltage = -1.0f; // one or more source cells have no nominal spec
    }
    
    // nomDischargeCurrent - add
    if(out->nomDischargeCurrent < 0.0f) {
      out->nomDischargeCurrent = 0.0f;
      for(i = 0; i < count; i++) {
        if(bats[i]->nomDischargeCurrent < 0.0f) break;
        out->nomDischargeCurrent += bats[i]->nomDischargeCurrent;
      }
      if(i < count) out->nomDischargeCurrent = -1.0f; // one or more source cells have no nominal spec
    }
  }

  // run bank algorithms
  void runSeries(void) { 
    int i;
    
    Serial.println("BANK SERIES RUN!");
    // run through each cell and build output...
    int base = 0;
    for(i = 0; i < count; i++) {
      if(i == 0) {
        // chargeCurrent
        out->chargeCurrent = bats[i]->chargeCurrent;
        // chargeVoltage
        out->chargeVoltage = bats[i]->chargeVoltage;
        // dischargeCurrent
        out->dischargeCurrent = bats[i]->dischargeCurrent;
        // voltage
        out->voltage = bats[i]->voltage;
        // current
        out->current = bats[i]->current;
        // temperature
        out->temperature = bats[i]->temperature;
        // soh
        out->soh = bats[i]->soh;
        // soc
        out->soc = bats[i]->soc;
        // minCellVoltage
        out->minCellVoltage = bats[i]->minCellVoltage;
        // maxCellVoltage
        out->maxCellVoltage = bats[i]->maxCellVoltage;
        // minCellVoltageNumber
        out->minCellVoltageNumber = bats[i]->minCellVoltageNumber;
        // maxCellVoltageNumber
        out->maxCellVoltageNumber = bats[i]->maxCellVoltageNumber;
      } else {
        // chargeCurrent (min)
        if(bats[i]->chargeCurrent < out->chargeCurrent) out->chargeCurrent = bats[i]->chargeCurrent;
        // chargeVoltage (total)
        out->chargeVoltage += bats[i]->chargeVoltage;
        // dischargeCurrent (min)
        if(bats[i]->dischargeCurrent < out->dischargeCurrent) out->dischargeCurrent = bats[i]->dischargeCurrent;
        // voltage (total)
        out->voltage += bats[i]->voltage;
        // current (average)
        out->current += bats[i]->current;
        // temperature (average)
        out->temperature += bats[i]->temperature;
        // soh (min)
        if(bats[i]->soh < out->soh) out->soh = bats[i]->soh;
        // soc (min)
        if(bats[i]->soc < out->soc) out->soc = bats[i]->soc;
        // minCellVoltage
        // minCellVoltageNumber
        if(bats[i]->minCellVoltage < out->minCellVoltage) {
          out->minCellVoltage = bats[i]->minCellVoltage;
          out->minCellVoltageNumber = bats[i]->minCellVoltageNumber + base;
        }
        // maxCellVoltage
        // maxCellVoltageNumber
        if(bats[i]->maxCellVoltage > out->maxCellVoltage) {
          out->maxCellVoltage = bats[i]->maxCellVoltage;
          out->maxCellVoltageNumber = bats[i]->maxCellVoltageNumber + base;
        }
      }
      // cells last, as we need to preserve base until we are done...
      for(int j = 0; j < bats[i]->numCells; j++) {
        if(base >= out->numCells) {
          Serial.println("MORE SERIES CELLS THAN OUTPUT BANK!!!");
        } else {
          out->cells[base].voltage = bats[i]->cells[j].voltage;
          out->cells[base].temperature = bats[i]->cells[j].temperature;
          base++;
        }
      }
    }
    out->current /= (float)count;
    out->temperature /= (float)count;
    
/*
  float chargeCurrent;
  float chargeVoltage;
  float dischargeCurrent;
*/
    out->balancing = false; // not really meaningful at this level?
    out->updateMillis = millis();
  }

  void runParallel(void) {
    Serial.println("BANK PARRALEL RUN!");
    for(int i = 0; i < count; i++) {
    }
  }

  batBat ** bats;
  int count;
  batBat * out;
  bool series;
  unsigned long lastTime;
};

#endif
