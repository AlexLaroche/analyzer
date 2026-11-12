// PARAM: --set lib.activated[+] apr
// Each pool has a single user at a time: the worker allocates from the pool
// it was handed (and from one it creates itself), the main thread only from
// its own pool. Separate pools must not be confused with each other.
#include "apr_min.h"

apr_pool_t *main_pool;

void *worker(apr_thread_t *thd, void *data) {
  apr_pool_t *c2_pool = data;
  apr_pool_t *scratch;
  char *s = apr_pstrdup(c2_pool, "worker"); // NORACE
  apr_pool_create(&scratch, c2_pool);
  char *u = apr_psprintf(scratch, "%s-%d", s, 1); // NORACE
  apr_pool_destroy(scratch); // NORACE
  return u;
}

int main(void) {
  apr_pool_t *c2_pool;
  apr_thread_t *t;
  apr_status_t rv;
  apr_pool_create(&main_pool, NULL);
  apr_pool_create(&c2_pool, NULL);
  apr_thread_create(&t, NULL, worker, c2_pool, main_pool);
  char *m = apr_pcalloc(main_pool, 16); // NORACE
  char *n = apr_pstrcat(main_pool, m, "x", NULL); // NORACE
  apr_thread_join(&rv, t);
  apr_pool_clear(c2_pool); // NORACE
  return 0;
}
