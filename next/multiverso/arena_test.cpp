#include "arena.h"

int main()
{
  multiverso::Arena arena;
  char* p = arena.Allocate(1024);
  arena.Reset();
  return 0;
}
