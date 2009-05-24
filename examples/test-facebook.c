#include <stdio.h>
#include <stdlib.h>
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

static GHashTable *user_hash;

static void
add_user (const char *uid, const char *name)
{
  int real_uid = atoi (uid);
  g_hash_table_insert (user_hash,
                       GINT_TO_POINTER (real_uid),
                       g_strdup (name));
}

static void
add_users (RestXmlNode *root)
{
  RestXmlNode *node;

  for (node = rest_xml_node_find (root, "user"); node; node = node->next) {
    add_user (rest_xml_node_find (node, "uid")->content,
              rest_xml_node_find (node, "name")->content);
  }
}

static const char *
get_username (const char *uid)
{
  int real_uid = atoi (uid);
  return g_hash_table_lookup (user_hash, GINT_TO_POINTER (real_uid));
}

static void
print_statuses (RestXmlNode *root)
{
  RestXmlNode *node, *msg;
  const char *name;

  for (node = rest_xml_node_find (root, "status"); node; node = node->next) {
    name = get_username (rest_xml_node_find (node, "uid")->content);
    msg = rest_xml_node_find (node, "message");
    g_print ("%s %s\n", name, msg->content);
  }
}

int
main (int argc, char **argv)
{
  RestProxy *proxy;
  RestProxyCall *call;
  RestXmlNode *root, *node;
  const char *secret, *session_key, *name;
  char *token, *url, *fql, *uid;
  time_t cutoff;

  g_thread_init (NULL);
  g_type_init ();

  user_hash = g_hash_table_new (NULL, NULL);

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

  /* Make some authenticated calls */

  /* Get the user name */
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "users.getInfo");
  rest_proxy_call_add_param (call, "uids", "1340627425");
  rest_proxy_call_add_param (call, "fields", "name");

  if (!rest_proxy_call_run (call, NULL, NULL))
    g_error ("Cannot get user info");

  root = get_xml (call);
  node = rest_xml_node_find (root, "uid");
  uid = g_strdup (node->content);
  node = rest_xml_node_find (root, "name");
  name = node->content;
  g_print ("Logged in as %s [%s]\n", name, uid);
  add_user (uid, name);
  rest_xml_node_unref (root);

  /* Get the user status messages */
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "status.get");
  rest_proxy_call_add_param (call, "limit", "5");

  if (!rest_proxy_call_run (call, NULL, NULL))
    g_error ("Cannot get statuses");

  root = get_xml (call);
  print_statuses (root);
  rest_xml_node_unref (root);

  /* Get the friend's names */
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "fql.query");
  fql = g_strdup_printf ("SELECT uid,name FROM user WHERE uid IN (SELECT uid2 FROM friend WHERE uid1= %s)", uid);
  rest_proxy_call_add_param (call, "query", fql);
  g_free (fql);

  if (!rest_proxy_call_run (call, NULL, NULL))
    g_error ("Cannot get statuses");

  root = get_xml (call);
  add_users (root);
  rest_xml_node_unref (root);

  /* Get the friends status messages */
  call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_function (call, "fql.query");
  cutoff = time (NULL);
  /* Get posts in the last 10 days */
  cutoff -= 10 * 24 * 60 * 60;
  fql = g_strdup_printf ("SELECT uid,status_id,message FROM status WHERE uid IN (SELECT uid2 FROM friend WHERE uid1 = %s) AND time > %d ORDER BY time", uid, cutoff);
  rest_proxy_call_add_param (call, "query", fql);
  g_free (fql);

  if (!rest_proxy_call_run (call, NULL, NULL))
    g_error ("Cannot get statuses");

  root = get_xml (call);
  print_statuses (root);
  rest_xml_node_unref (root);


  return 0;
}
