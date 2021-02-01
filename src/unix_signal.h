#ifndef SIGNALS_H
#define SIGNALS_H
#include <initializer_list>
#include <signal.h>
void ignoreUnixSignals(std::initializer_list<int> ignoreSignals);
void catchUnixSignals(std::initializer_list<int> signalsToCatch, __sighandler_t);
#endif
