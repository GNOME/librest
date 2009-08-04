#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <rest/flickr-proxy.h>
#include <rest/rest-xml-parser.h>

/*
 * Parse the payload and either return a RestXmlNode, or error. As a side-effect
 * the call is unreffed.
 */
static RestXmlNode *
get_xml (RestProxyCall *call)
{
  RestXmlParser *parser;
  RestXmlNode *root;

  parser = rest_xml_parser_new ();

  root = rest_xml_parser_parse_from_data (parser,
                                          rest_proxy_call_get_payload (call),
                                          rest_proxy_call_get_payload_length (call));

  if (strcmp (root->name, "rsp") != 0) {
    g_error ("Unexpected response from Flickr:\n%s",
             rest_proxy_call_get_payload (call));
  }

  if (strcmp (rest_xml_node_get_attr (root, "stat"), "ok") != 0) {
    root = rest_xml_node_find (root, "err");
    g_error ("Error %s from Flickr: %s",
             rest_xml_node_get_attr (root, "code"),
             rest_xml_node_get_attr (root, "msg"));
  }

  g_object_unref (call);

  g_object_unref (parser);

  return root;
}

static void
print_user_name (RestProxy *proxy)
{
  RestProxyCall *call;
  RestXmlNode *root, *node;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "flickr.auth.checkToken");

  if (!rest_proxy_call_sync (call, NULL))
    g_error ("Cannot check token");

  root = get_xml (call);
  node = rest_xml_node_find (root, "user");
  g_print ("Logged in as %s\n",
           rest_xml_node_get_attr (node, "fullname")
           ?: rest_xml_node_get_attr (node, "username"));
  rest_xml_node_unref (root);
}

static void
print_favourites (RestProxy *proxy)
{
  RestProxyCall *call;
  RestXmlNode *root, *node;

  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "flickr.favorites.getList");
  rest_proxy_call_add_param (call, "extras", "owner_name");
  rest_proxy_call_add_param (call, "per_page", "10");

  if (!rest_proxy_call_sync (call, NULL))
    g_error ("Cannot get favourites");

  root = get_xml (call);

  for (node = rest_xml_node_find (root, "photo"); node; node = node->next) {
    g_print ("%s by %s\n",
             rest_xml_node_get_attr (node, "title"),
             rest_xml_node_get_attr (node, "ownername"));
  }

  rest_xml_node_unref (root);
}

int
main (int argc, char **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  RestXmlNode *root;
  char *frob, *url;
  const char *token;

  g_thread_init (NULL);
  g_type_init ();

  proxy = flickr_proxy_new ("cf4e02fc57240a9b07346ad26e291080", "cdfa2329cb206e50");

  if (argc > 1) {
    flickr_proxy_set_token (FLICKR_PROXY (proxy), argv[1]);
  } else {
    call = rest_proxy_new_call (proxy);
    rest_proxy_call_set_function (call, "flickr.auth.getFrob");

    if (!rest_proxy_call_sync (call, NULL))
    g_error ("Cannot get frob");

    root = get_xml (call);
    frob = g_strdup (rest_xml_node_find (root, "frob")->content);
    rest_xml_node_unref (root);

    url = flickr_proxy_build_login_url (FLICKR_PROXY (proxy), frob);
    g_print ("Go to %s to authorise me and then press any key.\n", url);
    getchar ();

    call = rest_proxy_new_call (proxy);
    rest_proxy_call_set_function (call, "flickr.auth.getToken");
    rest_proxy_call_add_param (call, "frob", frob);

    if (!rest_proxy_call_sync (call, NULL))
      g_error ("Cannot get token");

    root = get_xml (call);
    token = rest_xml_node_find (root, "token")->content;
    g_print ("Got token %s\n", token);
    flickr_proxy_set_token (FLICKR_PROXY (proxy), token);
    rest_xml_node_unref (root);
  }

  print_user_name (proxy);

  print_favourites (proxy);

  g_object_unref (proxy);

  return 0;
}
