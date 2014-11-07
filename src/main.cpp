/*
 * main.cpp
 *
 *  Created on: Sep 10, 2014
 *      Author: matheus
 */

#include <cstdio>
#include <cstdlib>
#include <string>

using namespace std;

void execproc(int argc, char** argv);
void execprocd(int argc, char** argv);
void cancela_proc(int argc, char** argv);
void termina_execprocd(int argc, char** argv);

static void usagemode(char* exe) {
  fprintf(stderr, "Usage mode: %s <program>\n", exe);
  fprintf(stderr, "  Programs:\n");
  fprintf(stderr, "    execproc\n");
  fprintf(stderr, "    execprocd\n");
  fprintf(stderr, "    cancela_proc\n");
  fprintf(stderr, "    termina_execprocd\n");
}

int main(int argc, char** argv) {
  
  if (argc < 2) {
    usagemode(argv[0]);
    exit(0);
  }
  
  string cmd(argv[1]);
  
  if (cmd == "execproc") {
    execproc(argc, argv);
  }
  else if (cmd == "execprocd") {
    execprocd(argc, argv);
  }
  else if (cmd == "cancela_proc") {
    cancela_proc(argc, argv);
  }
  else if (cmd == "termina_execprocd") {
    termina_execprocd(argc, argv);
  }
  else {
    usagemode(argv[0]);
  }
  
  return 0;
}
