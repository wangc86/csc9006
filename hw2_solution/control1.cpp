#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <iostream>
#include <chrono>
#include <cmath>

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

int pressed = false;
std::chrono::system_clock::time_point startTime;
std::chrono::system_clock::time_point endTime;
int delta;
int count;


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

static void
handle_press(int sig, siginfo_t *si, void *unused)
{
    startTime = std::chrono::system_clock::now();
    pressed = true;
    //std::cout << "pressed!" << std::endl;
    count ++;
    if (count == 200)
    {
        std::cout << "Terminating our elevator control.." << std::endl;
        exit(EXIT_SUCCESS);
    }
}

static void
handle_timeout(int sig, siginfo_t *si, void *unused)
{
    if (pressed)
    {
        // run the 50-millisecond workload
        for (int i = 1; i < 100000; i++)
        {
            //for (int j = 1; j < 113; j++) // for powersave governor
            for (int j = 1; j < 57; j++) // for performance governor
            {
                sqrt(i*j*i);
            }
        }
        std::cout << "Sent the control command." << std::endl;
        endTime = std::chrono::system_clock::now();
        delta = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
        std::cerr << delta << std::endl;

        pressed = false;
    }
}

int main(int argc, char *argv[])
{
    char *p;
    struct sigaction sa_press, sa_timeout;

    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;

    pinCPU(0);

    // setup the signal handling for the press-button event
    sa_press.sa_flags = SA_SIGINFO;
    sigemptyset(&sa_press.sa_mask);
    sa_press.sa_sigaction = handle_press;
    if (sigaction(SIGUSR1, &sa_press, NULL) == -1)
    {
        handle_error("sigaction");
    }

    // setup the signal handling for the timer event
    sa_timeout.sa_flags = SA_SIGINFO;
    sigemptyset(&sa_timeout.sa_mask);
    sa_timeout.sa_sigaction = handle_timeout;
    if (sigaction(SIGUSR2, &sa_timeout, NULL) == -1)
    //if (sigaction(SIGRTMIN, &sa_timeout, NULL) == -1)
    {
        handle_error("sigaction");
    }

    // setup the periodic timer
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo = SIGUSR2;
    //sev.sigev_signo = SIGRTMIN;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
    {
        handle_error("timer_create");
    }
    its.it_value.tv_sec = 0;
    //its.it_value.tv_nsec = 500000000; // i.e., 500 ms
    its.it_value.tv_nsec = 100000000; // i.e., 100 ms
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;
    if (timer_settime(timerid, 0, &its, NULL) == -1)
    {
        handle_error("timer_settime");
    }

    count = 0;
    while (1)
    {
        pause();
    }
    return 0;
}
