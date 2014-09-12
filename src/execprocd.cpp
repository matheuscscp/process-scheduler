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
#include <list>
#include <map>

#include "defines.hpp"
#include "MessageBox.hpp"

using namespace std;

struct Process {
  pid_t pid;
  int nchange;
  clock_t initial_time, final_time;
  Process(pid_t pid = 0) :
  pid(pid), nchange(0), initial_time(clock()), final_time(0)
  {
    
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
list<Process> queues[3];
map<pid_t, Process> dead_processes;

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
      queues[priority].pop_front();
      return Schedule(p, get_quantum(priority));
    }
  }
  return Schedule(Process(0), 0);
}

void execute_process(const ExecMessage& msg) {
  rep.nexec++;
  queues[msg.priority].push_back(Process(msg.pid));
}

void stop_process(const StopMessage& msg) {
  rep.nstop++;
  
  Process p;
  
  // check if process is already in the dead pool
  map<pid_t, Process>::iterator it = dead_processes.find(msg.pid);
  if (it == dead_processes.end()) {
    clock_t final_time = clock();
    
    // search process in all queues
    for (int priority = PRIORITY_HIGH; priority <= PRIORITY_LOW; priority++) {
      list<Process>& pqueue = queues[priority];
      
      // search process in the current queue
      for (list<Process>::iterator it = pqueue.begin(); it != pqueue.end();) {
        if (it->pid == msg.pid) {
          p = *it;
          p.final_time = final_time;
          pqueue.erase(it);
          break;
        }
        else {
          it++;
        }
      }
      
      // stop searching the process was found
      if (p.pid) {
        break;
      }
    }
    
    // if the process exists
    if (p.pid) {
      dead_processes[p.pid] = p;
    }
  }
  
  // check if killer outbox is open
  MessageOutbox outbox(msg.key);
  if (!outbox.is_open()) {
    return;
  }
  
  // answer exec info
  Message execinfomsg(Message::EXECINFO);
  execinfomsg.content.execinfo.wclock = p.final_time - p.initial_time;
  execinfomsg.content.execinfo.nchange = p.nchange;
  outbox.send(execinfomsg);
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
      rep.nchange++;
      
      // calculate quantum final time
      clock_t t = clock();
      t += clock_t((schedule.quantum/1000000.0f)*CLOCKS_PER_SEC);
      
      kill(schedule.process.pid, SIGCONT);
      
      // wait until quantum is over or until process is killed
      while (clock() < t && kill(schedule.process.pid, 0) >= 0) {
        usleep(1000);
      }
      
      // if the process is alive, recalculate priority and push to queue
      if (kill(schedule.process.pid, 0) >= 0) {
        kill(schedule.process.pid, SIGSTOP);
        schedule.process.nchange++;
        queues[rand()%3].push_back(schedule.process);
      }
      // if the process is dead, mark final time and put in dead pool
      else {
        schedule.process.final_time = clock();
        dead_processes[schedule.process.pid] = schedule.process;
      }
    }
    else {
      usleep(SLEEP_WAIT);
    }
    process_messages();
  }
  
  // destroy socket
  delete inbox;
}
