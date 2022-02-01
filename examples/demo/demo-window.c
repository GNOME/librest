/* demo-window.c
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

#include "demo-window.h"
#include "demo-rest-page.h"

struct _DemoWindow
{
  AdwApplicationWindow parent_instance;
  GtkWidget *tabview;
};

G_DEFINE_FINAL_TYPE (DemoWindow, demo_window, ADW_TYPE_APPLICATION_WINDOW)

GtkWindow *
demo_window_new (GtkApplication *app)
{
  return g_object_new (DEMO_TYPE_WINDOW,
                       "application", app,
                       NULL);
}

static void
on_add_tab_clicked (GtkButton *btn,
                    gpointer   user_data)
{
  DemoWindow *self = (DemoWindow *)user_data;
  DemoRestPage *page;
  AdwTabPage *tabpage;
  GtkWidget *entry;

  g_return_if_fail (DEMO_IS_WINDOW (self));

  page = demo_rest_page_new ();
  adw_tab_view_append (ADW_TAB_VIEW (self->tabview), GTK_WIDGET (page));
  tabpage = adw_tab_view_get_page (ADW_TAB_VIEW (self->tabview), GTK_WIDGET (page));
  entry = demo_rest_page_get_entry (page);
  g_object_bind_property (GTK_EDITABLE (entry), "text", tabpage, "title", G_BINDING_DEFAULT);
}

static void
demo_window_class_init (DemoWindowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/RestDemo/demo-window.ui");
  gtk_widget_class_bind_template_child (widget_class, DemoWindow, tabview);
  gtk_widget_class_bind_template_callback (widget_class, on_add_tab_clicked);
}

static void
demo_window_init (DemoWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  on_add_tab_clicked (NULL, self);
}
