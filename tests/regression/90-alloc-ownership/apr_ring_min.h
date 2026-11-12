// APR ring macros as in APR 1.7 apr_ring.h (Apache License 2.0), without APR_RING_DEBUG.
// The sentinel is a fake element whose `link` field overlays the ring head:
// &head->next minus offsetof(elem, link).
#include <stddef.h>

#define APR_RING_ENTRY(elem) struct { struct elem * volatile next; struct elem * volatile prev; }
#define APR_RING_HEAD(head, elem) struct head { struct elem * volatile next; struct elem * volatile prev; }
#define APR_RING_SENTINEL(hp, elem, link) \
    (struct elem *)((char *)(&(hp)->next) - offsetof(struct elem, link))
#define APR_RING_FIRST(hp) (hp)->next
#define APR_RING_LAST(hp) (hp)->prev
#define APR_RING_NEXT(ep, link) (ep)->link.next
#define APR_RING_PREV(ep, link) (ep)->link.prev
#define APR_RING_INIT(hp, elem, link) do { \
    APR_RING_FIRST((hp)) = APR_RING_SENTINEL((hp), elem, link); \
    APR_RING_LAST((hp)) = APR_RING_SENTINEL((hp), elem, link); \
  } while (0)
#define APR_RING_EMPTY(hp, elem, link) (APR_RING_FIRST((hp)) == APR_RING_SENTINEL((hp), elem, link))
#define APR_RING_ELEM_INIT(ep, link) do { \
    APR_RING_NEXT((ep), link) = (ep); \
    APR_RING_PREV((ep), link) = (ep); \
  } while (0)
#define APR_RING_SPLICE_TAIL(hp, ep1, epN, elem, link) do { \
    APR_RING_NEXT((epN), link) = APR_RING_SENTINEL((hp), elem, link); \
    APR_RING_PREV((ep1), link) = APR_RING_LAST((hp)); \
    APR_RING_NEXT(APR_RING_LAST((hp)), link) = (ep1); \
    APR_RING_LAST((hp)) = (epN); \
  } while (0)
#define APR_RING_INSERT_TAIL(hp, nep, elem, link) APR_RING_SPLICE_TAIL((hp), (nep), (nep), elem, link)
#define APR_RING_UNSPLICE(ep1, epN, link) do { \
    APR_RING_NEXT(APR_RING_PREV((ep1), link), link) = APR_RING_NEXT((epN), link); \
    APR_RING_PREV(APR_RING_NEXT((epN), link), link) = APR_RING_PREV((ep1), link); \
  } while (0)
#define APR_RING_REMOVE(ep, link) APR_RING_UNSPLICE((ep), (ep), link)
