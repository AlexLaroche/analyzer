// PARAM: --set lib.activated[+] apr
// A shared pool is fine when every allocation holds the same APR mutex.
#include "apr_min.h"

apr_pool_t *pool;
apr_thread_mutex_t *lock;

void *worker(apr_thread_t *thd, void *data) {
  apr_thread_mutex_lock(lock);
  char *s = apr_pstrdup(pool, "worker"); // NORACE
  apr_thread_mutex_unlock(lock);
  return s;
}

int main(void) {
  apr_thread_t *t;
  apr_status_t rv;
  apr_pool_create(&pool, NULL);
  apr_thread_mutex_create(&lock, APR_THREAD_MUTEX_DEFAULT, pool);
  apr_thread_create(&t, NULL, worker, NULL, pool);
  apr_thread_mutex_lock(lock);
  char *m = apr_palloc(pool, 16); // NORACE
  apr_thread_mutex_unlock(lock);
  apr_thread_join(&rv, t);
  return 0;
}
