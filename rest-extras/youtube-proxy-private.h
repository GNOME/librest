/*
 * librest - RESTful web services access
 * Copyright (c) 2011 Collabora Ltd.
 *
 * Authors: Eitan Isaacson <eitan.isaacson@collabora.co.uk>
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

#include "youtube-proxy.h"

#define YOUTUBE_PROXY_GET_PRIVATE(o) \
  (G_TYPE_INSTANCE_GET_PRIVATE ((o), YOUTUBE_TYPE_PROXY, YoutubeProxyPrivate))

struct _YoutubeProxyPrivate {
  char *developer_key;
  char *user_auth;
};

