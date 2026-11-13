/* Whole-module concurrency harness for mod_http2: real worker pool
 * (h2_c1_child_init -> h2_workers_create -> slot_run threads via
 * apr_thread_create) and, on the main thread, a real h2 session that
 * receives two POST requests (Content-Type, no body) through its real
 * nghttp2 callbacks and then runs the real main-connection loop. */
#include "h2_session.c"   /* for the static nghttp2 callbacks */
#include "h2_c1.h"
#include "h2_config.h"
#include "h2_conn_ctx.h"

#define NMODULES 4

static void header(h2_session *session, const nghttp2_frame *f, const char *n, const char *v)
{
    on_header_cb(NULL, f, (const uint8_t *)n, strlen(n), (const uint8_t *)v, strlen(v), 0, session);
}

static void client_post(h2_session *session, int32_t id)
{
    nghttp2_frame f;
    memset(&f, 0, sizeof(f));
    f.hd.type = NGHTTP2_HEADERS;
    f.hd.stream_id = id;
    f.hd.flags = NGHTTP2_FLAG_END_HEADERS | NGHTTP2_FLAG_END_STREAM;
    f.headers.cat = NGHTTP2_HCAT_REQUEST;
    on_begin_headers_cb(NULL, &f, session);
    header(session, &f, ":method", "POST");
    header(session, &f, ":scheme", "http");
    header(session, &f, ":authority", "localhost");
    header(session, &f, ":path", "/");
    header(session, &f, "content-type", "text/plain");
    on_frame_recv_cb(NULL, &f, session);
}

int main(void)
{
    apr_pool_t *pchild, *cpool;
    apr_pool_create(&pchild, NULL);
    http2_module.module_index = 0;

    server_rec *s = apr_pcalloc(pchild, sizeof(*s));
    s->module_config = apr_pcalloc(pchild, NMODULES * sizeof(void *));
    ((void **)s->module_config)[0] = h2_config_create_svr(pchild, s);
    s->lookup_defaults = apr_pcalloc(pchild, NMODULES * sizeof(void *));
    ((void **)s->lookup_defaults)[0] = h2_config_create_dir(pchild, NULL);
    s->limit_req_fieldsize = 8190;
    s->limit_req_fields = 100;
    s->timeout = 60000000;
    s->keep_alive_timeout = 5000000;

    http2_module.register_hooks(pchild);  /* as httpd does at module load: fills the hook registry */
    h2_c1_child_init(pchild, s);          /* starts the worker threads */

    apr_pool_create(&cpool, pchild);
    conn_rec *c1 = apr_pcalloc(cpool, sizeof(*c1));
    c1->pool = cpool;
    c1->base_server = s;
    c1->conn_config = apr_pcalloc(cpool, NMODULES * sizeof(void *));
    c1->notes = apr_table_make(cpool, 5);
    c1->id = 1;
    c1->bucket_alloc = apr_bucket_alloc_create(cpool);

    h2_conn_ctx_create_for_c1(c1, s, "h2c");
    if (h2_c1_setup(c1, NULL, s) != APR_SUCCESS)
        return 1;
    h2_session *session = h2_conn_ctx_get(c1)->session;

    client_post(session, 1);
    client_post(session, 3);
    h2_c1_run(c1);                        /* main connection loop, concurrent with the workers */
    return 0;
}
