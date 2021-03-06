#pragma once

#include <gasnet.h>
#include <gasnet_tools.h>

namespace upcxx {
  class timer {
  private:
    gasnett_tick_t cur_tick, elap_tick;
  public:
    timer() : cur_tick(0), elap_tick(0) {}
  
    inline void start() {
      cur_tick = gasnett_ticks_now();
    }

    inline void stop() {
      elap_tick += gasnett_ticks_now() - cur_tick;
    }

    inline void reset() { elap_tick = 0; }
  
    inline double secs() const {
      return gasnett_ticks_to_ns(elap_tick) / 1.0E9;
    }

    inline double millis() const {
      return gasnett_ticks_to_ns(elap_tick) / 1.0E6;
    }

    inline double micros() const {
      return gasnett_ticks_to_ns(elap_tick) / 1.0E3;
    }

    inline double nanos() const {
      return gasnett_ticks_to_ns(elap_tick);
    }

    static inline double granularity() {
      return gasnett_tick_granularityus();
    }

    static inline double overhead() {
      return gasnett_tick_overheadus();
    }
  };
}
