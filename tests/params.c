#include <glib.h>
#include "rest/rest-params.h"
#include "rest/rest-param.h"
#include <glib-object.h>

static void
test_params (void)
{
  RestParamsIter iter;
  RestParam *param;
  const char *name;
  gint pos = 0;
  g_autoptr(RestParams) params = NULL;

  struct {
    char *name;
    char *value;
  } data[] = {
      {
        .name = "name1",
        .value = "value1"
      },
      {
        .name = "name2",
        .value = "value2"
      }
  };

  params = rest_params_new ();
  for (gint i = 0; i < sizeof (data)/sizeof (data[0]); i++)
    {
      RestParam *p = rest_param_new_string (data[i].name, REST_MEMORY_COPY, data[i].value);
      rest_params_add (params, p);
    }

  rest_params_iter_init (&iter, params);
  while (rest_params_iter_next (&iter, &name, &param))
    {
      g_assert_cmpstr (data[pos].name, ==, name);
      g_assert_cmpstr (data[pos].value, ==, rest_param_get_content (param));
      pos++;
    }

  rest_params_remove (params, "name2");
  pos = 0;
  rest_params_iter_init (&iter, params);
  while (rest_params_iter_next (&iter, &name, &param))
    {
      g_assert_cmpstr (data[pos].name, ==, name);
      g_assert_cmpstr (data[pos].value, ==, rest_param_get_content (param));
      pos++;
    }
}

static void
test_params_get (void)
{
  g_autoptr(RestParams) params = NULL;
  RestParam *p1;

  struct {
    char *name;
    char *value;
  } data[] = {
      {
        .name = "name1",
        .value = "value1"
      },
      {
        .name = "name2",
        .value = "value2"
      }
  };

  params = rest_params_new ();
  for (gint i = 0; i < sizeof (data)/sizeof (data[0]); i++)
    {
      RestParam *p = rest_param_new_string (data[i].name, REST_MEMORY_COPY, data[i].value);
      rest_params_add (params, p);
    }

  p1 = rest_params_get (params, "name2");

  g_assert_cmpstr (rest_param_get_name (p1), ==, "name2");
  g_assert_cmpstr (rest_param_get_content (p1), ==, "value2");
}

static void
test_params_is_string (void)
{
  g_autoptr(GError) error = NULL;
  g_autoptr(RestParams) params = NULL;
  g_autofree char *file = NULL;
  gsize length;
  gchar *contents;
  RestParam *p;

  struct {
    char *name;
    char *value;
  } data[] = {
      {
        .name = "name1",
        .value = "value1"
      },
      {
        .name = "name2",
        .value = "value2"
      }
  };

  params = rest_params_new ();
  for (gint i = 0; i < sizeof (data)/sizeof (data[0]); i++)
    {
      RestParam *p = rest_param_new_string (data[i].name, REST_MEMORY_COPY, data[i].value);
      rest_params_add (params, p);
    }

  g_assert_true (rest_params_are_strings (params));

  file = g_test_build_filename (G_TEST_DIST, "test-media.png", NULL);
  g_file_get_contents(file, &contents, &length, &error);

  p = rest_param_new_full ("nostring", REST_MEMORY_COPY, contents, length, "image/png", "test-media.png");
  rest_params_add (params, p);

  g_assert_false (rest_params_are_strings (params));
}

gint
main (gint   argc,
      gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func("/rest/params", test_params);
  g_test_add_func("/rest/params_get", test_params_get);
  g_test_add_func("/rest/params_is_strings", test_params_is_string);

  return g_test_run ();
}
