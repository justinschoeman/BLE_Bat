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
    out->chargeCurrent = -1.0f;
    out->chargeVoltage = -1.0f;
    out->dischargeCurrent = -1.0f;
    out->soh = -1.0f;
    out->soc = -1.0f;
    out->voltage = 0.0f; // voltage and current must always be provided - no checks required
    out->current = 0.0f;
    int current_count = 0;
    out->temperature = -1.0f;
    out->minCellVoltage = -1.0f;
    out->maxCellVoltage = -1.0f;
    out->minCellVoltageNumber = -1;
    out->maxCellVoltageNumber = -1;
    for(i = 0; i < count; i++) {
      // chargeCurrent (minimum)
      if(bats[i]->chargeCurrent >= 0.0f) {
       if(out->chargeCurrent < 0.0f) {
         out->chargeCurrent = bats[i]->chargeCurrent;
       } else {
        if(bats[i]->chargeCurrent < out->chargeCurrent) out->chargeCurrent = bats[i]->chargeCurrent;         
       }
      }
      // chargeVoltage (total)
      if(bats[i]->chargeVoltage >= 0.0f) {
       if(out->chargeVoltage < 0.0f) {
         out->chargeVoltage = bats[i]->chargeVoltage;
       } else {
        out->chargeVoltage += bats[i]->chargeVoltage;         
       }
      }
      // dischargeCurrent (minimum)
      if(bats[i]->dischargeCurrent >= 0.0f) {
       if(out->dischargeCurrent < 0.0f) {
         out->dischargeCurrent = bats[i]->dischargeCurrent;
       } else {
        if(bats[i]->dischargeCurrent < out->dischargeCurrent) out->dischargeCurrent = bats[i]->dischargeCurrent;         
       }
      }
      // voltage (total)
      out->voltage += bats[i]->voltage;
      // current (average of batteries not being balanced...)
      if(!bats[i]->balancing) {
        out->current += bats[i]->current;
        current_count++;
      }
      // temperature (in SA, max we will be the limit...)
      if(bats[i]->temperature >= 0.0f) {
       if(out->temperature < 0.0f) {
         out->temperature = bats[i]->temperature;
       } else {
        if(bats[i]->temperature > out->temperature) out->temperature = bats[i]->temperature;
       }
      }
      // soh (minimum)
      if(bats[i]->soh >= 0.0f) {
       if(out->soh < 0.0f) {
         out->soh = bats[i]->soh;
       } else {
        if(bats[i]->soh < out->soh) out->soh = bats[i]->soh;         
       }
      }
      // soc (we could work out actual energy, but really, the minimum will restrict it...)
      if(bats[i]->soc >= 0.0f) {
       if(out->soc < 0.0f) {
         out->soc = bats[i]->soc;
       } else {
        if(bats[i]->soc < out->soc) out->soc = bats[i]->soc;
       }
      }
      // minCellVoltage
      // minCellVoltageNumber
      if(bats[i]->minCellVoltage >= 0.0f) {
       if(out->minCellVoltage < 0.0f) {
         out->minCellVoltage = bats[i]->minCellVoltage;
         out->minCellVoltageNumber = bats[i]->minCellVoltageNumber + base;
       } else {
        if(bats[i]->minCellVoltage < out->minCellVoltage) {
          out->minCellVoltage = bats[i]->minCellVoltage;
          out->minCellVoltageNumber = bats[i]->minCellVoltageNumber + base;
        }          
       }
      }
      // maxCellVoltage
      // maxCellVoltageNumber
      if(bats[i]->maxCellVoltage >= 0.0f) {
       if(out->maxCellVoltage < 0.0f) {
         out->maxCellVoltage = bats[i]->maxCellVoltage;
         out->maxCellVoltageNumber = bats[i]->maxCellVoltageNumber + base;
       } else {
        if(bats[i]->maxCellVoltage > out->maxCellVoltage) {
          out->maxCellVoltage = bats[i]->maxCellVoltage;
          out->maxCellVoltageNumber = bats[i]->maxCellVoltageNumber + base;
        }          
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
    if(current_count > 0) out->current /= (float)current_count;
    out->balancing = false; // not really meaningful at this level?
    out->updateMillis = millis();
  }

  void runParallelDerate(void) {
    Serial.println("BANK PARRALEL RUN (DERATED)");
    // when derated, pick the minimum of everything...
    out->chargeCurrent = -1.0f;
    out->chargeVoltage = -1.0f;
    out->dischargeCurrent = -1.0f;
    for(int i = 0; i < count; i++) {
      // chargeCurrent (minimum)
      if(bats[i]->chargeCurrent >= 0.0f) {
       if(out->chargeCurrent < 0.0f) {
         out->chargeCurrent = bats[i]->chargeCurrent;
       } else {
        if(bats[i]->chargeCurrent < out->chargeCurrent) out->chargeCurrent = bats[i]->chargeCurrent;         
       }
      }
      // chargeVoltage (minimum)
      if(bats[i]->chargeVoltage >= 0.0f) {
       if(out->chargeVoltage < 0.0f) {
         out->chargeVoltage = bats[i]->chargeVoltage;
       } else {
        if(bats[i]->chargeVoltage < out->chargeVoltage) out->chargeVoltage = bats[i]->chargeVoltage;         
       }
      }
      // dischargeCurrent (total - never a need to derate this)
      if(bats[i]->dischargeCurrent >= 0.0f) {
       if(out->dischargeCurrent < 0.0f) {
         out->dischargeCurrent = bats[i]->dischargeCurrent;
       } else {
        out->dischargeCurrent += bats[i]->dischargeCurrent;         
       }
      }
    }
  }

  void runParallelNormal(void) {
    Serial.println("BANK PARRALEL RUN (NORMAL)");
    out->chargeCurrent = -1.0f;
    out->chargeVoltage = -1.0f;
    out->dischargeCurrent = -1.0f;
    for(int i = 0; i < count; i++) {
      // chargeCurrent (total)
      if(bats[i]->chargeCurrent >= 0.0f) {
       if(out->chargeCurrent < 0.0f) {
         out->chargeCurrent = bats[i]->chargeCurrent;
       } else {
        out->chargeCurrent += bats[i]->chargeCurrent;         
       }
      }
      // chargeVoltage (should be the same, but pick minimum to be safe)
      if(bats[i]->chargeVoltage >= 0.0f) {
       if(out->chargeVoltage < 0.0f) {
         out->chargeVoltage = bats[i]->chargeVoltage;
       } else {
        if(bats[i]->chargeVoltage < out->chargeVoltage) out->chargeVoltage = bats[i]->chargeVoltage;         
       }
      }
      // dischargeCurrent (total - never a need to derate this)
      if(bats[i]->dischargeCurrent >= 0.0f) {
       if(out->dischargeCurrent < 0.0f) {
         out->dischargeCurrent = bats[i]->dischargeCurrent;
       } else {
        out->dischargeCurrent += bats[i]->dischargeCurrent;         
       }
      }
    }
  }
  
  void runParallel(void) {
    Serial.println("BANK PARRALEL RUN!");
    // test if we are derating...
    bool derate = false;    
    for(int i = 0; i < count; i++) {
      if(bats[i]->chargeCurrent < bats[i]->nomChargeCurrent) derate = true;
      if(bats[i]->chargeVoltage < bats[i]->nomChargeVoltage) derate = true;
    }
    // set charge parameters
    if(derate) {
      runParallelDerate();
    } else {
      runParallelNormal();
    }
    // set remaining parameters
    out->soh = -1.0f;
    out->soc = -1.0f;
    out->voltage = 0.0f;
    out->current = 0.0f;
    out->temperature = -1.0f;
    out->minCellVoltage = -1.0f;
    out->maxCellVoltage = -1.0f;
    out->minCellVoltageNumber = -1;
    out->maxCellVoltageNumber = -1;
    for(int i = 0; i < count; i++) {
      // soh (minimum)
      if(bats[i]->soh >= 0.0f) {
       if(out->soh < 0.0f) {
         out->soh = bats[i]->soh;
       } else {
        if(bats[i]->soh < out->soh) out->soh = bats[i]->soh;         
       }
      }
      // soc (we could work out actual energy, but really, the minimum will restrict it...)
      if(bats[i]->soc >= 0.0f) {
       if(out->soc < 0.0f) {
         out->soc = bats[i]->soc;
       } else {
        if(bats[i]->soc < out->soc) out->soc = bats[i]->soc;
       }
      }
      // voltage (should be equal, but use average...)
      out->voltage += bats[i]->voltage;
      // current (total)
      out->current += bats[i]->current;
      // temperature (in SA, max we will be the limit...)
      if(bats[i]->temperature >= 0.0f) {
       if(out->temperature < 0.0f) {
         out->temperature = bats[i]->temperature;
       } else {
        if(bats[i]->temperature > out->temperature) out->temperature = bats[i]->temperature;
       }
      }
      // minCellVoltage
      // minCellVoltageNumber
      if(bats[i]->minCellVoltage >= 0.0f) {
       if(out->minCellVoltage < 0.0f) {
         out->minCellVoltage = bats[i]->minCellVoltage;
         out->minCellVoltageNumber = bats[i]->minCellVoltageNumber;
       } else {
        if(bats[i]->minCellVoltage < out->minCellVoltage) {
          out->minCellVoltage = bats[i]->minCellVoltage;
          out->minCellVoltageNumber = bats[i]->minCellVoltageNumber;
        }          
       }
      }
      // maxCellVoltage
      // maxCellVoltageNumber
      if(bats[i]->maxCellVoltage >= 0.0f) {
       if(out->maxCellVoltage < 0.0f) {
         out->maxCellVoltage = bats[i]->maxCellVoltage;
         out->maxCellVoltageNumber = bats[i]->maxCellVoltageNumber;
       } else {
        if(bats[i]->maxCellVoltage > out->maxCellVoltage) {
          out->maxCellVoltage = bats[i]->maxCellVoltage;
          out->maxCellVoltageNumber = bats[i]->maxCellVoltageNumber;
        }          
       }
      }
    }
    // average voltage
    out->voltage /= (float)count;
    // on the off chance that max and min cell are the same, bump max...
    if(out->maxCellVoltageNumber >= 0 && out->maxCellVoltageNumber == out->minCellVoltageNumber) {
      out->maxCellVoltageNumber++;
      if(out->maxCellVoltageNumber >= out->numCells) out->maxCellVoltageNumber = 0;
    }
    // fake cell data
    float tmpV = out->voltage;
    float tmpC = out->numCells;
    if(out->maxCellVoltageNumber >= 0) {
      tmpV -= out->maxCellVoltage;
      tmpC -= 1.0f;
    }
    if(out->minCellVoltageNumber >= 0) {
      tmpV -= out->minCellVoltage;
      tmpC -= 1.0f;
    }
    tmpV /= tmpC;
    for(int i = 0; i < out->numCells; i++) {
      out->cells[i].temperature = out->temperature;
      if(i == out->maxCellVoltageNumber) {
        out->cells[i].voltage = out->maxCellVoltage;
        continue;
      }
      if(i == out->minCellVoltageNumber) {
        out->cells[i].voltage = out->minCellVoltage;
        continue;
      }
      out->cells[i].voltage = tmpV;
    }
    out->balancing = false; // not really meaningful at this level?
    out->updateMillis = millis();
  }

  batBat ** bats;
  int count;
  batBat * out;
  bool series;
  unsigned long lastTime;
};

#endif
