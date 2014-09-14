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
#include "ProcessLauncher.hpp"

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
    fprintf(stderr, "execproc: execprocd is not running\n");
    return;
  }
  
  // run in background
  if (fork()) {
    return;
  }
  
  MessageInbox inbox;
  ProcessLauncher launcher(argc, argv);
  
  // send exec message to execprocd
  Message execmsg(Message::EXEC);
  execmsg.content.exec.priority = priority;
  execmsg.content.exec.msgkey = inbox.getKey();
  execmsg.content.exec.shmkey = launcher.key;
  execmsg.content.exec.bufsiz = launcher.bufsiz;
  outbox.send(execmsg);
  
  // receive process id
  Message ackmsg;
  while (!inbox.recv(ackmsg)) {
    Time::sleep(SLEEP_WAIT);
  }
  printf(
    "\nwaiting until process %d terminate...\n", ackmsg.content.ack.proc_id
  );
  launcher.closeshm();
  
  // wait until the process terminate
  Message execinfomsg;
  while (!inbox.recv(execinfomsg) && outbox.is_open()) {
    Time::sleep(SLEEP_WAIT);
  }
  
  // terminate if execprocd is not running
  if (!outbox.is_open()) {
    return;
  }
  
  printf("\nprocess: %d\n", ackmsg.content.ack.proc_id);
  printf(
    "wallclock time: %.3f s\n", execinfomsg.content.execinfo.wclock/1000.0f
  );
  printf("context changes: %d\n", execinfomsg.content.execinfo.nchange);
}
