// PARAM: --set lib.activated[+] glib
// GStringChunk is not thread-safe: inserting into the same chunk from two
// threads races on its internal storage.
#include "glib_min.h"

GStringChunk *chunk;

gpointer worker(gpointer data) {
  gchar *s = g_string_chunk_insert(chunk, "worker"); // RACE!
  return s;
}

int main(void) {
  chunk = g_string_chunk_new(64);
  GThread *t = g_thread_new("worker", worker, NULL);
  gchar *m = g_string_chunk_insert_const(chunk, "main"); // RACE!
  g_thread_join(t);
  g_string_chunk_free(chunk); // NORACE
  return 0;
}
