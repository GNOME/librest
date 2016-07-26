/*
 * librest - RESTful web services access
 * Copyright (c) 2009 Intel Corporation.
 *
 * Authors: Timm Bäder <mail@baedert.org>
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
#ifndef __TEST_SERVER_H
#define __TEST_SERVER_H

#include <libsoup/soup.h>

typedef struct {
  SoupServer *server;
  GMainLoop  *main_loop;
  char *url;
  GThread *thread;
} TestServer;

void        test_server_stop   (TestServer *server);
void        test_server_run    (TestServer *server);
TestServer *test_server_create (SoupServerCallback server_cb);

#endif

