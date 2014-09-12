/*
 * MessageBox.cpp
 *
 *  Created on: Sep 11, 2014
 *      Author: matheus
 */

#include "MessageBox.hpp"

#include <stddef.h>
#include <sys/msg.h>

#include "defines.hpp"

MessageInbox::MessageInbox(key_t key) {
  // create a message queue with fixed (known) key
  if (key) {
    // check if the message queue already exists
    msqid = msgget(key, 0);
    
    // if the message queue does not exist, create
    if (msqid < 0) {
      msqid = msgget(key, IPC_CREAT | 0777);
      this->key = key;
    }
    // if the message queue already exists, this object will be invalid
    else {
      this->key = 0;
    }
  }
  // create a message queue with any key
  else {
    this->key = KEY_EXECPROCD + 1;
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

bool MessageInbox::recv(Message& msg) {
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

MessageOutbox::MessageOutbox(key_t key) : msqid(msgget(key, 0)) {
  
}

void MessageOutbox::send(const Message& msg) {
  if (msqid >= 0) {
    msgsnd(msqid, &msg, sizeof(Message::Content), 0);
  }
}

bool MessageOutbox::opened() const {
  return (msqid >= 0);
}
