/*
 * execprocd.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#include <unistd.h>
#include <cstdio>
#include <cstring>

#include "defines.hpp"
#include "MessageBox.hpp"

using namespace std;

bool quit = false;
MessageInbox* inbox = NULL;
ReportMessage rep;

pid_t choose_process() {
  rep.nchange++;
  return 0;
}

void execute_process(const Message& msg) {
  rep.nexec++;
}

void stop_process(const Message& msg) {
  rep.nstop++;
}

void process_messages() {
  Message msg;
  while (inbox->recv(msg)) {
    switch (msg.type) {
      case Message::EXEC:
        execute_process(msg);
        break;
        
      case Message::STOP:
        stop_process(msg);
        break;
        
      case Message::TERM:
        quit = true;
        {
          Message repmsg(Message::REPORT);
          repmsg.content.report = rep;
          MessageOutbox outbox(msg.content.term.key);
          outbox.send(repmsg);
        }
        return;
        
      default:
        break;
    }
  }
}

void execprocd(int argc, char** argv) {
  // check if procexecd is already running
  inbox = new MessageInbox(KEY_EXECPROCD);
  if (inbox->getKey() == 0) {
    fprintf(stderr, "execprocd: already running\n");
    delete inbox;
    return;
  }
  
  // run in background
  if (fork()) {
    return;
  }
  
  // initialize report information
  memset(&rep, 0, sizeof(ReportMessage));
  
  // keep scheduling until termination message is sent
  while (!quit) {
    pid_t pid = choose_process();
    //kill(pid, SIGCONT);
    usleep(QUANTUM_HIGH);
    //kill(pid, SIGSTOP);
    process_messages();
  }
  
  // destroy socket
  delete inbox;
}
