#include "Consumer.h"
#include "orbsvcs/RtecEventChannelAdminC.h"
#include "orbsvcs/Event_Service_Constants.h"
#include "orbsvcs/CosNamingC.h"

#include <chrono>
#include <error.h>
#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

std::ofstream out_file;

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


int
ACE_TMAIN(int argc, ACE_TCHAR *argv[])
{
  pinCPU(2);

  Consumer consumer;

  return consumer.run (argc, argv);
}

// ****************************************************************

Consumer::Consumer (void)
  : event_count_ (0)
{
}

void
handle_sigint(int sig, siginfo_t *si, void *unused)
{
  //::out_file.close();
  exit(0);
}

int
Consumer::run (int argc, ACE_TCHAR* argv[])
{
  try
    {
      // ORB initialization boiler plate...
      CORBA::ORB_var orb =
        CORBA::ORB_init (argc, argv);

      // Do *NOT* make a copy because we don't want the ORB to outlive
      // the run() method.
      this->orb_ = orb.in ();

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
      RtecEventChannelAdmin::ConsumerAdmin_var consumer_admin =
        event_channel->for_consumers ();

      RtecEventChannelAdmin::ProxyPushSupplier_var supplier =
        consumer_admin->obtain_push_supplier ();

      RtecEventComm::PushConsumer_var consumer =
        this->_this ();

      // Simple subscription, but usually the helper classes in
      // $TAO_ROOT/orbsvcs/Event_Utils.h are a better way to do this.
      RtecEventChannelAdmin::ConsumerQOS qos;
      qos.dependencies.length (2);
      RtecEventComm::EventHeader& h0 =
        qos.dependencies[0].event.header;
      h0.type   = ACE_ES_DISJUNCTION_DESIGNATOR;
      h0.source = ACE_ES_EVENT_SOURCE_ANY;

      RtecEventComm::EventHeader& h1 =
        qos.dependencies[1].event.header;
      h1.type   = ACE_ES_EVENT_UNDEFINED; // first free event type
      h1.source = ACE_ES_EVENT_SOURCE_ANY;

      supplier->connect_push_consumer (consumer.in (), qos);

      // Open a file for logging;
      // catch the Ctrl+C event (i.e., SIGINT) and
      // close the file descriptor
      struct sigaction sa;
      sa.sa_flags = SA_SIGINFO;
      sigemptyset(&sa.sa_mask);
      sa.sa_sigaction = handle_sigint;
      //::out_file.open("hw7log.out", std::ios::out|std::ios::trunc);
      if (sigaction(SIGINT, &sa, NULL) == -1)
      {
          handle_error("sigaction");
      }

      // Wait for events, using work_pending()/perform_work() may help
      // or using another thread, this example is too simple for that.
      orb->run ();

      // We don't do any cleanup, it is hard to do it after shutdown,
      // and would complicate the example; plus it is almost
      // impossible to do cleanup after ORB->run() because the POA is
      // in the holding state.  Applications should use
      // work_pending()/perform_work() to do more interesting stuff.
      // Check the supplier for the proper way to do cleanup.
    }
  catch (const CORBA::Exception& ex)
    {
      ex._tao_print_exception ("Consumer::run");
      return 1;
    }
  return 0;
}

void
Consumer::push (const RtecEventComm::EventSet& events)
{
  if (events.length () == 0)
    {
      ACE_DEBUG ((LM_DEBUG,
                  "Consumer (%P|%t) no events\n"));
      return;
    }
//  ACE_DEBUG ((LM_DEBUG,
//              "Consumer (%P|%t) received an event\n"));

  std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
  std::chrono::system_clock::time_point startTime;
  memcpy(&startTime, &events[0].data.payload[0], events[0].data.payload.length ());
  int source_id = events[0].header.source;
  //::out_file << source_id << " " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime).count() << std::endl;
  ::std::cout << source_id << " " << std::chrono::duration_cast<std::chrono::milliseconds>(endTime-startTime).count() << std::endl;

}

void
Consumer::disconnect_push_consumer (void)
{
  // In this example we shutdown the ORB when we disconnect from the
  // EC (or rather the EC disconnects from us), but this doesn't have
  // to be the case....
  this->orb_->shutdown (0);
}

