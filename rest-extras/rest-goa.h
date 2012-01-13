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

#ifndef _REST_GOA
#define _REST_GOA

#define GOA_API_IS_SUBJECT_TO_CHANGE

#include <glib-object.h>
#include <goa/goa.h>
#include <rest/rest-proxy.h>

G_BEGIN_DECLS

/*
 * Very incomplete and experimental APIs to integrate librest with
 * gnome-online-accounts.  Much design work needed!
 */

/*
 * Create a new RestProxy based on a specific GoaObject. *authenticated is set
 * to TRUE if the account has valid credentials.
 *
 * Currently only accounts that are OAuth-based are handled.
 */
RestProxy *rest_goa_proxy_from_account (GoaObject *object,
                                        const char *url_format,
                                        gboolean binding_required,
                                        gboolean *authenticated);

G_END_DECLS

#endif /* _REST_GOA */

