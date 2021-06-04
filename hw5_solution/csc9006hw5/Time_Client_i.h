// -*- C++ -*-

//=============================================================================
/**
 *  @file    Time_Client_i.h
 *
 *  This class implements the client calls to the Time example.
 *
 *  @author Balachandran Natarajan <bala@cs.wustl.edu>
 */
//=============================================================================


#ifndef TIME_CLIENT_I_H
#define TIME_CLIENT_I_H

#include "../Simple_util.h"
#include "TimeC.h"

#include <error.h>
#define handle_error(msg) \
           do { perror(msg); exit(EXIT_FAILURE); } while (0)

/**
 * @class Time_Client_i
 *
 * @brief Time_Client interface subclass.
 *
 * This class implements the interface between the interface
 * objects and the client.
 */
class Time_Client_i
{
public:
  static Time_Client_i *getInstance () {
    if(uniqueInstance == NULL) {
      uniqueInstance = new Time_Client_i ();
    }
    return uniqueInstance;
  }

  /// Destructor
  ~Time_Client_i (void);

  /// Execute the methods
  int run (const char *, int, ACE_TCHAR**);

  bool initialized = false;

private:
  static Time_Client_i *uniqueInstance;
  Time_Client_i (void);
  int ref_count = 0;
  /// Instantiate the client object.
  Client<Time> client_;
};

#endif /* TIME_CLIENT_I_H */
