#include "Supplier.h"
#include "orbsvcs/RtecEventChannelAdminC.h"
#include "orbsvcs/Event_Service_Constants.h"
#include "orbsvcs/CosNamingC.h"
#include "ace/OS_NS_unistd.h"

#include <chrono>
#include <pthread.h>
#include <tuple>
#include <signal.h>
#include <time.h>
#include <unistd.h> // for usleep()
#include <error.h>
#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

pthread_t thread[5];

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


void
handle_sighup(int sig, siginfo_t *si, void *unused)
{
  ;// need to do nothing---this is designed to make pause() return
}

void *work(void *pack)
{
  int argc; ACE_TCHAR **argv;
  int id;
  std::tie (argc, argv, id) =
    (*(std::tuple<int, ACE_TCHAR **, int> *)pack);

  Supplier supplier(id);
printf("One supplier created.\n");

  // Set up the signal handling for this thread
  struct sigaction sa_hup;
  sa_hup.sa_flags = SA_SIGINFO;
  sigemptyset(&sa_hup.sa_mask);
  sa_hup.sa_sigaction = handle_sighup;
  if (sigaction(SIGHUP, &sa_hup, NULL) == -1)
  {
      handle_error("sigaction");
  }

  supplier.run (argc, argv);

  return (NULL);
}

void
handle_timeout(int sig, siginfo_t *si, void *unused)
{
  // figure out which pthread sent a signal
  // and then send a signal to that pthread
  if (sig == SIGRTMIN+0)
  {
    pthread_kill(thread[0], SIGHUP);
  }
  else if (sig == SIGRTMIN+1)
  {
    pthread_kill(thread[1], SIGHUP);
  }
  else if (sig == SIGRTMIN+2)
  {
    pthread_kill(thread[2], SIGHUP);
  }
  else if (sig == SIGRTMIN+3)
  {
    pthread_kill(thread[3], SIGHUP);
  }
  else if (sig == SIGRTMIN+4)
  {
    pthread_kill(thread[4], SIGHUP);
  }
  else
  {
    ACE_ERROR ((LM_ERROR,
                " (%P|%t) a signal was not handled.\n"));
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
  pinCPU(0);

  int s_id[5] = {0, 1, 2, 3, 4};
  int s_period[5] = {100, 100, 100, 200, 200};

  for (int i = 0; i < 5; i++)
  {
    std::tuple<int, ACE_TCHAR **, int> pack (argc, argv, s_id[i]);
    if (pthread_create(&thread[i], NULL, work, (void *) &pack) != 0)
    {
      perror("pthread_create");
    }
    usleep(300000);
  } 
    
  // set one timer per periodic thread
  for (int i = 0; i < 5; i++)
  {
    set_timer(SIGRTMIN+i, s_period[i]);
  }

//printf("Consumer's output will be written to file hw7log.out\n");
//printf("Use the `top' utility to view CPU utilization.\n");
//printf("Hit Ctrl+C to finish the experiment..\n");

  for (int i = 0; i < 5; i++)
  {
    if (pthread_join(thread[i], NULL) != 0)
    {
      perror("pthread_join"),exit(1);
    }
  }

  return 0;
}

// ****************************************************************

Supplier::Supplier (int id)
  : id_ (id)
{
}

pthread_mutex_t mutex_orb = PTHREAD_MUTEX_INITIALIZER;
int
Supplier::run (int argc, ACE_TCHAR* argv[])
{
  try
    {
// If we use multi-processing, we do not need to
// implement the following synchronization.
// If we use multi-threading, then we need to
// serialize the following setup before the calling
// of the push function.
// One way to do so is to make all Supplier
// objects global variables, and then serialize
// this setup and the connections to ec, and after
// that it is safe to use multiple threads to
// call push().
// Instead, what I did is another (sloppy) approach:
// I simply insert some waiting time between the
// invocation of pthread_create()...
// this kind of sloppy approach is the
// source of heisenbugs...
//
// I think the ORB object is a singleton,
// according to the documentation in https://www.dre.vanderbilt.edu/Doxygen/7.0.0/html/libtao-doc/a06619.html#details
// in particular, the description for CORBA::ORB::destory()
      // ORB initialization boiler plate...
      CORBA::ORB_var orb =
        CORBA::ORB_init (argc, argv);

      CORBA::Object_var object =
        orb->resolve_initial_references ("RootPOA");
      PortableServer::POA_var poa =
        PortableServer::POA::_narrow (object.in ());
      PortableServer::POAManager_var poa_manager =
        poa->the_POAManager ();
      poa_manager->activate ();

      // Obtain the event channel from the naming service
      CORBA::Object_var naming_obj =
        orb->resolve_initial_references ("NameService");

      if (CORBA::is_nil (naming_obj.in ()))
        ACE_ERROR_RETURN ((LM_ERROR,
                           " (%P|%t) Unable to get the Naming Service.\n"),
                          1);

      CosNaming::NamingContext_var naming_context =
        CosNaming::NamingContext::_narrow (naming_obj.in ());

      CosNaming::Name name (1);
      name.length (1);
      name[0].id = CORBA::string_dup ("EventService");

      CORBA::Object_var ec_obj =
        naming_context->resolve (name);

      RtecEventChannelAdmin::EventChannel_var event_channel =
        RtecEventChannelAdmin::EventChannel::_narrow (ec_obj.in ());

      // The canonical protocol to connect to the EC
      RtecEventChannelAdmin::SupplierAdmin_var supplier_admin =
        event_channel->for_suppliers ();

      RtecEventChannelAdmin::ProxyPushConsumer_var consumer =
        supplier_admin->obtain_push_consumer ();

      RtecEventComm::PushSupplier_var supplier =
        this->_this ();

      // Simple publication, but usually the helper classes in
      // $TAO_ROOT/orbsvcs/Event_Utils.h are a better way to do this.
      RtecEventChannelAdmin::SupplierQOS qos;
      qos.publications.length (1);
      RtecEventComm::EventHeader& h0 =
        qos.publications[0].event.header;
      h0.type   = ACE_ES_EVENT_UNDEFINED; // first free event type
      h0.source = 1;                      // first free event source

      consumer->connect_push_supplier (supplier.in (), qos);

      RtecEventComm::EventSet event (1);
      event.length (1);
      event[0].header.type   = ACE_ES_EVENT_UNDEFINED;
      event[0].header.source = this->id_;
      event[0].header.ttl    = 1;
      event[0].data.payload.length (sizeof(std::chrono::system_clock::time_point));

      std::chrono::system_clock::time_point startTime;

      while (1)
      {
        pause(); // waiting for the timeout
        startTime = std::chrono::system_clock::now();
        memcpy (&event[0].data.payload[0], &startTime, sizeof(std::chrono::system_clock::time_point));
        consumer->push (event);
      }

      // Disconnect from the EC
      consumer->disconnect_push_consumer ();

      // Destroy the EC....
      event_channel->destroy ();

      // Deactivate this object...
      PortableServer::ObjectId_var id =
        poa->servant_to_id (this);
      poa->deactivate_object (id.in ());

      // Destroy the POA
      poa->destroy (1, 0);
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Supplier::run");
      return 1;
    }
  return 0;
}

void
Supplier::disconnect_push_supplier (void)
{
}

