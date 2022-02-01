/* demo-table-row.h
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

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define DEMO_TYPE_TABLE_ROW (demo_table_row_get_type())

G_DECLARE_FINAL_TYPE (DemoTableRow, demo_table_row, DEMO, TABLE_ROW, GObject)

DemoTableRow *demo_table_row_new       (void);
gchar        *demo_table_row_get_key   (DemoTableRow *self);
gchar        *demo_table_row_get_value (DemoTableRow *self);

G_END_DECLS
