
#include <ctime>

int main() {
  
  time_t t = time(NULL) + 31;
  while (t > time(NULL)) {
    
  }
  
  return 0;
}
