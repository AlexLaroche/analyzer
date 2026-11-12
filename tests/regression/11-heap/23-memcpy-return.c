#include <stdlib.h>
#include <string.h>
#include <goblint.h>

struct s { int *p; int x; };

int main() {
  int g = 42;
  struct s b = { &g, 1 };
  struct s *d = malloc(sizeof(struct s));
  // memcpy returns dest
  struct s *c = memcpy(d, &b, sizeof(struct s));
  __goblint_check(c->x == 1);
  __goblint_check(*(c->p) == 42);
  return 0;
}
