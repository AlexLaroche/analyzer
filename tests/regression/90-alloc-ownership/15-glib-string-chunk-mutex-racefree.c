// PARAM: --set lib.activated[+] glib
// A shared GStringChunk is fine under a common GMutex.
#include "glib_min.h"

GStringChunk *chunk;
GMutex lock;

gpointer worker(gpointer data) {
  g_mutex_lock(&lock);
  gchar *s = g_string_chunk_insert(chunk, "worker"); // NORACE
  g_mutex_unlock(&lock);
  return s;
}

int main(void) {
  g_mutex_init(&lock);
  chunk = g_string_chunk_new(64);
  GThread *t = g_thread_new("worker", worker, NULL);
  g_mutex_lock(&lock);
  gchar *m = g_string_chunk_insert(chunk, "main"); // NORACE
  g_mutex_unlock(&lock);
  g_thread_join(t);
  return 0;
}
