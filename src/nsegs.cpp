
#include <ctime>

#ifndef NSEGS
#define NSEGS 1
#endif

int main() {
  
  int cont = 0;
  time_t t = time(NULL) + NSEGS;
  while (t > time(NULL)) {
    cont++;
  }
  
  return 0;
}