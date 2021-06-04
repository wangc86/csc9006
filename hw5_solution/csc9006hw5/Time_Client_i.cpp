#include "Time_Client_i.h"
#include "ace/OS_NS_time.h"
#include <iostream>

#include <pthread.h>

pthread_mutex_t mutex_client = PTHREAD_MUTEX_INITIALIZER;

// This is the interface program that accesses the remote object

Time_Client_i *Time_Client_i::uniqueInstance;

void
handle_sighup(int sig, siginfo_t *si, void *unused)
{
  ;// need to do nothing---this is designed to make pause() return
}

// Constructor.
Time_Client_i::Time_Client_i (void)
{
  //no-op
}

//Destructor.
Time_Client_i::~Time_Client_i (void)
{
  //no-op
}

int
Time_Client_i::run (const char *name,
                    int argc,
                    ACE_TCHAR *argv[])
{
  pthread_mutex_lock( &mutex_client );
  if (!this->initialized)
  {
    // Initialize the client.
    if (client_.init (name, argc, argv) == -1)
    {
      pthread_mutex_unlock( &mutex_client );
      return -1;
    }
    this->initialized = true;
  }
  this->ref_count ++;
  ACE_DEBUG ((LM_DEBUG, ACE_TEXT ("%d "), this->ref_count));
  pthread_mutex_unlock( &mutex_client );

  // Set up the signal handling for this thread
  struct sigaction sa_hup;
  sa_hup.sa_flags = SA_SIGINFO;
  sigemptyset(&sa_hup.sa_mask);
  sa_hup.sa_sigaction = handle_sighup;
  if (sigaction(SIGHUP, &sa_hup, NULL) == -1)
  {
      handle_error("sigaction");
  }

  try
    {
      // 64-bit OS's require pointers to be aligned on an
      // 8 byte boundary.  64-bit HP-UX requires a double to do this
      // while a long does it for 64-bit Solaris.
#if defined (HPUX)
      CORBA::Double padding = 0.0;
#else
      CORBA::Long padding = 0;
#endif /* HPUX */
      time_t timedate;
      CORBA::Long number_of_requests;

      ACE_UNUSED_ARG (padding);

      for (int i = 0; i < 15; i++)
      {
          pause();

          //Make the RMI.
          timedate = static_cast <time_t> (client_->current_time ());

          // Print out value
          // Use ACE_OS::ctime_r(), ctime() doesn't seem to work properly
          // under 64-bit solaris.
          ACE_TCHAR ascii_timedate[64] = ACE_TEXT ("");

          ACE_OS::ctime_r (&timedate, ascii_timedate, 64);

          ACE_DEBUG ((LM_DEBUG,
                      //ACE_TEXT ("string time is %s\n"),
                      ACE_TEXT ("string time is %s"),
                      ascii_timedate));

          number_of_requests = client_->count_requests ();
          ACE_DEBUG ((LM_DEBUG,
                      "thread (%P|%t): # of requests so far: %d\n\n",
                      number_of_requests));
          //std::cout << "# of requests so far : "
          //          << number_of_requests << std::endl;
      }

      if (client_.do_shutdown () == 1)
      {
        pthread_mutex_lock( &mutex_client );
        this->ref_count --;
        if (this->ref_count == 0)
        {
          client_->shutdown ();
        }
        pthread_mutex_unlock( &mutex_client );
      }
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("\tException");
      return -1;
    }

  return 0;
}
