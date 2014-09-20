/*
 * ProcessLauncher.cpp
 *
 *  Created on: Sep 14, 2014
 *      Author: matheus
 */

#include "ProcessLauncher.hpp"

#include <cstdlib>
#include <sys/shm.h>
#include <unistd.h>
#include <cstring>
#include <list>

#include "defines.hpp"

using namespace std;

ProcessLauncher::ProcessLauncher(int argc, char** argv) : argv(NULL) {
  char* tmpdir = getcwd(NULL, 0);
  
  // calculating buffer size
  bufsiz = strlen(tmpdir) + 1; // working directory
  for (int i = 3; i < argc; i++) {
    bufsiz += (strlen(argv[i]) + 1); // arguments
  }
  bufsiz++; // null termination of arguments array
  
  // allocating shared memory
  key = KEY_EXECPROCD;
  do {
    shmid = shmget(key, bufsiz, (IPC_CREAT | IPC_EXCL) | 0777);
    if (shmid < 0) {
      key++;
    }
  } while (shmid < 0);
  buf = (char*)shmat(shmid, NULL, 0);
  
  // copying launch information
  size_t ind = 0;
  for (size_t i = 0; i < strlen(tmpdir); i++) { // working directory
    buf[ind++] = tmpdir[i];
  }
  buf[ind++] = '\0';
  for (int i = 3; i < argc; i++) { // arguments
    for (size_t j = 0; j < strlen(argv[i]); j++) {
      buf[ind++] = argv[i][j];
    }
    buf[ind++] = '\0';
  }
  buf[ind++] = '\0'; // null termination of arguments array
  
  free(tmpdir);
}

ProcessLauncher::ProcessLauncher(size_t bufsiz, key_t key) : bufsiz(0), key(0) {
  // getting shared memory
  shmid = shmget(key, bufsiz, 0);
  buf = (char*)shmat(shmid, NULL, 0);
  
  // copying launch information
  size_t i = 0;
  for (; buf[i] != '\0'; i++) { // working directory
    dir += buf[i];
  }
  i++;
  list<string> args;
  while (buf[i] != '\0') { // arguments to temporary args buffer
    string tmp;
    for (; buf[i] != '\0'; i++) {
      tmp += buf[i];
    }
    i++;
    args.push_back(tmp);
  }
  
  // copying arguments from temporary buffer to execv syscall format buffers
  argv = new char*[args.size() + 1];
  i = 0;
  for (list<string>::iterator it = args.begin(); it != args.end(); it++) {
    argv[i] = new char[it->size() + 1];
    strcpy(argv[i++], it->c_str());
  }
  argv[i++] = NULL;
  
  // detach shared memory
  closeshm();
}

ProcessLauncher::~ProcessLauncher() {
  closeshm();
  freeargv();
}

void ProcessLauncher::closeshm() {
  if (buf) {
    shmdt(buf);
    buf = NULL;
    if (key) {
      shmctl(shmid, IPC_RMID, NULL);
    }
  }
}

void ProcessLauncher::freeargv() {
  if (argv) {
    for (size_t i = 0; argv[i] != NULL; i++) {
      delete[] argv[i];
    }
    delete[] argv;
    argv = NULL;
  }
}
