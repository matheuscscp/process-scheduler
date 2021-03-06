/*
 * execprocd.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <list>
#include <map>
#include <string>

#include "defines.hpp"
#include "MessageBox.hpp"
#include "ProcessLauncher.hpp"
#include "Time.hpp"

using namespace std;

struct Process {
  int proc_id;
  pid_t pid;
  key_t key;
  int nchange;
  Time::type initial_time, final_time;
  int current_priority;
  int schedule_count;
  Process(int proc_id = 0, pid_t pid = 0, key_t key = 0, int curr_pri = 0) :
  proc_id(proc_id),
  pid(pid), key(key), nchange(0), initial_time(Time::get()), final_time(0),
  current_priority(curr_pri), schedule_count(TIMES_SCHEDULE - 1)
  {
    
  }
  
  int priority() {
    if (schedule_count > 0) {
      schedule_count--;
    }
    else {
      schedule_count = TIMES_SCHEDULE - 1;
      current_priority = rand()%PRIORITY_MAX;
    }
    return current_priority;
  }
};

struct Schedule {
  Process process;
  Time::type quantum;
  Schedule(const Process& process, Time::type quantum) :
  process(process), quantum(quantum)
  {
    
  }
};

static bool quit = false;
static MessageInbox* inbox = NULL;
static ReportMessage rep;
static list<Process> queues[PRIORITY_MAX];
static map<int, Process> dead_processes;
static int next_proc_id = 1;
static bool is_running_proc = false;
static Process running_proc;
static bool running_proc_error = false;
static map<int, int> changes;

static void notify_launcher(const Process& process) {
  MessageOutbox outbox(process.key);
  Message execinfomsg(Message::EXECINFO);
  execinfomsg.content.info.wclock = process.final_time-process.initial_time;
  execinfomsg.content.info.nchange = process.nchange;
  outbox.send(execinfomsg);
}

static Schedule choose_process() {
  struct Quanta {
    Time::type value[PRIORITY_MAX];
    Quanta() {
      value[PRIORITY_HIGH]  = QUANTUM_HIGH;
      value[PRIORITY_MED]   = QUANTUM_MED;
      value[PRIORITY_LOW]   = QUANTUM_LOW;
    }
  };
  static Quanta quanta;
  
  for (int priority = PRIORITY_HIGH; priority <= PRIORITY_LOW; priority++) {
    if (queues[priority].size()) {
      Process p = queues[priority].front();
      queues[priority].pop_front();
      return Schedule(p, quanta.value[priority]);
    }
  }
  return Schedule(Process(0, 0), 0);
}

static void execute_process(const ExecMessage& msg) {
  int proc_id = next_proc_id++;
  ProcessLauncher launcher(msg.bufsiz, msg.shmkey);
  pid_t pid;
  
  // the actual process
  if ((pid = fork()) == 0) {
    kill(getpid(), SIGSTOP);
    chdir(launcher.dir.c_str());
    execv(launcher.argv[0], launcher.argv);
    
    // will run if execv returned (obviously an error)
    launcher.freeargv();
    
    // notify execution error to execprocd and execproc
    Message execerrormsg(Message::EXECERROR);
    execerrormsg.content.error.proc_id = proc_id;
    MessageOutbox outbox(IPCKEY);
    outbox.send(execerrormsg);
    outbox = MessageOutbox(msg.msgkey);
    outbox.send(execerrormsg);
    
    exit(0);
  }
  
  launcher.freeargv();
  
  // incrementing number of executed processes
  rep.nexec++;
  
  // sending acknowledgement message
  Message ackmsg(Message::EXECACK);
  ackmsg.content.ack.proc_id = proc_id;
  MessageOutbox outbox(msg.msgkey);
  outbox.send(ackmsg);
  
  // enqueuing process
  queues[msg.priority].push_back(
    Process(ackmsg.content.ack.proc_id, pid, msg.msgkey, msg.priority)
  );
}

static void handle_execerror(const ExecErrorMessage& msg) {
  // decrementing number of executed processes
  rep.nexec--;
  rep.nchange -= changes[msg.proc_id];
  
  // if the process is running
  if (is_running_proc && running_proc.proc_id == msg.proc_id) {
    running_proc_error = true;
  }
  // if the process is already dead
  else if (dead_processes.find(msg.proc_id) != dead_processes.end()) {
    dead_processes.erase(msg.proc_id);
  }
  // if the process is enqueued
  else {
    // search process in all queues
    for (int priority = PRIORITY_HIGH; priority <= PRIORITY_LOW; priority++) {
      bool found = false;
      
      list<Process>& pqueue = queues[priority];
      
      // search process in the current queue
      for (list<Process>::iterator it = pqueue.begin(); it != pqueue.end();) {
        if (it->proc_id == msg.proc_id) {
          found = true;
          pqueue.erase(it);
          break;
        }
        else {
          it++;
        }
      }
      
      // stop searching if the process was found
      if (found) {
        break;
      }
    }
  }
}

static void stop_process(const StopMessage& msg) {
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
      if (p.proc_id) {
        break;
      }
    }
  }
  
  // check if the process to stop is the process running
  if (!p.proc_id && is_running_proc && running_proc.proc_id == msg.proc_id) {
    p = running_proc;
  }
  
  // if the process exists, it is not in the dead pool
  if (p.proc_id) {
    kill(p.pid, SIGKILL);
    waitpid(p.pid, NULL, 0);
    p.final_time = Time::get();
    rep.nstop++;
    dead_processes[p.proc_id] = p;
    notify_launcher(p);
  }
  // if the process is in the dead pool
  else if (it != dead_processes.end()) {
    p = it->second;
  }
  
  // answer exec info
  MessageOutbox outbox(msg.key);
  Message execinfomsg(p.proc_id ? Message::EXECINFO : Message::NOTFOUND);
  execinfomsg.content.info.wclock = p.final_time - p.initial_time;
  execinfomsg.content.info.nchange = p.nchange;
  outbox.send(execinfomsg);
}

static void terminate(const TermMessage& msg) {
  quit = true;
  
  // send report
  Message repmsg(Message::REPORT);
  repmsg.content.report = rep;
  MessageOutbox outbox(msg.key);
  outbox.send(repmsg);
  
  // kill all processes in the queues
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
  
  // kill running process
  if (is_running_proc) {
    running_proc.final_time = Time::get();
    kill(running_proc.pid, SIGKILL);
    notify_launcher(running_proc);
  }
}

static void process_messages() {
  Message msg;
  while (inbox->recv(msg)) {
    switch (msg.type) {
      case Message::EXEC:       execute_process(msg.content.exec);    break;
      case Message::EXECERROR:  handle_execerror(msg.content.error);  break;
      case Message::STOP:       stop_process(msg.content.stop);       break;
      case Message::TERM:       terminate(msg.content.term);          return;
      default:                                                        break;
    }
  }
}

void execprocd(int argc, char** argv) {
  // check if procexecd is already running
  inbox = new MessageInbox(IPCKEY);
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
      
      // calculate quantum final time
      Time::type t = Time::get() + schedule.quantum;
      
      kill(schedule.process.pid, SIGCONT);
      
      // wait until quantum is over or until process is killed
      while (
        !quit &&
        Time::get() < t &&
        waitpid(schedule.process.pid, NULL, WNOHANG) != schedule.process.pid &&
        !running_proc_error
      ) {
        Time::sleep(SLEEP_WAITFAST);
        process_messages();
      }
      
      // execprocd was terminated
      if (quit) {
        break;
      }
      // if executable file was not found
      else if (running_proc_error) {
        running_proc_error = false;
      }
      // if the process is alive, recalculate priority and push to queue
      else if (kill(schedule.process.pid, 0) >= 0) {
        kill(schedule.process.pid, SIGSTOP);
        schedule.process.nchange++;
        rep.nchange++;
        changes[schedule.process.proc_id]++;
        queues[schedule.process.priority()].push_back(schedule.process);
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
  
  // destroy inbox
  delete inbox;
}
