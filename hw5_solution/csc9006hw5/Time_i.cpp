#include "Time_i.h"
#include "ace/OS_NS_time.h"
#include <iostream>

// Constructor
Time_i::Time_i (void)
{
  this->count = 0;
}

// Destructor
Time_i::~Time_i (void)
{
}

// Set the ORB pointer.
void
Time_i::orb (CORBA::ORB_ptr o)
{
  this->orb_ = CORBA::ORB::_duplicate (o);
}

// Return the current date/time on the server.
CORBA::Long
Time_i::current_time (void)
{
  this->count ++;
  //std::cout << this->count << std::endl;
  return CORBA::Long (ACE_OS::time (0));
}

// Return the current count for client requests.
CORBA::Long
Time_i::count_requests (void)
{
  return this->count;
}

// Shutdown.
void
Time_i::shutdown (void)
{
  ACE_DEBUG ((LM_DEBUG,
              ACE_TEXT ("Time_i is shutting down\n")));

  // Instruct the ORB to shutdown.
  this->orb_->shutdown ();
}
