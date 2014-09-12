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
  int proc_id;
  pid_t pid;
  key_t key;
  int nchange;
  Time::type initial_time, final_time;
  Process(int proc_id = 0, pid_t pid = 0, key_t key = 0) :
  proc_id(proc_id),
  pid(pid), key(key), nchange(0), initial_time(Time::get()), final_time(0)
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
map<int, Process> dead_processes;
int next_proc_id = 1;
bool is_running_proc = false;
Process running_proc;

inline useconds_t get_quantum(int priority) {
  switch (priority) {
    case PRIORITY_HIGH: return QUANTUM_HIGH;
    case PRIORITY_MED:  return QUANTUM_MED;
    case PRIORITY_LOW:  return QUANTUM_LOW;
    default:            return QUANTUM_NA;
  }
}

void notify_launcher(const Process& process) {
  // check if launcher is still alive
  MessageOutbox outbox(process.key);
  if (!outbox.is_open()) {
    return;
  }
  
  // notify
  Message execinfomsg(Message::EXECINFO);
  execinfomsg.content.execinfo.wclock = process.final_time-process.initial_time;
  execinfomsg.content.execinfo.nchange = process.nchange;
  outbox.send(execinfomsg);
}

Schedule choose_process() {
  for (int priority = PRIORITY_HIGH; priority <= PRIORITY_LOW; priority++) {
    if (queues[priority].size()) {
      Process p = queues[priority].front();
      queues[priority].pop_front();
      return Schedule(p, get_quantum(priority));
    }
  }
  return Schedule(Process(0, 0), 0);
}

void execute_process(const ExecMessage& msg) {
  rep.nexec++;
  Message ackmsg(Message::EXECACK);
  ackmsg.content.ack.proc_id = next_proc_id++;
  queues[msg.priority].push_back(
    Process(ackmsg.content.ack.proc_id, msg.pid, msg.key)
  );
  MessageOutbox outbox(msg.key);
  outbox.send(ackmsg);
}

void stop_process(const StopMessage& msg) {
  Process p;
  
  // check if process is not in the dead pool
  map<int, Process>::iterator it = dead_processes.find(msg.proc_id);
  if (it == dead_processes.end()) {
    Time::type final_time = Time::get();
    
    // search process in all queues
    for (int priority = PRIORITY_HIGH; priority <= PRIORITY_LOW; priority++) {
      list<Process>& pqueue = queues[priority];
      
      // search process in the current queue
      for (list<Process>::iterator it = pqueue.begin(); it != pqueue.end();) {
        if (it->proc_id == msg.proc_id) {
          p = *it;
          p.final_time = final_time;
          pqueue.erase(it);
          break;
        }
        else {
          it++;
        }
      }
      
      // stop searching if the process was found
      if (p.pid) {
        break;
      }
    }
    
  }
  
  // check if the process to stop is the process running
  if (!p.pid && is_running_proc && running_proc.proc_id == msg.proc_id) {
    p = running_proc;
  }
  
  // if the process exists, it is not in the dead pool
  if (p.pid) {
    kill(p.pid, SIGKILL);
    p.final_time = Time::get();
    rep.nstop++;
    dead_processes[p.proc_id] = p;
    notify_launcher(p);
  }
  // if the process is in the dead pool
  else if (it != dead_processes.end()) {
    p = it->second;
  }
  
  // check if killer outbox is open
  MessageOutbox outbox(msg.key);
  if (!outbox.is_open()) {
    return;
  }
  
  // answer exec info
  Message execinfomsg(p.pid ? Message::EXECINFO : Message::NOTFOUND);
  execinfomsg.content.execinfo.wclock = p.final_time - p.initial_time;
  execinfomsg.content.execinfo.nchange = p.nchange;
  outbox.send(execinfomsg);
}

void killall() {
  for (int priority = PRIORITY_HIGH; priority <= PRIORITY_LOW; priority++) {
    list<Process>& pqueue = queues[priority];
    while (pqueue.size()) {
      Process p = pqueue.front();
      p.final_time = Time::get();
      pqueue.pop_front();
      kill(p.pid, SIGKILL);
      notify_launcher(p);
    }
  }
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
          killall();
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
      running_proc = schedule.process;
      is_running_proc = true;
      
      rep.nchange++;
      
      // calculate quantum final time
      Time::type t = Time::get() + schedule.quantum;
      
      kill(schedule.process.pid, SIGCONT);
      
      // wait until quantum is over or until process is killed
      while (Time::get() < t && kill(schedule.process.pid, 0) >= 0) {
        Time::sleep(1);
        process_messages();
      }
      
      // if the process is alive, recalculate priority and push to queue
      if (kill(schedule.process.pid, 0) >= 0) {
        kill(schedule.process.pid, SIGSTOP);
        schedule.process.nchange++;
        queues[rand()%3].push_back(schedule.process);
      }
      // if the process is dead, mark final time, put in dead pool and notify
      else if (
        dead_processes.find(schedule.process.proc_id) == dead_processes.end()
      ) {
        schedule.process.final_time = Time::get();
        dead_processes[schedule.process.proc_id] = schedule.process;
        notify_launcher(schedule.process);
      }
      
      is_running_proc = false;
    }
    else {
      Time::sleep(SLEEP_WAIT);
      process_messages();
    }
  }
  
  // destroy socket
  delete inbox;
}
