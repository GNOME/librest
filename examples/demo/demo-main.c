/* main.c
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <adwaita.h>
#include <gtksourceview/gtksource.h>
#include "demo-window.h"
#include "demo-table.h"

static void
on_activate (GApplication *app, gpointer user_data)
{
  GtkWindow *active_window;

  active_window = gtk_application_get_active_window (GTK_APPLICATION (app));
  if (active_window == NULL)
    {
      active_window = demo_window_new (GTK_APPLICATION (app));
    }

  gtk_window_present (active_window);
}

static void
show_about (GSimpleAction *action,
            GVariant      *state,
            gpointer       user_data)
{
  const char *authors[] = {
    "Günther Wagner",
    NULL
  };

  const char *artists[] = {
    "Günther Wagner",
    NULL
  };

  GtkApplication *app = GTK_APPLICATION (user_data);
  GtkWindow *window = gtk_application_get_active_window (app);

  gtk_show_about_dialog (window,
                         "program-name", _("Librest Demo"),
                         "title", _("About Librest Demo"),
                         /* "logo-icon-name", "org.gnome.RestDemo", */
                         "copyright", "Copyright © 2022 Günther Wagner",
                         "comments", _("Tour of the features in Librest"),
                         "website", "https://gitlab.gnome.org/GNOME/librest",
                         "license-type", GTK_LICENSE_LGPL_2_1,
                         "authors", authors,
                         "artists", artists,
                         "translator-credits", _("translator-credits"),
                         NULL);
}

gint
main (gint   argc,
      gchar *argv[])
{
  adw_init ();
  gtk_source_init ();
  g_type_ensure (DEMO_TYPE_TABLE);

  g_set_prgname ("librest-demo");
  g_set_application_name ("librest-demo");

  static GActionEntry app_entries[] = {
    { "about", show_about, NULL, NULL, NULL },
  };

  AdwApplication *app = adw_application_new ("org.gnome.RestDemo", G_APPLICATION_FLAGS_NONE);
  g_action_map_add_action_entries (G_ACTION_MAP (app),
                                   app_entries, G_N_ELEMENTS (app_entries),
                                   app);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
