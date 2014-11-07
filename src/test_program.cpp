
#include <cstdio>
#include <ctime>

int main(int argc, char** argv) {
  
  int nsegs = 30;
  if (argc > 1) {
    sscanf(argv[1], "%d", &nsegs);
  }
  
  int cont = 0;
  for (time_t t = time(NULL) + nsegs + 1; t > time(NULL);) {
    cont++;
  }
  
  return 0;
}
