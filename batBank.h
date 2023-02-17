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
    
    // nomVoltage;
    if(out->nomVoltage < 0.0f) {
      out->nomVoltage = 0.0f;
      for(i = 0; i < count; i++) {
        if(bats[i]->nomVoltage < 0.0f) break;
        out->nomVoltage += bats[i]->nomVoltage;
      }
      if(i < count) out->nomVoltage = -1.0f; // one or more source cells have no nominal spec
    }
    
    // nomAH;
    
    // nomChargeCurrent;
    
    // nomChargeVoltage;
    
    // nomDischargeCurrent;
  }

  void initParallel(void) {
    for(int i = 0; i < count; i++) {
    }
  }

  // run bank algorithms
  void runSeries(void) {
    Serial.println("BANK SERIES RUN!");
    for(int i = 0; i < count; i++) {
    }
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
