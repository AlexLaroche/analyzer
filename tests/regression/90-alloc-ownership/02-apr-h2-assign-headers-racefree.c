// PARAM: --set lib.activated[+] apr
// Reduced from httpd 2.4.71 mod_http2 h2_request.c assign_headers(), the fix
// for CVE-2026-119618: the worker adds "Content-Length: 0" to its own
// request_rec copy of the headers, so the stream's table stays read-only
// for the worker.
#include "apr_min.h"

typedef struct { const char *method; apr_table_t *headers; } h2_request;
typedef struct { apr_pool_t *pool; apr_table_t *headers_in; } request_rec;

h2_request *req; // owned by the stream on the main connection

void *worker(apr_thread_t *thd, void *data) {
  request_rec *r = data;
  r->headers_in = apr_table_clone(r->pool, req->headers);
  if (apr_table_get(r->headers_in, "Content-Type"))
    apr_table_setn(r->headers_in, "Content-Length", "0"); // NORACE
  return NULL;
}

int main(void) {
  apr_pool_t *stream_pool, *c2_pool;
  apr_thread_t *t;
  apr_status_t rv;
  apr_pool_create(&stream_pool, NULL);
  apr_pool_create(&c2_pool, NULL);
  req = apr_pcalloc(stream_pool, sizeof(*req));
  req->headers = apr_table_make(stream_pool, 10);
  apr_table_setn(req->headers, "Content-Type", "text/plain");
  request_rec *r = apr_pcalloc(c2_pool, sizeof(*r));
  r->pool = c2_pool;
  apr_thread_create(&t, NULL, worker, r, c2_pool);
  apr_table_get(req->headers, "Content-Type"); // NORACE
  apr_thread_join(&rv, t);
  return 0;
}
