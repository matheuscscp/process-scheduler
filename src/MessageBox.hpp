/*
 * MessageBox.hpp
 *
 *  Created on: Sep 11, 2014
 *      Author: matheus
 */

#ifndef MESSAGEBOX_HPP_
#define MESSAGEBOX_HPP_

#include "Message.hpp"

class MessageInbox {
  private:
    int msqid;
    key_t key;
  public:
    MessageInbox(key_t key = 0);
    ~MessageInbox();
    bool recv(Message& msg);
    key_t getKey() const;
    void close();
};

class MessageOutbox {
  private:
    int msqid;
  public:
    MessageOutbox(key_t key);
    void send(const Message& msg);
    bool opened() const;
};

#endif /* MESSAGEBOX_HPP_ */
