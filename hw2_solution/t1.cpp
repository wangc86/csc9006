#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define _GNU_SOURCE

std::chrono::system_clock::time_point startTime;
std::chrono::system_clock::time_point endTime;
int delta;
int result[5000];
int count = 0;

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

void workload()
{
    // run the 50-millisecond workload;
    // adjust it for your hardware spec.
    startTime = std::chrono::system_clock::now();
    for (int i = 1; i < 100000; i++)
    {
        for (int j = 1; j < 57; j++)
        {
            sqrt(i*j*i);
        }
    }
    endTime = std::chrono::system_clock::now();
    result[count] = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count(); 
    count++;
}

static void
handle_timeout(int sig, siginfo_t *si, void *unused)
{
    workload();
}

int main(int argc, char *argv[])
{
    char *p;
    struct sigaction sa_timeout;

    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;

    pinCPU(0);

    // setup the signal handling for the timeout event
    sa_timeout.sa_flags = SA_SIGINFO;
    sigemptyset(&sa_timeout.sa_mask);
    sa_timeout.sa_sigaction = handle_timeout;
    if (sigaction(SIGUSR2, &sa_timeout, NULL) == -1)
    {
        handle_error("sigaction");
    }

    // setup a periodic timer
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR2;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
    {
        handle_error("timer_create");
    }
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 100000000; // i.e., 100 ms
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    // start the timer
    if (timer_settime(timerid, 0, &its, NULL) == -1)
    {
        handle_error("timer_settime");
    }
    workload();

    while (count<500)
    {
        pause();
    }
    // dump the result to a file
    std::ofstream out_file;
    out_file.open("resp_t1.out", std::ios::out|std::ios::trunc);
    for (int i = 0; i < count; i++)
    {
        out_file << result[i] << std::endl;
    }
    out_file.close();
    return 0;
}
