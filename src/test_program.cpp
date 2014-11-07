
#include <cstdio>
#include <ctime>

int main(int argc, char** argv) {
  
  if (argc < 2) {
    fprintf(stderr, "Usage mode: %s <number_of_seconds_to_last>\n", argv[0]);
    return 0;
  }
  int nsec;
  sscanf(argv[1], "%d", &nsec);
  
  int cont = 0;
  for (time_t t = time(NULL) + nsec + 1; t > time(NULL);) {
    cont++;
  }
  
  return 0;
}
