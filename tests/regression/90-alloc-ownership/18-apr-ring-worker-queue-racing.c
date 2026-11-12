// PARAM: --set lib.activated[+] apr --set ana.activated[+] threadJoins --set ana.malloc.unique_address_count 1
// Reduced from httpd mod_http2 h2_workers.c: producers wait in an APR ring inside a
// pool-allocated struct; a worker takes one under the lock and calls its fn_next
// outside the lock. fn_next allocates from the producer's pool, which the main thread
// also uses: the race is only visible if the ring dispatch is followed.
#include "apr_min.h"
#include "apr_ring_min.h"

typedef struct prod prod;
struct prod {
  const char *name;
  APR_RING_ENTRY(prod) link;
  void (*fn_next)(void *baton);
  void *baton;
};

typedef struct {
  apr_thread_mutex_t *lock;
  APR_RING_HEAD(prod_active, prod) prod_active;
} workers_t;

void next_c2(void *baton) {
  apr_pool_t *p = baton;
  apr_palloc(p, 16); // RACE!
}

void *slot_run(apr_thread_t *thd, void *data) {
  workers_t *w = data;
  prod *pr = NULL;
  apr_thread_mutex_lock(w->lock);
  if (!APR_RING_EMPTY(&w->prod_active, prod, link)) {
    pr = APR_RING_FIRST(&w->prod_active);
    APR_RING_REMOVE(pr, link);
  }
  apr_thread_mutex_unlock(w->lock);
  if (pr)
    pr->fn_next(pr->baton);
  return NULL;
}

int main(void) {
  apr_pool_t *pool, *c1_pool;
  apr_thread_t *t;
  apr_status_t rv;
  apr_pool_create(&pool, NULL);
  apr_pool_create(&c1_pool, pool);

  workers_t *w = apr_pcalloc(pool, sizeof(workers_t));
  apr_thread_mutex_create(&w->lock, 0, pool);
  APR_RING_INIT(&w->prod_active, prod, link);

  prod *pr = apr_pcalloc(pool, sizeof(prod));
  pr->fn_next = next_c2;
  pr->baton = c1_pool;
  apr_thread_mutex_lock(w->lock);
  APR_RING_INSERT_TAIL(&w->prod_active, pr, prod, link);
  apr_thread_mutex_unlock(w->lock);

  apr_thread_create(&t, NULL, slot_run, w, pool);
  apr_palloc(c1_pool, 32); // RACE!
  apr_thread_join(&rv, t);
  return 0;
}
