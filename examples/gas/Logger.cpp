#include "Logger.h"

string Logger::bb = "begin barrier";
string Logger::eb = "end barrier";

bool Logger::logging = false;  // are you using the logger?

execution_log Logger::log;

int Logger::barrierCount = 0;
