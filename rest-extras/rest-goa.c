#include <rest/oauth-proxy.h>
#include "rest-goa.h"

RestProxy *
rest_goa_proxy_from_account (GoaObject *object, const char *url_format, gboolean binding_required, gboolean *authenticated)
{
  GoaOAuthBased *oauth;
  GoaAccount *account;
  RestProxy *proxy;
  GError *error = NULL;

  g_return_val_if_fail (GOA_IS_OBJECT (object), NULL);
  g_return_val_if_fail (url_format, NULL);

  oauth = goa_object_get_oauth_based (object);
  if (oauth == NULL) {
    g_warning ("object is not OAuth-based");
    return NULL;
  }

  account = goa_object_get_account (object);

  /* Create an unauthorised proxy */
  proxy = oauth_proxy_new
    (goa_oauth_based_get_consumer_key (oauth),
     goa_oauth_based_get_consumer_secret (oauth),
     url_format, binding_required);

  if (goa_account_call_ensure_credentials_sync (account, NULL, NULL, &error)) {
    char *access_token = NULL, *access_token_secret = NULL;

    if (goa_oauth_based_call_get_access_token_sync
        (oauth, &access_token, &access_token_secret, NULL, NULL, &error)) {
      g_object_set (proxy,
                    "token", access_token,
                    "token-secret", access_token_secret,
                    NULL);

      g_free (access_token);
      g_free (access_token_secret);

      if (authenticated) *authenticated = TRUE;
    } else {
      g_message ("Could not get access token: %s", error->message);
      g_clear_error (&error);

      if (authenticated) *authenticated = FALSE;
    }
  } else {
    g_message ("Could not ensure credentials: %s", error->message);
    g_clear_error (&error);
    /* TODO: ignore and return partial proxy? critical failure and return NULL? */
  }

  g_object_unref (account);
  g_object_unref (oauth);

  return proxy;
}

