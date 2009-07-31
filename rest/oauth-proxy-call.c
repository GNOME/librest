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

#include <string.h>
#include <libsoup/soup.h>
#include <rest/rest-proxy-call.h>
#include "oauth-proxy-call.h"
#include "oauth-proxy-private.h"
#include "rest-proxy-call-private.h"
#include "sha1.h"

G_DEFINE_TYPE (OAuthProxyCall, oauth_proxy_call, REST_TYPE_PROXY_CALL)

#define OAUTH_ENCODE_STRING(x_) (x_ ? soup_uri_encode( (x_), "!$&'()*+,;=@") : g_strdup (""))

static char *
sign_plaintext (OAuthProxyPrivate *priv)
{
  char *cs;
  char *ts;
  char *rv;

  cs = OAUTH_ENCODE_STRING (priv->consumer_secret);
  ts = OAUTH_ENCODE_STRING (priv->token_secret);
  rv = g_strconcat (cs, "&", ts, NULL);

  g_free (cs);
  g_free (ts);

  return rv;
}

static char *
encode_params (GHashTable *hash)
{
  GList *keys;
  GString *s;

  s = g_string_new (NULL);

  keys = g_hash_table_get_keys (hash);
  keys = g_list_sort (keys, (GCompareFunc)strcmp);

  while (keys) {
    const char *key;
    const char *value;
    char *k, *v;

    key = keys->data;
    value = g_hash_table_lookup (hash, key);

    k = OAUTH_ENCODE_STRING (key);
    v = OAUTH_ENCODE_STRING (value);

    if (s->len)
      g_string_append (s, "&");

    g_string_append_printf (s, "%s=%s", k, v);

    g_free (k);
    g_free (v);

    keys = keys->next;
  }

  return g_string_free (s, FALSE);
}

/*
 * Add the keys in @from to @hash.
 */
static void
merge_hashes (GHashTable *hash, GHashTable *from)
{
  GHashTableIter iter;
  gpointer key, value;

  g_hash_table_iter_init (&iter, from);
  while (g_hash_table_iter_next (&iter, &key, &value)) {
    g_hash_table_insert (hash, key, g_strdup (value));
  }
}

static char *
sign_hmac (OAuthProxy *proxy, RestProxyCall *call, GHashTable *oauth_params)
{
  OAuthProxyPrivate *priv;
  RestProxyCallPrivate *callpriv;
  char *key, *signature, *ep, *eep;
  GString *text;
  GHashTable *all_params;

  priv = PROXY_GET_PRIVATE (proxy);
  callpriv = call->priv;

  text = g_string_new (NULL);
  g_string_append (text, rest_proxy_call_get_method (REST_PROXY_CALL (call)));
  g_string_append_c (text, '&');
  g_string_append_uri_escaped (text, callpriv->url, NULL, FALSE);
  g_string_append_c (text, '&');

  /* Merge the OAuth parameters with the query parameters */
  all_params = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);
  merge_hashes (all_params, oauth_params);
  merge_hashes (all_params, callpriv->params);

  ep = encode_params (all_params);
  eep = OAUTH_ENCODE_STRING (ep);
  g_string_append (text, eep);
  g_free (ep);
  g_free (eep);
  g_hash_table_destroy (all_params);

  /* PLAINTEXT signature value is the HMAC-SHA1 key value */
  key = sign_plaintext (priv);

  signature = hmac_sha1 (key, text->str);

  g_free (key);
  g_string_free (text, TRUE);

  return signature;
}

/*
 * From the OAuth parameters in @params, construct a HTTP Authorized header.
 */
static char *
make_authorized_header (GHashTable *oauth_params)
{
  GString *auth;
  GHashTableIter iter;
  const char *key, *value;

  g_assert (oauth_params);

  /* TODO: is "" okay for the realm, or should this be magically calculated or a
     parameter? */
  auth = g_string_new ("OAuth realm=\"\"");

  g_hash_table_iter_init (&iter, oauth_params);
  while (g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&value)) {
    g_string_append_printf (auth, ", %s=\"%s\"", key, OAUTH_ENCODE_STRING (value));
  }

  return g_string_free (auth, FALSE);
}

static gboolean
_prepare (RestProxyCall *call, GError **error)
{
  OAuthProxy *proxy = NULL;
  OAuthProxyPrivate *priv;
  char *s;
  GHashTable *oauth_params;

  g_object_get (call, "proxy", &proxy, NULL);
  priv = PROXY_GET_PRIVATE (proxy);

  oauth_params = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, g_free);

  g_hash_table_insert (oauth_params, "oauth_version", g_strdup ("1.0"));

  s = g_strdup_printf ("%lli", (long long int) time (NULL));
  g_hash_table_insert (oauth_params, "oauth_timestamp", s);

  s = g_strdup_printf ("%u", g_random_int ());
  g_hash_table_insert (oauth_params, "oauth_nonce", s);

  g_hash_table_insert (oauth_params, "oauth_consumer_key",
                       g_strdup (priv->consumer_key));

  if (priv->token)
    g_hash_table_insert (oauth_params, "oauth_token", g_strdup (priv->token));

  switch (priv->method) {
  case PLAINTEXT:
    g_hash_table_insert (oauth_params, "oauth_signature_method", g_strdup ("PLAINTEXT"));
    s = sign_plaintext (priv);
    break;
  case HMAC_SHA1:
    g_hash_table_insert (oauth_params, "oauth_signature_method", g_strdup ("HMAC-SHA1"));
    s = sign_hmac (proxy, call, oauth_params);
    break;
  }
  g_hash_table_insert (oauth_params, "oauth_signature", s);

  s = make_authorized_header (oauth_params);
  rest_proxy_call_add_header (call, "Authorization", s);
  g_free (s);
  g_hash_table_destroy (oauth_params);

  g_object_unref (proxy);

  return TRUE;
}

static void
oauth_proxy_call_class_init (OAuthProxyCallClass *klass)
{
  RestProxyCallClass *call_class = REST_PROXY_CALL_CLASS (klass);

  call_class->prepare = _prepare;
}

static void
oauth_proxy_call_init (OAuthProxyCall *self)
{
}

#if BUILD_TESTS
/* Test cases from http://wiki.oauth.net/TestCases */
void
test_param_encoding (void)
{
  GHashTable *hash;
  char *s;

#define TEST(expected) \
  s = encode_params (hash);                      \
  g_assert_cmpstr (s, ==, expected);             \
  g_free (s);                                    \
  g_hash_table_remove_all (hash);

  hash = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (hash, "name", NULL);
  TEST("name=");

  g_hash_table_insert (hash, "a", "b");
  TEST("a=b");

  g_hash_table_insert (hash, "a", "b");
  g_hash_table_insert (hash, "c", "d");
  TEST("a=b&c=d");

  /* Because we don't (yet) support multiple parameters with the same key we've
     changed this case slightly */
  g_hash_table_insert (hash, "b", "x!y");
  g_hash_table_insert (hash, "a", "x y");
  TEST("a=x%20y&b=x%21y");

#undef TEST
}
#endif
