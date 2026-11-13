/* Models of the few external functions the mod_http2 analysis depends on.
 * Everything else from httpd, apr-util and nghttp2 stays unknown. */
#include "httpd.h"
#include "http_protocol.h"
#include "http_connection.h"
#include <nghttp2/nghttp2.h>
#include <stdlib.h>

/* nghttp2 per-stream user data: how h2_session finds its h2_stream by id. */
static void *ngh2_stream_ud[16];
int nghttp2_session_set_stream_user_data(nghttp2_session *session, int32_t stream_id, void *stream_user_data)
{
    (void)session;
    ngh2_stream_ud[stream_id & 15] = stream_user_data;
    return 0;
}
void *nghttp2_session_get_stream_user_data(nghttp2_session *session, int32_t stream_id)
{
    (void)session;
    return ngh2_stream_ud[stream_id & 15];
}
int nghttp2_session_server_new2(nghttp2_session **session_ptr, const nghttp2_session_callbacks *callbacks,
                                void *user_data, const nghttp2_option *option)
{
    *session_ptr = malloc(64); /* opaque */
    return 0;
}

/* The allocation-relevant part of httpd 2.4.71 server/protocol.c ap_create_request(). */
request_rec *ap_create_request(conn_rec *conn)
{
    request_rec *r;
    apr_pool_t *p;
    apr_pool_create(&p, conn->pool);
    r = apr_pcalloc(p, sizeof(request_rec));
    r->pool            = p;
    r->connection      = conn;
    r->server          = conn->base_server;
    r->headers_in      = apr_table_make(r->pool, 25);
    r->trailers_in     = apr_table_make(r->pool, 5);
    r->subprocess_env  = apr_table_make(r->pool, 25);
    r->headers_out     = apr_table_make(r->pool, 12);
    r->err_headers_out = apr_table_make(r->pool, 5);
    r->trailers_out    = apr_table_make(r->pool, 5);
    r->notes           = apr_table_make(r->pool, 5);
    r->proto_output_filters = conn->output_filters;
    r->output_filters  = r->proto_output_filters;
    r->proto_input_filters = conn->input_filters;
    r->input_filters   = r->proto_input_filters;
    r->per_dir_config  = r->server->lookup_defaults;
    r->status          = HTTP_OK;
    return r;
}

/* httpd server/util.c ap_thread_create(): wraps func for thread-local setup, then apr_thread_create(). */
apr_status_t ap_thread_create(apr_thread_t **thread, apr_threadattr_t *attr,
                              apr_thread_start_t func, void *data, apr_pool_t *pool)
{
    return apr_thread_create(thread, attr, func, data, pool);
}

/* httpd core hook registry for process_connection (server/connection.c,
 * AP_IMPLEMENT_HOOK_RUN_FIRST): registered functions run in order until one
 * returns something other than DECLINED. */
static ap_HOOK_process_connection_t *process_connection_hooks[4];
static int n_process_connection_hooks;

void ap_hook_process_connection(ap_HOOK_process_connection_t *pf, const char * const *aszPre,
                                const char * const *aszSucc, int nOrder)
{
    if (n_process_connection_hooks < 4)
        process_connection_hooks[n_process_connection_hooks++] = pf;
}

int ap_run_process_connection(conn_rec *c)
{
    int i, rv;
    for (i = 0; i < n_process_connection_hooks; ++i) {
        rv = process_connection_hooks[i](c);
        if (rv != DECLINED)
            return rv;
    }
    return DECLINED;
}
