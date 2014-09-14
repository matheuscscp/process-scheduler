/*
 * Message.hpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

#include <unistd.h>
#include <sys/ipc.h>

#include "Time.hpp"

struct ExecMessage {
  int priority;
  key_t msgkey, shmkey;
  size_t bufsiz;
};

struct ExecAckMessage {
  int proc_id;
};

struct StopMessage {
  int proc_id;
  key_t key;
};

struct TermMessage {
  key_t key;
};

struct ExecInfoMessage {
  Time::type wclock;
  int nchange;
};

struct ReportMessage {
  int nexec, nstop, nchange;
};

struct Message {
  enum {
    NA = 0, EXEC, EXECACK, STOP, TERM, EXECINFO, REPORT, NOTFOUND
  };
  long type;
  union Content {
    ExecMessage exec;
    ExecAckMessage ack;
    StopMessage stop;
    TermMessage term;
    ExecInfoMessage execinfo;
    ReportMessage report;
  } content;
  Message(long type = NA);
};

#endif /* MESSAGE_HPP_ */
