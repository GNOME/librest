/* demo-table-row.c
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

#include "demo-table-row.h"

struct _DemoTableRow
{
  GObject parent_instance;

  gchar *key;
  gchar *value;
};

G_DEFINE_FINAL_TYPE (DemoTableRow, demo_table_row, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_KEY,
  PROP_VALUE,
  N_PROPS
};

static GParamSpec *properties [N_PROPS];

DemoTableRow *
demo_table_row_new (void)
{
  return g_object_new (DEMO_TYPE_TABLE_ROW, NULL);
}

static void
demo_table_row_finalize (GObject *object)
{
  DemoTableRow *self = (DemoTableRow *)object;

  g_clear_pointer (&self->key, g_free);
  g_clear_pointer (&self->value, g_free);

  G_OBJECT_CLASS (demo_table_row_parent_class)->finalize (object);
}

static void
demo_table_row_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  DemoTableRow *self = DEMO_TABLE_ROW (object);

  switch (prop_id)
    {
    case PROP_KEY:
      g_value_set_string (value, self->key);
      break;
    case PROP_VALUE:
      g_value_set_string (value, self->value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
demo_table_row_set_property (GObject      *object,
                             guint         prop_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  DemoTableRow *self = DEMO_TABLE_ROW (object);

  switch (prop_id)
    {
    case PROP_KEY:
      self->key = g_value_dup_string (value);
      break;
    case PROP_VALUE:
      self->value = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
demo_table_row_class_init (DemoTableRowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = demo_table_row_finalize;
  object_class->get_property = demo_table_row_get_property;
  object_class->set_property = demo_table_row_set_property;

  properties [PROP_KEY] =
    g_param_spec_string ("key",
                         "Key",
                         "Key",
                         "",
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  properties [PROP_VALUE] =
    g_param_spec_string ("value",
                         "Value",
                         "Value",
                         "",
                         (G_PARAM_READWRITE |
                          G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
demo_table_row_init (DemoTableRow *self)
{
}

gchar *
demo_table_row_get_key (DemoTableRow *self)
{
  g_return_val_if_fail (DEMO_IS_TABLE_ROW (self), NULL);

  return self->key;
}

gchar *
demo_table_row_get_value (DemoTableRow *self)
{
  g_return_val_if_fail (DEMO_IS_TABLE_ROW (self), NULL);

  return self->value;
}
