/* Loop-free model of h2_bucket_beam.c (identical API in 2.4.70 and 2.4.71) for
 * hand-off harnesses. Kept from the real code: allocation from the beam's pool,
 * beam->lock around every operation, the event callbacks (invoked under the lock,
 * as the real code does), and the transfer of one bucket per send/receive through
 * the beam's bucket lists, so that ownership of buckets still moves between threads.
 * Dropped: the blocking wait loops, memory accounting and proxy buckets. */
#include <apr_lib.h>
#include <apr_atomic.h>
#include <apr_strings.h>
#include <apr_time.h>
#include <apr_buckets.h>
#include <apr_thread_mutex.h>
#include <apr_thread_cond.h>
#include <httpd.h>
#include <http_protocol.h>
#include <http_request.h>
#include <http_log.h>
#include "h2_private.h"
#include "h2_conn_ctx.h"
#include "h2_headers.h"
#include "h2_util.h"
#include "h2_bucket_beam.h"

/* list macros, as in h2_bucket_beam.c */
#define H2_BLIST_INIT(b)        APR_RING_INIT(&(b)->list, apr_bucket, link);
#define H2_BLIST_SENTINEL(b)    APR_RING_SENTINEL(&(b)->list, apr_bucket, link)
#define H2_BLIST_EMPTY(b)       APR_RING_EMPTY(&(b)->list, apr_bucket, link)
#define H2_BLIST_FIRST(b)       APR_RING_FIRST(&(b)->list)
#define H2_BLIST_LAST(b)	APR_RING_LAST(&(b)->list)
#define H2_BLIST_INSERT_HEAD(b, e) do {				\
	apr_bucket *ap__b = (e);                                        \
	APR_RING_INSERT_HEAD(&(b)->list, ap__b, apr_bucket, link);	\
    } while (0)
#define H2_BLIST_INSERT_TAIL(b, e) do {				\
	apr_bucket *ap__b = (e);					\
	APR_RING_INSERT_TAIL(&(b)->list, ap__b, apr_bucket, link);	\
    } while (0)
#define H2_BLIST_CONCAT(a, b) do {					\
        APR_RING_CONCAT(&(a)->list, &(b)->list, apr_bucket, link);	\
    } while (0)
#define H2_BLIST_PREPEND(a, b) do {					\
        APR_RING_PREPEND(&(a)->list, &(b)->list, apr_bucket, link);	\
    } while (0)


static apr_status_t beam_cleanup(void *data) { (void)data; return APR_SUCCESS; }

apr_status_t h2_beam_create(h2_bucket_beam **pbeam, conn_rec *from, apr_pool_t *pool,
                            int id, const char *tag, apr_size_t max_buf_size,
                            apr_interval_time_t timeout)
{
    h2_bucket_beam *beam = apr_pcalloc(pool, sizeof(*beam));
    h2_conn_ctx_t *conn_ctx = h2_conn_ctx_get(from);
    apr_status_t rv;
    beam->pool = pool;
    beam->from = from;
    beam->id = id;
    beam->name = apr_psprintf(pool, "%s-%d-%s", conn_ctx->id, id, tag);
    H2_BLIST_INIT(&beam->buckets_to_send);
    H2_BLIST_INIT(&beam->buckets_consumed);
    H2_BLIST_INIT(&beam->buckets_eor);
    beam->tx_mem_limits = 1;
    beam->max_buf_size = max_buf_size;
    beam->timeout = timeout;
    rv = apr_thread_mutex_create(&beam->lock, APR_THREAD_MUTEX_DEFAULT, pool);
    if (rv == APR_SUCCESS) rv = apr_thread_cond_create(&beam->change, pool);
    if (rv == APR_SUCCESS) apr_pool_pre_cleanup_register(pool, beam, beam_cleanup);
    *pbeam = (rv == APR_SUCCESS) ? beam : NULL;
    return rv;
}

apr_status_t h2_beam_destroy(h2_bucket_beam *beam, conn_rec *c)
{
    (void)c;
    apr_pool_cleanup_kill(beam->pool, beam, beam_cleanup);
    return APR_SUCCESS;
}

#define LOCKED(stmt) do { apr_thread_mutex_lock(beam->lock); stmt; apr_thread_mutex_unlock(beam->lock); } while (0)

void h2_beam_set_copy_files(h2_bucket_beam *beam, int enabled) { LOCKED(beam->copy_files = enabled); }
void h2_beam_buffer_size_set(h2_bucket_beam *beam, apr_size_t s) { LOCKED(beam->max_buf_size = s); }
apr_size_t h2_beam_buffer_size_get(h2_bucket_beam *beam) { apr_size_t s; LOCKED(s = beam->max_buf_size); return s; }
void h2_beam_timeout_set(h2_bucket_beam *beam, apr_interval_time_t t) { LOCKED(beam->timeout = t); }
apr_interval_time_t h2_beam_timeout_get(h2_bucket_beam *beam) { apr_interval_time_t t; LOCKED(t = beam->timeout); return t; }

static void notify_send(h2_bucket_beam *beam)
{
    if (beam->send_cb) beam->send_cb(beam->send_ctx, beam);
    if (beam->was_empty_cb) beam->was_empty_cb(beam->was_empty_ctx, beam);
    apr_thread_cond_broadcast(beam->change);
}

void h2_beam_abort(h2_bucket_beam *beam, conn_rec *c)
{
    (void)c;
    apr_thread_mutex_lock(beam->lock);
    beam->aborted = 1;
    notify_send(beam);
    apr_thread_mutex_unlock(beam->lock);
}

void h2_beam_close(h2_bucket_beam *beam, conn_rec *c)
{
    (void)c;
    apr_thread_mutex_lock(beam->lock);
    beam->closed = 1;
    notify_send(beam);
    apr_thread_mutex_unlock(beam->lock);
}

apr_status_t h2_beam_send(h2_bucket_beam *beam, conn_rec *from, apr_bucket_brigade *bb,
                          apr_read_type_e block, apr_off_t *pwritten)
{
    apr_status_t rv = APR_SUCCESS;
    (void)block;
    apr_thread_mutex_lock(beam->lock);
    beam->from = from;
    *pwritten = 0;
    if (beam->aborted) {
        rv = APR_ECONNABORTED;
    }
    else if (bb && !APR_BRIGADE_EMPTY(bb)) {
        apr_bucket *b = APR_BRIGADE_FIRST(bb);   /* ownership moves to the beam */
        APR_BUCKET_REMOVE(b);
        if (APR_BUCKET_IS_EOS(b)) beam->closed = 1;
        *pwritten = b->length;
        H2_BLIST_INSERT_TAIL(&beam->buckets_to_send, b);
        notify_send(beam);
    }
    apr_thread_mutex_unlock(beam->lock);
    return rv;
}

apr_status_t h2_beam_receive(h2_bucket_beam *beam, conn_rec *to, apr_bucket_brigade *bb,
                             apr_read_type_e block, apr_off_t readbytes)
{
    apr_status_t rv = APR_EAGAIN;
    (void)block; (void)readbytes; (void)to;
    apr_thread_mutex_lock(beam->lock);
    if (beam->aborted) {
        rv = APR_ECONNABORTED;
    }
    else if (!H2_BLIST_EMPTY(&beam->buckets_to_send)) {
        apr_bucket *b = H2_BLIST_FIRST(&beam->buckets_to_send);
        APR_BUCKET_REMOVE(b);
        H2_BLIST_INSERT_TAIL(&beam->buckets_consumed, b);
        /* the receiver gets a bucket that reads the sender's data */
        if (bb) {
            apr_bucket *p = apr_bucket_alloc(sizeof(*p), bb->bucket_alloc);
            *p = *b;
            APR_BRIGADE_INSERT_TAIL(bb, p);
        }
        beam->recv_bytes += b->length;
        rv = APR_SUCCESS;
        if (beam->recv_cb) beam->recv_cb(beam->recv_ctx, beam);
    }
    else if (beam->closed) {
        rv = APR_EOF;
    }
    if (rv == APR_EAGAIN && beam->eagain_cb) beam->eagain_cb(beam->eagain_ctx, beam);
    apr_thread_mutex_unlock(beam->lock);
    return rv;
}

void h2_beam_on_consumed(h2_bucket_beam *beam, h2_beam_io_callback *io_cb, void *ctx)
{ LOCKED((beam->cons_io_cb = io_cb, beam->cons_ctx = ctx)); }
void h2_beam_on_received(h2_bucket_beam *beam, h2_beam_ev_callback *cb, void *ctx)
{ LOCKED((beam->recv_cb = cb, beam->recv_ctx = ctx)); }
void h2_beam_on_eagain(h2_bucket_beam *beam, h2_beam_ev_callback *cb, void *ctx)
{ LOCKED((beam->eagain_cb = cb, beam->eagain_ctx = ctx)); }
void h2_beam_on_send(h2_bucket_beam *beam, h2_beam_ev_callback *cb, void *ctx)
{ LOCKED((beam->send_cb = cb, beam->send_ctx = ctx)); }
void h2_beam_on_was_empty(h2_bucket_beam *beam, h2_beam_ev_callback *cb, void *ctx)
{ LOCKED((beam->was_empty_cb = cb, beam->was_empty_ctx = ctx)); }

int h2_beam_report_consumption(h2_bucket_beam *beam)
{
    int r = 0;
    apr_thread_mutex_lock(beam->lock);
    if (beam->cons_io_cb && beam->recv_bytes > beam->recv_bytes_reported) {
        apr_off_t len = beam->recv_bytes - beam->recv_bytes_reported;
        h2_beam_io_callback *cb = beam->cons_io_cb;
        void *ctx = beam->cons_ctx;
        beam->recv_bytes_reported += len;
        apr_thread_mutex_unlock(beam->lock);   /* as report_consumption(locked) does */
        cb(ctx, beam, len);
        apr_thread_mutex_lock(beam->lock);
        r = 1;
    }
    apr_thread_mutex_unlock(beam->lock);
    return r;
}

apr_off_t h2_beam_get_buffered(h2_bucket_beam *beam)
{ apr_off_t l = 0; LOCKED(if (!H2_BLIST_EMPTY(&beam->buckets_to_send)) l = H2_BLIST_FIRST(&beam->buckets_to_send)->length); return l; }
apr_off_t h2_beam_get_mem_used(h2_bucket_beam *beam) { return h2_beam_get_buffered(beam); }
int h2_beam_empty(h2_bucket_beam *beam) { int e; LOCKED(e = H2_BLIST_EMPTY(&beam->buckets_to_send)); return e; }
int h2_beam_is_complete(h2_bucket_beam *beam)
{ int c; LOCKED(c = beam->closed && H2_BLIST_EMPTY(&beam->buckets_to_send)); return c; }
