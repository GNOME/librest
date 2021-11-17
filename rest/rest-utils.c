/* rest-utils.c
 *
 * Copyright 2021 Günther Wagner <info@gunibert.de>
 *
 * This file is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3 of the
 * License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 */

#include "rest-utils.h"

/**
 * random_string:
 * @length: the length of the random string
 *
 * Creates a random string from a given alphabeth with length @length
 *
 * Returns: (transfer full): a random string
 */
gchar *
random_string (guint length)
{
  g_autoptr(GRand) rand = g_rand_new ();
  gchar *buffer = g_slice_alloc0 (sizeof (gchar) * length + 1);
  gchar alphabeth[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~";

  for (guint i = 0; i < length; i++)
    {
      buffer[i] = alphabeth[g_rand_int (rand) % (sizeof (alphabeth) - 1)];
    }
  buffer[length] = '\0';

  return buffer;
}

