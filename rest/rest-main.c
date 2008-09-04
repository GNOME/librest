#include "rest-private.h"

guint rest_debug_flags = 0;

/*
 * "Private" function used to set debugging flags based on environment
 * variables. Called upon entry into all public functions.
 */
void
_rest_setup_debugging (void)
{
  static gboolean setup_done = FALSE;
  static const GDebugKey keys[] = {
    { "xml-parser", REST_DEBUG_XML_PARSER },
    { "proxy", REST_DEBUG_PROXY }
  };

  if (G_LIKELY (setup_done))
    return;

  rest_debug_flags = g_parse_debug_string (g_getenv ("REST_DEBUG"),
                                           keys, G_N_ELEMENTS (keys));
  
  setup_done = TRUE;
}
