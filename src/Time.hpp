/*
 * Time.hpp
 *
 *  Created on: Sep 12, 2014
 *      Author: matheus
 */

#ifndef TIME_HPP_
#define TIME_HPP_

class Time {
  public:
    typedef unsigned int type;
    static type get();
    static void sleep(type ms);
};

#endif /* TIME_HPP_ */
