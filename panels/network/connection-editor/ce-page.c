/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2012 Red Hat, Inc
 *
 * Licensed under the GNU General Public License Version 2
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <string.h>

#include <net/if_arp.h>
#include <netinet/ether.h>

#include <nm-utils.h>

#include "ce-page.h"


G_DEFINE_ABSTRACT_TYPE (CEPage, ce_page, G_TYPE_OBJECT)

enum {
        PROP_0,
        PROP_CONNECTION,
        PROP_INITIALIZED,
};

enum {
        CHANGED,
        INITIALIZED,
        LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

gboolean
ce_page_validate (CEPage *page, NMConnection *connection, GError **error)
{
        g_return_val_if_fail (CE_IS_PAGE (page), FALSE);
        g_return_val_if_fail (NM_IS_CONNECTION (connection), FALSE);

        if (CE_PAGE_GET_CLASS (page)->validate)
                return CE_PAGE_GET_CLASS (page)->validate (page, connection, error);

        return TRUE;
}

static void
dispose (GObject *object)
{
        CEPage *self = CE_PAGE (object);

        g_clear_object (&self->page);
        g_clear_object (&self->builder);
        g_clear_object (&self->connection);

        G_OBJECT_CLASS (ce_page_parent_class)->dispose (object);
}

static void
finalize (GObject *object)
{
        CEPage *self = CE_PAGE (object);

        g_free (self->title);

        G_OBJECT_CLASS (ce_page_parent_class)->finalize (object);
}

GtkWidget *
ce_page_get_page (CEPage *self)
{
        g_return_val_if_fail (CE_IS_PAGE (self), NULL);

        return self->page;
}

const char *
ce_page_get_title (CEPage *self)
{
        g_return_val_if_fail (CE_IS_PAGE (self), NULL);

        return self->title;
}

gboolean
ce_page_get_initialized (CEPage *self)
{
        g_return_val_if_fail (CE_IS_PAGE (self), FALSE);

        return self->initialized;
}

void
ce_page_changed (CEPage *self)
{
        g_return_if_fail (CE_IS_PAGE (self));

        g_signal_emit (self, signals[CHANGED], 0);
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
        CEPage *self = CE_PAGE (object);

        switch (prop_id) {
        case PROP_CONNECTION:
                g_value_set_object (value, self->connection);
                break;
        case PROP_INITIALIZED:
                g_value_set_boolean (value, self->initialized);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
        CEPage *self = CE_PAGE (object);

        switch (prop_id) {
        case PROP_CONNECTION:
                if (self->connection)
                        g_object_unref (self->connection);
                self->connection = g_value_dup_object (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
ce_page_init (CEPage *self)
{
        self->builder = gtk_builder_new ();
}

static void
ce_page_class_init (CEPageClass *page_class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (page_class);

        /* virtual methods */
        object_class->dispose      = dispose;
        object_class->finalize     = finalize;
        object_class->get_property = get_property;
        object_class->set_property = set_property;

        /* Properties */
        g_object_class_install_property
                (object_class, PROP_CONNECTION,
                 g_param_spec_object ("connection",
                                      "Connection",
                                      "Connection",
                                      NM_TYPE_CONNECTION,
                                      G_PARAM_READABLE | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

        g_object_class_install_property
                (object_class, PROP_INITIALIZED,
                 g_param_spec_boolean ("initialized",
                                       "Initialized",
                                       "Initialized",
                                       FALSE,
                                       G_PARAM_READABLE));

        signals[CHANGED] =
                g_signal_new ("changed",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (CEPageClass, changed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);

        signals[INITIALIZED] =
                g_signal_new ("initialized",
                              G_OBJECT_CLASS_TYPE (object_class),
                              G_SIGNAL_RUN_FIRST,
                              G_STRUCT_OFFSET (CEPageClass, initialized),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__POINTER,
                              G_TYPE_NONE, 1, G_TYPE_POINTER);
}

CEPage *
ce_page_new (GType             type,
             NMConnection     *connection,
             NMClient         *client,
             NMRemoteSettings *settings,
             const gchar      *ui_file,
             const gchar      *title)
{
        CEPage *page;
        GError *error = NULL;

        page = CE_PAGE (g_object_new (type,
                                      "connection", connection,
                                      NULL));
        page->title = g_strdup (title);
        page->client = client;
        page->settings= settings;
        if (!gtk_builder_add_from_file (page->builder, ui_file, &error)) {
                g_warning ("Couldn't load builder file: %s", error->message);
                g_error_free (error);
                g_object_unref (page);
                return NULL;
        }
        page->page = GTK_WIDGET (gtk_builder_get_object (page->builder, "page"));
        if (!page->page) {
                g_warning ("Couldn't load page widget from %s", ui_file);
                g_object_unref (page);
                return NULL;
        }

        g_object_ref_sink (page->page);

        return page;
}

static void
emit_initialized (CEPage *page,
                  GError *error)
{
        page->initialized = TRUE;
        g_signal_emit (page, signals[INITIALIZED], 0, error);
}

void
ce_page_complete_init (CEPage      *page,
                       const gchar *setting_name,
                       GHashTable  *secrets,
                       GError      *error)
{
        GHashTable *setting_hash;
        GError *update_error = NULL;

        if (error
            && !dbus_g_error_has_name (error, "org.freedesktop.NetworkManager.Settings.InvalidSetting")
            && !dbus_g_error_has_name (error, "org.freedesktop.NetworkManager.AgentManager.NoSecrets")) {
                emit_initialized (page, error);
                return;
        } else if (!setting_name || !secrets || !g_hash_table_size (secrets)) {
                /* Success, no secrets */
                emit_initialized (page, NULL);
                return;
        }

        setting_hash = g_hash_table_lookup (secrets, setting_name);
        if (!setting_hash) {
                /* Success, no secrets */
                emit_initialized (page, NULL);
                return;
        }

        if (nm_connection_update_secrets (page->connection,
                                          setting_name,
                                          secrets,
                                          &update_error)) {
                emit_initialized (page, NULL);
                return;
        }

        if (!update_error) {
                g_set_error_literal (&update_error, NM_CONNECTION_ERROR, NM_CONNECTION_ERROR_UNKNOWN,
                                     "Failed to update connection secrets due to an unknown error.");
        }

        emit_initialized (page, update_error);
        g_clear_error (&update_error);
}

gchar **
ce_page_get_mac_list (NMClient    *client,
                      GType        device_type,
                      const gchar *mac_property)
{
        const GPtrArray *devices;
        GPtrArray *macs;
        int i;

        macs = g_ptr_array_new ();
        devices = nm_client_get_devices (client);
        for (i = 0; devices && (i < devices->len); i++) {
                NMDevice *dev = g_ptr_array_index (devices, i);
                const char *iface;
                char *mac, *item;

                if (!G_TYPE_CHECK_INSTANCE_TYPE (dev, device_type))
                        continue;

                g_object_get (G_OBJECT (dev), mac_property, &mac, NULL);
                iface = nm_device_get_iface (NM_DEVICE (dev));
                item = g_strdup_printf ("%s (%s)", mac, iface);
                g_free (mac);
                g_ptr_array_add (macs, item);
        }

        g_ptr_array_add (macs, NULL);
        return (char **)g_ptr_array_free (macs, FALSE);
}

void
ce_page_setup_mac_combo (GtkComboBoxText  *combo,
                         const gchar      *current_mac,
                         gchar           **mac_list)
{
        gchar **m, *active_mac = NULL;
        gint current_mac_len;
        GtkWidget *entry;

        if (current_mac)
                current_mac_len = strlen (current_mac);
        else
                current_mac_len = -1;

        for (m= mac_list; m && *m; m++) {
                gtk_combo_box_text_append_text (combo, *m);
                if (current_mac &&
                    g_ascii_strncasecmp (*m, current_mac, current_mac_len) == 0
                    && ((*m)[current_mac_len] == '\0' || (*m)[current_mac_len] == ' '))
                        active_mac = *m;
        }

        if (current_mac) {
                if (!active_mac) {
                        gtk_combo_box_text_prepend_text (combo, current_mac);
                }

                entry = gtk_bin_get_child (GTK_BIN (combo));
                if (entry)
                        gtk_entry_set_text (GTK_ENTRY (entry), active_mac ? active_mac : current_mac);
        }
}

void
ce_page_mac_to_entry (const GByteArray *mac,
                      gint              type,
                      GtkEntry         *entry)
{
        char *str_addr;

        g_return_if_fail (entry != NULL);
        g_return_if_fail (GTK_IS_ENTRY (entry));

        if (!mac || !mac->len)
                return;

        if (mac->len != nm_utils_hwaddr_len (type))
                return;

        str_addr = nm_utils_hwaddr_ntoa (mac->data, type);
        gtk_entry_set_text (entry, str_addr);
        g_free (str_addr);
}

static gboolean
utils_ether_addr_valid (const struct ether_addr *test_addr)
{
        guint8 invalid_addr1[ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
        guint8 invalid_addr2[ETH_ALEN] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        guint8 invalid_addr3[ETH_ALEN] = {0x44, 0x44, 0x44, 0x44, 0x44, 0x44};
        guint8 invalid_addr4[ETH_ALEN] = {0x00, 0x30, 0xb4, 0x00, 0x00, 0x00}; /* prism54 dummy MAC */

        g_return_val_if_fail (test_addr != NULL, FALSE);

        /* Compare the AP address the card has with invalid ethernet MAC addresses. */
        if (!memcmp (test_addr->ether_addr_octet, &invalid_addr1, ETH_ALEN))
                return FALSE;

        if (!memcmp (test_addr->ether_addr_octet, &invalid_addr2, ETH_ALEN))
                return FALSE;

        if (!memcmp (test_addr->ether_addr_octet, &invalid_addr3, ETH_ALEN))
                return FALSE;
        if (!memcmp (test_addr->ether_addr_octet, &invalid_addr4, ETH_ALEN))
                return FALSE;

        if (test_addr->ether_addr_octet[0] & 1)  /* Multicast addresses */
                return FALSE;
        
        return TRUE;
}

GByteArray *
ce_page_entry_to_mac (GtkEntry *entry,
                      gint      type,
                      gboolean *invalid)
{
        const char *temp, *sp;
        char *buf = NULL;
        GByteArray *mac;

        g_return_val_if_fail (entry != NULL, NULL);
        g_return_val_if_fail (GTK_IS_ENTRY (entry), NULL);

        if (invalid)
                *invalid = FALSE;

        temp = gtk_entry_get_text (entry);
        if (!temp || !strlen (temp))
                return NULL;

        sp = strchr (temp, ' ');
        if (sp)
                temp = buf = g_strndup (temp, sp - temp);

        mac = nm_utils_hwaddr_atoba (temp, type);
        g_free (buf);
        if (!mac) {
                if (invalid)
                        *invalid = TRUE;
                return NULL;
        }

        if (type == ARPHRD_ETHER && !utils_ether_addr_valid ((struct ether_addr *)mac->data)) {
                g_byte_array_free (mac, TRUE);
                if (invalid)
                        *invalid = TRUE;
                return NULL;
        }

        return mac;
}

const gchar *
ce_page_get_security_setting (CEPage *page)
{
        return page->security_setting;
}