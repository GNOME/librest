/*
 * librest - RESTful web services access
 * Copyright (c) 2009 Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 *          Ross Burton <ross@linux.intel.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include <config.h>

#include <string.h>
#include <stdlib.h>
#include <libsoup/soup.h>
#include <rest/rest-proxy.h>

static volatile int errors = 0;
static const gboolean verbose = FALSE;

#define REST_TYPE_CUSTOM_PROXY_CALL custom_proxy_call_get_type()

#define REST_CUSTOM_PROXY_CALL(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), REST_TYPE_CUSTOM_PROXY_CALL, CustomProxyCall))

#define REST_CUSTOM_PROXY_CALL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), REST_TYPE_CUSTOM_PROXY_CALL, CustomProxyCallClass))

#define REST_IS_CUSTOM_PROXY_CALL(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REST_TYPE_CUSTOM_PROXY_CALL))

#define REST_IS_CUSTOM_PROXY_CALL_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), REST_TYPE_CUSTOM_PROXY_CALL))

#define REST_CUSTOM_PROXY_CALL_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), REST_TYPE_CUSTOM_PROXY_CALL, CustomProxyCallClass))

typedef struct {
  RestProxyCall parent;
} CustomProxyCall;

typedef struct {
  RestProxyCallClass parent_class;
} CustomProxyCallClass;

GType custom_proxy_call_get_type (void);

G_DEFINE_TYPE (CustomProxyCall, custom_proxy_call, REST_TYPE_PROXY_CALL)

static gboolean
custom_proxy_call_serialize (RestProxyCall *self,
                             gchar **content_type,
                             gchar **content, gsize *content_len,
                             GError **error)
{
  g_return_val_if_fail (REST_IS_CUSTOM_PROXY_CALL (self), FALSE);

  *content_type = g_strdup ("application/json");
  *content = g_strdup ("{}");
  *content_len = strlen (*content);
  rest_proxy_call_set_function (self, "ping");

  return TRUE;
}

static void
custom_proxy_call_class_init (CustomProxyCallClass *klass)
{
  RestProxyCallClass *parent_klass = (RestProxyCallClass*) klass;

  parent_klass->serialize_params = custom_proxy_call_serialize;
}

static void
custom_proxy_call_init (CustomProxyCall *self)
{
}

static void
server_callback (SoupServer *server, SoupMessage *msg,
                 const char *path, GHashTable *query,
                 SoupClientContext *client, gpointer user_data)
{
  if (g_str_equal (path, "/ping")) {
    const char *content_type = NULL;
    SoupMessageHeaders *headers = msg->request_headers;
    SoupMessageBody *body = msg->request_body;
    content_type = soup_message_headers_get_content_type (headers, NULL);
    g_assert_cmpstr (content_type, ==, "application/json");

    g_assert_cmpstr (body->data, ==, "{}");

    soup_message_set_status (msg, SOUP_STATUS_OK);
  } else {
    soup_message_set_status (msg, SOUP_STATUS_NOT_IMPLEMENTED);
  }
}

int
main (int argc, char **argv)
{
  SoupServer *server;
  RestProxy *proxy;
  RestProxyCall *call;
  char *url;
  GError *error = NULL;

#if !GLIB_CHECK_VERSION (2, 36, 0)
  g_type_init ();
#endif

  server = soup_server_new (NULL);
  soup_server_add_handler (server, NULL, server_callback, NULL, NULL);
  url = g_strdup_printf ("http://127.0.0.1:%d/", soup_server_get_port (server));

  g_thread_create ((GThreadFunc)soup_server_run, server, FALSE, NULL);

  proxy = rest_proxy_new (url, FALSE);
  call = g_object_new (REST_TYPE_CUSTOM_PROXY_CALL, "proxy", proxy, NULL);

  rest_proxy_call_set_function (call, "wrong-function");

  if (!rest_proxy_call_sync (call, &error)) {
    g_printerr ("Call failed: %s\n", error->message);
    g_error_free (error);
    g_atomic_int_add (&errors, 1);
    goto done;
  }

  if (rest_proxy_call_get_status_code (call) != SOUP_STATUS_OK) {
    g_printerr ("Wrong response code, got %d\n", rest_proxy_call_get_status_code (call));
    g_atomic_int_add (&errors, 1);
    goto done;
  }

  done:
  
  g_object_unref (call);
  g_object_unref (proxy);
  soup_server_quit (server);
  g_free (url);

  return errors != 0;
}
