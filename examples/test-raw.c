#include <rest/rest-proxy.h>

static void
proxy_call_raw_async_cb (RestProxy *proxy,
                         guint status_code,
                         const gchar *response_message,
                         GHashTable *headers,
                         const gchar *payload,
                         gssize len,
                         GObject *weak_object,
                         gpointer userdata)
{
  write (1, payload, len);
}

gint
main (gint argc, gchar **argv)
{
  RestProxy *proxy;
  GMainLoop *loop;

  g_type_init ();
  g_thread_init (NULL);

  loop = g_main_loop_new (NULL, FALSE);

  proxy = rest_proxy_new ("http://www.o-hand.com/", FALSE);
  rest_proxy_call_raw_async (proxy,
      NULL,
      "GET",
      proxy_call_raw_async_cb,
      NULL,
      NULL,
      NULL,
      NULL);

  g_main_loop_run (loop);
}
