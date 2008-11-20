#include <rest/rest-proxy.h>
#include <unistd.h>

static void
proxy_call_async_cb (RestProxyCall *call,
                     GError        *error,
                     GObject       *weak_object,
                     gpointer       userdata)
{
  const gchar *payload;
  goffset len;

  payload = rest_proxy_call_get_payload (call);
  len = rest_proxy_call_get_payload_length (call);
  write (1, payload, len);
  g_main_loop_quit ((GMainLoop *)userdata);
}

gint
main (gint argc, gchar **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  GMainLoop *loop;
  const gchar *payload;
  gssize len;

  g_type_init ();
  g_thread_init (NULL);

  loop = g_main_loop_new (NULL, FALSE);

  proxy = rest_proxy_new ("http://www.flickr.com/services/rest/", FALSE);
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_add_params (call,
                              "method", "flickr.test.echo",
                              "api_key", "314691be2e63a4d58994b2be01faacfb",
                              "format", "json",
                              NULL);
  rest_proxy_call_async (call, 
                         proxy_call_async_cb,
                         NULL,
                         loop,
                         NULL);

  g_main_loop_run (loop);

  rest_proxy_call_run (call, NULL, NULL);

  payload = rest_proxy_call_get_payload (call);
  len = rest_proxy_call_get_payload_length (call);
  write (1, payload, len);

  g_object_unref (call);
  g_object_unref (proxy);
}
