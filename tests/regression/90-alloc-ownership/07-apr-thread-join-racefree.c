// PARAM: --set lib.activated[+] apr --set ana.activated[+] threadJoins
// After apr_thread_join the main thread is again the only user.
#include "apr_min.h"

int counter;

void *worker(apr_thread_t *thd, void *data) {
  int *c = data;
  (*c)++; // NORACE
  return NULL;
}

int main(void) {
  apr_pool_t *pool;
  apr_thread_t *t;
  apr_status_t rv;
  apr_pool_create(&pool, NULL);
  apr_thread_create(&t, NULL, worker, &counter, pool);
  apr_thread_join(&rv, t);
  counter++; // NORACE
  return 0;
}
