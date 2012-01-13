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

#include <rest-extras/rest-goa.h>
#include <goa/goa.h>

static GoaObject *
find_twitter (void)
{
  GError *error = NULL;
  GoaClient *client;
  GList *accounts, *l;
  GoaObject *found = NULL;

  client = goa_client_new_sync (NULL, &error);
  if (!client) {
    g_error ("Could not create GoaClient: %s", error->message);
    return NULL;
  }

  accounts = goa_client_get_accounts (client);

  for (l = accounts; l != NULL; l = l->next) {
    GoaObject *object = GOA_OBJECT (l->data);
    GoaAccount *account;

    account = goa_object_peek_account (object);
    if (g_strcmp0 (goa_account_get_provider_type (account), "twitter") == 0) {
      /* Incremenet reference count for our returned object */
      found = g_object_ref (object);
      break;
    }
  }

  g_list_free_full (accounts, (GDestroyNotify) g_object_unref);

  return found;
}

int
main (int argc, char **argv)
{
  GoaObject *twitter;
  RestProxy *proxy;
  RestProxyCall *call;
  GError *error = NULL;
  gboolean authenticated = FALSE;

  g_thread_init (NULL);
  g_type_init ();

  if (argc != 2) {
    g_printerr ("$ post-twitter \"message\"\n");
    return 1;
  }

  twitter = find_twitter ();
  if (twitter == NULL) {
    g_print ("Cannot find a Twitter account\n");
    return 1;
  }

  proxy = rest_goa_proxy_from_account (twitter,
                                       "https://api.twitter.com/", FALSE,
                                       &authenticated);

  if (!authenticated) {
    g_print ("Found Twitter account but not authenticated\n");
    return 1;
  }

  /* Post the status message */
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "1/statuses/update.xml");
  rest_proxy_call_set_method (call, "POST");
  rest_proxy_call_add_param (call, "status", argv[1]);

  if (!rest_proxy_call_sync (call, &error))
    g_error ("Cannot make call: %s", error->message);

  /* TODO: parse the XML and print something useful */
  g_print ("%s\n", rest_proxy_call_get_payload (call));

  g_object_unref (call);
  g_object_unref (proxy);

  return 0;
}
