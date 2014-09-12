/*
 * execproc.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#include <signal.h>
#include <cstdio>
#include <string>

#include "defines.hpp"
#include "MessageBox.hpp"

using namespace std;

int parse_priority(const char* priority) {
  string tmp(priority);
  if (tmp == "high") {
    return PRIORITY_HIGH;
  }
  if (tmp == "medium") {
    return PRIORITY_MED;
  }
  if (tmp == "low") {
    return PRIORITY_LOW;
  }
  return PRIORITY_NA;
}

static void usagemode() {
  fprintf(
    stderr,
    "execproc: Usage mode: trabalho-so "
    "execproc <priority> <exec_file> [<args>...]\n"
  );
  fprintf(stderr, "  Priorities:\n");
  fprintf(stderr, "    high\n");
  fprintf(stderr, "    medium\n");
  fprintf(stderr, "    low\n");
}

void execproc(int argc, char** argv) {
  // check usage mode
  if (argc < 4) {
    usagemode();
    return;
  }
  
  // check priority
  int priority = parse_priority(argv[2]);
  if (priority == PRIORITY_NA) {
    usagemode();
    return;
  }
  
  // check if execprocd is running
  MessageOutbox outbox(KEY_EXECPROCD);
  if (!outbox.is_open()) {
    fprintf(stderr, "execproc: execproc is not running\n");
    return;
  }
  
  // run in background
  pid_t pid = fork();
  if (pid) {
    printf("process id: %d\n", pid);
    return;
  }
  
  // send exec message to execprocd
  Message execmsg(Message::EXEC);
  execmsg.content.exec.pid = getpid();
  execmsg.content.exec.priority = priority;
  outbox.send(execmsg);
  
  // wait until the scheduler starts the process
  raise(SIGSTOP);
  
  // execute
  execv(argv[3], &argv[3]);
}
