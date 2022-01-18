/* demo-rest-page.c
 *
 * Copyright 2022 Günther Wagner <info@gunibert.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "demo-rest-page.h"
#include <rest/rest.h>
#include <gtksourceview/gtksource.h>
#include <json-glib/json-glib.h>

struct _DemoRestPage
{
  GtkBox parent_instance;

  GtkWidget *httpmethod;
  GtkWidget *host;
  GtkWidget *function;
  GtkWidget *sourceview;
  GtkWidget *body;
  GtkWidget *notebook;
};

G_DEFINE_FINAL_TYPE (DemoRestPage, demo_rest_page, GTK_TYPE_BOX)

DemoRestPage *
demo_rest_page_new (void)
{
  return g_object_new (DEMO_TYPE_REST_PAGE, NULL);
}

static void
demo_rest_page_finalize (GObject *object)
{
  DemoRestPage *self = (DemoRestPage *)object;

  G_OBJECT_CLASS (demo_rest_page_parent_class)->finalize (object);
}

static void
populate_http_method (DemoRestPage *self)
{
  GtkDropDown *dropdown = GTK_DROP_DOWN (self->httpmethod);

  GtkStringList *model = gtk_string_list_new ((const char *[]){
                                              "GET",
                                              "POST",
                                              "DELETE",
                                              "UPDATE",
                                              NULL});
  gtk_drop_down_set_model (dropdown, G_LIST_MODEL (model));
}

static void
set_json_response (DemoRestPage  *self,
                   RestProxyCall *call)
{
  const gchar *payload = rest_proxy_call_get_payload (call);
  goffset payload_length = rest_proxy_call_get_payload_length (call);

  JsonParser *parser = json_parser_new ();
  json_parser_load_from_data (parser, payload, payload_length, NULL);
  JsonNode *root = json_parser_get_root (parser);

  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->sourceview));
  gtk_text_buffer_set_text (buffer, json_to_string (root, TRUE), -1);

  GtkSourceLanguageManager *manager = gtk_source_language_manager_get_default ();
  GtkSourceLanguage *lang = gtk_source_language_manager_guess_language (manager, NULL, "application/json");
  gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), lang);
}

static void
call_returned (GObject      *object,
               GAsyncResult *result,
               gpointer      user_data)
{
  DemoRestPage *self = (DemoRestPage *)user_data;
  RestProxyCall *call = (RestProxyCall *)object;
  g_autoptr(GError) error = NULL;

  g_assert (G_IS_OBJECT (object));
  g_assert (G_IS_ASYNC_RESULT (result));

  rest_proxy_call_invoke_finish (call, result, &error);
  GHashTable *response_headers = rest_proxy_call_get_response_headers (call);
  gchar *content_type = g_hash_table_lookup (response_headers, "Content-Type");
  if (content_type != NULL)
    {
      if (g_strcmp0 (content_type, "application/json") == 0)
        set_json_response (self, call);
    }
}

static void
on_send_clicked (GtkButton *btn,
                 gpointer   user_data)
{
  DemoRestPage *self = (DemoRestPage *)user_data;
  const gchar *http;
  const gchar *url;
  const gchar *function;
  guint selected_method;
  GListModel *model;

  g_return_if_fail (DEMO_IS_REST_PAGE (self));

  selected_method = gtk_drop_down_get_selected (GTK_DROP_DOWN (self->httpmethod));
  model = gtk_drop_down_get_model (GTK_DROP_DOWN (self->httpmethod));
  http = gtk_string_list_get_string (GTK_STRING_LIST (model), selected_method);

  url = gtk_editable_get_text (GTK_EDITABLE (self->host));
  function = gtk_editable_get_text (GTK_EDITABLE (self->function));
  RestProxy *proxy = rest_proxy_new (url, FALSE);

  RestProxyCall *call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_method (call, http);
  rest_proxy_call_set_function (call, function);
  rest_proxy_call_invoke_async (call, NULL, call_returned, self);
}

static void
demo_rest_page_class_init (DemoRestPageClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = demo_rest_page_finalize;
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/RestDemo/demo-rest-page.ui");
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, httpmethod);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, host);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, function);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, sourceview);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, body);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, notebook);
  gtk_widget_class_bind_template_callback (widget_class, on_send_clicked);
}

static void
demo_rest_page_init (DemoRestPage *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
  populate_http_method (self);

  GtkSourceStyleSchemeManager *manager = gtk_source_style_scheme_manager_get_default ();
  GtkSourceStyleScheme *style = gtk_source_style_scheme_manager_get_scheme (manager, "Adwaita");
  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->sourceview));
  gtk_source_buffer_set_style_scheme (GTK_SOURCE_BUFFER (buffer), style);

  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (self->notebook), self->body, "Body");
}

GtkWidget *
demo_rest_page_get_entry (DemoRestPage *self)
{
  g_return_val_if_fail (DEMO_IS_REST_PAGE (self), NULL);

  return self->host;
}
