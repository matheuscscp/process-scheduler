/*
 * defines.hpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#ifndef DEFINES_HPP_
#define DEFINES_HPP_

// Unix key for interprocess communication
#define IPCKEY  0x90125789

// milliseconds
#define QUANTUM_NA    0
#define QUANTUM_HIGH  500
#define QUANTUM_MED   500
#define QUANTUM_LOW   500

// milliseconds
#define SLEEP_WAIT      20
#define SLEEP_WAITFAST  5

// milliseconds
#define TIMEOUT_EXECINFO  2000

// number of repetitions
#define TIMES_SCHEDULE  4

enum Priority {
  // don't change
  PRIORITY_NA = -1,
  
  // insert new priorities here
  PRIORITY_HIGH,
  PRIORITY_MED,
  PRIORITY_LOW,
  
  // don't remove
  PRIORITY_MAX
};

#endif /* DEFINES_HPP_ */
