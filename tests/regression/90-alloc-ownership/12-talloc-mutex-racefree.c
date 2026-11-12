// PARAM: --set lib.activated[+] talloc --set ana.activated[+] threadJoins
// A shared talloc context is fine under a common mutex.
#include <pthread.h>
#include "talloc_min.h"

void *ctx;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

void *worker(void *arg) {
  pthread_mutex_lock(&m);
  char *s = talloc_strdup(ctx, "worker"); // NORACE
  pthread_mutex_unlock(&m);
  return s;
}

int main(void) {
  pthread_t t;
  ctx = talloc_new(NULL);
  pthread_create(&t, NULL, worker, NULL);
  pthread_mutex_lock(&m);
  int *x = talloc_zero(ctx, int); // NORACE
  pthread_mutex_unlock(&m);
  pthread_join(t, NULL);
  return 0;
}
