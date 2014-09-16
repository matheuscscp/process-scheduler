/*
 * MessageBox.cpp
 *
 *  Created on: Sep 11, 2014
 *      Author: matheus
 */

#include "MessageBox.hpp"

#include <sys/msg.h>

#include "defines.hpp"

MessageInbox::MessageInbox(key_t key) {
  // create a message queue with fixed (known) key
  if (key) {
    msqid = msgget(key, (IPC_CREAT | IPC_EXCL) | 0777);
    
    // if the message queue already exists, this object will be invalid
    if (msqid < 0) {
      this->key = 0;
    }
    // save the key
    else {
      this->key = key;
    }
  }
  // create a message queue with any key
  else {
    this->key = 1;
    do {
      msqid = msgget(this->key, (IPC_CREAT | IPC_EXCL) | 0777);
      if (msqid < 0) {
        this->key++;
      }
    } while (msqid < 0);
  }
}

MessageInbox::~MessageInbox() {
  close();
}

bool MessageInbox::recv(Message& msg) const {
  if (key) {
    return (msgrcv(msqid, &msg, sizeof(Message::Content), 0, IPC_NOWAIT) >= 0);
  }
  return false;
}

key_t MessageInbox::getKey() const {
  return key;
}

void MessageInbox::close() {
  if (key) {
    msgctl(msqid, IPC_RMID, NULL);
    key = 0;
  }
}

MessageOutbox::MessageOutbox(key_t key) : key(key) {
  
}

void MessageOutbox::send(const Message& msg) const {
  int msqid = msgget(key, 0);
  if (msqid >= 0) {
    msgsnd(msqid, &msg, sizeof(Message::Content), 0);
  }
}

bool MessageOutbox::is_open() const {
  return (msgget(key, 0) >= 0);
}
