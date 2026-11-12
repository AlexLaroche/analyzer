// PARAM: --set lib.activated[+] apr --set ana.activated[+] threadJoins
// APR pools are not thread-safe: two threads allocating from the same pool
// race on the pool's internal free-list, even though each gets new memory.
#include "apr_min.h"

apr_pool_t *pool;

void *worker(apr_thread_t *thd, void *data) {
  char *s = apr_pstrdup(pool, "worker"); // RACE!
  return s;
}

int main(void) {
  apr_thread_t *t;
  apr_status_t rv;
  apr_pool_create(&pool, NULL);
  apr_thread_create(&t, NULL, worker, NULL, pool); // NORACE
  char *m = apr_palloc(pool, 16); // RACE!
  apr_thread_join(&rv, t);
  apr_pool_destroy(pool); // NORACE
  return 0;
}
