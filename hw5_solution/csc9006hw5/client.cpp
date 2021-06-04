# include "Time_Client_i.h"
#include <pthread.h>
#include <tuple>
#include <signal.h>
#include <time.h>
#include <unistd.h>


#define _GNU_SOURCE

// The client program for the application.
pthread_t thread1, thread2;

void *work(void *pack)
{
  int argc; ACE_TCHAR **argv;
  std::tie (argc, argv) = (*(std::tuple<int, ACE_TCHAR **> *)pack);

  Time_Client_i *client = Time_Client_i::getInstance();

  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("\n\tTime and date client\n\n")));

  if (client->run ("Time", argc, argv) == -1)
      ACE_DEBUG ((LM_DEBUG,
                  ACE_TEXT ("Error in client->run()\n")));
  return(NULL);
}

void
handle_timeout(int sig, siginfo_t *si, void *unused)
{
  // figure out which pthread sent a signal
  // and then send a signal to that pthread
  if (sig == SIGUSR1)
  {
    pthread_kill(thread1, SIGHUP);
  }
  else if (sig == SIGUSR2)
  {
    pthread_kill(thread2, SIGHUP);
  }
  else
  {
    ;
  }
}

void set_timer(int sig, int period)
{
  struct sigaction sa_timeout;
  timer_t timerid;
  struct sigevent sev;
  struct itimerspec its;
  // First, set up the signal handling for the timeout event
  sa_timeout.sa_flags = SA_SIGINFO;
  sigemptyset(&sa_timeout.sa_mask);
  sa_timeout.sa_sigaction = handle_timeout;
  if (sigaction(sig, &sa_timeout, NULL) == -1)
  {
      handle_error("sigaction");
  }
  // create a periodic timer
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = sig;
  sev.sigev_value.sival_ptr = &timerid;
  if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1)
  {
      handle_error("timer_create");
  }
  its.it_value.tv_sec = 0;
  its.it_value.tv_nsec = period*1000000; // i.e., period ms
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;
  // start the timer
  if (timer_settime(timerid, 0, &its, NULL) == -1)
  {
      handle_error("timer_settime");
  }
}

int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  std::tuple<int, ACE_TCHAR **> pack (argc, argv);
  if (pthread_create(&thread1, NULL, work, (void *) &pack) != 0)
  {
    perror("pthread_create");
  }
  usleep(20000); // sleep for 20 ms to avoid the heisenbug for singleton
  if (pthread_create(&thread2, NULL, work, (void *) &pack) != 0)
  {
    perror("pthread_create");
  }

  // Now, set the timer for each pthread
  set_timer(SIGUSR1, 200);
  set_timer(SIGUSR2, 300);

  if (pthread_join(thread1, NULL) != 0)
    perror("pthread_join"),exit(1);
  if (pthread_join(thread2, NULL) != 0)
    perror("pthread_join"),exit(1);

  return 0;
}
