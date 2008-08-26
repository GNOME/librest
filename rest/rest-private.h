#ifndef _REST_PRIVATE
#define _REST_PRIVATE

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  REST_DEBUG_XML_PARSER = 1 << 0,
  REST_DEBUG_PROXY = 1 << 1,
  REST_DEBUG_ALL = REST_DEBUG_XML_PARSER | REST_DEBUG_PROXY
} RestDebugFlags;

extern guint rest_debug_flags;

#define REST_DEBUG(category,x,a...)             G_STMT_START {      \
        if (rest_debug_flags & REST_DEBUG_##category)               \
          { g_message ("[" #category "] " G_STRLOC ": " x, ##a); }  \
                                                } G_STMT_END

void _rest_setup_debugging (void);

G_END_DECLS
#endif /* _REST_PRIVATE */
