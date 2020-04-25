#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>

#include <iostream>
#include <chrono>
#include <cmath>

void setSchedulingPolicy (int policy, int priority)
{
    sched_param sched;
    sched_getparam(0, &sched);
    sched.sched_priority = priority;
    if (sched_setscheduler(0, policy, &sched)) {
        perror("sched_setscheduler");
        exit(EXIT_FAILURE);
    }
}

void workload_1ms (void)
{
	double c = 10.1;
	int repeat = 70000;
	for (int i = 1; i <= repeat; i++)
	{
		c = sqrt(i*i*c);
	}
}

int main (void)
{
	setSchedulingPolicy (SCHED_FIFO, 99);
	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
	workload_1ms ();
	std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
        const int delta = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
	std::cout << "response time = " << delta << " us" << std::endl;

	return 0;
}
