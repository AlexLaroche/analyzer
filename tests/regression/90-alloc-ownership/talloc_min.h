// Minimal talloc declarations, with the real signatures and macros from
// Samba's lib/talloc/talloc.h. A talloc context is not thread-safe: two
// threads must not allocate from (or free under) the same context at once.
#include <stddef.h>

#define __location__ __FILE__

void *talloc_named_const(const void *context, size_t size, const char *name);
void *_talloc_zero(const void *ctx, size_t size, const char *name);
void *_talloc_memdup(const void *t, const void *p, size_t size, const char *name);
void *_talloc_array(const void *ctx, size_t el_size, unsigned count, const char *name);
char *talloc_strdup(const void *t, const char *p);
char *talloc_strndup(const void *t, const char *p, size_t n);
char *talloc_asprintf(const void *t, const char *fmt, ...);
void *_talloc_steal_loc(const void *new_ctx, const void *ptr, const char *location);
int _talloc_free(void *ptr, const char *location);

#define talloc(ctx, type) (type *)talloc_named_const(ctx, sizeof(type), #type)
#define talloc_new(ctx) talloc_named_const(ctx, 0, "talloc_new: " __location__)
#define talloc_size(ctx, size) talloc_named_const(ctx, size, __location__)
#define talloc_zero(ctx, type) (type *)_talloc_zero(ctx, sizeof(type), #type)
#define talloc_memdup(t, p, size) _talloc_memdup(t, p, size, __location__)
#define talloc_free(ctx) _talloc_free(ctx, __location__)
