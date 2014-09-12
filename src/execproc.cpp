/*
 * execproc.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include <string>

#include "defines.hpp"
#include "MessageBox.hpp"

using namespace std;

static MessageInbox* message_inbox = NULL;

static void close_inbox(int) {
  if (message_inbox) {
    message_inbox->close();
    exit(0);
  }
}

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
  
  pid_t pid = fork();
  
  // the parent process will show execution data
  if (pid) {
    // check if execprocd is running
    MessageOutbox outbox(KEY_EXECPROCD);
    if (!outbox.is_open()) {
      fprintf(stderr, "execproc: execproc is not running\n");
      kill(pid, SIGKILL);
      return;
    }
    
    MessageInbox inbox;
    
    // send exec message to execprocd
    Message execmsg(Message::EXEC);
    execmsg.content.exec.pid = pid;
    execmsg.content.exec.priority = priority;
    execmsg.content.exec.key = inbox.getKey();
    outbox.send(execmsg);
    
    // set interruption signal to close the message inbox
    message_inbox = &inbox;
    signal(SIGINT, close_inbox);
    
    // wait until the process terminate
    Message execinfomsg;
    printf("waiting until the process %d terminate...\n", pid);
    while (!inbox.recv(execinfomsg)) {
      Time::sleep(SLEEP_WAIT);
    }
    
    printf(
      "wallclock time: %.3f s\n", execinfomsg.content.execinfo.wclock/1000.0f
    );
    printf("context changes: %d\n", execinfomsg.content.execinfo.nchange);
  }
  // the child process is the actual process
  else {
    // wait until the scheduler starts the process
    raise(SIGSTOP);
    
    // execute
    execv(argv[3], &argv[3]);
  }
}
