// Minimal Apache Portable Runtime (APR 1.7) declarations, with the real
// signatures from apr_pools.h, apr_strings.h, apr_tables.h,
// apr_thread_proc.h, apr_thread_mutex.h and apr_thread_cond.h.
#include <stddef.h>
#include <string.h>

typedef int apr_status_t;
typedef size_t apr_size_t;
typedef struct apr_pool_t apr_pool_t;
typedef struct apr_allocator_t apr_allocator_t;
typedef int (*apr_abortfunc_t)(int retcode);

apr_status_t apr_pool_create_ex(apr_pool_t **newpool, apr_pool_t *parent,
                                apr_abortfunc_t abort_fn, apr_allocator_t *allocator);
#define apr_pool_create(newpool, parent) apr_pool_create_ex(newpool, parent, NULL, NULL)
void apr_pool_clear(apr_pool_t *p);
void apr_pool_destroy(apr_pool_t *p);
void *apr_palloc(apr_pool_t *p, apr_size_t size);
void *apr_pcalloc(apr_pool_t *p, apr_size_t size);
// As in apr_pools.h without APR_POOL_DEBUG.
#define apr_pcalloc(p, size) memset(apr_palloc(p, size), 0, size)

char *apr_pstrdup(apr_pool_t *p, const char *s);
char *apr_pstrndup(apr_pool_t *p, const char *s, apr_size_t n);
void *apr_pmemdup(apr_pool_t *p, const void *m, apr_size_t n);
char *apr_psprintf(apr_pool_t *p, const char *fmt, ...);
char *apr_pstrcat(apr_pool_t *p, ...);

typedef struct apr_table_t apr_table_t;
apr_table_t *apr_table_make(apr_pool_t *p, int nelts);
apr_table_t *apr_table_clone(apr_pool_t *p, const apr_table_t *t);
const char *apr_table_get(const apr_table_t *t, const char *key);
void apr_table_set(apr_table_t *t, const char *key, const char *val);
void apr_table_setn(apr_table_t *t, const char *key, const char *val);
void apr_table_add(apr_table_t *t, const char *key, const char *val);
void apr_table_addn(apr_table_t *t, const char *key, const char *val);
void apr_table_merge(apr_table_t *t, const char *key, const char *val);
void apr_table_mergen(apr_table_t *t, const char *key, const char *val);
void apr_table_unset(apr_table_t *t, const char *key);
void apr_table_clear(apr_table_t *t);

typedef struct apr_thread_t apr_thread_t;
typedef struct apr_threadattr_t apr_threadattr_t;
typedef void *(*apr_thread_start_t)(apr_thread_t *thd, void *data);
apr_status_t apr_thread_create(apr_thread_t **new_thread, apr_threadattr_t *attr,
                               apr_thread_start_t func, void *data, apr_pool_t *cont);
apr_status_t apr_thread_join(apr_status_t *retval, apr_thread_t *thd);
apr_status_t apr_thread_exit(apr_thread_t *thd, apr_status_t retval);

typedef struct apr_thread_mutex_t apr_thread_mutex_t;
#define APR_THREAD_MUTEX_DEFAULT 0x0
apr_status_t apr_thread_mutex_create(apr_thread_mutex_t **mutex, unsigned int flags,
                                     apr_pool_t *pool);
apr_status_t apr_thread_mutex_lock(apr_thread_mutex_t *mutex);
apr_status_t apr_thread_mutex_trylock(apr_thread_mutex_t *mutex);
apr_status_t apr_thread_mutex_unlock(apr_thread_mutex_t *mutex);
apr_status_t apr_thread_mutex_destroy(apr_thread_mutex_t *mutex);

typedef struct apr_thread_cond_t apr_thread_cond_t;
apr_status_t apr_thread_cond_create(apr_thread_cond_t **cond, apr_pool_t *pool);
apr_status_t apr_thread_cond_wait(apr_thread_cond_t *cond, apr_thread_mutex_t *mutex);
apr_status_t apr_thread_cond_signal(apr_thread_cond_t *cond);
apr_status_t apr_thread_cond_broadcast(apr_thread_cond_t *cond);
apr_status_t apr_thread_cond_destroy(apr_thread_cond_t *cond);
