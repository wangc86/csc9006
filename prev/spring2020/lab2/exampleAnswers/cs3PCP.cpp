#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>

#include <iostream>
#include <chrono>
#include <cmath>

#include <unistd.h>

//#include <signal.h>
#include <fstream>
#include <string>

#include <pthread.h>

// our task set period (in microseconds)
#define TASK_PERIOD_1 50000
//#define TASK_PERIOD_2 100000
//#define TASK_PERIOD_3 200000
#define TASK_PERIOD_2 107000
#define TASK_PERIOD_3 167000
#define TASK_PERIOD_MID 87000
// our task set WCET (in milliseconds)
#define TASK_WCET_1 1
#define TASK_WCET_2 50
#define TASK_WCET_3 10
#define TASK_WCET_MID 7

std::string rep; // number of repetitions

//pthread_mutex_t mutex_section1 = PTHREAD_MUTEX_INITIALIZER;
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
        perror("pthread_setschedparam");
        exit(EXIT_FAILURE);
    }
    sched.sched_priority = priority;
    if (pthread_setschedparam(pthread_self(), newPolicy, &sched)) {
        perror("pthread_setschedparam");
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

void workload_500us ()
{
    double c = 10.1;
    int repeat = 35000;
    for (int i = 1; i <= repeat; i++)
    {
        c = sqrt(i*i*c);
    }
}

void *taskTau_1 (void *arg1)
{
    setSchedulingPolicy (SCHED_FIFO, 99);
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime, endTime2;
    int delta, delta2;
    std::ofstream outFile;
    outFile.open ("cs_1.out" + ::rep);
    while (1)
    {
        startTime = std::chrono::system_clock::now();

        // entering the critical section ...
        {
        pthread_mutex_lock (&mutex_section1);
	    workload_500us ();
        pthread_mutex_unlock (&mutex_section1);
        }
        // leaving the critical section ...

        for (int j = 0; j < TASK_WCET_1; j++)
        {
	    workload_1ms ();
        }
        endTime = std::chrono::system_clock::now();
        delta = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        outFile << delta << std::endl;
        endTime2 = std::chrono::system_clock::now();
        delta2 = std::chrono::duration_cast<std::chrono::microseconds>(endTime2 - startTime).count();
	if (delta2 < TASK_PERIOD_1)
	{
            usleep (TASK_PERIOD_1 - delta2);
	}
    }
    outFile.close ();
    return (NULL);
}

void *taskTau_2 (void *arg2)
{
    setSchedulingPolicy (SCHED_FIFO, 98);
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime, endTime2;
    int delta, delta2;
    std::ofstream outFile;
    outFile.open ("cs_2.out" + ::rep);
    while (1)
    {
        startTime = std::chrono::system_clock::now();
        for (int j = 0; j < TASK_WCET_2; j++)
        {
	    workload_1ms ();
        }
        endTime = std::chrono::system_clock::now();
        delta = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        outFile << delta << std::endl;
        endTime2 = std::chrono::system_clock::now();
        delta2 = std::chrono::duration_cast<std::chrono::microseconds>(endTime2 - startTime).count();
	if (delta2 < TASK_PERIOD_2)
	{
            usleep (TASK_PERIOD_2 - delta2);
	}
    }
    outFile.close ();
    return (NULL);
}

void *taskTau_3 (void *arg3)
{
    setSchedulingPolicy (SCHED_FIFO, 97);
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime, endTime2;
    int delta, delta2;
    std::ofstream outFile;
    outFile.open ("cs_3.out" + ::rep);
    while (1)
    {
        startTime = std::chrono::system_clock::now();

        // entering the critical section ...
        {
        pthread_mutex_lock (&mutex_section1);
	    workload_1ms ();
	    workload_1ms ();
	    workload_1ms ();
        pthread_mutex_unlock (&mutex_section1);
        }
        // leaving the critical section ...

        for (int j = 0; j < TASK_WCET_3; j++)
        {
	    workload_1ms ();
        }
        endTime = std::chrono::system_clock::now();
        delta = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
        outFile << delta << std::endl;
        endTime2 = std::chrono::system_clock::now();
        delta2 = std::chrono::duration_cast<std::chrono::microseconds>(endTime2 - startTime).count();
	if (delta2 < TASK_PERIOD_3)
	{
            usleep (TASK_PERIOD_3 - delta2);
	}
    }
    outFile.close ();
    return (NULL);
}

// We skip the logs for these middle-priority tasks
// to keep things simple and thread-safe.
void *taskTau_mid (void *arg)
{
    setSchedulingPolicy (SCHED_FIFO, 98);
    std::chrono::system_clock::time_point startTime;
    std::chrono::system_clock::time_point endTime;
    int delta;
    while (1)
    {
        startTime = std::chrono::system_clock::now();

        for (int j = 0; j < TASK_WCET_MID; j++)
        {
	    workload_1ms ();
        }
        endTime = std::chrono::system_clock::now();
        delta = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
	if (delta < TASK_PERIOD_MID)
	{
            usleep (TASK_PERIOD_MID - delta);
	}
    }
    return (NULL);
}



int arg1, arg2, arg3, arg_mid[7];
int main (int argc, char *argv[])
{
    std::string exename(argv[0]);
    if (argc != 2)
    {
        std::cerr << "Usage: " + exename + " (# of repetitions)" << std::endl;
        exit(0);
    }
    //::rep = std::stoi(argv[1]);
    std::string s1(argv[1]);
    ::rep = "-" + s1;
    //std::cout << rep << std::endl;

    pinCPU (0);

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


    pthread_t tau_1, tau_2, tau_3;
    pthread_t tau_mid[7];
    if (pthread_create(&tau_1, 
		       NULL,
		       taskTau_1,
                       (void *) &arg1) != 0)
    {
        perror("pthread_create"), exit(1);
    }

    if (pthread_create(&tau_2, 
		       NULL,
		       taskTau_2,
                       (void *) &arg2) != 0)
    {
        perror("pthread_create"), exit(1);
    }

    if (pthread_create(&tau_3, 
		       NULL,
		       taskTau_3,
                       (void *) &arg3) != 0)
    {
        perror("pthread_create"), exit(1);
    }

    for (int i = 0; i < 7; i++)
    {
      if (pthread_create(&tau_mid[i], 
		       NULL,
		       taskTau_mid,
                       (void *) &arg_mid[i]) != 0)
      {
          perror("pthread_create"), exit(1);
      }
    }

    pinCPU (1);

    if (pthread_join(tau_1, NULL) != 0)
    {
	perror("pthread_join"),exit(1);
    }
    if (pthread_join(tau_2, NULL) != 0)
    {
	perror("pthread_join"),exit(1);
    }
    if (pthread_join(tau_3, NULL) != 0)
    {
	perror("pthread_join"),exit(1);
    }
    for (int i = 0; i < 7; i++)
    {
      if (pthread_join(tau_mid[i], NULL) != 0)
      {
  	  perror("pthread_join"),exit(1);
      }
    }

    return 0;
}
