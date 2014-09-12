/*
 * Message.cpp
 *
 *  Created on: Sep 11, 2014
 *      Author: matheus
 */

#include "Message.hpp"

#include <cstring>

Message::Message(long type) : type(type) {
  memset(&(this->content), 0, sizeof(Content));
}
