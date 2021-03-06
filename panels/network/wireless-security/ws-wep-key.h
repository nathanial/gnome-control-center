/* -*- Mode: C; tab-width: 4; indent-tabs-mode: t; c-basic-offset: 4 -*- */
/* NetworkManager Applet -- allow user control over networking
 *
 * Dan Williams <dcbw@redhat.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright 2007 - 2014 Red Hat, Inc.
 */

#ifndef WS_WEP_KEY_H
#define WS_WEP_KEY_H

#if defined (LIBNM_BUILD)
#include <NetworkManager.h>
#elif defined (LIBNM_GLIB_BUILD)
#include <nm-setting-wireless-security.h>
#else
#error neither LIBNM_BUILD nor LIBNM_GLIB_BUILD defined
#endif

typedef struct _WirelessSecurityWEPKey WirelessSecurityWEPKey;

WirelessSecurityWEPKey *ws_wep_key_new (NMConnection *connection,
                                        NMWepKeyType type,
                                        gboolean adhoc_create,
                                        gboolean secrets_only);

#endif /* WS_WEP_KEY_H */

