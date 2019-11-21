/* Copyright (C) 2009 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

#include "android/boot-properties.h"

#include "android/emulation/android_qemud.h"
#include "android/globals.h"
#include "android/utils/debug.h"
#include "android/utils/stream.h"
#include "android/utils/system.h"

#include <errno.h>
#include <stdio.h>

#define  D(...)  VERBOSE_PRINT(init,__VA_ARGS__)

/* define T_ACTIVE to 1 to debug transport communications */
#define  T_ACTIVE  0

#if T_ACTIVE
#define  T(...)  VERBOSE_PRINT(init,__VA_ARGS__)
#else
#define  T(...)   ((void)0)
#endif

/* this code supports the list of system properties that will
 * be set on boot in the emulated system.
 */

typedef struct BootProperty {
    struct BootProperty*  next;
    char*                 property;
    int                   length;
} BootProperty;

static BootProperty*
boot_property_alloc( const char*  name,  int  namelen,
                     const char*  value, int  valuelen )
{
    int            length = namelen + 1 + valuelen;
    BootProperty*  prop = android_alloc( sizeof(*prop) + length + 1 );
    char*          p;

    prop->next     = NULL;
    prop->property = p = (char*)(prop + 1);
    prop->length   = length;

    memcpy( p, name, namelen );
    p += namelen;
    *p++ = '=';
    memcpy( p, value, valuelen );
    p += valuelen;
    *p = '\0';

    return prop;
}

static BootProperty*   _boot_properties = NULL;
/* address to store pointer to next new list element */
static BootProperty**  _boot_properties_tail = &_boot_properties;
static int             _inited;

/* Clears all existing boot properties
 */
static void
boot_property_clear_all()
{
    /* free all elements of the linked list */
    BootProperty *p = _boot_properties;
    BootProperty *next = NULL;
    while (p) {
        next = p->next;
        AFREE(p);
        p = next;
    }

    /* reset list administration to initial state */
    _boot_properties = NULL;
    _boot_properties_tail = &_boot_properties;
}

/* Appends a new boot property to the end of the internal list.
 */
int
boot_property_add2( const char*  name, int  namelen,
                    const char*  value, int  valuelen )
{
    BootProperty*  prop;

    /* check the lengths
     */
    if (namelen > PROPERTY_MAX_NAME)
        return -1;

    if (valuelen > PROPERTY_MAX_VALUE)
        return -2;

    /* check that there are not invalid characters in the
     * property name
     */
    const char*  reject = " =$*?'\"";
    int          nn;

    for (nn = 0; nn < namelen; nn++) {
        if (strchr(reject, name[nn]) != NULL)
            return -3;
    }

    /* init the service */
    boot_property_init_service();

    D("Adding boot property: '%.*s' = '%.*s'",
      namelen, name, valuelen, value);

    /* add to the end of the internal list */
    prop = boot_property_alloc(name, namelen, value, valuelen);

    *_boot_properties_tail = prop;
    _boot_properties_tail  = &prop->next;

    return 0;
}


void boot_property_add_logcat_pipe(const char* logcat_filter) {
    if (!logcat_filter) {
        return;
    }
    char buf[1024];
    snprintf(buf, sizeof(buf),
            "androidboot.logcat=%s androidboot.consolepipe=qemu_pipe,pipe:logcat", logcat_filter);
    boot_property_add("qemu.cmdline", buf);
    boot_property_add("qemu.logcat", "start");
}

/* Prints the warning string corresponding to the error code returned by
 * boot_propery_add2().
 */
static void
boot_property_raise_warning( int ret, const char*  name, int  namelen,
                             const char*  value, int  valuelen )
{
    switch (ret) {
    case -1:
        dwarning("boot property name too long: '%.*s'",
                    namelen, name);
        break;
    case -2:
        dwarning("boot property value too long: '%.*s'",
                    valuelen, value);
        break;
    case -3:
        dwarning("boot property name contains invalid chars: %.*s",
                    namelen, name);
        break;
    }
}

int
boot_property_add( const char*  name, const char*  value )
{
    int  namelen = strlen(name);
    int  valuelen = strlen(value);

    return boot_property_add2(name, namelen, value, valuelen);
}

/* Saves a single BootProperty to file.
 */
static int boot_property_save_property(Stream* f, BootProperty* p) {
    /* split in key and value, so we can re-use boot_property_add (and its
     * sanity checks) when loading
     */

    char *split = strchr(p->property, '=');
    if (split == NULL) {
        D("%s: save failed: illegal key/value pair \"%s\" (missing '=')\n",
          __FUNCTION__, p->property);
        // qemu_file_set_error(f, -EINVAL);
        return -1;
    }

    *split = '\0';  /* p->property is now "<key>\0<value>\0" */

    uint32_t key_buf_len = (split - p->property) + 1; // +1: '\0' terminator
    stream_put_be32(f, key_buf_len);
    stream_write(f, (uint8_t*)p->property, key_buf_len);

    uint32_t value_buf_len = p->length - key_buf_len + 1; // +1: '\0' terminator
    stream_put_be32(f, value_buf_len);
    stream_write(f, (uint8_t*)split + 1, value_buf_len);

    *split = '=';  /* restore property to "<key>=<value>\0" */

    return 0;
}

/* Loads a single boot property from a snapshot file
 */
static int boot_property_load_property(Stream* f) {
    int ret;

    /* load key */
    uint32_t key_buf_len = stream_get_be32(f);
    char* key = android_alloc(key_buf_len);
    if ((ret = stream_read(f, (uint8_t*)key, key_buf_len) !=
               (int)key_buf_len)) {
        D("%s: key load failed: expected %d bytes, got %d\n",
          __FUNCTION__, key_buf_len, ret);
        goto fail_key;
    }

    /* load value */
    uint32_t value_buf_len = stream_get_be32(f);
    char* value = android_alloc(value_buf_len);
    if ((ret = stream_read(f, (uint8_t*)value, value_buf_len) !=
               (int)value_buf_len)) {
        D("%s: value load failed: expected %d bytes, got %d\n",
          __FUNCTION__, value_buf_len, ret);
        goto fail_value;
    }

    /* add the property */
    ret = boot_property_add2(key, key_buf_len - 1, value, value_buf_len - 1);
    if (ret < 0) {
        D("%s: load failed: cannot add boot property (details follow)\n",
          __FUNCTION__);
        boot_property_raise_warning(ret, key, key_buf_len - 1, value, value_buf_len - 1);
        goto fail_value;
    }

    return 0;

    /* in case of errors, clean up before return */
    fail_value:
        AFREE(value);
    fail_key:
        AFREE(key);
        return -EIO;
}

/* Saves the number of available boot properties to file
 */
static void boot_property_save_count(Stream* f, BootProperty* p) {
    uint32_t property_count = 0;
    for (; p; p = p->next) {
        property_count++;
    }

    stream_put_be32(f, property_count);
}

/* Saves all available boot properties to snapshot.
 */
static void boot_property_save(Stream* f, QemudService* service, void* opaque) {
    boot_property_save_count(f, _boot_properties);

    BootProperty *p = _boot_properties;
    for ( ; p; p = p->next) {
        if (boot_property_save_property(f, p)) {
            break;  /* abort on error */
        }
    }
}

/* Replaces the currently available boot properties by those stored
 * in a snapshot.
 */
static int boot_property_load(Stream* f, QemudService* service, void* opaque) {
    int ret;

    /* remove properties from old run */
    boot_property_clear_all();

    /* load properties from snapshot */
    uint32_t i, property_count = stream_get_be32(f);
    for (i = 0; i < property_count; i++) {
        if ((ret = boot_property_load_property(f))) {
            return ret;
        }
    }

    return 0;
}

#define SERVICE_NAME  "boot-properties"

static void
boot_property_client_recv( void*         opaque,
                           uint8_t*      msg,
                           int           msglen,
                           QemudClient*  client )
{
    /* the 'list' command shall send all boot properties
     * to the client, then close the connection.
     */
    if (msglen == 4 && !memcmp(msg, "list", 4)) {
        BootProperty*  prop;
        for (prop = _boot_properties; prop != NULL; prop = prop->next) {
            qemud_client_send(client, (uint8_t*)prop->property, prop->length);
        }

        /* Send a NUL to signal the end of the list. */
        qemud_client_send(client, (uint8_t*)"", 1);

        return;
    }

    /* unknown command ? */
    D("%s: ignoring unknown command: %.*s", __FUNCTION__, msglen, msg);
}

static QemudClient*
boot_property_service_connect( void*          opaque,
                               QemudService*  serv,
                               int            channel,
                               const char*    client_param )
{
    QemudClient*  client;

    client = qemud_client_new( serv, channel, client_param, NULL,
                               boot_property_client_recv,
                               NULL, NULL, NULL );

    qemud_client_set_framing(client, 1);
    return client;
}


void
boot_property_init_service( void )
{
    if (!_inited) {
        QemudService*  serv = qemud_service_register( SERVICE_NAME,
                                                      1, NULL,
                                                      boot_property_service_connect,
                                                      boot_property_save,
                                                      boot_property_load);
        if (serv == NULL) {
            derror("could not register '%s' service", SERVICE_NAME);
            return;
        }
        D("registered '%s' qemud service", SERVICE_NAME);

        _inited = 1;
    }
}



void
boot_property_parse_option( const char*  param )
{
    char* q = strchr(param,'=');
    const char* name;
    const char* value;
    int   namelen, valuelen, ret;

    if (q == NULL) {
        dwarning("boot property missing (=) separator: %s", param);
        return;
    }

    name    = param;
    namelen = q - param;

    value    = q+1;
    valuelen = strlen(name) - (namelen+1);

    ret = boot_property_add2(name, namelen, value, valuelen);
    if (ret < 0) {
        boot_property_raise_warning(ret, name, namelen, value, valuelen);
    }
}
