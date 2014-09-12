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

struct ExecMessage {
  pid_t pid;
  int priority;
  key_t key;
};

struct StopMessage {
  pid_t pid;
  key_t key;
};

struct TermMessage {
  key_t key;
};

struct ExecInfoMessage {
  int wclock, nchange;
};

struct ReportMessage {
  int nexec, nstop, nchange;
};

struct Message {
  enum {
    NA = 0, EXEC, STOP, TERM, EXECINFO, REPORT
  };
  long type;
  union Content {
    ExecMessage exec;
    StopMessage stop;
    TermMessage term;
    ExecInfoMessage execinfo;
    ReportMessage report;
  } content;
  Message(long type = NA);
};

#endif /* MESSAGE_HPP_ */
