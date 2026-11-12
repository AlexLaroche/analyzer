// PARAM: --set lib.activated[+] talloc --set ana.activated[+] threadJoins
// Each thread allocates under its own top-level context.
#include <pthread.h>
#include "talloc_min.h"

void *main_ctx;

void *worker(void *arg) {
  void *mine = talloc_new(NULL);
  char *s = talloc_asprintf(mine, "%s-%d", "worker", 1); // NORACE
  char *u = talloc_strndup(mine, s, 3); // NORACE
  talloc_free(mine); // NORACE
  return NULL;
}

int main(void) {
  pthread_t t;
  main_ctx = talloc_new(NULL);
  pthread_create(&t, NULL, worker, NULL);
  int *x = talloc(main_ctx, int); // NORACE
  char *b = talloc_size(main_ctx, 32); // NORACE
  pthread_join(t, NULL);
  talloc_free(main_ctx); // NORACE
  return 0;
}
