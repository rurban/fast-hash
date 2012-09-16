#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <signal.h>
#include <unistd.h>
#include <ulib/periodic.h>

volatile long cnt_1s = 0;
volatile long cnt_3s = 0;
volatile long cnt_5s = 0;

static void sig_alarm_handler(int)
{
	printf("1s counter: %ld\n", cnt_1s);
	printf("3s counter: %ld\n", cnt_3s);
	printf("5s counter: %ld\n", cnt_5s);

	alarm(1);
}

void register_sig_handler()
{
	struct sigaction sigact;

	sigact.sa_handler = sig_alarm_handler;
	sigact.sa_flags = 0;
	if (sigaction(SIGALRM, &sigact, NULL)) {
		perror("sigaction");
		exit(-1);
	}
	alarm(1);
}

void *print_thread(void *interval)
{
	long n = (long)interval;
	
	switch (n) {
	case 1: ++cnt_1s;
		break;
	case 3: ++cnt_3s;
		break;
	case 5: ++cnt_5s;
		break;
	}

	return NULL;
}

int main()
{
	register_sig_handler();
	ulib::periodic thdmgr;

	thdmgr.schedule_repeated(ulib::us_from_now(0), 1000000, print_thread, (void *)1);
	thdmgr.schedule_repeated(ulib::us_from_now(0), 3000000, print_thread, (void *)3);
	thdmgr.schedule_repeated(ulib::us_from_now(0), 5000000, print_thread, (void *)5);

	assert(thdmgr.start() == 0);

	while (cnt_1s < 12)
		sleep(1);
	thdmgr.stop_and_join();

	printf("passed\n");
	
	return 0;
}
