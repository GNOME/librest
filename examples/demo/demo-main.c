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
#include <adwaita.h>
#include <gtksourceview/gtksource.h>
#include "demo-window.h"

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

gint
main (gint   argc,
      gchar *argv[])
{
  adw_init ();
  gtk_source_init ();

  g_set_prgname ("librest-demo");
  g_set_application_name ("librest-demo");

  GtkApplication *app = gtk_application_new ("org.gnome.RestDemo", G_APPLICATION_FLAGS_NONE);
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);

  return g_application_run (G_APPLICATION (app), argc, argv);
}
