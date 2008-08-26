#include "rest-private.h"

guint rest_debug_flags = 0;

/*
 * "Private" function used to set debugging flags based on environment
 * variables. Called upon entry into all public functions.
 */
void
_rest_setup_debugging (void)
{
  const gchar *tmp;
  gchar **parts;
  gint i = 0;
  static gboolean setup_done = FALSE;

  if (setup_done)
    return;

  tmp = g_getenv ("REST_DEBUG");

  if (tmp)
  {
    parts = g_strsplit (tmp, ",", -1);

    for (i = 0; parts[i] != NULL; i++)
    {
      if (g_str_equal (tmp, "xml-parser"))
      {
        rest_debug_flags |= REST_DEBUG_XML_PARSER;
      } else if (g_str_equal (tmp, "proxy")) {
        rest_debug_flags |= REST_DEBUG_PROXY;
      } else if (g_str_equal (tmp, "all")) {
        rest_debug_flags |= REST_DEBUG_ALL;
      }
    }

    g_strfreev (parts);
  }

  setup_done = TRUE;
}
