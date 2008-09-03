#include <rest/rest-proxy-call.h>

G_DEFINE_TYPE (RestProxyCall, rest_proxy_call, G_TYPE_OBJECT)

#define GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REST_TYPE_PROXY_CALL, RestProxyCallPrivate))

typedef struct _RestProxyCallPrivate RestProxyCallPrivate;

struct _RestProxyCallPrivate {
};

static void
rest_proxy_call_get_property (GObject *object, guint property_id,
                              GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_call_set_property (GObject *object, guint property_id,
                              const GValue *value, GParamSpec *pspec)
{
  switch (property_id) {
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
rest_proxy_call_dispose (GObject *object)
{
  if (G_OBJECT_CLASS (rest_proxy_call_parent_class)->dispose)
    G_OBJECT_CLASS (rest_proxy_call_parent_class)->dispose (object);
}

static void
rest_proxy_call_finalize (GObject *object)
{
  if (G_OBJECT_CLASS (rest_proxy_call_parent_class)->finalize)
    G_OBJECT_CLASS (rest_proxy_call_parent_class)->finalize (object);
}

static void
rest_proxy_call_class_init (RestProxyCallClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (RestProxyCallPrivate));

  object_class->get_property = rest_proxy_call_get_property;
  object_class->set_property = rest_proxy_call_set_property;
  object_class->dispose = rest_proxy_call_dispose;
  object_class->finalize = rest_proxy_call_finalize;
}

static void
rest_proxy_call_init (RestProxyCall *self)
{
}

RestProxyCall*
rest_proxy_call_new (void)
{
  return g_object_new (REST_TYPE_PROXY_CALL, NULL);
}

