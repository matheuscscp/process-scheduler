/*
 * termina_execprocd.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#include <cstdio>

#include "defines.hpp"
#include "MessageBox.hpp"

using namespace std;

void termina_execprocd(int argc, char** argv) {
  // check if execprocd is not running
  MessageOutbox outbox(KEY_EXECPROCD);
  if (!outbox.opened()) {
    fprintf(stderr, "termina_execprocd: execprocd is not running\n");
    return;
  }
  
  // send termination message
  MessageInbox inbox;
  Message termmsg(Message::TERM);
  termmsg.content.term.key = inbox.getKey();
  outbox.send(termmsg);
  
  // receive report message
  Message repmsg;
  while (!inbox.recv(repmsg)) {
    usleep(100000);
  }
  
  printf("processes executed: %d\n", repmsg.content.report.nexec);
  printf("processes stopped: %d\n", repmsg.content.report.nstop);
  printf("context changes: %d\n", repmsg.content.report.nchange);
}
