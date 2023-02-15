#ifndef _BAT_BMS_MANAGER_H_
#define _BAT_BMS_MANAGER_H_

#include "batBMS.h"
#include "config.h"

class batBMSManager {
public:
  batBMSManager(batBMS ** bmss_, int count_, unsigned long poll_time_) :
    bmss(bmss_), 
    count(count_),
    poll_time(poll_time_),
    mode(0)
    {}
  
  // -1 = error, 0 = run some more, 1 = done
  int run(void) {
    if(mode == 0) {
      run_time = millis();
      mode = 1;
    }
    for (int i = 0; i < count; i++) {
      if (bmss[i]->run() < 0) {
        Serial.print("Error running battery ");
        Serial.println(bmss[i]->myId());
      }
    }
    // waiting for poll
    if (millis() - run_time < poll_time) return 0;
    // advance poll timer
    run_time += BAT_CFG_POLL_TIME;
    return poll();
  }

private:
  // parallel poll
  int poll(void) {
    int ret = 0;
    // scan all batteries
    for(int i = 0; i < count; i++) {
      if(i) delay(BAT_CFG_POLL_DELAY);
      if(!bmss[i]->poll()) {
        Serial.print("Poll failed: ");
        Serial.println(bmss[i]->myId());
        ret = -1;
      }
    }
    Serial.println("polls sent");
    return ret;
  }

  batBMS ** bmss;
  int count;
  unsigned long poll_time;
  int mode;
  unsigned long run_time;
};

/*

  old code for sequential poll - consider reimplementing this if required...

  // sequential poll
  if (bmsNum < 0) {
    // waiting for poll
    if (millis() - runtime < BAT_CFG_POLL_TIME) return;
    // timer expired - start poll and schedule next...
    bmsNum = 0;
    isPolled = 0;
    runtime += BAT_CFG_POLL_TIME;
  }
  if (!isPolled) {
    Serial.print("POLL: ");
    Serial.println(bmsNum);
    if (!myBMSs[bmsNum].poll()) {
      Serial.print("Poll failed: ");
      Serial.println(myBMSs[bmsNum].myId());
      // fall through and advance battery
    } else {
      Serial.println("poll sent");
      isPolled = true;
    }
  }
  if (isPolled) {
    // busy polling
    // fixme - timeout? or trust the battery driver?
    if (myBMSs[bmsNum].isBusy()) return;
  }
  Serial.println("poll done");
  bmsNum++;
  isPolled = false;
  if (bmsNum >= bmsCount) {
    Serial.println("poll complete");
    bmsNum = -1;
    if (millis() - runtime >= BAT_CFG_POLL_TIME) {
      Serial.println("WARNING - POLL OVERRUN!");
    }
  }

*/

#endif