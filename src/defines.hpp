/*
 * defines.hpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#ifndef DEFINES_HPP_
#define DEFINES_HPP_

// Unix keys for interprocess communication
#define KEY_EXECPROCD 0x505C

// array indexes
#define PRIORITY_NA   -1
#define PRIORITY_HIGH 0
#define PRIORITY_MED  1
#define PRIORITY_LOW  2

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

#endif /* DEFINES_HPP_ */
