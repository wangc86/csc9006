#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>

#include <iostream>
#include <chrono>
#include <cmath>

#include <unistd.h>

#include <signal.h>

#include <fstream>

void pinCPU (int cpu_number)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);

    CPU_SET(cpu_number, &mask);

    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1)
    {
        perror("sched_setaffinity");
        exit(EXIT_FAILURE);
    }
}

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

void workload_1ms ()
{
    double c = 10.1;
    int repeat = 70000;
    for (int i = 1; i <= repeat; i++)
    {
        c = sqrt(i*i*c);
    }
}

sig_atomic_t out[2000];
sig_atomic_t t = 0;
void collectData (int param)
{
    std::ofstream outFile;
    outFile.open ("responseTime_t1rm.out");
    for (int i = 0; i < 2000; i++)
    {
        outFile << ::out[i] << std::endl;
    }
    outFile.close();
}

int main (void)
{
    void (*prev_handler)(int);
    prev_handler = signal (SIGINT, collectData);
  
    int period = 5000; // unit: microsecond
    int delta;
    pinCPU (0);
    setSchedulingPolicy (SCHED_FIFO, 99);
    while (1)
    {
        std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
        for (int j = 0; j < 2; j++)
        {
	    workload_1ms ();
        }
        std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
        delta = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        if (::t < 2000)
        {
	    ::out[::t++] = delta;
	}
	if (delta > period)
	{
	    continue;
	}
	else
	{
            usleep (period-delta);
	}
    }
    return 0;
}
