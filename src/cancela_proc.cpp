/*
 * cancela_proc.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <signal.h>

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

void cancela_proc(int argc, char** argv) {
  // check usage mode
  if (argc < 3) {
    fprintf(
      stderr, "cancela_proc: Usage mode: trabalho-so cancela_proc <pid>\n"
    );
    return;
  }
  
  // check if execprocd is running
  MessageOutbox outbox(KEY_EXECPROCD);
  if (!outbox.is_open()) {
    fprintf(stderr, "cancela_proc: execprocd is not running\n");
    return;
  }
  
  Message stopmsg(Message::STOP);
  
  // parse pid
  stringstream ss;
  ss << argv[2];
  ss >> stopmsg.content.stop.pid;
  
  // get inbox key
  MessageInbox inbox;
  stopmsg.content.stop.key = inbox.getKey();
  
  kill(stopmsg.content.stop.pid, SIGKILL);
  
  outbox.send(stopmsg);
  
  // set interruption signal to close the message inbox
  message_inbox = &inbox;
  signal(SIGINT, close_inbox);
  
  // receive exec info message
  Message execinfomsg;
  while (!inbox.recv(execinfomsg)) {
    Time::sleep(SLEEP_WAIT);
  }
  
  // process id not found
  if (execinfomsg.type != Message::EXECINFO) {
    fprintf(stderr, "cancela_proc: process id not found\n");
    return;
  }
  
  printf("process: %d\n", stopmsg.content.stop.pid);
  printf(
    "wallclock time: %.3f s\n", execinfomsg.content.execinfo.wclock/1000.0f
  );
  printf("context changes: %d\n", execinfomsg.content.execinfo.nchange);
}
