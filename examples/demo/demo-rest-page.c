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
#include <libsoup/soup.h>
#include <gtksourceview/gtksource.h>
#include <json-glib/json-glib.h>
#include "demo-table.h"
#include "demo-table-row.h"

struct _DemoRestPage
{
  GtkBox parent_instance;

  GtkWidget *httpmethod;
  GtkWidget *host;
  GtkWidget *function;
  GtkWidget *sourceview;
  GtkWidget *body;
  GtkWidget *notebook;
  GtkWidget *authentication;
  GtkWidget *authmethods;
  GtkWidget *authentication_stack;
  GtkWidget *headers;
  GtkWidget *headerslb;
  GtkWidget *params;
  GtkWidget *paramslb;

  /* basic auth */
  GtkWidget *basic_username;
  GtkWidget *basic_password;

  /* digest auth */
  GtkWidget *digest_username;
  GtkWidget *digest_password;

  /* oauth 2 auth */
  GtkWidget *oauth2_client_identifier;
  GtkWidget *oauth2_client_secret;
  GtkWidget *oauth2_auth_url;
  GtkWidget *oauth2_token_url;
  GtkWidget *oauth2_redirect_url;
  GtkWidget *oauth2_get_access_token;
  RestProxy *oauth2_proxy;
  RestPkceCodeChallenge *pkce;
};

typedef enum {
  AUTHMODE_NO,
  AUTHMODE_BASIC,
  AUTHMODE_DIGEST,
  AUTHMODE_OAUTH2
} AuthMode;

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

  g_clear_object (&self->oauth2_proxy);
  g_clear_pointer (&self->pkce, rest_pkce_code_challenge_free);

  G_OBJECT_CLASS (demo_rest_page_parent_class)->finalize (object);
}

static AuthMode
get_current_auth_mode (DemoRestPage *self)
{
  const gchar *stack_name;

  g_return_val_if_fail (DEMO_IS_REST_PAGE (self), 0);

  stack_name = gtk_stack_get_visible_child_name (GTK_STACK (self->authentication_stack));
  if (g_strcmp0 (stack_name, "no_auth") == 0)
    return AUTHMODE_NO;
  else if (g_strcmp0 (stack_name, "basic") == 0)
    return AUTHMODE_BASIC;
  else if (g_strcmp0 (stack_name, "digest") == 0)
    return AUTHMODE_DIGEST;
  else if (g_strcmp0 (stack_name, "oauth2") == 0)
    return AUTHMODE_OAUTH2;

  return AUTHMODE_NO;
}


static void
set_oauth_btn_active (DemoRestPage *self,
                      GtkButton    *btn,
                      RestProxy    *proxy,
                      gboolean      active)
{
  if (active)
    {
      gtk_button_set_label (btn, "Remove access token...");
      gtk_widget_set_css_classes (GTK_WIDGET (btn), (const char*[]){ "destructive-action", NULL });
    }
  else
    {
      gtk_button_set_label (btn, "Get access token...");
      gtk_widget_set_css_classes (GTK_WIDGET (btn), (const char*[]){ "suggested-action", NULL });
      if (proxy == self->oauth2_proxy)
        g_clear_object (&self->oauth2_proxy);
    }
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
  gtk_notebook_set_current_page (GTK_NOTEBOOK (self->notebook), 0);
}

static void
set_text_response (DemoRestPage  *self,
                   RestProxyCall *call)
{
  g_autoptr(GHashTable) response_headers = NULL;

  const gchar *payload = rest_proxy_call_get_payload (call);
  goffset payload_length = rest_proxy_call_get_payload_length (call);
  response_headers = rest_proxy_call_get_response_headers (call);

  GtkTextBuffer *buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (self->sourceview));
  gtk_text_buffer_set_text (buffer, payload, payload_length);

  GtkSourceLanguageManager *manager = gtk_source_language_manager_get_default ();
  GtkSourceLanguage *lang = gtk_source_language_manager_guess_language (manager, NULL, g_hash_table_lookup (response_headers, "Content-Type"));
  gtk_source_buffer_set_language (GTK_SOURCE_BUFFER (buffer), lang);
  gtk_notebook_set_current_page (GTK_NOTEBOOK (self->notebook), 0);
}

static void
demo_rest_page_fetched_oauth2_access_token (GObject      *object,
                                            GAsyncResult *result,
                                            gpointer      user_data)
{
  DemoRestPage *self = (DemoRestPage *)user_data;
  RestProxy *proxy = (RestProxy *)object;
  g_autoptr(GError) error = NULL;

  g_assert (G_IS_OBJECT (object));
  g_assert (G_IS_ASYNC_RESULT (result));

  rest_oauth2_proxy_fetch_access_token_finish (REST_OAUTH2_PROXY (proxy), result, &error);
  if (error)
    {
      set_oauth_btn_active (self, GTK_BUTTON (self->oauth2_get_access_token), proxy, FALSE);
    }
}

static void
oauth2_dialog_response (GtkDialog    *dialog,
                        gint          response_id,
                        DemoRestPage *self)
{
  switch (response_id)
    {
    case GTK_RESPONSE_OK:
      {
        const gchar *code = NULL;
        GtkWidget *content_area = gtk_dialog_get_content_area (dialog);
        GtkWidget *box = gtk_widget_get_first_child (content_area);
        GtkWidget *entry = gtk_widget_get_last_child (box);

        code = gtk_editable_get_text (GTK_EDITABLE (entry));
        rest_oauth2_proxy_fetch_access_token_async (REST_OAUTH2_PROXY (self->oauth2_proxy),
                                                    code,
                                                    rest_pkce_code_challenge_get_verifier (self->pkce),
                                                    NULL,
                                                    demo_rest_page_fetched_oauth2_access_token,
                                                    self);
        break;
      }
    case GTK_RESPONSE_CANCEL:
      set_oauth_btn_active (self, GTK_BUTTON (self->oauth2_get_access_token), self->oauth2_proxy, FALSE);
      break;
    }
}

static GtkWidget *
demo_rest_page_create_oauth2_dialog (DemoRestPage *self,
                                     RestProxy    *proxy)
{
  GtkWidget *dialog = NULL;
  GtkWidget *content_area;
  GtkWidget *box, *lbl, *token_lbl, *verifier_entry;
  g_autofree char *token_str = NULL;

  dialog = gtk_dialog_new_with_buttons ("Get Code...",
                                        GTK_WINDOW (gtk_widget_get_root (GTK_WIDGET (self))),
                                        GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_USE_HEADER_BAR,
                                        "Ok",
                                        GTK_RESPONSE_OK,
                                        "Cancel",
                                        GTK_RESPONSE_CANCEL,
                                        NULL);

  content_area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_widget_set_margin_top (content_area, 6);
  gtk_widget_set_margin_start (content_area, 6);
  gtk_widget_set_margin_bottom (content_area, 6);
  gtk_widget_set_margin_end (content_area, 6);

  box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  lbl = gtk_label_new ("Open a browser and authorize this application:");
  gtk_box_append (GTK_BOX (box), lbl);
  token_str = g_markup_printf_escaped ("Use this <a href=\"%s\">link</a>", rest_oauth2_proxy_build_authorization_url (REST_OAUTH2_PROXY (self->oauth2_proxy),
                                                                                               rest_pkce_code_challenge_get_challenge (self->pkce),
                                                                                               NULL,
                                                                                               NULL));
  token_lbl = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (token_lbl), token_str);
  gtk_label_set_selectable (GTK_LABEL (token_lbl), TRUE);
  gtk_box_append (GTK_BOX (box), token_lbl);
  verifier_entry = gtk_entry_new ();
  gtk_box_append (GTK_BOX (box), verifier_entry);

  gtk_box_append (GTK_BOX (content_area), box);

  g_signal_connect (dialog, "response", G_CALLBACK (oauth2_dialog_response), self);
  g_signal_connect_swapped (dialog, "response", G_CALLBACK (gtk_window_destroy), dialog);

  return dialog;
}

static void
on_oauth2_get_access_token_clicked (GtkButton *btn,
                                    gpointer   user_data)
{
  DemoRestPage *self = (DemoRestPage *)user_data;
  GtkWidget *dialog = NULL;
  const char *url = NULL;
  const char *client_id = NULL, *client_secret = NULL;
  const char *authurl = NULL, *tokenurl = NULL, *redirecturl = NULL;

  if (self->oauth2_proxy != NULL)
    {
      set_oauth_btn_active (self, btn, self->oauth2_proxy, FALSE);
      return;
    }

  url = gtk_editable_get_text (GTK_EDITABLE (self->host));
  client_id = gtk_editable_get_text (GTK_EDITABLE (self->oauth2_client_identifier));
  client_secret = gtk_editable_get_text (GTK_EDITABLE (self->oauth2_client_secret));
  authurl = gtk_editable_get_text (GTK_EDITABLE (self->oauth2_auth_url));
  tokenurl = gtk_editable_get_text (GTK_EDITABLE (self->oauth2_token_url));
  redirecturl = gtk_editable_get_text (GTK_EDITABLE (self->oauth2_redirect_url));

  self->oauth2_proxy = REST_PROXY (rest_oauth2_proxy_new (authurl, tokenurl, redirecturl, client_id, client_secret, url));
  self->pkce = rest_pkce_code_challenge_new_random ();

  dialog = demo_rest_page_create_oauth2_dialog (self, self->oauth2_proxy);

  gtk_widget_show (dialog);
  set_oauth_btn_active (self, btn, self->oauth2_proxy, TRUE);
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
      else
        set_text_response (self, call);
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
  AuthMode auth_mode;
  RestProxy *proxy;

  g_return_if_fail (DEMO_IS_REST_PAGE (self));

  selected_method = gtk_drop_down_get_selected (GTK_DROP_DOWN (self->httpmethod));
  model = gtk_drop_down_get_model (GTK_DROP_DOWN (self->httpmethod));
  http = gtk_string_list_get_string (GTK_STRING_LIST (model), selected_method);
  auth_mode = get_current_auth_mode (self);

  url = gtk_editable_get_text (GTK_EDITABLE (self->host));
  function = gtk_editable_get_text (GTK_EDITABLE (self->function));
  switch (auth_mode)
    {
    case AUTHMODE_NO:
      proxy = rest_proxy_new (url, FALSE);
      break;
    // TODO: provide a direct auth possibility in librest
    case AUTHMODE_BASIC:
      {
        const gchar *username, *password;

        username = gtk_editable_get_text (GTK_EDITABLE (self->basic_username));
        password = gtk_editable_get_text (GTK_EDITABLE (self->basic_password));

        proxy = rest_proxy_new_with_authentication (url, FALSE, username, password);
        break;
      }
    // thanks to libsoup there is no difference as the server defines the used authentication mechanism
    case AUTHMODE_DIGEST:
      {
        const gchar *username, *password;

        username = gtk_editable_get_text (GTK_EDITABLE (self->basic_username));
        password = gtk_editable_get_text (GTK_EDITABLE (self->basic_password));

        proxy = rest_proxy_new_with_authentication (url, FALSE, username, password);
        break;
      }
    case AUTHMODE_OAUTH2:
      {
        proxy = rest_proxy_new (url, FALSE);
        break;
      }
    }
#if WITH_SOUP_2
  SoupLogger *logger = soup_logger_new (SOUP_LOGGER_LOG_BODY, -1);
#else
  SoupLogger *logger = soup_logger_new (SOUP_LOGGER_LOG_BODY);
#endif
  rest_proxy_add_soup_feature (proxy, SOUP_SESSION_FEATURE (logger));

  RestProxyCall *call = rest_proxy_new_call (proxy);
  rest_proxy_call_set_method (call, http);
  rest_proxy_call_set_function (call, function);

  /* get params */
  model = demo_table_get_model (DEMO_TABLE (self->paramslb));
  for (guint i = 0; i < g_list_model_get_n_items (model); i++)
    {
      DemoTableRow *h = (DemoTableRow *)g_list_model_get_item (model, i);
      rest_proxy_call_add_param (call, demo_table_row_get_key (h), demo_table_row_get_value (h));
    }

  /* get headers */
  model = demo_table_get_model (DEMO_TABLE (self->headerslb));
  for (guint i = 0; i < g_list_model_get_n_items (model); i++)
    {
      DemoTableRow *h = (DemoTableRow *)g_list_model_get_item (model, i);
      rest_proxy_call_add_header (call, demo_table_row_get_key (h), demo_table_row_get_value (h));
    }

  rest_proxy_call_invoke_async (call, NULL, call_returned, self);
}

static void
on_auth_method_activated (GtkDropDown *dropdown,
                          GParamSpec *pspec,
                          gpointer     user_data)
{
  DemoRestPage *self = (DemoRestPage *)user_data;
  g_autofree gchar *page_name = NULL;
  GtkStringObject *obj = NULL;

  g_return_if_fail (DEMO_IS_REST_PAGE (self));

  obj = GTK_STRING_OBJECT (gtk_drop_down_get_selected_item (dropdown));
  page_name = g_utf8_strdown (gtk_string_object_get_string (obj), -1);
  gtk_stack_set_visible_child_name (GTK_STACK (self->authentication_stack), g_strdelimit (page_name, " ", '_'));
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
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, authentication);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, authmethods);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, authentication_stack);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, headers);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, params);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, paramslb);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, headerslb);

  /* basic auth */
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, basic_username);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, basic_password);
  /* digest auth */
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, digest_username);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, digest_password);
  /* oauth 2 auth */
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, oauth2_client_identifier);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, oauth2_client_secret);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, oauth2_auth_url);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, oauth2_token_url);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, oauth2_redirect_url);
  gtk_widget_class_bind_template_child (widget_class, DemoRestPage, oauth2_get_access_token);

  /* callbacks */
  gtk_widget_class_bind_template_callback (widget_class, on_send_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_auth_method_activated);
  gtk_widget_class_bind_template_callback (widget_class, on_oauth2_get_access_token_clicked);
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
  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (self->notebook), self->params, "Params");
  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (self->notebook), self->authentication, "Authentication");
  gtk_notebook_set_tab_label_text (GTK_NOTEBOOK (self->notebook), self->headers, "Headers");
}

GtkWidget *
demo_rest_page_get_entry (DemoRestPage *self)
{
  g_return_val_if_fail (DEMO_IS_REST_PAGE (self), NULL);

  return self->host;
}
