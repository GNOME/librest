/* rest-pkce-code-challenge.h
 *
 * Copyright 2021 Günther Wagner <info@gunibert.de>
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

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define REST_TYPE_PKCE_CODE_CHALLENGE (rest_pkce_code_challenge_get_type ())

/**
 * RestPkceCodeChallenge:
 *
 * In order to play a Pkce Code Verification during a OAuth2 authorization
 * you need this structure which handles the algorithmic part.
 */
typedef struct _RestPkceCodeChallenge RestPkceCodeChallenge;

GType                  rest_pkce_code_challenge_get_type      (void) G_GNUC_CONST;
RestPkceCodeChallenge *rest_pkce_code_challenge_new_random    (void);
RestPkceCodeChallenge *rest_pkce_code_challenge_copy          (RestPkceCodeChallenge *self);
void                   rest_pkce_code_challenge_free          (RestPkceCodeChallenge *self);
const gchar           *rest_pkce_code_challenge_get_challenge (RestPkceCodeChallenge *self);
const gchar           *rest_pkce_code_challenge_get_verifier  (RestPkceCodeChallenge *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RestPkceCodeChallenge, rest_pkce_code_challenge_free)

G_END_DECLS
