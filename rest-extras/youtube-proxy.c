/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, Intel Corporation.
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
#include <stdlib.h>
#include <string.h>
#include <rest/rest-proxy.h>
#include <rest/rest-xml-node.h>
#include <libsoup/soup.h>

#include "rest/rest-private.h"
#include "youtube-proxy.h"

typedef struct {
  char *developer_key;
  char *user_auth;
} YoutubeProxyPrivate;

G_DEFINE_TYPE_WITH_PRIVATE (YoutubeProxy, youtube_proxy, REST_TYPE_PROXY)

#define UPLOAD_URL \
  "http://uploads.gdata.youtube.com/feeds/api/users/default/uploads"

enum {
  PROP_0,
  PROP_DEVELOPER_KEY,
  PROP_USER_AUTH,
  N_PROPS,
};

static GParamSpec *properties [N_PROPS];

G_DEFINE_QUARK (rest-youtube-proxy-error-quark, youtube_proxy_error)

static void
youtube_proxy_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  YoutubeProxy *self = YOUTUBE_PROXY (object);
  YoutubeProxyPrivate *priv = youtube_proxy_get_instance_private (self);

  switch (property_id) {
  case PROP_DEVELOPER_KEY:
    g_value_set_string (value, priv->developer_key);
    break;
  case PROP_USER_AUTH:
    g_value_set_string (value, priv->user_auth);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
youtube_proxy_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  YoutubeProxy *self = YOUTUBE_PROXY (object);
  YoutubeProxyPrivate *priv = youtube_proxy_get_instance_private (self);

  switch (property_id) {
  case PROP_DEVELOPER_KEY:
    g_free (priv->developer_key);
    priv->developer_key = g_value_dup_string (value);
    break;
  case PROP_USER_AUTH:
    g_free (priv->user_auth);
    priv->user_auth = g_value_dup_string (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
youtube_proxy_finalize (GObject *object)
{
  YoutubeProxy *self = YOUTUBE_PROXY (object);
  YoutubeProxyPrivate *priv = youtube_proxy_get_instance_private (self);

  g_free (priv->developer_key);
  g_free (priv->user_auth);

  G_OBJECT_CLASS (youtube_proxy_parent_class)->finalize (object);
}

static void
youtube_proxy_class_init (YoutubeProxyClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = youtube_proxy_get_property;
  object_class->set_property = youtube_proxy_set_property;
  object_class->finalize = youtube_proxy_finalize;

  properties [PROP_DEVELOPER_KEY] =
    g_param_spec_string ("developer-key",
                         "developer-key",
                         "The developer API key",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_CONSTRUCT_ONLY |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_USER_AUTH] =
    g_param_spec_string ("user-auth",
                         "user-auth",
                         "The ClientLogin token",
                         NULL,
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
youtube_proxy_init (YoutubeProxy *self)
{
}

RestProxy *
youtube_proxy_new (const char *developer_key)
{
  return youtube_proxy_new_with_auth (developer_key,
                                      NULL);
}

RestProxy *
youtube_proxy_new_with_auth (const char *developer_key,
                            const char *user_auth)
{
  return g_object_new (YOUTUBE_TYPE_PROXY,
                       "developer-key", developer_key,
                       "user-auth", user_auth,
                       NULL);
}

void
youtube_proxy_set_user_auth (YoutubeProxy *self,
                             const gchar  *user_auth)
{
  YoutubeProxyPrivate *priv = youtube_proxy_get_instance_private (self);

  priv->user_auth = g_strdup (user_auth);
}

static gchar *
_construct_upload_atom_xml (GHashTable *fields,
                            gboolean    incomplete)
{
  GHashTableIter iter;
  gpointer key, value;
  RestXmlNode *entry = rest_xml_node_add_child (NULL, "entry");
  RestXmlNode *group = rest_xml_node_add_child (entry, "media:group");
  gchar *bare_xml;
  gchar *full_xml;

  rest_xml_node_add_attr (entry, "xmlns", "http://www.w3.org/2005/Atom");
  rest_xml_node_add_attr (entry, "xmlns:media",
                          "http://search.yahoo.com/mrss/");
  rest_xml_node_add_attr (entry, "xmlns:yt",
                          "http://gdata.youtube.com/schemas/2007");

  if (incomplete)
    rest_xml_node_add_child (group, "yt:incomplete");

  g_hash_table_iter_init (&iter, fields);

  while (g_hash_table_iter_next (&iter, &key, &value)) {
    RestXmlNode *node;
    gchar *tag_name;
    const gchar* field_value = value;
    const gchar* field_key = key;

    tag_name = g_strdup_printf ("media:%s", field_key);

    node = rest_xml_node_add_child (group, tag_name);

    if (g_strcmp0 (field_key, "title") == 0 ||
        g_strcmp0 (field_key, "description") == 0)
      rest_xml_node_add_attr (node, "type", "plain");

    if (g_strcmp0 (field_key, "category") == 0)
      rest_xml_node_add_attr (node, "scheme", "http://gdata.youtube.com/"
                              "schemas/2007/categories.cat");

    rest_xml_node_set_content (node, field_value);
  }

  bare_xml = rest_xml_node_print (entry);
  full_xml = g_strdup_printf ("<?xml version=\"1.0\"?>\n%s", bare_xml);

  rest_xml_node_unref (entry);
  g_free (bare_xml);

  return full_xml;
}

static void
_set_upload_headers (YoutubeProxy *self,
                     SoupMessageHeaders *headers,
                     const gchar *filename)
{
  YoutubeProxyPrivate *priv = youtube_proxy_get_instance_private (self);
  gchar *user_auth_header;
  gchar *devkey_header;
  gchar *basename;
  const gchar *user_agent;

  /* Set the user agent, if one was set in the proxy */
  user_agent = rest_proxy_get_user_agent (REST_PROXY (self));
  if (user_agent)
    soup_message_headers_append (headers, "User-Agent", user_agent);

  g_print ("%s\n", priv->user_auth);

  user_auth_header = g_strdup_printf ("GoogleLogin auth=%s", priv->user_auth);
  soup_message_headers_append (headers, "Authorization", user_auth_header);
  devkey_header = g_strdup_printf ("key=%s", priv->developer_key);
  soup_message_headers_append (headers, "X-GData-Key", devkey_header);
  basename = g_path_get_basename (filename);
  soup_message_headers_append (headers, "Slug", basename);

  g_free (user_auth_header);
  g_free (devkey_header);
}

typedef struct {
  YoutubeProxy *proxy;
  YoutubeProxyUploadCallback callback;
  SoupMessage *message;
  GObject *weak_object;
  gpointer user_data;
  gsize uploaded;
#ifndef WITH_SOUP_2
  GCancellable *cancellable;
#endif
} YoutubeProxyUploadClosure;

static void
_upload_async_weak_notify_cb (gpointer *data,
                              GObject  *dead_object)
{
  YoutubeProxyUploadClosure *closure =
    (YoutubeProxyUploadClosure *) data;

#ifdef WITH_SOUP_2
  _rest_proxy_cancel_message (REST_PROXY (closure->proxy), closure->message);
#else
  g_cancellable_cancel (closure->cancellable);
#endif
}

static void
_upload_async_closure_free (YoutubeProxyUploadClosure *closure)
{
  if (closure->weak_object != NULL)
    g_object_weak_unref (closure->weak_object,
                         (GWeakNotify) _upload_async_weak_notify_cb,
                         closure);

  g_object_unref (closure->proxy);
#ifndef WITH_SOUP_2
  g_object_unref (closure->cancellable);
#endif

  g_slice_free (YoutubeProxyUploadClosure, closure);
}

static YoutubeProxyUploadClosure *
_upload_async_closure_new (YoutubeProxy *self,
                           YoutubeProxyUploadCallback callback,
                           SoupMessage *message,
                           GObject *weak_object,
                           gpointer user_data)
{
  YoutubeProxyUploadClosure *closure =
    g_slice_new0 (YoutubeProxyUploadClosure);

  closure->proxy = g_object_ref (self);
  closure->callback = callback;
  closure->message = message;
  closure->weak_object = weak_object;
  closure->user_data = user_data;
#ifndef WITH_SOUP_2
  closure->cancellable = g_cancellable_new ();
#endif

  if (weak_object != NULL)
    g_object_weak_ref (weak_object,
                       (GWeakNotify) _upload_async_weak_notify_cb,
                       closure);
  return closure;
}

static void
_upload_completed_cb (SoupMessage *message,
                      GBytes      *payload,
                      GError      *error,
                      gpointer     user_data)
{
  YoutubeProxyUploadClosure *closure =
    (YoutubeProxyUploadClosure *) user_data;
  gsize length;
  gconstpointer data;
  guint status_code;
  const char *reason_phrase;

  if (closure->callback == NULL)
    return;

#ifdef WITH_SOUP_2
  status_code = message->status_code;
  reason_phrase = message->reason_phrase;
#else
  status_code = soup_message_get_status (message);
  reason_phrase = soup_message_get_reason_phrase (message);
#endif

  if (status_code < 200 || status_code >= 300)
    {
      g_clear_error (&error);
      error = g_error_new_literal (REST_PROXY_ERROR,
                                   status_code,
                                   reason_phrase);
    }

  data = g_bytes_get_data (payload, &length);
  closure->callback (closure->proxy, data, length, length,
                     error, closure->weak_object, closure->user_data);
  g_bytes_unref (payload);

  _upload_async_closure_free (closure);
}

static void
_message_wrote_data_cb (SoupMessage               *msg,
#ifdef WITH_SOUP_2
                        SoupBuffer                *chunk,
#else
                        gsize                      chunk_size,
#endif
                        YoutubeProxyUploadClosure *closure)
{
#ifdef WITH_SOUP_2
  gsize chunk_size = chunk->length;
  goffset content_length = msg->request_body->length;
#else
  goffset content_length = soup_message_headers_get_content_length (soup_message_get_request_headers (msg));
#endif

  closure->uploaded = closure->uploaded + chunk_size;

  if (closure->uploaded < content_length)
    closure->callback (closure->proxy,
                       NULL,
                       content_length,
                       closure->uploaded,
                       NULL,
                       closure->weak_object,
                       closure->user_data);
}

/**
 * youtube_proxy_upload_async:
 * @self: a #YoutubeProxy
 * @filename: filename
 * @fields: (element-type utf8 utf8): fields
 * @incomplete: incomplete
 * @callback: (scope call): callback to invoke upon completion
 * @weak_object: (nullable): an object instance used to tie the life cycle of
 * the proxy to 
 * @user_data: user data to pass to the callback
 * @error: a #GError pointer, or %NULL
 *
 * Upload a file.
 *
 * Returns: %TRUE, or %FALSE if the file could not be opened
 */
gboolean
youtube_proxy_upload_async (YoutubeProxy              *self,
                            const gchar               *filename,
                            GHashTable                *fields,
                            gboolean                   incomplete,
                            YoutubeProxyUploadCallback callback,
                            GObject                   *weak_object,
                            gpointer                   user_data,
                            GError                   **error)
{
  SoupMultipart *mp;
  SoupMessage *message;
  SoupMessageHeaders *part_headers;
  SoupMessageHeaders *request_headers;
#ifdef WITH_SOUP_2
  SoupBuffer *sb;
#else
  GBytes *sb;
#endif
  gchar *content_type;
  gchar *atom_xml;
  GMappedFile *map;
  YoutubeProxyUploadClosure *closure;

  map = g_mapped_file_new (filename, FALSE, error);
  if (*error != NULL) {
    g_warning ("Error opening file %s: %s", filename, (*error)->message);
    return FALSE;
  }

  mp = soup_multipart_new ("multipart/related");

  atom_xml = _construct_upload_atom_xml (fields, incomplete);

#ifdef WITH_SOUP_2
  sb = soup_buffer_new_with_owner (atom_xml,
                                   strlen(atom_xml),
                                   atom_xml,
                                   (GDestroyNotify) g_free);
#else
  sb = g_bytes_new_with_free_func (atom_xml,
                                   strlen (atom_xml),
                                   (GDestroyNotify) g_free,
                                   atom_xml);
#endif

  part_headers = soup_message_headers_new (SOUP_MESSAGE_HEADERS_MULTIPART);

  soup_message_headers_append (part_headers, "Content-Type",
                               "application/atom+xml; charset=UTF-8");


  soup_multipart_append_part (mp, part_headers, sb);

#ifdef WITH_SOUP_2
  soup_buffer_free (sb);
#else
  g_bytes_unref (sb);
#endif

  content_type = g_content_type_guess (
      filename,
      (const guchar*) g_mapped_file_get_contents (map),
      g_mapped_file_get_length (map),
      NULL);

#ifdef WITH_SOUP_2
  sb = soup_buffer_new_with_owner (g_mapped_file_get_contents (map),
                                   g_mapped_file_get_length (map),
                                   map,
                                   (GDestroyNotify) g_mapped_file_unref);
#else
  sb = g_bytes_new_with_free_func (g_mapped_file_get_contents (map),
                                   g_mapped_file_get_length (map),
                                   (GDestroyNotify) g_mapped_file_unref,
                                   map);
#endif

  soup_message_headers_replace (part_headers, "Content-Type", content_type);

  soup_multipart_append_part (mp, part_headers, sb);

#ifdef WITH_SOUP_2
  soup_buffer_free (sb);
  soup_message_headers_free (part_headers);
  message = soup_form_request_new_from_multipart (UPLOAD_URL, mp);
  request_headers = message->request_headers;
#else
  g_bytes_unref (sb);
  soup_message_headers_unref (part_headers);
  message = soup_message_new_from_multipart (UPLOAD_URL, mp);
  request_headers = soup_message_get_request_headers (message);
#endif

  soup_multipart_free (mp);

  _set_upload_headers (self, request_headers, filename);

  closure = _upload_async_closure_new (self, callback, message, weak_object,
                                       user_data);

  g_signal_connect (message,
                    "wrote-body-data",
                    (GCallback) _message_wrote_data_cb,
                    closure);


  _rest_proxy_queue_message (REST_PROXY (self), message,
#ifdef WITH_SOUP_2
                             NULL,
#else
                             closure->cancellable,
#endif
                             _upload_completed_cb,
                             closure);

  return TRUE;
}
