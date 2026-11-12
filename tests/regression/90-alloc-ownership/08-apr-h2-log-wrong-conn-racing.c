// PARAM: --set lib.activated[+] apr --set ana.activated[+] threadJoins
// Reduced from CVE-2020-11993 (httpd 2.4.20-2.4.43 mod_http2): with trace
// logging on, a worker logged a stream event on the MAIN connection, which
// formats the message in that connection's pool, while the main connection
// thread was using the same pool ("concurrent use of memory pools").
#include "apr_min.h"

typedef struct { apr_pool_t *pool; int id; } conn_rec;

conn_rec *c1; // main connection, served by the main thread

void log_cerror(conn_rec *c, const char *what, int stream_id) {
  const char *msg = apr_psprintf(c->pool, "h2_stream(%d-%d): %s", c->id, stream_id, what); // RACE!
}

void *worker(apr_thread_t *thd, void *data) {
  conn_rec *c2 = data;
  log_cerror(c1, "processing", 1); // logs on the wrong connection
  return NULL;
}

int main(void) {
  apr_pool_t *p1, *p2;
  apr_thread_t *t;
  apr_status_t rv;
  apr_pool_create(&p1, NULL);
  apr_pool_create(&p2, NULL);
  c1 = apr_pcalloc(p1, sizeof(conn_rec));
  c1->pool = p1;
  conn_rec *c2 = apr_pcalloc(p2, sizeof(conn_rec));
  c2->pool = p2;
  c2->id = 1;
  apr_thread_create(&t, NULL, worker, c2, p2);
  char *frame = apr_pstrdup(c1->pool, "HEADERS"); // RACE!
  apr_thread_join(&rv, t);
  return 0;
}
