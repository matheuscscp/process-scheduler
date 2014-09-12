/*
 * execprocd.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#include <sys/ipc.h>
#include <signal.h>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <queue>

#include "defines.hpp"
#include "MessageBox.hpp"

using namespace std;

struct Process {
  pid_t pid;
  key_t key;
  int nchange;
  Process(pid_t pid, key_t key) : pid(pid), key(key), nchange(0) {
    
  }
};

struct Schedule {
  Process process;
  useconds_t quantum;
  Schedule(const Process& process, useconds_t quantum) :
  process(process), quantum(quantum)
  {
    
  }
};

bool quit = false;
MessageInbox* inbox = NULL;
ReportMessage rep;
queue<Process> queues[3];

inline useconds_t get_quantum(int priority) {
  switch (priority) {
    case PRIORITY_HIGH: return QUANTUM_HIGH;
    case PRIORITY_MED:  return QUANTUM_MED;
    case PRIORITY_LOW:  return QUANTUM_LOW;
    default:            return QUANTUM_NA;
  }
}

Schedule choose_process() {
  for (int priority = PRIORITY_HIGH; priority <= PRIORITY_LOW; priority++) {
    if (queues[priority].size()) {
      Process p = queues[priority].front();
      queues[priority].pop();
      return Schedule(p, get_quantum(priority));
    }
  }
  return Schedule(Process(0, 0), 0);
}

void execute_process(const ExecMessage& msg) {
  rep.nexec++;
  queues[msg.priority].push(Process(msg.pid, msg.key));
}

void stop_process(const StopMessage& msg) {
  rep.nstop++;
  //TODO
}

void process_messages() {
  Message msg;
  while (inbox->recv(msg)) {
    switch (msg.type) {
      case Message::EXEC:
        execute_process(msg.content.exec);
        break;
        
      case Message::STOP:
        stop_process(msg.content.stop);
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
  srand(time(NULL));
  while (!quit) {
    Schedule schedule = choose_process();
    if (schedule.quantum) {
      kill(schedule.process.pid, SIGCONT);
      usleep(schedule.quantum);
      kill(schedule.process.pid, SIGSTOP);
      schedule.process.nchange++;
      rep.nchange++;
      queues[rand()%3].push(schedule.process);
    }
    else {
      usleep(SLEEP_WAIT);
    }
    process_messages();
  }
  
  // destroy socket
  delete inbox;
}
