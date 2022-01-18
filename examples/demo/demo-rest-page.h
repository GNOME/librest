/* demo-rest-page.h
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

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DEMO_TYPE_REST_PAGE (demo_rest_page_get_type())

G_DECLARE_FINAL_TYPE (DemoRestPage, demo_rest_page, DEMO, REST_PAGE, GtkBox)

DemoRestPage *demo_rest_page_new (void);
GtkWidget *demo_rest_page_get_entry (DemoRestPage *self);

G_END_DECLS
