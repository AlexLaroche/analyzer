// PARAM: --set lib.activated[+] glib
// Each thread inserts into its own GStringChunk.
#include "glib_min.h"

GStringChunk *main_chunk;

gpointer worker(gpointer data) {
  GStringChunk *mine = g_string_chunk_new(64);
  gchar *s = g_string_chunk_insert_len(mine, "worker", 6); // NORACE
  g_string_chunk_free(mine); // NORACE
  return NULL;
}

int main(void) {
  main_chunk = g_string_chunk_new(64);
  GThread *t = g_thread_new("worker", worker, NULL);
  gchar *m = g_string_chunk_insert(main_chunk, "main"); // NORACE
  g_thread_join(t);
  g_string_chunk_clear(main_chunk); // NORACE
  return 0;
}
