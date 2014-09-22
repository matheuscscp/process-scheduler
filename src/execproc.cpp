/*
 * execproc.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#include <unistd.h>
#include <cstdio>
#include <string>

#include "defines.hpp"
#include "MessageBox.hpp"
#include "ProcessLauncher.hpp"
#include "Time.hpp"

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
  MessageOutbox outbox(IPCKEY);
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
  launcher.closeshm();
  if (ackmsg.type != Message::EXECERROR) { // ack received
    printf("\nstarting process %d...\n", ackmsg.content.ack.proc_id);
  }
  else { // execution error
    fprintf(stderr, "\nexecprocd: no such file or directory\n");
    return;
  }
  
  // wait until the process terminate
  Message execinfomsg;
  bool recvd;
  for (
    recvd = inbox.recv(execinfomsg);
    !recvd && outbox.is_open();
    recvd = inbox.recv(execinfomsg)
  ) {
    Time::sleep(SLEEP_WAIT);
  }
  
  // terminate if execprocd is not running
  if (!recvd && !outbox.is_open()) {
    // wait a few seconds until exec info message is received, then return
    Time::type t = Time::get() + TIMEOUT_EXECINFO;
    for (
      recvd = inbox.recv(execinfomsg);
      !recvd && t > Time::get();
      recvd = inbox.recv(execinfomsg)
    ) {
      Time::sleep(SLEEP_WAIT);
    }
    
    if (!recvd) {
      fprintf(stderr, "\nexecproc: execprocd has returned\n");
      return;
    }
  }
  
  // execution error
  if (execinfomsg.type == Message::EXECERROR) {
    fprintf(stderr, "\nexecprocd: no such file or directory\n");
    return;
  }
  
  printf("\nprocess: %d\n", ackmsg.content.ack.proc_id);
  printf("wallclock time: %.3f s\n", execinfomsg.content.info.wclock/1000.0f);
  printf("context changes: %d\n", execinfomsg.content.info.nchange);
}
