/* rest-utils.c
 *
 * Copyright 2021-2022 Günther Wagner <info@gunibert.de>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 2.1, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
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
  gchar *buffer = g_malloc0 (sizeof (gchar) * length + 1);
  gchar alphabeth[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~";

  for (guint i = 0; i < length; i++)
    {
      buffer[i] = alphabeth[g_rand_int (rand) % (sizeof (alphabeth) - 1)];
    }
  buffer[length] = '\0';

  return buffer;
}

