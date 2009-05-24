#include <stdio.h>
#include <string.h>
#include <rest/facebook-proxy.h>
#include <rest/rest-xml-parser.h>

static RestXmlNode *
get_xml (RestProxyCall *call)
{
  static RestXmlParser *parser = NULL;
  RestXmlNode *root;

  if (parser == NULL)
    parser = rest_xml_parser_new ();

  root = rest_xml_parser_parse_from_data (parser,
                                          rest_proxy_call_get_payload (call),
                                          rest_proxy_call_get_payload_length (call));

  if (strcmp (root->name,"error_response") == 0) {
    RestXmlNode *node;
    node = rest_xml_node_find (root, "error_msg");
    g_error ("Error from facebook: %s", node->content);
  }

  g_object_unref (call);

  return root;
}

int
main (int argc, char **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  RestXmlNode *root, *node;
  const char *secret, *session_key;
  char *token, *url;

  g_thread_init (NULL);
  g_type_init ();

  proxy = facebook_proxy_new ("9632214752c7dfb3a84890fbb6846dad",
                              "9dfdb14b9f110e0b14bb0607a14caefa");

  if (argc == 3) {
    facebook_proxy_set_app_secret (FACEBOOK_PROXY (proxy), argv[1]);
    facebook_proxy_set_session_key (FACEBOOK_PROXY (proxy), argv[2]);
  } else {
    call = rest_proxy_new_call (proxy);
    rest_proxy_call_set_function (call, "auth.createToken");

    if (!rest_proxy_call_run (call, NULL, NULL))
    g_error ("Cannot get token");

    root = get_xml (call);
    if (strcmp (root->name, "auth_createToken_response") != 0)
      g_error ("Unexpected response to createToken");

    token = g_strdup (root->content);
    rest_xml_node_unref (root);

    g_print ("Got token %s\n", token);

    url = facebook_proxy_build_login_url (FACEBOOK_PROXY (proxy), token);

    g_print ("Login URL %s\n", url);

    getchar ();

    call = rest_proxy_new_call (proxy);
    rest_proxy_call_set_function (call, "auth.getSession");
    rest_proxy_call_add_param (call, "auth_token", token);

    if (!rest_proxy_call_run (call, NULL, NULL))
      g_error ("Cannot get token");

    root = get_xml (call);

    session_key = rest_xml_node_find (root, "session_key")->content;
    secret = rest_xml_node_find (root, "secret")->content;
    g_print ("Got new secret %s and session key %s\n", secret, session_key);

    facebook_proxy_set_session_key (FACEBOOK_PROXY (proxy), session_key);
    facebook_proxy_set_app_secret (FACEBOOK_PROXY (proxy), secret);
  }

  /* Make an authenticated call */
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "users.getInfo");
  rest_proxy_call_add_param (call, "uids", "1340627425");
  rest_proxy_call_add_param (call, "fields", "uid,name");

  if (!rest_proxy_call_run (call, NULL, NULL))
    g_error ("Cannot get user info");

  root = get_xml (call);
  node = rest_xml_node_find (root, "name");
  g_print ("Logged in as %s\n", node->content);

  return 0;
}
