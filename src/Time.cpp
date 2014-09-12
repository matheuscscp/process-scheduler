/*
 * Time.cpp
 *
 *  Created on: Sep 12, 2014
 *      Author: matheus
 */

#include "Time.hpp"

#include <SDL2/SDL.h>

Time::type Time::get() {
  return SDL_GetTicks();
}

void Time::sleep(type ms) {
  SDL_Delay(ms);
}
