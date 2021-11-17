/* test-server.h
 *
 * Copyright 2021 Günther Wagner <info@gunibert.de>
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

#include <glib.h>
#include <libsoup/soup.h>

G_BEGIN_DECLS

SoupServer *test_server_new           (void);
void        test_server_run_in_thread (SoupServer *server);
gchar      *test_server_get_uri       (SoupServer *server,
                                       const char *scheme,
                                       const char *host);

G_END_DECLS
