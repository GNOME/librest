/* rest-pkce-code-challenge.c
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

#include "rest-pkce-code-challenge.h"
#include "rest-utils.h"

G_DEFINE_BOXED_TYPE (RestPkceCodeChallenge, rest_pkce_code_challenge, rest_pkce_code_challenge_copy, rest_pkce_code_challenge_free)

struct _RestPkceCodeChallenge
{
  gchar *code_verifier;
  gchar *code_challenge;
};

/**
 * rest_pkce_code_challenge_new_random:
 *
 * Creates a new #RestPkceCodeChallenge.
 *
 * Returns: (transfer full): A newly created #RestPkceCodeChallenge
 */
RestPkceCodeChallenge *
rest_pkce_code_challenge_new_random (void)
{
  RestPkceCodeChallenge *self;
  gint length = g_random_int_range (43, 128);
  gsize digest_len = 200;
  guchar code_verifier_sha256[200];
  GChecksum *sha256 = g_checksum_new (G_CHECKSUM_SHA256);

  self = g_slice_new0 (RestPkceCodeChallenge);
  self->code_verifier = random_string (length);
  g_checksum_update (sha256, (guchar *)self->code_verifier, -1);
  g_checksum_get_digest (sha256, (guchar *)&code_verifier_sha256, &digest_len);

  self->code_challenge = g_base64_encode (code_verifier_sha256, digest_len);
  g_strdelimit (self->code_challenge, "=", '\0');
  g_strdelimit (self->code_challenge, "+", '-');
  g_strdelimit (self->code_challenge, "/", '_');

  return self;
}

/**
 * rest_pkce_code_challenge_copy:
 * @self: a #RestPkceCodeChallenge
 *
 * Makes a deep copy of a #RestPkceCodeChallenge.
 *
 * Returns: (transfer full): A newly created #RestPkceCodeChallenge with the same
 *   contents as @self
 */
RestPkceCodeChallenge *
rest_pkce_code_challenge_copy (RestPkceCodeChallenge *self)
{
  RestPkceCodeChallenge *copy;

  g_return_val_if_fail (self, NULL);

  copy = g_slice_new0 (RestPkceCodeChallenge);
  copy->code_verifier = self->code_verifier;
  copy->code_challenge = self->code_challenge;

  return copy;
}

/**
 * rest_pkce_code_challenge_free:
 * @self: a #RestPkceCodeChallenge
 *
 * Frees a #RestPkceCodeChallenge allocated using rest_pkce_code_challenge_new()
 * or rest_pkce_code_challenge_copy().
 */
void
rest_pkce_code_challenge_free (RestPkceCodeChallenge *self)
{
  g_return_if_fail (self);

  g_slice_free (RestPkceCodeChallenge, self);
}

const gchar *
rest_pkce_code_challenge_get_challenge (RestPkceCodeChallenge *self)
{
  return self->code_challenge;
}

const gchar *
rest_pkce_code_challenge_get_verifier (RestPkceCodeChallenge *self)
{
  return self->code_verifier;
}
