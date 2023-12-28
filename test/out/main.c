#include <assert.h>
#include <stdio.h>
extern int square(int);
extern int test();

int main() {
  printf("square(4): %d\n", square(4));
  assert(square(4) == 16);
  printf("test(): %d\n", test());
  assert(test() == 20);
  return 0;
}
