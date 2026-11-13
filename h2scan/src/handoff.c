/* Hand-off scenario harness (H2SCEN selects the scenario). Same real setup as
 * harness.c (worker pool, h2 session, nghttp2 callbacks), but instead of the
 * endless main-connection loop the main thread performs, once and in a straight
 * line, what httpd's c1 does while a worker processes the stream. Nothing here
 * shares objects on purpose: every hand-off is done by mod_http2's own code.
 * Former description: whole-module concurrency harness for mod_http2: real worker pool
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

static void client_post(h2_session *session, int32_t id, int end_stream)
{
    nghttp2_frame f;
    memset(&f, 0, sizeof(f));
    f.hd.type = NGHTTP2_HEADERS;
    f.hd.stream_id = id;
    f.hd.flags = NGHTTP2_FLAG_END_HEADERS | (end_stream ? NGHTTP2_FLAG_END_STREAM : 0);
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
    unsigned int nstreams;
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

    http2_module.register_hooks(pchild);
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

#if H2SCEN == 3
    client_post(session, 1, 0);           /* request body follows */
#else
    client_post(session, 1, 1);
#endif
    /* what h2_session_process does next: hand ready streams to the workers */
    h2_mplx_c1_process(session->mplx, session->ready_to_process, get_stream,
                       stream_pri_cmp, session, &nstreams);
    h2_stream *stream = get_stream(session, 1);
    if (!stream)
        return 1;

    /* c1, concurrently with the worker processing stream 1: */
#if H2SCEN == 1
    /* S1 response: the worker signalled output; c1 handles it */
    on_stream_output(session, stream);
#elif H2SCEN == 2
    /* S2 client reset: client cancels the stream while it is processed */
    on_stream_close_cb(NULL, 1, NGHTTP2_CANCEL, session);
#elif H2SCEN == 3
    /* S3 request body: DATA arrives (and ends the stream) while the worker reads input */
    {
        nghttp2_frame f;
        on_data_chunk_recv_cb(NULL, 0, 1, (const uint8_t *)"abc", 3, session);
        memset(&f, 0, sizeof(f));
        f.hd.type = NGHTTP2_DATA;
        f.hd.stream_id = 1;
        f.hd.flags = NGHTTP2_FLAG_END_STREAM;
        on_frame_recv_cb(NULL, &f, session);
        on_stream_input(session, stream);
    }
#elif H2SCEN == 4
    /* S4 connection shutdown while the stream is processed */
    h2_session_shutdown(session, 0, "harness", 1);
    h2_mplx_c1_shutdown(session->mplx);
#elif H2SCEN == 5
    /* S5 stream cleanup: c1 is done with the stream (e.g. response sent) */
    h2_mplx_c1_stream_cleanup(session->mplx, stream, &session->open_streams);
#else
#error "H2SCEN must be 1..5"
#endif
    return 0;
}
