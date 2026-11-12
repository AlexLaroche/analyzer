// PARAM: --set lib.activated[+] apr --set ana.activated[+] threadJoins
// apr_thread_create passes `data` as the SECOND argument of the start
// function (the first is the apr_thread_t handle).
#include "apr_min.h"

int counter;

void *worker(apr_thread_t *thd, void *data) {
  int *c = data;
  (*c)++; // RACE!
  return NULL;
}

int main(void) {
  apr_pool_t *pool;
  apr_thread_t *t;
  apr_status_t rv;
  apr_pool_create(&pool, NULL);
  apr_thread_create(&t, NULL, worker, &counter, pool);
  counter++; // RACE!
  apr_thread_join(&rv, t);
  return 0;
}
