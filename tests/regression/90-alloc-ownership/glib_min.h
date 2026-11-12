// Minimal GLib declarations, with the real signatures from glib/gstringchunk.h
// and glib/gthread.h (GLib 2.80). GStringChunk is not thread-safe.
#include <stddef.h>

typedef char gchar;
typedef void *gpointer;
typedef size_t gsize;
typedef long gssize;
typedef int gboolean;
typedef unsigned int guint;

typedef struct _GStringChunk GStringChunk;
GStringChunk *g_string_chunk_new(gsize size);
void g_string_chunk_free(GStringChunk *chunk);
void g_string_chunk_clear(GStringChunk *chunk);
gchar *g_string_chunk_insert(GStringChunk *chunk, const gchar *string);
gchar *g_string_chunk_insert_len(GStringChunk *chunk, const gchar *string, gssize len);
gchar *g_string_chunk_insert_const(GStringChunk *chunk, const gchar *string);

typedef struct _GThread GThread;
typedef gpointer (*GThreadFunc)(gpointer data);
GThread *g_thread_new(const gchar *name, GThreadFunc func, gpointer data);
gpointer g_thread_join(GThread *thread);

typedef union _GMutex {
  gpointer p;
  guint i[2];
} GMutex;
void g_mutex_init(GMutex *mutex);
void g_mutex_clear(GMutex *mutex);
void g_mutex_lock(GMutex *mutex);
gboolean g_mutex_trylock(GMutex *mutex);
void g_mutex_unlock(GMutex *mutex);
