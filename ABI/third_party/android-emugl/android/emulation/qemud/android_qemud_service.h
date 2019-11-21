// Copyright 2015 The Android Open Source Project
//
// This software is licensed under the terms of the GNU General Public
// License version 2, as published by the Free Software Foundation, and
// may be copied, distributed, and modified under those terms.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#pragma once

#include "android/emulation/android_qemud.h"
#include "android/emulation/qemud/android_qemud_client.h"
#include "android/utils/compiler.h"

ANDROID_BEGIN_HEADER

/* A QemudService models a _named_ service facility implemented
 * by the emulator, that clients in the emulated system can connect
 * to.
 *
 * Each service can have a limit on the number of clients they
 * accept (this number if unlimited if 'max_clients' is 0).
 *
 * Each service maintains a list of active QemudClients and
 * can also be used to create new QemudClient objects through
 * its 'serv_opaque' and 'serv_connect' fields.
 */

struct QemudService {
    const char* name;
    int max_clients;
    int num_clients;
    QemudClient* clients;
    QemudServiceConnect serv_connect;
    QemudServiceSave serv_save;
    QemudServiceLoad serv_load;
    void* serv_opaque;
    QemudService* next;
};

/* used internally to populate a QemudService object with a
 * new QemudClient */
extern void qemud_service_add_client(QemudService* s, QemudClient* c);

/* used internally to remove a QemudClient from a QemudService */
extern void qemud_service_remove_client(QemudService* s, QemudClient* c);

/* ask the service to create a new QemudClient. Note that we
 * assume that this calls qemud_client_new() which will add
 * the client to the service's list automatically.
 *
 * returns the client or NULL if an error occurred
 */
extern QemudClient* qemud_service_connect_client(QemudService* sv,
                             int channel_id,
                             const char* client_param);

/* find a registered service by name.
 */
extern QemudService* qemud_service_find(QemudService* service_list, const char* service_name);

/* Save the name of the given service.
 */
extern void qemud_service_save_name(Stream* f, QemudService* s);

/* Load the name of a service. Returns a pointer to the loaded name, or NULL
 * on failure.
 */
extern char* qemud_service_load_name(Stream* f);

/* Saves state of a service.
 */
extern void qemud_service_save(Stream* f, QemudService* s);

/* Loads service state from file, then updates the currently running instance
 * of that service to mirror the loaded state. If the service is not running,
 * the load process is aborted.
 *
 * Parameter 'current_services' should be the list of active services.
 */
extern int qemud_service_load(Stream* f, QemudService* current_services);

ANDROID_END_HEADER
