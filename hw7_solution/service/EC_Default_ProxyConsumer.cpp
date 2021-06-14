#include "orbsvcs/Event/EC_Default_ProxyConsumer.h"
#include "orbsvcs/Event/EC_Event_Channel_Base.h"
#include "orbsvcs/Event/EC_Supplier_Filter_Builder.h"
#include "orbsvcs/Event/EC_Supplier_Filter.h"
#include "ace/Reverse_Lock_T.h"

#include "orbsvcs/Event/EC_Default_ProxySupplier.h"

#include <cmath>

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

typedef ACE_Reverse_Lock<ACE_Lock> TAO_EC_Unlock;

pthread_mutex_t mtx_workerObj = PTHREAD_MUTEX_INITIALIZER;
OurSingletonWorker *OurSingletonWorker::uniqueInstance;

void* ThreadEntryFunction(void *obj_param)
{
  OurSingletonWorker *pxy =
    ((OurSingletonWorker *) obj_param);
  // The whole purpose of this worker thread is to
  // first process the events from the EDF queue, and then
  // push them to the FIFO queue.
  while (true)
  {
//    printf("worker thread: waiting on EDFQ...\n");
    pthread_mutex_lock(&pxy->mtx_EDFQ);
    while (pxy->EDFQ.empty())
    {
      pthread_cond_wait(&pxy->cond_EDFQ, &pxy->mtx_EDFQ);
    }
    msg item = pxy->EDFQ.top();
    pxy->EDFQ.pop();
    pthread_mutex_unlock(&pxy->mtx_EDFQ);
//    printf("worker thread: working on an event from EDFQ...\n");


    // workload for 30 ms
    for (int i = 1; i < 300000; i++)
    {
        for (int j = 1; j < 13; j++)
        {
            sqrt(i*j*i);
        }
    }


//    printf("worker thread: done. Waiting for FIFOQ to push the result...\n");
    pthread_mutex_lock(&pxy->mtx_FIFOQ);
    pxy->FIFOQ.push(std::get<0>(item));
    pxy->FIFOQ_nonempty = true;
    pthread_mutex_unlock(&pxy->mtx_FIFOQ);
//    printf("worker thread: done. Seeking for another work...\n");
  }
  return NULL;
}

OurSingletonWorker::OurSingletonWorker ()
{
  pthread_create(&thread_, NULL, &ThreadEntryFunction, this);
  printf("Server: one worker thread created\n");
  mtx_EDFQ = PTHREAD_MUTEX_INITIALIZER;
  cond_EDFQ = PTHREAD_COND_INITIALIZER;
  mtx_FIFOQ = PTHREAD_MUTEX_INITIALIZER;
  FIFOQ_nonempty = false;
}

OurSingletonWorker::~OurSingletonWorker ()
{
  pthread_join(this->thread_, NULL);
}

bool
OurSingletonWorker::nonEmptyFIFOQ ()
{
  bool nonEmpty;
  pthread_mutex_lock(&mtx_FIFOQ);
  nonEmpty = FIFOQ_nonempty;
  pthread_mutex_unlock(&mtx_FIFOQ);
  return nonEmpty;
}

TAO_EC_Default_ProxyPushConsumer::
    TAO_EC_Default_ProxyPushConsumer (TAO_EC_Event_Channel_Base* ec)
  : TAO_EC_ProxyPushConsumer (ec)
{
  // Note: it seems better to put this in the activation method
  pthread_mutex_lock(&mtx_workerObj);
  workerObj = OurSingletonWorker::getInstance();
  pthread_mutex_unlock(&mtx_workerObj);
}

TAO_EC_Default_ProxyPushConsumer::~TAO_EC_Default_ProxyPushConsumer ()
{
}

void
TAO_EC_Default_ProxyPushConsumer::connect_push_supplier (
      RtecEventComm::PushSupplier_ptr push_supplier,
      const RtecEventChannelAdmin::SupplierQOS& qos)
{
  {
    ACE_GUARD_THROW_EX (
        ACE_Lock, ace_mon, *this->lock_,
        CORBA::INTERNAL ());
    // @@ RtecEventChannelAdmin::EventChannel::SYNCHRONIZATION_ERROR ());

    if (this->is_connected_i ())
      {
        if (this->event_channel_->supplier_reconnect () == 0)
          throw RtecEventChannelAdmin::AlreadyConnected ();

        // Re-connections are allowed, go ahead and disconnect the
        // consumer...
        this->cleanup_i ();

        // @@ Please read the comments in EC_ProxySuppliers about
        //     possible race conditions in this area...
        TAO_EC_Unlock reverse_lock (*this->lock_);

        {
          ACE_GUARD_THROW_EX (
              TAO_EC_Unlock, ace_mon, reverse_lock,
              CORBA::INTERNAL ());
          // @@ RtecEventChannelAdmin::EventChannel::SYNCHRONIZATION_ERROR ());

          this->event_channel_->reconnected (this);
        }

        // A separate thread could have connected siomultaneously,
        // this is probably an application error, handle it as
        // gracefully as possible
        if (this->is_connected_i ())
          return; // @@ Should we throw
      }

    this->supplier_ =
      RtecEventComm::PushSupplier::_duplicate (push_supplier);
    this->connected_ = true;
    this->qos_ = qos;

#if TAO_EC_ENABLE_DEBUG_MESSAGES
    ORBSVCS_DEBUG ((LM_DEBUG,
                 "Building filter for supplier <%x>.\n",
                this));
#endif /* TAO_EC_ENABLED_DEBUG_MESSAGES */
    this->filter_ =
      this->event_channel_->supplier_filter_builder ()->create (this->qos_);
    this->filter_->bind (this);
  }

  // Notify the event channel...
  this->event_channel_->connected (this);
}

void
TAO_EC_Default_ProxyPushConsumer::push (const RtecEventComm::EventSet& event)
{
/*  TAO_EC_ProxyPushConsumer_Guard ace_mon (this->lock_,
                                          this->ec_refcount_,
                                          this->event_channel_,
                                          this);
  if (!ace_mon.locked ())
    return;

  ace_mon.filter->push (event, this);
*/
  int interval = 0; // in milliseconds, from hw7 definition
  if (event[0].header.source == 0)
  {
    interval = 100;
  }
  else if (event[0].header.source == 1)
  {
    interval = 50;
  }
  else if (event[0].header.source == 2)
  {
    interval = 200;
  }
  else if (event[0].header.source == 3)
  {
    interval = 150;
  }
  else if (event[0].header.source == 4)
  {
    interval = 70;
  }
  else
  {
    printf("error: undefined event source\n");
    exit (1);
  }
  
  // determining the absolute deadline for each arriving event
  std::chrono::system_clock::time_point startTime, currentTime, deadline;
  memcpy (&startTime, &event[0].data.payload[0], event[0].data.payload.length());
  currentTime = std::chrono::system_clock::now();
  std::chrono::microseconds dtn (interval*1000 - std::chrono::duration_cast<std::chrono::microseconds>(currentTime-startTime).count());
  deadline = currentTime + dtn;

  msg current_message (event, deadline);

//  printf("Server: waiting on EDFQ to push an event...\n");
  pthread_mutex_lock(&workerObj->mtx_EDFQ);
  workerObj->EDFQ.push(current_message);
  pthread_cond_signal(&workerObj->cond_EDFQ);
  pthread_mutex_unlock(&workerObj->mtx_EDFQ);

//  printf("Server: done. waiting on FIFOQ to see if there's event there...\n");
  RtecEventComm::EventSet ready_event;
  while (workerObj->nonEmptyFIFOQ())
  {
    pthread_mutex_lock(&workerObj->mtx_FIFOQ);
    ready_event = workerObj->FIFOQ.front();
    workerObj->FIFOQ.pop();
    if (workerObj->FIFOQ.empty())
    {
      workerObj->FIFOQ_nonempty = false;
    }
    pthread_mutex_unlock(&workerObj->mtx_FIFOQ);

    //printf("Server: pushing an event to consumers\n");
    TAO_EC_HW6_Worker hw6_worker (ready_event);
    this->event_channel_->for_each_consumer (&hw6_worker);
  }

//  printf("Server: done. returning for other incoming events\n");
}

void
TAO_EC_Default_ProxyPushConsumer::disconnect_push_consumer ()
{
  RtecEventComm::PushSupplier_var supplier;
  int connected = 0;

  {
    ACE_GUARD_THROW_EX (
        ACE_Lock, ace_mon, *this->lock_,
        CORBA::INTERNAL ());
    // @@ RtecEventChannelAdmin::EventChannel::SYNCHRONIZATION_ERROR ());

    connected = this->is_connected_i ();
    supplier = this->supplier_._retn ();
    this->connected_ = false;

    if (connected)
      this->cleanup_i ();
  }

  // Notify the event channel...
  this->event_channel_->disconnected (this);

  if (CORBA::is_nil (supplier.in ()))
    {
      return;
    }

  if (this->event_channel_->disconnect_callbacks ())
    {
      try
        {
          supplier->disconnect_push_supplier ();
        }
      catch (const CORBA::Exception&)
        {
          // Ignore exceptions, we must isolate other clients from
          // failures on this one.
        }
    }
}

PortableServer::POA_ptr
TAO_EC_Default_ProxyPushConsumer::_default_POA ()
{
  return PortableServer::POA::_duplicate (this->default_POA_.in ());
}

void
TAO_EC_Default_ProxyPushConsumer::_add_ref ()
{
  this->_incr_refcnt ();
}

void
TAO_EC_Default_ProxyPushConsumer::_remove_ref ()
{
  this->_decr_refcnt ();
}

void
TAO_EC_Default_ProxyPushConsumer::activate (
   RtecEventChannelAdmin::ProxyPushConsumer_ptr &proxy)
{
  proxy = this->_this ();
}

PortableServer::ObjectId
TAO_EC_Default_ProxyPushConsumer::object_id ()
{
  PortableServer::ObjectId_var result =
    this->default_POA_->servant_to_id (this);
  return result.in ();
}

TAO_EC_HW6_Worker::TAO_EC_HW6_Worker (const RtecEventComm::EventSet event)
  :  event_ (event)
{
}

void
TAO_EC_HW6_Worker::work (TAO_EC_ProxyPushSupplier *supplier)
{
  supplier->reactive_push_to_consumer (supplier->consumer(), this->event_);
}

TAO_END_VERSIONED_NAMESPACE_DECL
