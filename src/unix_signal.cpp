#include "unix_signal.h"
#include <unistd.h>
//based on https://gist.github.com/azadkuh/a2ac6869661ebd3f8588

void ignoreUnixSignals(std::initializer_list<int> ignoreSignals) {
	// all these signals will be ignored.
	for (int sig : ignoreSignals)
		signal(sig, SIG_IGN);
}

void catchUnixSignals(std::initializer_list<int> signalsToCatch, __sighandler_t handler) {
	sigset_t blocking_mask;
	sigemptyset(&blocking_mask);
	for (auto sig : signalsToCatch)
		sigaddset(&blocking_mask, sig);

	struct sigaction sa;
	sa.sa_handler = handler;
	sa.sa_mask    = blocking_mask;
	sa.sa_flags   = 0;

	for (auto sig : signalsToCatch)
		sigaction(sig, &sa, nullptr);
}
