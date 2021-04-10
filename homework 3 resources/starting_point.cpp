#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>

#include <iostream>
#include <chrono>
#include <cmath>

#include <unistd.h>
#include <signal.h>
#include <fstream>
#include <string>
#include <errno.h>

#include <pthread.h>

// our task set period (Unit: ms)
#define TASK_PERIOD_1 139
#define TASK_PERIOD_2 257

#define BUFFER_SIZE 500000

#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)
#define _GNU_SOURCE

std::chrono::system_clock::time_point startTime, endTime;
int delta;
int rel; // number of releases of the high-priority task
int result[BUFFER_SIZE]; // TODO: the size should be larger than rel
int count = 0;

pthread_mutex_t mutex_section1;

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

void setSchedulingPolicy (int newPolicy, int priority)
{
    sched_param sched;
    int oldPolicy;
    if (pthread_getschedparam(pthread_self(), &oldPolicy, &sched)) {
        perror("pthread_getschedparam");
        exit(EXIT_FAILURE);
    }
    sched.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), newPolicy, &sched)) {
        perror("pthread_setschedparam");
        exit(EXIT_FAILURE);
    }
}

void workload_10ms ()
{
    // FIXME: tune it for your machine
    for (int i = 1; i < 100000; i++)
    {
        for (int j = 1; j < 13; j++)
        {
            sqrt(i*j*i);
        }
    }
}

void taskTau_1 (union sigval nouse)
{
    setSchedulingPolicy (SCHED_FIFO, 98);

    startTime = std::chrono::system_clock::now();

    // we give the WCET = 30 ms
    workload_10ms ();
    {
    pthread_mutex_lock (&mutex_section1);
        workload_10ms ();
    pthread_mutex_unlock (&mutex_section1);
    }
    workload_10ms ();

    endTime = std::chrono::system_clock::now();
    result[count] = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count(); 
    count++;

    // dump the result to a file
    if (count >= ::rel)
    {
        std::ofstream out_file;
        out_file.open("priority_inversion.out", std::ios::out|std::ios::trunc);
        for (int i = 0; i < count; i++) // TODO: boundary check
        {
            out_file << result[i] << std::endl;
            if (i == BUFFER_SIZE-1)
            {
                // discard the rest of data
                break;
            }
        }
        out_file.close();
        exit(EXIT_SUCCESS);
    }
}

// You may add some logging for this low-priority task
// to help understand the behavior of your task set.
void taskTau_2 (union sigval nouse)
{
    setSchedulingPolicy (SCHED_FIFO, 96);

    // we give the WCET = 100 ms
    for (int i = 0; i < 3; i++)
    {
        workload_10ms ();
    }
    {
    pthread_mutex_lock (&mutex_section1);
        for (int i = 0; i < 2; i++)
        {
            workload_10ms ();
        }
    pthread_mutex_unlock (&mutex_section1);
    }
    for (int i = 0; i < 5; i++)
    {
        workload_10ms ();
    }
}

void taskTau_3 (union sigval nouse)
{
    setSchedulingPolicy (SCHED_FIFO, 97);
    // FIXME: ...
}
// FIXME: ...

void initialize_task (int sigid, int period, void (*target_function) (union sigval))
{
    struct sigaction sa_timeout;
    timer_t timerid;
    struct sigevent sev;
    struct itimerspec its;

    // setup a periodic timer
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = target_function;
    sev.sigev_signo = SIGRTMIN+sigid;
    sev.sigev_value.sival_ptr = &timerid;
    if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
    {
        handle_error("timer_create");
    }
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 1000000*period;
    its.it_interval.tv_sec = its.it_value.tv_sec;
    its.it_interval.tv_nsec = its.it_value.tv_nsec;

    // start the timer
    if (timer_settime(timerid, 0, &its, NULL) == -1)
    {
        handle_error("timer_settime");
    }
}

int main (int argc, char *argv[])
{
    std::string exename(argv[0]);
    if (argc != 3)
    {
        std::cerr << "Usage: sudo " + exename + " (none, pip, or pcp) (# of releases)" << std::endl;
        exit(EXIT_FAILURE);
    }
    std::string config(argv[1]);
    if (config.compare("none") == 0)
    {
        mutex_section1 = PTHREAD_MUTEX_INITIALIZER;
    }
    else if (config.compare("pip") == 0)
    {
        pthread_mutexattr_t mutexattr_prioinherit;
        int mutex_protocol;
        pthread_mutexattr_init (&mutexattr_prioinherit);
        pthread_mutexattr_getprotocol (&mutexattr_prioinherit,
                                       &mutex_protocol);
        pthread_mutexattr_setprotocol (&mutexattr_prioinherit,
                                       PTHREAD_PRIO_INHERIT);
        pthread_mutex_init (&mutex_section1, &mutexattr_prioinherit);
    }
    else if (config.compare("pcp") == 0)
    {
        pthread_mutexattr_t mutexattr_prioceiling;
        int mutex_protocol, high_prio;
        high_prio = sched_get_priority_max(SCHED_FIFO);
        pthread_mutexattr_init (&mutexattr_prioceiling);
        pthread_mutexattr_getprotocol (&mutexattr_prioceiling,
                                       &mutex_protocol);
        pthread_mutexattr_setprotocol (&mutexattr_prioceiling,
                                       PTHREAD_PRIO_PROTECT);
        pthread_mutexattr_setprioceiling (&mutexattr_prioceiling,
                                          high_prio);
        pthread_mutex_init (&mutex_section1, &mutexattr_prioceiling);
    }
    else
    {
        std::cerr << "Usage: sudo " + exename + " (none, pip, or pcp) (# of releases)" << std::endl;
        exit(EXIT_FAILURE);
    }
    ::rel = std::stoi(argv[2]);

    pinCPU (0);
    sched_param sched;
    sched_getparam(0, &sched);
    sched.sched_priority = 99;
    if (sched_setscheduler(0, SCHED_FIFO, &sched)) {
        perror("sched_setscheduler");
        exit(EXIT_FAILURE);
    }

    initialize_task (1, TASK_PERIOD_1, taskTau_1);
    initialize_task (2, TASK_PERIOD_2, taskTau_2);
    // FIXME: initialize your middle-priority tasks here

    while (true)
    {
        pause();
    }

    return 0;
}
