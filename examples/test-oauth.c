#include <rest/oauth-proxy.h>
#include <stdio.h>

static GMainLoop *loop;

int
main (int argc, char **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  GError *error = NULL;

  g_thread_init (NULL);
  g_type_init ();

  loop = g_main_loop_new (NULL, TRUE);
  
  /* Create the proxy */
  proxy = oauth_proxy_new (
                           /* Consumer Key */
                           "NmUm6hxQ9a4u",
                           /* Consumer Secret */
                           "t4FM7LiUeD4RBwKSPa6ichKPDh5Jx4kt",
                           /* FireEagle endpoint, which we bind as required */
                           "https://fireeagle.yahooapis.com/%s", TRUE);

  /* First stage authentication, this gets a request token */
  oauth_proxy_auth_step (OAUTH_PROXY (proxy), "oauth/request_token");

  /* From the token construct a URL for the user to visit */
  g_print ("Go to https://fireeagle.yahoo.net/oauth/authorize?oauth_token=%s then hit any key\n",
           oauth_proxy_get_token (OAUTH_PROXY (proxy)));
  getchar ();

  /* Second stage authentication, this gets an access token */
  oauth_proxy_auth_step (OAUTH_PROXY (proxy), "oauth/access_token");
  
  /* We're now authenticated */
  g_print ("Got access token\n");

  /* Get the user's current location */
  rest_proxy_bind (proxy, "api/0.1/user");
  call = rest_proxy_new_call (proxy);

  if (!rest_proxy_call_run (call, NULL, &error))
    g_error ("Cannot make call: %s", error->message);

  g_print ("%s\n", rest_proxy_call_get_payload (call));

  g_object_unref (call);

  g_object_unref (proxy);

  return 0;
}
