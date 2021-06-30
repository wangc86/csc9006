// -*- C++ -*-

/**
 *  @file   EC_Default_ProxyConsumer.h
 *
 *  @author Carlos O'Ryan (coryan@cs.wustl.edu)
 *  @author Marina Spivak (marina@atdesk.com)
 *  @author Jason Smith (jason@atdesk.com)
 */

#ifndef TAO_EC_DEFAULT_PROXYCONSUMER_H
#define TAO_EC_DEFAULT_PROXYCONSUMER_H

#include /**/ "ace/pre.h"

#include "orbsvcs/RtecEventChannelAdminS.h"

#if !defined (ACE_LACKS_PRAGMA_ONCE)
# pragma once
#endif /* ACE_LACKS_PRAGMA_ONCE */

#include "orbsvcs/Event/EC_ProxyConsumer.h"

#include "orbsvcs/RtecEventCommC.h"
#include "orbsvcs/ESF/ESF_Worker.h"

#include <queue>
#include <mutex>
#include <vector>

TAO_BEGIN_VERSIONED_NAMESPACE_DECL

class TAO_EC_Event_Channel_Base;
class TAO_EC_ProxyPushSupplier;
class TAO_EC_Supplier_Filter;

typedef std::tuple<RtecEventComm::EventSet,
                   std::chrono::system_clock::time_point // computed deadline
                   > msg;  // message to put in the EDF queue

// for our priority queue
class myComparison
{
public:
  myComparison() {};
  bool operator() (const msg& lhs, const msg& rhs) const
  {
    if (std::chrono::duration_cast<std::chrono::microseconds>(std::get<1>(lhs) - std::get<1>(rhs)).count() > 0)
      return true;
    else
      return false;
  }
};

class OurSingletonWorker
{
public:
  static OurSingletonWorker *getInstance (TAO_EC_Event_Channel_Base* event_channel) {
    if(uniqueInstance == NULL) {
      uniqueInstance = new OurSingletonWorker (event_channel);
    }
    return uniqueInstance;
  }

  ~OurSingletonWorker (void);

  // Our EDF queue
  std::priority_queue< msg, std::vector<msg>, myComparison > EDFQ;
  pthread_mutex_t mtx_EDFQ;
  // pthread condition variable
  pthread_cond_t cond_EDFQ;

private:
  OurSingletonWorker (void);
  OurSingletonWorker (TAO_EC_Event_Channel_Base* event_channel);
  // Our singleton object for this class
  static OurSingletonWorker *uniqueInstance; 

  // Our worker thread
  pthread_t thread_;

  TAO_EC_Event_Channel_Base* event_channel_;

  friend void* ThreadEntryFunction(void *);
};

/**
 * @class TAO_EC_Default_ProxyPushConsumer
 *
 * @brief implements RtecEventChannelAdmin::ProxyPushConsumer interface.
 */
class TAO_RTEvent_Serv_Export TAO_EC_Default_ProxyPushConsumer :
  public POA_RtecEventChannelAdmin::ProxyPushConsumer,
  public TAO_EC_ProxyPushConsumer
{
public:

  /// Constructor...
  TAO_EC_Default_ProxyPushConsumer (TAO_EC_Event_Channel_Base* event_channel);

  /// Destructor...
  virtual ~TAO_EC_Default_ProxyPushConsumer ();

 virtual void activate (
     RtecEventChannelAdmin::ProxyPushConsumer_ptr &proxy);

  // = The RtecEventChannelAdmin::ProxyPushConsumer methods...
  virtual void connect_push_supplier (
                RtecEventComm::PushSupplier_ptr push_supplier,
                const RtecEventChannelAdmin::SupplierQOS& qos);
  virtual void push (const RtecEventComm::EventSet& event);
  virtual void disconnect_push_consumer ();

  // = The Servant methods
  virtual PortableServer::POA_ptr _default_POA ();
  virtual void _add_ref ();
  virtual void _remove_ref ();

private:

  virtual PortableServer::ObjectId
         object_id ();

  OurSingletonWorker *workerObj;

};

class TAO_EC_HW6_Worker : public TAO_ESF_Worker<TAO_EC_ProxyPushSupplier>
{
public:
  TAO_EC_HW6_Worker (const RtecEventComm::EventSet event);
  virtual void work (TAO_EC_ProxyPushSupplier *supplier);

private:
  RtecEventComm::EventSet event_;
};

TAO_END_VERSIONED_NAMESPACE_DECL

#include /**/ "ace/post.h"

#endif /* TAO_EC_DEFAULT_PROXYCONSUMER_H */
