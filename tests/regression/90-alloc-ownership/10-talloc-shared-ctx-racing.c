// PARAM: --set lib.activated[+] talloc
// talloc is not thread-safe: allocating children of the same parent context
// from two threads races on the parent's child list.
#include <pthread.h>
#include "talloc_min.h"

void *ctx;

void *worker(void *arg) {
  char *s = talloc_strdup(ctx, "worker"); // RACE!
  return s;
}

int main(void) {
  pthread_t t;
  ctx = talloc_new(NULL);
  pthread_create(&t, NULL, worker, NULL);
  int *x = talloc_zero(ctx, int); // RACE!
  pthread_join(t, NULL);
  talloc_free(ctx); // NORACE
  return 0;
}
