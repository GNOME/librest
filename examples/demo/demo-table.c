/* demo-table.c
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

#include "demo-table.h"
#include "demo-table-row.h"

struct _DemoTable
{
  GtkWidget parent_instance;
  char *title;

  GtkListBox *lb;
  GListModel *model;

  GtkWidget *dummy_row;
};

G_DEFINE_FINAL_TYPE (DemoTable, demo_table, GTK_TYPE_WIDGET)

enum {
  PROP_0,
  PROP_TITLE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

GtkWidget *
demo_table_new (char *title)
{
  return g_object_new (DEMO_TYPE_TABLE, "title", title, NULL);
}

static void
demo_table_finalize (GObject *object)
{
  DemoTable *self = (DemoTable *)object;

  g_clear_pointer (&self->title, g_free);

  G_OBJECT_CLASS (demo_table_parent_class)->finalize (object);
}

static void
demo_table_dispose (GObject *object)
{
  DemoTable *self = (DemoTable *)object;

  gtk_widget_unparent (GTK_WIDGET (self->lb));

  G_OBJECT_CLASS (demo_table_parent_class)->dispose (object);
}

static void
demo_table_activated (GtkListBox    *box,
                      GtkListBoxRow *row,
                      gpointer       user_data)
{
  DemoTable *self = (DemoTable *)user_data;

  g_return_if_fail (DEMO_IS_TABLE (self));

  if (GTK_WIDGET (row) == self->dummy_row)
    {
      GListStore *s = G_LIST_STORE (self->model);
      g_list_store_append (s, demo_table_row_new ());
    }
}

static GtkWidget *
create_row (gpointer item,
            gpointer user_data)
{
  GtkBuilder *builder;

  builder = gtk_builder_new_from_resource ("/org/gnome/RestDemo/demo-table-row.ui");

  GObject *key_entry = gtk_builder_get_object (builder, "key");
  g_object_bind_property (key_entry, "text", item, "key", G_BINDING_DEFAULT);

  GObject *value_entry = gtk_builder_get_object (builder, "value");
  g_object_bind_property (value_entry, "text", item, "value", G_BINDING_DEFAULT);
  return GTK_WIDGET (gtk_builder_get_object (builder, "row"));
}

static GtkWidget *
create_dummy_row (DemoTable *self)
{
  GtkWidget *row;
  GtkWidget *label;

  g_return_val_if_fail (DEMO_IS_TABLE (self), NULL);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", self->title,
                        "visible", TRUE,
                        "xalign", 0.0f,
                        NULL);
  g_object_bind_property (self, "title", label, "label", G_BINDING_DEFAULT);
  gtk_style_context_add_class (gtk_widget_get_style_context (label), "dim-label");

  row = g_object_new (GTK_TYPE_LIST_BOX_ROW,
                      "child", label,
                      "visible", TRUE,
                      NULL);

  return row;
}

static void
demo_table_get_property (GObject    *object,
                         guint       prop_id,
                         GValue     *value,
                         GParamSpec *pspec)
{
  DemoTable *self = DEMO_TABLE (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
demo_table_set_property (GObject      *object,
                         guint         prop_id,
                         const GValue *value,
                         GParamSpec   *pspec)
{
  DemoTable *self = DEMO_TABLE (object);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_clear_pointer (&self->title, g_free);
      self->title = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
demo_table_class_init (DemoTableClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = demo_table_finalize;
  object_class->dispose = demo_table_dispose;
  object_class->set_property = demo_table_set_property;
  object_class->get_property = demo_table_get_property;

  properties [PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "Title",
                         "",
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (object_class, PROP_TITLE,
                                   properties [PROP_TITLE]);

  gtk_widget_class_set_layout_manager_type (widget_class, GTK_TYPE_BOX_LAYOUT);
}

static void
demo_table_init (DemoTable *self)
{
  self->lb = GTK_LIST_BOX (gtk_list_box_new ());
  gtk_widget_set_hexpand (GTK_WIDGET (self->lb), TRUE);
  gtk_list_box_set_selection_mode (GTK_LIST_BOX (self->lb), GTK_SELECTION_NONE);
  gtk_widget_set_parent (GTK_WIDGET (self->lb), GTK_WIDGET (self));
  g_signal_connect (self->lb, "row-activated", G_CALLBACK (demo_table_activated), self);

  self->model = G_LIST_MODEL (g_list_store_new (DEMO_TYPE_TABLE_ROW));
  gtk_list_box_bind_model (self->lb, self->model, create_row, self, NULL);

  self->dummy_row = create_dummy_row (self);
  gtk_list_box_append (self->lb, self->dummy_row);
}

GListModel *
demo_table_get_model (DemoTable *self)
{
  g_return_val_if_fail (DEMO_IS_TABLE (self), NULL);

  return self->model;
}
