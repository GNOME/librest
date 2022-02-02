/*
 * librest - RESTful web services access
 * Copyright (c) 2008, 2009, Intel Corporation.
 *
 * Authors: Rob Bradford <rob@linux.intel.com>
 *          Ross Burton <ross@linux.intel.com>
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
 *
 */

#pragma once

#include <glib-object.h>
#include "rest-param.h"

G_BEGIN_DECLS

#define REST_TYPE_PARAMS (rest_params_get_type ())

typedef struct _RestParams RestParams;
typedef struct _RestParamsIter RestParamsIter;

struct _RestParams
{
  /*< private >*/
  guint ref_count;

  GList *params;
};

struct _RestParamsIter
{
  /*< private >*/
  RestParams *params;
  gint position;
};

GType           rest_params_get_type (void) G_GNUC_CONST;
RestParams *rest_params_new                  (void);
RestParams *rest_params_copy                 (RestParams      *self);
RestParams *rest_params_ref                  (RestParams      *self);
void        rest_params_unref                (RestParams      *self);
void        rest_params_add                  (RestParams      *params,
                                              RestParam       *param);
RestParam  *rest_params_get                  (RestParams      *params,
                                              const char      *name);
void        rest_params_remove               (RestParams      *params,
                                              const char      *name);
gboolean    rest_params_are_strings          (RestParams      *params);
GHashTable *rest_params_as_string_hash_table (RestParams      *self);
void        rest_params_iter_init            (RestParamsIter  *iter,
                                              RestParams      *params);
gboolean    rest_params_iter_next            (RestParamsIter  *iter,
                                              const char     **name,
                                              RestParam      **param);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (RestParams, rest_params_unref)

G_END_DECLS
