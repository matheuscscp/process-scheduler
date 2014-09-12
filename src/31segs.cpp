
#include <ctime>

int main() {
  
  int cont = 0;
  time_t t = time(NULL) + 31;
  while (t > time(NULL)) {
    cont++;
  }
  
  return 0;
}
