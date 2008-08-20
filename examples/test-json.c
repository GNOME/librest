#include <rest/rest-proxy.h>

static void
proxy_call_json_async_cb (RestProxy *proxy,
                          guint status_code,
                          const gchar *response_message,
                          GHashTable *headers,
                          JsonNode *root,
                          GObject *weak_object,
                          gpointer userdata)
{
  g_main_loop_quit ((GMainLoop *)userdata);
}

gint
main (gint argc, gchar **argv)
{
  RestProxy *proxy;
  GMainLoop *loop;
  gchar *payload;
  gssize len;

  g_type_init ();
  g_thread_init (NULL);

  loop = g_main_loop_new (NULL, FALSE);

  proxy = rest_proxy_new ("http://www.flickr.com/services/rest/", FALSE);
  rest_proxy_call_json_async (proxy,
      NULL,
      "GET",
      proxy_call_json_async_cb,
      NULL,
      loop,
      NULL,
      "method",
      "flickr.test.echo",
      "api_key",
      "314691be2e63a4d58994b2be01faacfb",
      "format",
      "json",
      NULL);

  g_main_loop_run (loop);
}
