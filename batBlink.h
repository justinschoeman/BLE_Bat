#ifndef _BAT_BLINK_H_
#define _BAT_BLINK_H_

class batBlink {
public:
  batBlink(int pin_, unsigned long mark_, unsigned long space_) : pin(pin_), mark(mark_), space(space_), isInit(false) {}

  void setTimes(unsigned long mark_, unsigned long space_) {
    mark = mark_;
    space = space_;
  }

  void run(void) {
    if(!isInit) doInit();
    if(active) {
      // led is on, evaulate turn-off
      if(millis() - lastMS > mark) {
        digitalWrite(pin, LOW);
        active = false;
        lastMS = millis();
      }
    } else {
      if(millis() - lastMS > space) {
        digitalWrite(pin, HIGH);
        active = true;
        lastMS = millis();
      }
    }
  }

private:
  int pin;
  unsigned long mark;
  unsigned long space;
  bool active;
  bool isInit;
  unsigned long lastMS;

  void doInit(void) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
    active = false;
    isInit = true;
    lastMS = millis();
  }
};

#endif
