
/* PROFILING INFORMATION */
/* #define TIMERS_ENABLED */
// #define COUNTERS_ENABLED
/* the following only applies if counters are enabled (to set what the counters will measure) */
#define PAPI_MEASURE PAPICounter.PAPI_FP_INS

/*** DO NOT CHANGE THE FOLLOWING ***/

#ifdef TIMERS_ENABLED
#define TIMER_START(timername)  timername.start()
#define TIMER_STOP(timername)   timername.stop()
#define TIMER_STOP_READ(timername, dest) do { \
    timername.stop();          \
    dest = timername.secs();   \
    timername.reset();         \
} while (false)
#else
#define TIMER_START(timername)           do {} while (false)
#define TIMER_STOP(timername)            do {} while (false)
#define TIMER_STOP_READ(timername, dest) do {} while (false)
#endif

#ifdef COUNTERS_ENABLED
#define COUNTER_START(countername)  countername.start()
#define COUNTER_STOP(countername)   countername.stop()
#define COUNTER_STOP_READ(countername, dest) do { \
    countername.stop();                           \
    dest = countername.getCounterValue();         \
    countername.clear();                          \
} while (false)
#else
#define COUNTER_START(countername)           do {} while (false)
#define COUNTER_STOP(countername)            do {} while (false)
#define COUNTER_STOP_READ(countername, dest) do {} while (false)
#endif
