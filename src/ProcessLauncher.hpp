/*
 * ProcessLauncher.hpp
 *
 *  Created on: Sep 14, 2014
 *      Author: matheus
 */

#ifndef PROCESSLAUNCHER_HPP_
#define PROCESSLAUNCHER_HPP_

#include <sys/ipc.h>
#include <string>

struct ProcessLauncher {
  size_t bufsiz;
  key_t key;
  int shmid;
  char* buf;
  std::string dir;
  char** argv;
  ProcessLauncher(int argc, char** argv);
  ProcessLauncher(size_t bufsiz, key_t key);
  ~ProcessLauncher();
  void closeshm();
  void freeargv();
};

#endif /* PROCESSLAUNCHER_HPP_ */
