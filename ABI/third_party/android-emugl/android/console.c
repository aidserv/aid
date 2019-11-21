/* Copyright (C) 2007-2017 The Android Open Source Project
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
/*
 *  Android emulator control console
 *
 *  this console is enabled automatically at emulator startup, on port 5554 by default,
 *  unless some other emulator is already running. See (android_emulation_start in android_sdl.c
 *  for details)
 *
 *  you can telnet to the console, then use commands like 'help' or others to dynamically
 *  change emulator settings.
 *
 */

#include "android/console.h"

#include "android/android.h"
#include "android/cmdline-option.h"
#include "android/console_auth.h"
#include "android/crashreport/crash-handler.h"
#include "android/globals.h"
#include "android/hw-events.h"
#include "android/hw-sensors.h"
#include "android/network/control.h"
#include "android/network/constants.h"
#include "android/network/globals.h"
#include "android/shaper.h"
#include "android/tcpdump.h"
#include "android/telephony/modem_driver.h"
#include "android/utils/bufprint.h"
#include "android/utils/debug.h"
#include "android/utils/eintr_wrapper.h"
#include "android/utils/http_utils.h"
#include "android/utils/ipaddr.h"
#include "android/utils/looper.h"
#include "android/utils/sockets.h"
#include "android/utils/stralloc.h"
#include "android/utils/utf8_utils.h"

#include "config-host.h"

#include <assert.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define  DEBUG  1

#if 1
#  define  D_ACTIVE   VERBOSE_CHECK(console)
#else
#  define  D_ACTIVE   DEBUG
#endif

#if DEBUG
#  define  D(x)   do { if (D_ACTIVE) ( printf x , fflush(stdout) ); } while (0)
#else
#  define  D(x)   do{}while(0)
#endif

#define DINIT(...) do {  if (VERBOSE_CHECK(init)) dprint(__VA_ARGS__); } while (0)

typedef struct ControlGlobalRec_*  ControlGlobal;

typedef struct ControlClientRec_*  ControlClient;

typedef struct {
    int           host_port;
    int           host_udp;
    unsigned int  guest_ip;
    int           guest_port;
} RedirRec, *Redir;


typedef int Socket;
typedef const struct CommandDefRec_* CommandDef;

typedef struct ControlClientRec_
{
    struct ControlClientRec_*  next;       /* next client in list           */
    Socket                     sock;       /* socket used for communication */
    // The loopIo currently communicating over |sock|. May change over the
    // lifetime of a ControlClient.
    LoopIo* loopIo;
    ControlGlobal              global;
    char                       finished;
    char                       buff[ 4096 ];
    int                        buff_len;
    CommandDef                 commands;
} ControlClientRec;


typedef struct ControlGlobalRec_
{
    // Interfaces to call into QEMU specific code.
#define ANDROID_CONSOLE_DEFINE_FIELD(type, name) type name ## _agent [1];
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_CONSOLE_DEFINE_FIELD)

    /* IO */
    Looper* looper;
    LoopIo* listen4_loopio;
    LoopIo* listen6_loopio;

    /* the list of current clients */
    ControlClient   clients;

    /* the list of redirections currently active */
    Redir     redirs;
    int       num_redirs;
    int       max_redirs;

} ControlGlobalRec;

static inline const QAndroidVmOperations* vmopers(ControlClient client) {
    return client->global->vm_agent;
}

//////////// Module level globals //////////////////////////
static ControlGlobalRec _g_global;

static int
control_global_add_redir( ControlGlobal  global,
                          int            host_port,
                          int            host_udp,
                          unsigned int   guest_ip,
                          int            guest_port )
{
    Redir  redir;

    if (global->num_redirs >= global->max_redirs)
    {
        int  old_max = global->max_redirs;
        int  new_max = old_max + (old_max >> 1) + 4;

        Redir  new_redirs = realloc( global->redirs, new_max*sizeof(global->redirs[0]) );
        if (new_redirs == NULL)
            return -1;

        global->redirs     = new_redirs;
        global->max_redirs = new_max;
    }

    redir = &global->redirs[ global->num_redirs++ ];

    redir->host_port  = host_port;
    redir->host_udp   = host_udp;
    redir->guest_ip   = guest_ip;
    redir->guest_port = guest_port;

    return 0;
}

static int
control_global_del_redir( ControlGlobal  global,
                          int            host_port,
                          int            host_udp )
{
    int  nn;

    for (nn = 0; nn < global->num_redirs; nn++)
    {
        Redir  redir = &global->redirs[nn];

        if ( redir->host_port == host_port &&
             redir->host_udp  == host_udp  )
        {
            memmove( redir, redir + 1, ((global->num_redirs - nn)-1)*sizeof(*redir) );
            global->num_redirs -= 1;
            return 0;
        }
    }
    /* we didn't find it */
    return -1;
}

/* Detach the socket descriptor from a given ControlClient
 * and return its value. This is useful either when destroying
 * the client, or redirecting the socket to another service.
 *
 * NOTE: this does not close the socket.
 */
static int
control_client_detach( ControlClient  client )
{
    int  result;

    if (client->sock < 0)
        return -1;

    loopIo_free(client->loopIo);
    client->loopIo = NULL;
    result = client->sock;
    client->sock = -1;

    return result;
}

static void
control_client_destroy( ControlClient  client )
{
    ControlGlobal  global = client->global;
    ControlClient  *pnode = &global->clients;
    int            sock;

    D(( "destroying control client %p\n", client ));

    sock = control_client_detach( client );
    if (sock >= 0)
        socket_close(sock);

    for ( ;; ) {
        ControlClient  node = *pnode;
        if ( node == NULL )
            break;
        if ( node == client ) {
            *pnode     = node->next;
            node->next = NULL;
            break;
        }
        pnode = &node->next;
    }

    free( client );
}

// /////////////////////////////////////////////////////////////////////////////
// Different write callbacks used to report back to the client.
static bool send_chunk(int sock, const char* buff, int len) {
    while (len > 0) {
        int ret = HANDLE_EINTR(socket_send(sock, buff, len));
        if (ret < 0) {
            if (errno != EWOULDBLOCK && errno != EAGAIN) {
                return false;
            }
        } else {
            buff += ret;
            len  -= ret;
        }
    }
    return true;
}

static void control_control_write(ControlClient client, const char* buff, int len)
{
    if (len < 0)
        len = strlen(buff);

    // Terminal requires an explicit \r\n symbol pair for newlines, and most of
    // the clients use \n alone. Make sure we insert missing \r characters
    // before each \n while sending.
    int offset = 0;
    for (;;) {
        const char* pos = buff + offset;
        const char* newline = memchr(pos, '\n', len - offset);
        if (!newline) {
            // Done, send the rest as a single chunk.
            send_chunk(client->sock, buff + offset, len - offset);
            break;
        }

        if (newline == buff || // The very first character is '\n'.
                newline[-1] != '\r') { // Previous character is not '\r'.
            if (!send_chunk(client->sock, pos, newline - pos) ||
                !send_chunk(client->sock, "\r\n", 2)) {
                break;
            }
        } else {
            // The correct '\r\n' is there.
            if (!send_chunk(client->sock, pos, newline - pos + 1)) {
                break;
            }
        }
        offset += newline - pos + 1;
    }
}

static int  control_vwrite( ControlClient  client, const char*  format, va_list args )
{
    static char  temp[1024];
    int ret = vsnprintf( temp, sizeof(temp), format, args );
    temp[ sizeof(temp)-1 ] = 0;
    control_control_write( client, temp, -1 );

    return ret;
}

static int  control_write( ControlClient  client, const char*  format, ... )
{
    int ret;
    va_list      args;
    va_start(args, format);
    ret = control_vwrite(client, format, args);
    va_end(args);

    return ret;
}

static int control_write_out_cb(void* opaque, const char* str, int strsize) {
    ControlClient client = opaque;
    control_control_write(client, str, strsize);
    return strsize;
}

static int control_write_err_cb(void* opaque, const char* str, int strsize) {
    int ret = 0;
    ControlClient client = opaque;
    ret += control_write(client, "KO: ");
    control_control_write(client, str, strsize);
    return ret + strsize;
}

// forward declaration
static void control_client_read(void* opaque, int fd, unsigned events);

typedef const struct CommandDefRec_* CommandDef;

typedef struct CommandDefRec_ {
    const char* names;
    const char* abstract;
    const char* description;
    void (*descriptor)(ControlClient client);
    int (*handler)(ControlClient client, char* args);
    CommandDef subcommands; /* if handler is NULL */

} CommandDefRec;

static const CommandDefRec main_commands[];         /* forward */
static const CommandDefRec main_commands_preauth[]; /* forward */

static ControlClient
control_client_create( Socket         socket,
                       ControlGlobal  global )
{
    ControlClient  client = calloc( sizeof(*client), 1 );

    if (client) {
        socket_set_nodelay( socket );
        socket_set_nonblock( socket );
        client->commands = main_commands_preauth;
        client->finished = 0;
        client->global  = global;
        client->sock    = socket;
        client->loopIo =
                loopIo_new(global->looper, socket, control_client_read, client);
        client->next    = global->clients;
        global->clients = client;

        loopIo_wantRead(client->loopIo);
        loopIo_dontWantWrite(client->loopIo);
    }
    return client;
}

static CommandDef
find_command( char*  input, CommandDef  commands, char*  *pend, char*  *pargs )
{
    int    nn;
    char*  args = strchr(input, ' ');

    if (args != NULL) {
        while (*args == ' ')
            args++;

        if (args[0] == 0)
            args = NULL;
    }

    for (nn = 0; commands[nn].names != NULL; nn++)
    {
        const char*  name = commands[nn].names;
        const char*  sep;

        do {
            int  len, c;

            sep = strchr( name, '|' );
            if (sep)
                len = sep - name;
            else
                len = strlen(name);

            c = input[len];
            if ( !memcmp( name, input, len ) && (c == ' ' || c == 0) ) {
                *pend  = input + len;
                *pargs = args;
                return &commands[nn];
            }

            if (sep)
                name = sep + 1;

        } while (sep != NULL && *name);
    }
    /* NOTE: don't touch *pend and *pargs if no command is found */
    return NULL;
}

static void
dump_help( ControlClient  client,
           CommandDef     cmd,
           const char*    prefix )
{
    if (cmd->description) {
        control_write( client, "%s", cmd->description );
    } else if (cmd->descriptor) {
        cmd->descriptor( client );
    } else {
        control_write( client, "%s\r\n", cmd->abstract );
    }

    if (cmd->subcommands) {
        cmd = cmd->subcommands;
        control_write( client, "\r\navailable sub-commands:\r\n" );
        for ( ; cmd->names != NULL; cmd++ ) {
            control_write( client, "   %s %-15s  %s\r\n", prefix, cmd->names, cmd->abstract );
        }
        control_write( client, "\r\n" );
    }
}

static int do_quit(ControlClient client, char* args);  // forward

static void control_send_help_prompt(ControlClient client) {
    control_write(client,
                  "Android Console: type 'help' for a list of commands\r\n");
}

static void control_client_do_command(ControlClient client) {
    char*       line     = client->buff;
    char*       args     = NULL;
    CommandDef  commands = client->commands;
    char*       cmdend   = client->buff;
    CommandDef  cmd = NULL;

    // do security checks before executing find_command
    size_t line_len = strlen(line);
    if (android_http_is_request_line(line, line_len)) {
        control_write(client, "KO: Forbidden HTTP request. Aborting\r\n");
        do_quit(client, NULL);
        return;
    } else if (!android_utf8_is_valid(line, line_len)) {
        control_write(client, "KO: Forbidden binary request. Aborting\r\n");
        do_quit(client, NULL);
        return;
    }

    cmd = find_command(line, commands, &cmdend, &args);

    if (cmd == NULL) {
        control_write(client, "KO: unknown command, try 'help'\r\n");
        return;
    }

    for (;;) {
        CommandDef  subcmd;

        if (cmd->handler) {
            if ( !cmd->handler( client, args ) ) {
                control_write( client, "OK\r\n" );
            }
            break;
        }

        /* no handler means we should have sub-commands */
        if (cmd->subcommands == NULL) {
            control_write( client, "KO: internal error: buggy command table for '%.*s'\r\n",
                           cmdend - client->buff, client->buff );
            break;
        }

        /* we need a sub-command here */
        if ( !args ) {
            dump_help( client, cmd, "" );
            control_write( client, "KO: missing sub-command\r\n" );
            break;
        }

        line     = args;
        commands = cmd->subcommands;
        subcmd   = find_command( line, commands, &cmdend, &args );
        if (subcmd == NULL) {
            dump_help( client, cmd, "" );
            control_write( client, "KO:  bad sub-command\r\n" );
            break;
        }
        cmd = subcmd;
    }
}

/* implement the 'help' command */
static int
do_help( ControlClient  client, char*  args )
{
    char*       line;
    char*       start = args;
    char*       end = start;
    CommandDef  cmd = client->commands;

    /* without arguments, simply list all commands */
    if (args == NULL) {
        control_write( client, "Android console commands:\r\n" );
        for ( ; cmd->names != NULL; cmd++ ) {
            control_write( client, "    %s\r\n", cmd->names );
        }
        control_write( client, "\r\nTry 'help-verbose' for more description"
                               "\r\nTry 'help <command>' for command-specific help\r\n" );
        return 0;
    }

    /* with an argument, find the corresponding command */
    for (;;) {
        CommandDef  subcmd;

        line    = args;
        subcmd  = find_command( line, cmd, &end, &args );
        if (subcmd == NULL) {
            /* Not found. Recurse to print the short list of commands. */
            do_help(client, NULL);
            control_write( client, "\r\nKO: unknown command\r\n" );
            return -1;
        }

        if ( !args || !subcmd->subcommands ) {
            dump_help( client, subcmd, start );
            return 0;
        }
        cmd = subcmd->subcommands;
    }
}

/* implement the 'help-verbose' command */
static int
do_help_verbose( ControlClient  client, char*  args )
{
    CommandDef cmd;

    /* Dump all commands */
    control_write( client, "Android console command help:\r\n\r\n" );
    for (cmd = client->commands; cmd->names != NULL; cmd++) {
        control_write( client, "    %-15s  %s\r\n", cmd->names, cmd->abstract );
    }
    control_write( client, "\r\ntry 'help <command>' for command-specific help\r\n" );
    return 0;
}


/* implement the 'ping' command */
static int
do_ping( ControlClient  client, char*  args )
{
    control_write( client, "I am alive!\r\n" );
    return 0;
}

static void
control_client_read_byte( ControlClient  client, unsigned char  ch )
{
    if (ch == '\r')
    {
        /* filter them out */
    }
    else if (ch == '\n')
    {
        client->buff[ client->buff_len ] = 0;
        control_client_do_command( client );
        if (client->finished)
            return;

        client->buff_len = 0;
    }
    else
    {
        if (client->buff_len >= sizeof(client->buff)-1)
            client->buff_len = 0;

        client->buff[ client->buff_len++ ] = ch;
    }
}

static void control_client_read(void* opaque, int fd, unsigned events) {
    ControlClient client = opaque;
    unsigned char buf[4096];
    int size;

    assert(fd == client->sock);
    assert((events & LOOP_IO_READ) != 0);

    D(( "in control_client read: " ));
    size = socket_recv( client->sock, buf, sizeof(buf) );
    if (size < 0) {
        D(( "size < 0, exiting with %d: %s\n", errno, errno_str ));
        if (errno != EWOULDBLOCK && errno != EAGAIN)
            control_client_destroy( client );
        return;
    }

    if (size == 0) {
        /* end of connection */
        D(( "end of connection detected !!\n" ));
        control_client_destroy( client );
    }
    else {
        int  nn;
#ifdef _WIN32
#  if DEBUG
        char  temp[16];
        int   count = size > sizeof(temp)-1 ? sizeof(temp)-1 : size;
        for (nn = 0; nn < count; nn++) {
                int  c = buf[nn];
                if (c == '\n')
                        temp[nn] = '!';
            else if (c < 32)
                        temp[nn] = '.';
                else
                    temp[nn] = (char)c;
        }
        temp[nn] = 0;
        D(( "received %d bytes: %s\n", size, temp ));
#  endif
#else
        D(( "received %.*s\n", size, buf ));
#endif
        for (nn = 0; nn < size; nn++) {
            control_client_read_byte( client, buf[nn] );
            if (client->finished) {
                control_client_destroy(client);
                return;
            }
        }
    }
}

/* this function is called on each new client connection */
static void control_global_accept(void* opaque,
                                  int listen_fd,
                                  unsigned events) {
    ControlGlobal global = opaque;
    ControlClient client;
    Socket fd;

    D(( "control_global_accept: just in (fd=%d)\n", listen_fd ));
    // connect()s are mapped to LOOP_IO_READ
    assert(events & LOOP_IO_READ);

    fd = HANDLE_EINTR(socket_accept(listen_fd, NULL));
    if (fd < 0) {
        D(( "problem in accept: %d: %s\n", errno, errno_str ));
        return;
    }

    socket_set_xreuseaddr( fd );

    D(( "control_global_accept: creating new client\n" ));
    client = control_client_create( fd, global );
    if (client) {
        char* emulator_console_auth_token_path =
                android_console_auth_token_path_dup();

        int console_auth_status = android_console_auth_get_status();
        switch (console_auth_status) {
            case CONSOLE_AUTH_STATUS_DISABLED:
                client->commands = main_commands;
                control_send_help_prompt(client);
                control_write(client, "OK\r\n");
                D(("control_global_accept: new client %p\n", client));
                break;
            case CONSOLE_AUTH_STATUS_REQUIRED:
                // Note that Studio looks for the first line of this message
                // /exactly/
                control_write(client,
                              "Android Console: Authentication required\r\n");
                control_write(client,
                              "Android Console: type 'auth <auth_token>' to "
                              "authenticate\r\n");
                control_write(client,
                              "Android Console: you can find your <auth_token> "
                              "in \r\n'");
                control_write(client, emulator_console_auth_token_path);
                control_write(client, "'\r\nOK\r\n");
                D(("control_global_accept: auth required %p\n", client));
                break;
            case CONSOLE_AUTH_STATUS_ERROR:
                control_client_destroy(client);
                D(("control_global_accept: rejected client\n"));
                break;
        }
        free(emulator_console_auth_token_path);
    }
}

static void control_global_accept4(void* opaque,
                                   int listen_fd,
                                   unsigned events) {
    assert(listen_fd == loopIo_fd(((ControlGlobal)opaque)->listen4_loopio));
    control_global_accept(opaque, listen_fd, events);
}

static void control_global_accept6(void* opaque,
                                   int listen_fd,
                                   unsigned events) {
    assert(listen_fd == loopIo_fd(((ControlGlobal)opaque)->listen6_loopio));
    control_global_accept(opaque, listen_fd, events);
}


int android_console_start(int control_port,
                          const AndroidConsoleAgents* agents) {
    ControlGlobal global = &_g_global;

    memset( global, 0, sizeof(*global) );
    // Copy the QEMU specific interfaces passed in to make lifetime management
    // simpler.
#define ANDROID_CONSOLE_COPY_AGENT(type, name) \
        global-> name ## _agent [0] = agents-> name [0];
    ANDROID_CONSOLE_AGENTS_LIST(ANDROID_CONSOLE_COPY_AGENT)

    int console_auth_status = android_console_auth_get_status();
    if (console_auth_status == CONSOLE_AUTH_STATUS_ERROR) {
        // Banners don't get sent reliably in QEMU2 if we disconnect quickly, so
        // we output the error message on stderr there.  Do the same here.
        char* emulator_console_auth_token_path =
                android_console_auth_token_path_dup();
        fprintf(stderr,
                "ERROR: Unable to access '%s': emulator console will not "
                "work\n",
                emulator_console_auth_token_path);
        free(emulator_console_auth_token_path);
    }


    Socket fd4 = socket_loopback4_server(control_port, SOCKET_STREAM);
    Socket fd6 = -1;
    // Only try to listen on ipv6 if ipv4 is not supported by system.
    // This is another work around for b.android.com/213734
    if (fd4 < 0 && errno == EAFNOSUPPORT) {
      fd6 = socket_loopback6_server(control_port, SOCKET_STREAM);
    }

    if (fd4 < 0 && fd6 < 0) {
        // Could not bind to either IPv4 or IPv6 interface?
        perror("bind");
        return -1;
    }

    global->looper = looper_getForThread();
    if (fd4 >= 0) {
        global->listen4_loopio =
                loopIo_new(global->looper, fd4, control_global_accept4, global);
        loopIo_wantRead(global->listen4_loopio);
        loopIo_dontWantWrite(global->listen4_loopio);
    } else {
        global->listen4_loopio = NULL;
    }

    if (fd6 >= 0) {
        global->listen6_loopio =
                loopIo_new(global->looper, fd6, control_global_accept6, global);
        loopIo_wantRead(global->listen6_loopio);
        loopIo_dontWantWrite(global->listen6_loopio);
    } else {
        global->listen6_loopio = NULL;
    }

    return 0;
}



static int
do_quit( ControlClient  client, char*  args )
{
    client->finished = 1;
    return -1;
}

/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                        N E T W O R K   S E T T I N G S                          ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static int
do_network_status( ControlClient  client, char*  args )
{
    control_write( client, "Current network status:\r\n" );

    control_write( client, "  download speed:   %8d bits/s (%.1f KB/s)\r\n",
                   (long)android_net_download_speed, android_net_download_speed/8192. );

    control_write( client, "  upload speed:     %8d bits/s (%.1f KB/s)\r\n",
                   (long)android_net_upload_speed, android_net_upload_speed/8192. );

    control_write( client, "  minimum latency:  %ld ms\r\n", android_net_min_latency );
    control_write( client, "  maximum latency:  %ld ms\r\n", android_net_max_latency );
    return 0;
}

static void
dump_network_speeds( ControlClient  client )
{
    const AndroidNetworkSpeed* speed = android_network_speeds;
    const AndroidNetworkSpeed* speed_end = speed + android_network_speeds_count;
    const char* const  format = "  %-8s %s\r\n";
    for ( ; speed < speed_end; ++speed ) {
        control_write( client, format, speed->name, speed->display_name );
    }
    control_write( client, format, "<num>", "selects both upload and download speed" );
    control_write( client, format, "<up>:<down>", "select individual upload/download speeds" );
}


static int
do_network_speed( ControlClient  client, char*  args )
{
    if ( !args ) {
        control_write( client, "KO: missing <speed> argument, see 'help network speed'\r\n" );
        return -1;
    }
    if (!android_network_set_speed(args)) {
        control_write( client, "KO: invalid <speed> argument, see 'help network speed' for valid values\r\n" );
        return -1;
    }

    netshaper_set_rate( android_net_shaper_in,  android_net_download_speed );
    netshaper_set_rate( android_net_shaper_out, android_net_upload_speed );

    if (android_modem) {
        amodem_set_data_network_type( android_modem,
                                    android_parse_network_type( args ) );
    }
    return 0;
}

static void
describe_network_speed( ControlClient  client )
{
    control_write( client,
                   "'network speed <speed>' allows you to dynamically change the speed of the emulated\r\n"
                   "network on the device, where <speed> is one of the following:\r\n\r\n" );
    dump_network_speeds( client );
}

static int
do_network_delay( ControlClient  client, char*  args )
{
    if ( !args ) {
        control_write( client, "KO: missing <delay> argument, see 'help network delay'\r\n" );
        return -1;
    }
    if (!android_network_set_latency(args)) {
        control_write( client, "KO: invalid <delay> argument, see 'help network delay' for valid values\r\n" );
        return -1;
    }
    netdelay_set_latency( android_net_delay_in, android_net_min_latency, android_net_max_latency );
    return 0;
}

static void
describe_network_delay( ControlClient  client )
{
    control_write( client,
                   "'network delay <latency>' allows you to dynamically change the latency of the emulated\r\n"
                   "network on the device, where <latency> is one of the following:\r\n\r\n" );
    /* XXX: TODO */
}

static int
do_network_capture_start( ControlClient  client, char*  args )
{
    if ( !args ) {
        control_write( client, "KO: missing <file> argument, see 'help network capture start'\r\n" );
        return -1;
    }
    if ( qemu_tcpdump_start(args) < 0) {
        control_write( client, "KO: could not start capture: %s", strerror(errno) );
        return -1;
    }
    return 0;
}

static int
do_network_capture_stop( ControlClient  client, char*  args )
{
    /* no need to return an error here */
    qemu_tcpdump_stop();
    return 0;
}

static const CommandDefRec  network_capture_commands[] =
{
    { "start", "start network capture",
      "'network capture start <file>' starts a new capture of network packets\r\n"
      "into a specific <file>. This will stop any capture already in progress.\r\n"
      "the capture file can later be analyzed by tools like WireShark. It uses\r\n"
      "the libpcap file format.\r\n\r\n"
      "you can stop the capture anytime with 'network capture stop'\r\n", NULL,
      do_network_capture_start, NULL },

    { "stop", "stop network capture",
      "'network capture stop' stops a currently running packet capture, if any.\r\n"
      "you can start one with 'network capture start <file>'\r\n", NULL,
      do_network_capture_stop, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CommandDefRec  network_commands[] =
{
    { "status", "dump network status", NULL, NULL,
       do_network_status, NULL },

    { "speed", "change network speed", NULL, describe_network_speed,
      do_network_speed, NULL },

    { "delay", "change network latency", NULL, describe_network_delay,
       do_network_delay, NULL },

    { "capture", "dump network packets to file",
      "allows to start/stop capture of network packets to a file for later analysis\r\n", NULL,
      NULL, network_capture_commands },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};

/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                       P O R T   R E D I R E C T I O N S                         ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static int
do_redir_list( ControlClient  client, char*  args )
{
    ControlGlobal  global = client->global;

    if (global->num_redirs == 0)
        control_write( client, "no active redirections\r\n" );
    else {
        int  nn;
        for (nn = 0; nn < global->num_redirs; nn++) {
            Redir  redir = &global->redirs[nn];
            control_write( client, "%s:%-5d => %-5d\r\n",
                          redir->host_udp ? "udp" : "tcp",
                          redir->host_port,
                          redir->guest_port );
        }
    }
    return 0;
}

/* parse a protocol:port specification */
static int
redir_parse_proto_port( char*  args, int  *pport, int  *pproto )
{
    int  proto = -1;
    int  len   = 0;
    char*  end;

    if ( !memcmp( args, "tcp:", 4 ) ) {
        proto = 0;
        len   = 4;
    }
    else if ( !memcmp( args, "udp:", 4 ) ) {
        proto = 1;
        len   = 4;
    }
    else
        return 0;

    args   += len;
    *pproto = proto;
    *pport  = strtol( args, &end, 10 );
    if (end == args)
        return 0;

    len += end - args;
    return len;
}

static int
redir_parse_guest_port( char*  arg, int  *pport )
{
    char*  end;

    *pport = strtoul( arg, &end, 10 );
    if (end == arg)
        return 0;

    return end - arg;
}

static Redir
redir_find( ControlGlobal  global, int  port, int  isudp )
{
    int  nn;

    for (nn = 0; nn < global->num_redirs; nn++) {
        Redir  redir = &global->redirs[nn];

        if (redir->host_port == port && redir->host_udp == isudp)
            return redir;
    }
    return NULL;
}


static int
do_redir_add( ControlClient  client, char*  args )
{
    int       len, host_proto, host_port, guest_port;
    uint32_t  guest_ip;
    Redir     redir;

    if ( !args )
        goto BadFormat;

    if (!client->global->net_agent->isSlirpInited())
    {
        control_write( client, "KO: network emulation disabled\r\n");
        return -1;
    }

    len = redir_parse_proto_port( args, &host_port, &host_proto );
    if (len == 0 || args[len] != ':')
        goto BadFormat;

    args += len + 1;
    len = redir_parse_guest_port( args, &guest_port );
    if (len == 0 || args[len] != 0)
        goto BadFormat;

    redir = redir_find( client->global, host_port, host_proto );
    if ( redir != NULL ) {
        control_write( client, "KO: host port already active, use 'redir del' to remove first\r\n" );
        return -1;
    }

    if (inet_strtoip("10.0.2.15", &guest_ip) < 0) {
        control_write( client, "KO: unexpected internal failure when resolving 10.0.2.15\r\n" );
        return -1;
    }

    D(("pattern hport=%d gport=%d proto=%d\n", host_port, guest_port, host_proto ));
    if ( control_global_add_redir( client->global, host_port, host_proto,
                                   guest_ip, guest_port ) < 0 )
    {
        control_write( client, "KO: not enough memory to allocate redirection\r\n" );
        return -1;
    }

    if (!client->global->net_agent->slirpRedir(host_proto, host_port, guest_ip, guest_port)) {
        control_write( client, "KO: can't setup redirection, port probably used by another program on host\r\n" );
        control_global_del_redir( client->global, host_port, host_proto );
        return -1;
    }

    return 0;

BadFormat:
    control_write( client, "KO: bad redirection format, try (tcp|udp):hostport:guestport\r\n", -1 );
    return -1;
}


static int
do_redir_del( ControlClient  client, char*  args )
{
    int    len, proto, port;
    Redir  redir;

    if ( !args )
        goto BadFormat;
    len = redir_parse_proto_port( args, &port, &proto );
    if ( len == 0 || args[len] != 0 )
        goto BadFormat;

    redir = redir_find( client->global, port, proto );
    if (redir == NULL) {
        control_write( client, "KO: can't remove unknown redirection (%s:%d)\r\n",
                        proto ? "udp" : "tcp", port );
        return -1;
    }

    client->global->net_agent->slirpUnredir( redir->host_udp, redir->host_port );
    control_global_del_redir( client->global, port, proto );

    return 0;

BadFormat:
    control_write( client, "KO: bad redirection format, try (tcp|udp):hostport\r\n" );
    return -1;
}

static const CommandDefRec  redir_commands[] =
{
    { "list", "list current redirections",
    "list current port redirections. use 'redir add' and 'redir del' to add and remove them\r\n", NULL,
    do_redir_list, NULL },

    { "add",  "add new redirection",
    "add a new port redirection, arguments must be:\r\n\r\n"
            "  redir add <protocol>:<host-port>:<guest-port>\r\n\r\n"
            "where:   <protocol>     is either 'tcp' or 'udp'\r\n"
            "         <host-port>    a number indicating which port on the host to open\r\n"
            "         <guest-port>   a number indicating which port to route to on the device\r\n"
            "\r\nas an example, 'redir  tcp:5000:6000' will allow any packets sent to\r\n"
            "the host's TCP port 5000 to be routed to TCP port 6000 of the emulated device\r\n", NULL,
    do_redir_add, NULL },

    { "del",  "remove existing redirection",
    "remove a port redirecion that was created with 'redir add', arguments must be:\r\n\r\n"
            "  redir  del <protocol>:<host-port>\r\n\r\n"
            "see the 'help redir add' for the meaning of <protocol> and <host-port>\r\n", NULL,
    do_redir_del, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};



/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                          C D M A   M O D E M                                    ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static const struct {
    const char *            name;
    const char *            display;
    ACdmaSubscriptionSource source;
} _cdma_subscription_sources[] = {
    { "nv",            "Read subscription from non-volatile RAM", A_SUBSCRIPTION_NVRAM },
    { "ruim",          "Read subscription from RUIM", A_SUBSCRIPTION_RUIM },
};

static void
dump_subscription_sources( ControlClient client )
{
    int i;
    for (i = 0;
         i < sizeof(_cdma_subscription_sources) / sizeof(_cdma_subscription_sources[0]);
         i++) {
        control_write( client, "    %s: %s\r\n",
                       _cdma_subscription_sources[i].name,
                       _cdma_subscription_sources[i].display );
    }
}

static void
describe_subscription_source( ControlClient client )
{
    control_write( client,
                   "'cdma ssource <ssource>' allows you to specify where to read the subscription from\r\n" );
    dump_subscription_sources( client );
}

static int
do_cdma_ssource( ControlClient  client, char*  args )
{
    int nn;
    if (!args) {
        control_write( client, "KO: missing argument, try 'cdma ssource <source>'\r\n" );
        return -1;
    }

    for (nn = 0; nn < sizeof(_cdma_subscription_sources) / sizeof(_cdma_subscription_sources[0]);
            nn++) {
        const char*         name    = _cdma_subscription_sources[nn].name;
        ACdmaSubscriptionSource ssource = _cdma_subscription_sources[nn].source;

        if (!name)
            break;

        if (!strcasecmp( args, name )) {
            amodem_set_cdma_subscription_source( android_modem, ssource );
            return 0;
        }
    }
    control_write( client, "KO: Don't know source %s\r\n", args );
    return -1;
}

static int
do_cdma_prl_version( ControlClient client, char * args )
{
    int version = 0;
    char *endptr;

    if (!args) {
        control_write( client, "KO: missing argument, try 'cdma prl_version <version>'\r\n");
        return -1;
    }

    version = strtol(args, &endptr, 0);
    if (endptr != args) {
        amodem_set_cdma_prl_version( android_modem, version );
    }
    return 0;
}
/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                           G S M   M O D E M                                     ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static const struct {
    const char*         name;
    const char*         display;
    ARegistrationState  state;
} _gsm_states[] = {
    { "unregistered",  "no network available", A_REGISTRATION_UNREGISTERED },
    { "home",          "on local network, non-roaming", A_REGISTRATION_HOME },
    { "roaming",       "on roaming network", A_REGISTRATION_ROAMING },
    { "searching",     "searching networks", A_REGISTRATION_SEARCHING },
    { "denied",        "emergency calls only", A_REGISTRATION_DENIED },
    { "off",           "same as 'unregistered'", A_REGISTRATION_UNREGISTERED },
    { "on",            "same as 'home'", A_REGISTRATION_HOME },
    { NULL, NULL, A_REGISTRATION_UNREGISTERED }
};

static const char*
gsm_state_to_string( ARegistrationState  state )
{
    int  nn;
    for (nn = 0; _gsm_states[nn].name != NULL; nn++) {
        if (state == _gsm_states[nn].state)
            return _gsm_states[nn].name;
    }
    return "<unknown>";
}

static int
do_gsm_status( ControlClient  client, char*  args )
{
    if (args) {
        control_write( client, "KO: no argument required\r\n" );
        return -1;
    }
    if (!android_modem) {
        control_write( client, "KO: modem emulation not running\r\n" );
        return -1;
    }
    control_write( client, "gsm voice state: %s\r\n",
                   gsm_state_to_string(
                       amodem_get_voice_registration(android_modem) ) );
    control_write( client, "gsm data state:  %s\r\n",
                   gsm_state_to_string(
                       amodem_get_data_registration(android_modem) ) );
    return 0;
}


static void
help_gsm_data( ControlClient  client )
{
    int  nn;
    control_write( client,
            "the 'gsm data <state>' allows you to change the state of your GPRS connection\r\n"
            "valid values for <state> are the following:\r\n\r\n" );
    for (nn = 0; ; nn++) {
        const char*         name    = _gsm_states[nn].name;
        const char*         display = _gsm_states[nn].display;

        if (!name)
            break;

        control_write( client, "  %-15s %s\r\n", name, display );
    }
    control_write( client, "\r\n" );
}


static int
do_gsm_data( ControlClient  client, char*  args )
{
    int  nn;

    if (!args) {
        control_write( client, "KO: missing argument, try 'gsm data <state>'\r\n" );
        return -1;
    }

    for (nn = 0; ; nn++) {
        const char*         name    = _gsm_states[nn].name;
        ARegistrationState  state   = _gsm_states[nn].state;

        if (!name)
            break;

        if ( !strcmp( args, name ) ) {
            if (!android_modem) {
                control_write( client, "KO: modem emulation not running\r\n" );
                return -1;
            }
            amodem_set_data_registration( android_modem, state );
            android_net_disable = (state != A_REGISTRATION_HOME    &&
                                state != A_REGISTRATION_ROAMING );
            return 0;
        }
    }
    control_write( client, "KO: bad GSM data state name, try 'help gsm data' for list of valid values\r\n" );
    return -1;
}

static void
help_gsm_voice( ControlClient  client )
{
    int  nn;
    control_write( client,
            "the 'gsm voice <state>' allows you to change the state of your GPRS connection\r\n"
            "valid values for <state> are the following:\r\n\r\n" );
    for (nn = 0; ; nn++) {
        const char*         name    = _gsm_states[nn].name;
        const char*         display = _gsm_states[nn].display;

        if (!name)
            break;

        control_write( client, "  %-15s %s\r\n", name, display );
    }
    control_write( client, "\r\n" );
}


static int
do_gsm_voice( ControlClient  client, char*  args )
{
    int  nn;

    if (!args) {
        control_write( client, "KO: missing argument, try 'gsm voice <state>'\r\n" );
        return -1;
    }

    for (nn = 0; ; nn++) {
        const char*         name    = _gsm_states[nn].name;
        ARegistrationState  state   = _gsm_states[nn].state;

        if (!name)
            break;

        if ( !strcmp( args, name ) ) {
            if (!android_modem) {
                control_write( client, "KO: modem emulation not running\r\n" );
                return -1;
            }
            amodem_set_voice_registration( android_modem, state );
            return 0;
        }
    }
    control_write( client, "KO: bad GSM data state name, try 'help gsm voice' for list of valid values\r\n" );
    return -1;
}


static int
gsm_check_number( char*  args )
{
    int  nn;

    for (nn = 0; args[nn] != 0; nn++) {
        int  c = args[nn];
        if ( !isdigit(c) && c != '+' && c != '#' ) {
            return -1;
        }
    }
    if (nn == 0)
        return -1;

    return 0;
}

static int
do_gsm_call( ControlClient  client, char*  args )
{
    /* check that we have a phone number made of digits */
    if (!args) {
        control_write( client, "KO: missing argument, try 'gsm call <phonenumber>'\r\n" );
        return -1;
    }

    if (gsm_check_number(args)) {
        control_write( client, "KO: bad phone number format, use digits, # and + only\r\n" );
        return -1;
    }

    if (!android_modem) {
        control_write( client, "KO: modem emulation not running\r\n" );
        return -1;
    }
    amodem_add_inbound_call( android_modem, args );
    return 0;
}

static int
do_gsm_cancel( ControlClient  client, char*  args )
{
    if (!args) {
        control_write( client, "KO: missing argument, try 'gsm call <phonenumber>'\r\n" );
        return -1;
    }
    if (gsm_check_number(args)) {
        control_write( client, "KO: bad phone number format, use digits, # and + only\r\n" );
        return -1;
    }
    if (!android_modem) {
        control_write( client, "KO: modem emulation not running\r\n" );
        return -1;
    }
    if ( amodem_disconnect_call( android_modem, args ) < 0 ) {
        control_write( client, "KO: could not cancel this number\r\n" );
        return -1;
    }
    return 0;
}


static const char*
call_state_to_string( ACallState  state )
{
    switch (state) {
        case A_CALL_ACTIVE:   return "active";
        case A_CALL_HELD:     return "held";
        case A_CALL_ALERTING: return "ringing";
        case A_CALL_WAITING:  return "waiting";
        case A_CALL_INCOMING: return "incoming";
        default: return "unknown";
    }
}

static int
do_gsm_list( ControlClient  client, char*  args )
{
    /* check that we have a phone number made of digits */
    int   count = amodem_get_call_count( android_modem );
    int   nn;
    for (nn = 0; nn < count; nn++) {
        ACall        call = amodem_get_call( android_modem, nn );
        const char*  dir;

        if (call == NULL)
            continue;

        if (call->dir == A_CALL_OUTBOUND)
            dir = "outbound to ";
         else
            dir = "inbound from";

        control_write( client, "%s %-10s : %s\r\n", dir,
                       call->number, call_state_to_string(call->state) );
    }
    return 0;
}

static int
do_gsm_busy( ControlClient  client, char*  args )
{
    ACall  call;

    if (!args) {
        control_write( client, "KO: missing argument, try 'gsm busy <phonenumber>'\r\n" );
        return -1;
    }
    call = amodem_find_call_by_number( android_modem, args );
    if (call == NULL || call->dir != A_CALL_OUTBOUND) {
        control_write( client, "KO: no current outbound call to number '%s' (call %p)\r\n", args, call );
        return -1;
    }
    if ( amodem_disconnect_call( android_modem, args ) < 0 ) {
        control_write( client, "KO: could not cancel this number\r\n" );
        return -1;
    }
    return 0;
}

static int
do_gsm_hold( ControlClient  client, char*  args )
{
    ACall  call;

    if (!args) {
        control_write( client, "KO: missing argument, try 'gsm hold <phonenumber>'\r\n" );
        return -1;
    }
    call = amodem_find_call_by_number( android_modem, args );
    if (call == NULL) {
        control_write( client, "KO: no current call to/from number '%s'\r\n", args );
        return -1;
    }
    if ( amodem_update_call( android_modem, args, A_CALL_HELD ) < 0 ) {
        control_write( client, "KO: could put this call on hold\r\n" );
        return -1;
    }
    return 0;
}


static int
do_gsm_accept( ControlClient  client, char*  args )
{
    ACall  call;

    if (!args) {
        control_write( client, "KO: missing argument, try 'gsm accept <phonenumber>'\r\n" );
        return -1;
    }
    call = amodem_find_call_by_number( android_modem, args );
    if (call == NULL) {
        control_write( client, "KO: no current call to/from number '%s'\r\n", args );
        return -1;
    }
    if ( amodem_update_call( android_modem, args, A_CALL_ACTIVE ) < 0 ) {
        control_write( client, "KO: could not activate this call\r\n" );
        return -1;
    }
    return 0;
}

static int
do_gsm_signal( ControlClient  client, char*  args )
{
    enum { SIGNAL_RSSI = 0, SIGNAL_BER, NUM_SIGNAL_PARAMS };
    char*   p = args;
    int     top_param = -1;
    int     params[ NUM_SIGNAL_PARAMS ];

    static  int  last_ber = 99;

    if (!p)
        p = "";

    /* tokenize */
    while (*p) {
        char*   end;
        int  val = strtol( p, &end, 10 );

        if (end == p) {
            control_write( client, "KO: argument '%s' is not a number\n", p );
            return -1;
        }

        params[++top_param] = val;
        if (top_param + 1 == NUM_SIGNAL_PARAMS)
            break;

        p = end;
        while (*p && (p[0] == ' ' || p[0] == '\t'))
            p += 1;
    }

    /* sanity check */
    if (top_param < SIGNAL_RSSI) {
        control_write( client, "KO: not enough arguments: see 'help gsm signal' for details\r\n" );
        return -1;
    }

    int rssi = params[SIGNAL_RSSI];
    if ((rssi < 0 || rssi > 31) && rssi != 99) {
        control_write( client, "KO: invalid RSSI - must be 0..31 or 99\r\n");
        return -1;
    }

    /* check ber is 0..7 or 99 */
    if (top_param >= SIGNAL_BER) {
        int ber = params[SIGNAL_BER];
        if ((ber < 0 || ber > 7) && ber != 99) {
            control_write( client, "KO: invalid BER - must be 0..7 or 99\r\n");
            return -1;
        }
        last_ber = ber;
    }

    amodem_set_signal_strength( android_modem, rssi, last_ber );

    return 0;
}

static int
do_gsm_signal_profile( ControlClient  client, char*  args )
{
    char* end;
    char*   p = args;
    if (!p)
        p = "";
    int  val = strtol( p, &end, 10 );

    if (end == p) {
        control_write( client, "KO: argument '%s' is not a number\r\n", p );
        return -1;
    }

    if (val < 0 || val > 4) {
        control_write( client, "KO: invalid signal strength - must be 0..4\r\n");
        return -1;
    }

    amodem_set_signal_strength_profile( android_modem, val );

    return 0;
}


#if 0
static const CommandDefRec  gsm_in_commands[] =
{
    { "new", "create a new 'waiting' inbound call",
    "'gsm in create <phonenumber>' creates a new inbound phone call, placed in\r\n"
    "the 'waiting' state by default, until the system answers/holds/closes it\r\n", NULL
    do_gsm_in_create, NULL },

    { "hold", "change the state of an outbound call to 'held'",
    "change the state of an outbound call to 'held'. this is only possible\r\n"
    "if the call in the 'waiting' or 'active' state\r\n", NULL,
    do_gsm_out_hold, NULL },

    { "accept", "change the state of an outbound call to 'active'",
    "change the state of an outbound call to 'active'. this is only possible\r\n"
    "if the call is in the 'waiting' or 'held' state\r\n", NULL,
    do_gsm_out_accept, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};
#endif


static const CommandDefRec  cdma_commands[] =
{
    { "ssource", "Set the current CDMA subscription source",
      NULL, describe_subscription_source,
      do_cdma_ssource, NULL },
    { "prl_version", "Dump the current PRL version",
      NULL, NULL,
      do_cdma_prl_version, NULL },
    { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const CommandDefRec  gsm_commands[] =
{
    { "list", "list current phone calls",
    "'gsm list' lists all inbound and outbound calls and their state\r\n", NULL,
    do_gsm_list, NULL },

    { "call", "create inbound phone call",
    "'gsm call <phonenumber>' allows you to simulate a new inbound call\r\n", NULL,
    do_gsm_call, NULL },

    { "busy", "close waiting outbound call as busy",
    "'gsm busy <remoteNumber>' closes an outbound call, reporting\r\n"
    "the remote phone as busy. only possible if the call is 'waiting'.\r\n", NULL,
    do_gsm_busy, NULL },

    { "hold", "change the state of an outbound call to 'held'",
    "'gsm hold <remoteNumber>' change the state of a call to 'held'. this is only possible\r\n"
    "if the call in the 'waiting' or 'active' state\r\n", NULL,
    do_gsm_hold, NULL },

    { "accept", "change the state of an outbound call to 'active'",
    "'gsm accept <remoteNumber>' change the state of a call to 'active'. this is only possible\r\n"
    "if the call is in the 'waiting' or 'held' state\r\n", NULL,
    do_gsm_accept, NULL },

    { "cancel", "disconnect an inbound or outbound phone call",
    "'gsm cancel <phonenumber>' allows you to simulate the end of an inbound or outbound call\r\n", NULL,
    do_gsm_cancel, NULL },

    { "data", "modify data connection state", NULL, help_gsm_data,
    do_gsm_data, NULL },

    { "voice", "modify voice connection state", NULL, help_gsm_voice,
    do_gsm_voice, NULL },

    { "status", "display GSM status",
    "'gsm status' displays the current state of the GSM emulation\r\n", NULL,
    do_gsm_status, NULL },

    { "signal", "set sets the rssi and ber",
    "'gsm signal <rssi> [<ber>]' changes the reported strength and error rate on next (15s) update.\r\n"
    "rssi range is 0..31 and 99 for unknown\r\n"
    "ber range is 0..7 percent and 99 for unknown\r\n",
    NULL, do_gsm_signal, NULL },

    { "signal-profile", "set the signal strength profile",
    "'gsm signal-profile <strength>' changes the reported strength on next (15s) update.\r\n"
    "strength range is 0..4\r\n",
    NULL, do_gsm_signal_profile, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};

/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                           S M S   C O M M A N D                                 ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static int
do_sms_send( ControlClient  client, char*  args )
{
    char*          p;
    int            textlen;
    SmsAddressRec  sender;
    SmsPDU*        pdus;
    int            nn;

    /* check that we have a phone number made of digits */
    if (!args) {
    MissingArgument:
        control_write( client, "KO: missing argument, try 'sms send <phonenumber> <text message>'\r\n" );
        return -1;
    }
    p = strchr( args, ' ' );
    if (!p) {
        goto MissingArgument;
    }

    if ( sms_address_from_str( &sender, args, p - args ) < 0 ) {
        control_write( client, "KO: bad phone number format, must be [+](0-9)*\r\n" );
        return -1;
    }


    /* un-escape message text into proper utf-8 (conversion happens in-site) */
    p      += 1;
    textlen = strlen(p);
    textlen = sms_utf8_from_message_str( p, textlen, (unsigned char*)p, textlen );
    if (textlen < 0) {
        control_write( client, "message must be utf8 and can use the following escapes:\r\n"
                       "    \\n      for a newline\r\n"
                       "    \\xNN    where NN are two hexadecimal numbers\r\n"
                       "    \\uNNNN  where NNNN are four hexadecimal numbers\r\n"
                       "    \\\\     to send a '\\' character\r\n\r\n"
                       "    anything else is an error\r\n"
                       "KO: badly formatted text\r\n" );
        return -1;
    }

    if (!android_modem) {
        control_write( client, "KO: modem emulation not running\r\n" );
        return -1;
    }

    /* create a list of SMS PDUs, then send them */
    pdus = smspdu_create_deliver_utf8( (cbytes_t)p, textlen, &sender, NULL );
    if (pdus == NULL) {
        control_write( client, "KO: internal error when creating SMS-DELIVER PDUs\n" );
        return -1;
    }

    for (nn = 0; pdus[nn] != NULL; nn++)
        amodem_receive_sms( android_modem, pdus[nn] );

    smspdu_free_list( pdus );
    return 0;
}

static int
do_sms_sendpdu( ControlClient  client, char*  args )
{
    SmsPDU  pdu;

    /* check that we have a phone number made of digits */
    if (!args) {
        control_write( client, "KO: missing argument, try 'sms sendpdu <hexstring>'\r\n" );
        return -1;
    }

    if (!android_modem) {
        control_write( client, "KO: modem emulation not running\r\n" );
        return -1;
    }

    pdu = smspdu_create_from_hex( args, strlen(args) );
    if (pdu == NULL) {
        control_write( client, "KO: badly formatted <hexstring>\r\n" );
        return -1;
    }

    amodem_receive_sms( android_modem, pdu );
    smspdu_free( pdu );
    return 0;
}

static const CommandDefRec  sms_commands[] =
{
    { "send", "send inbound SMS text message",
    "'sms send <phonenumber> <message>' allows you to simulate a new inbound sms message\r\n", NULL,
    do_sms_send, NULL },

    { "pdu", "send inbound SMS PDU",
    "'sms pdu <hexstring>' allows you to simulate a new inbound sms PDU\r\n"
    "(used internally when one emulator sends SMS messages to another instance).\r\n"
    "you probably don't want to play with this at all\r\n", NULL,
    do_sms_sendpdu, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};

/********************************************************************************************/
/********************************************************************************************/
/***** ******/
/*****                         P O W E R   C O M M A N D ******/
/***** ******/
/********************************************************************************************/
/********************************************************************************************/

void do_control_write(void* opaque, const char* args) {
    control_write((ControlClient)opaque, args);
}

static int
do_power_display( ControlClient client, char*  args )
{
    client->global->battery_agent->batteryDisplay(client, control_write_out_cb);
    return 0;
}

static int
do_ac_state( ControlClient  client, char*  args )
{
    if (args) {
        if (strcasecmp(args, "on") == 0) {
            client->global->battery_agent->setIsCharging(true);
            return 0;
        }
        if (strcasecmp(args, "off") == 0) {
            client->global->battery_agent->setIsCharging(false);
            return 0;
        }
    }

    control_write( client, "KO: Usage: \"ac on\" or \"ac off\"\n" );
    return -1;
}

static int
do_battery_status( ControlClient  client, char*  args )
{
    if (args) {
        if (strcasecmp(args, "unknown") == 0) {
            client->global->battery_agent->setStatus(BATTERY_STATUS_UNKNOWN);
            return 0;
        }
        if (strcasecmp(args, "charging") == 0) {
            client->global->battery_agent->setStatus(BATTERY_STATUS_CHARGING);
            return 0;
        }
        if (strcasecmp(args, "discharging") == 0) {
            client->global->battery_agent->setStatus(
                    BATTERY_STATUS_DISCHARGING);
            return 0;
        }
        if (strcasecmp(args, "not-charging") == 0) {
            client->global->battery_agent->setStatus(
                    BATTERY_STATUS_NOT_CHARGING);
            return 0;
        }
        if (strcasecmp(args, "full") == 0) {
            client->global->battery_agent->setStatus(BATTERY_STATUS_FULL);
            return 0;
        }
    }

    control_write( client, "KO: Usage: \"status unknown|charging|discharging|not-charging|full\"\n" );
    return -1;
}

static int
do_battery_present( ControlClient  client, char*  args )
{
    if (args) {
        if (strcasecmp(args, "true") == 0) {
            client->global->battery_agent->setIsBatteryPresent(true);
            return 0;
        }
        if (strcasecmp(args, "false") == 0) {
            client->global->battery_agent->setIsBatteryPresent(true);
            return 0;
        }
    }

    control_write( client, "KO: Usage: \"present true\" or \"present false\"\n" );
    return -1;
}

static int
do_battery_health( ControlClient  client, char*  args )
{
    if (args) {
        if (strcasecmp(args, "unknown") == 0) {
            client->global->battery_agent->setHealth(BATTERY_HEALTH_UNKNOWN);
            return 0;
        }
        if (strcasecmp(args, "good") == 0) {
            client->global->battery_agent->setHealth(BATTERY_HEALTH_GOOD);
            return 0;
        }
        if (strcasecmp(args, "overheat") == 0) {
            client->global->battery_agent->setHealth(BATTERY_HEALTH_OVERHEATED);
            return 0;
        }
        if (strcasecmp(args, "dead") == 0) {
            client->global->battery_agent->setHealth(BATTERY_HEALTH_DEAD);
            return 0;
        }
        if (strcasecmp(args, "overvoltage") == 0) {
            client->global->battery_agent->setHealth(
                    BATTERY_HEALTH_OVERVOLTAGE);
            return 0;
        }
        if (strcasecmp(args, "failure") == 0) {
            client->global->battery_agent->setHealth(BATTERY_HEALTH_FAILED);
            return 0;
        }
    }

    control_write( client, "KO: Usage: \"health unknown|good|overheat|dead|overvoltage|failure\"\n" );
    return -1;
}

static int
do_battery_capacity( ControlClient  client, char*  args )
{
    if (args) {
        int capacity;

        if (sscanf(args, "%d", &capacity) == 1 && capacity >= 0 && capacity <= 100) {
            client->global->battery_agent->setChargeLevel(capacity);
            return 0;
        }
    }

    control_write( client, "KO: Usage: \"capacity <percentage>\"\n" );
    return -1;
}


static const CommandDefRec  power_commands[] =
{
    { "display", "display battery and charger state",
    "display battery and charger state\r\n", NULL,
    do_power_display, NULL },

    { "ac", "set AC charging state",
    "'ac on|off' allows you to set the AC charging state to on or off\r\n", NULL,
    do_ac_state, NULL },

    { "status", "set battery status",
    "'status unknown|charging|discharging|not-charging|full' allows you to set battery status\r\n", NULL,
    do_battery_status, NULL },

    { "present", "set battery present state",
    "'present true|false' allows you to set battery present state to true or false\r\n", NULL,
    do_battery_present, NULL },

    { "health", "set battery health state",
    "'health unknown|good|overheat|dead|overvoltage|failure' allows you to set battery health state\r\n", NULL,
    do_battery_health, NULL },

    { "capacity", "set battery capacity state",
    "'capacity <percentage>' allows you to set battery capacity to a value 0 - 100\r\n", NULL,
    do_battery_capacity, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};

/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                         E  V  E  N  T   C O M M A N D S                         ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/


static int
do_event_send( ControlClient  client, char*  args )
{
    char*   p;

    if (!args) {
        control_write( client, "KO: Usage: event send <type>:<code>:<value> ...\r\n" );
        return -1;
    }

    p = args;
    while (*p) {
        char*  q;
        char   temp[128];
        int    type, code, value, ret;

        p += strspn( p, " \t" );  /* skip spaces */
        if (*p == 0)
            break;

        q  = p + strcspn( p, " \t" );

        if (q == p)
            break;

        snprintf(temp, sizeof temp, "%.*s", (int)(intptr_t)(q-p), p);
        ret = android_event_from_str( temp, &type, &code, &value );
        if (ret < 0) {
            if (ret == -1) {
                control_write( client,
                               "KO: invalid event type in '%.*s', try 'event list types' for valid values\r\n",
                               q-p, p );
            } else if (ret == -2) {
                control_write( client,
                               "KO: invalid event code in '%.*s', try 'event list codes <type>' for valid values\r\n",
                               q-p, p );
            } else {
                control_write( client,
                               "KO: invalid event value in '%.*s', must be an integer\r\n",
                               q-p, p);
            }
            return -1;
        }

        client->global->user_event_agent->sendGenericEvent(type, code, value);
        p = q;
    }
    return 0;
}

static int
do_event_types( ControlClient  client, char*  args )
{
    int  count = android_event_get_type_count();
    int  nn;

    control_write( client, "event <type> can be an integer or one of the following aliases\r\n" );
    for (nn = 0; nn < count; nn++) {
        char  tmp[16];
        char* p = tmp;
        char* end = p + sizeof(tmp);
        int   count2 = android_event_get_code_count( nn );;

        p = android_event_bufprint_type_str( p, end, nn );

        control_write( client, "    %-8s", tmp );
        if (count2 > 0)
            control_write( client, "  (%d code aliases)", count2 );

        control_write( client, "\r\n" );
    }
    return 0;
}

static int
do_event_codes( ControlClient  client, char*  args )
{
    int  count;
    int  nn, type, dummy;

    if (!args) {
        control_write( client, "KO: argument missing, try 'event codes <type>'\r\n" );
        return -1;
    }

    if ( android_event_from_str( args, &type, &dummy, &dummy ) < 0 ) {
        control_write( client, "KO: bad argument, see 'event types' for valid values\r\n" );
        return -1;
    }

    count = android_event_get_code_count( type );
    if (count == 0) {
        control_write( client, "no code aliases defined for this type\r\n" );
    } else {
        control_write( client, "type '%s' accepts the following <code> aliases:\r\n",
                       args );
        for (nn = 0; nn < count; nn++) {
            char  temp[20], *p = temp, *end = p + sizeof(temp);
            android_event_bufprint_code_str( p, end, type, nn );
            control_write( client, "    %-12s\r\n", temp );
        }
    }

    return 0;
}

static int
do_event_text( ControlClient  client, char*  args )
{
    if (!args) {
        control_write( client, "KO: argument missing, try 'event text <message>'\r\n" );
        return -1;
    }

    unsigned char*  p   = (unsigned char*) args;
    int             textlen = strlen(args);

    /* un-secape message text into proper utf-8 (conversion happens in-site) */
    textlen = sms_utf8_from_message_str( args, textlen, (unsigned char*)p, textlen );
    if (textlen < 0) {
        control_write( client, "message must be utf8 and can use the following escapes:\r\n"
                       "    \\n      for a newline\r\n"
                       "    \\xNN    where NN are two hexadecimal numbers\r\n"
                       "    \\uNNNN  where NNNN are four hexadecimal numbers\r\n"
                       "    \\\\     to send a '\\' character\r\n\r\n"
                       "    anything else is an error\r\n"
                       "KO: badly formatted text\r\n" );
        return -1;
    }

    if (!client->global->libui_agent->convertUtf8ToKeyCodeEvents(p, textlen,
        (LibuiKeyCodeSendFunc)client->global->user_event_agent->sendKeyCodes)) {
        control_write( client, "KO: device is unable to recieve text input now\r\n" );
        return -1;
    }

    return 0;
}

static const CommandDefRec  event_commands[] =
{
    { "send", "send a series of events to the kernel",
    "'event send <type>:<code>:<value> ...' allows you to send one or more hardware events\r\n"
    "to the Android kernel. You can use text names or integers for <type> and <code>\r\n", NULL,
    do_event_send, NULL },

    { "types", "list all <type> aliases",
    "'event types' list all <type> string aliases supported by the 'event' subcommands\r\n",
    NULL, do_event_types, NULL },

    { "codes", "list all <code> aliases for a given <type>",
    "'event codes <type>' lists all <code> string aliases for a given event <type>\r\n",
    NULL, do_event_codes, NULL },

    { "text", "simulate keystrokes from a given text",
    "'event text <message>' allows you to simulate keypresses to generate a given text\r\n"
    "message. <message> must be an utf-8 string. Unicode points will be reverse-mapped\r\n"
    "according to the current device keyboard. Unsupported characters will be discarded\r\n"
    "silently\r\n", NULL, do_event_text, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};


/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                      S N A P S H O T   C O M M A N D S                          ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static int do_snapshot_list(ControlClient client, char* args) {
    bool success = vmopers(client)->snapshotList(client, control_write_out_cb,
                                                 control_write_err_cb);
    return success ? 0 : -1;
}

static int do_snapshot_save(ControlClient client, char* args) {
    if (args == NULL) {
        control_write(
                client,
                "KO: Argument missing, try 'avd snapshot save <name>'\r\n");
        return -1;
    }

    bool success =
            vmopers(client)->snapshotSave(args, client, control_write_out_cb);
    return success ? 0 : -1;
}

static int do_snapshot_load(ControlClient client, char* args) {
    if (args == NULL) {
        control_write(
                client,
                "KO: Argument missing, try 'avd snapshot load <name>'\r\n");
        return -1;
    }

    bool success =
            vmopers(client)->snapshotLoad(args, client, control_write_out_cb);
    return success ? 0 : -1;
}

static int do_snapshot_del(ControlClient client, char* args) {
    if (args == NULL) {
        control_write(
                client,
                "KO: Argument missing, try 'avd snapshot list <name>'\r\n");
        return -1;
    }

    bool success =
            vmopers(client)->snapshotDelete(args, client, control_write_out_cb);
    return success ? 0 : -1;
}

static const CommandDefRec  snapshot_commands[] =
{
    { "list", "list available state snapshots",
    "'avd snapshot list' will show a list of all state snapshots that can be loaded\r\n",
    NULL, do_snapshot_list, NULL },

    { "save", "save state snapshot",
    "'avd snapshot save <name>' will save the current (run-time) state to a snapshot with the given name\r\n",
    NULL, do_snapshot_save, NULL },

    { "load", "load state snapshot",
    "'avd snapshot load <name>' will load the state snapshot of the given name\r\n",
    NULL, do_snapshot_load, NULL },

    { "del", "delete state snapshot",
    "'avd snapshot del <name>' will delete the state snapshot with the given name\r\n",
    NULL, do_snapshot_del, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};



/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                               V M   C O M M A N D S                             ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static int
do_avd_stop( ControlClient  client, char*  args )
{
    if (!(vmopers(client)->vmIsRunning())) {
        control_write( client, "KO: virtual device already stopped\r\n" );
        return -1;
    }
    return vmopers(client)->vmStop() ? 0 : -1;
}

static int
do_avd_start( ControlClient  client, char*  args )
{
    if (vmopers(client)->vmIsRunning()) {
        control_write( client, "KO: virtual device already running\r\n" );
        return -1;
    }
    return vmopers(client)->vmStart() ? 0 : -1;
}

static int
do_avd_status( ControlClient  client, char*  args )
{
    control_write(client, "virtual device is %s\r\n",
                  vmopers(client)->vmIsRunning() ? "running" : "stopped");
    return 0;
}

static int
do_avd_name( ControlClient  client, char*  args )
{
    control_write( client, "%s\r\n", android_hw->avd_name);
    return 0;
}

static const CommandDefRec  vm_commands[] =
{
    { "stop", "stop the virtual device",
    "'avd stop' stops the virtual device immediately, use 'avd start' to continue execution\r\n",
    NULL, do_avd_stop, NULL },

    { "start", "start/restart the virtual device",
    "'avd start' will start or continue the virtual device, use 'avd stop' to stop it\r\n",
    NULL, do_avd_start, NULL },

    { "status", "query virtual device status",
    "'avd status' will indicate whether the virtual device is running or not\r\n",
    NULL, do_avd_status, NULL },

    { "name", "query virtual device name",
    "'avd name' will return the name of this virtual device\r\n",
    NULL, do_avd_name, NULL },

    { "snapshot", "state snapshot commands",
    "allows you to save and restore the virtual device state in snapshots\r\n",
    NULL, NULL, snapshot_commands },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};

/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                             G E O   C O M M A N D S                             ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static int
do_geo_nmea( ControlClient  client, char*  args )
{
    if (!args) {
        control_write( client, "KO: NMEA sentence missing, try 'help geo nmea'\r\n" );
        return -1;
    }
    if (!client->global->location_agent->gpsIsSupported()) {
        control_write( client, "KO: no GPS emulation in this virtual device\r\n" );
        return -1;
    }
    client->global->location_agent->gpsSendNmea(args);
    return 0;
}

static int
do_geo_fix( ControlClient  client, char*  args )
{
    // GEO_SAT2 provides bug backwards compatibility.
    enum { GEO_LONG = 0, GEO_LAT, GEO_ALT, GEO_SAT, GEO_SAT2, NUM_GEO_PARAMS };
    char*   p = args;
    int     top_param = -1;
    double  altitude;
    double  params[ NUM_GEO_PARAMS ];
    int     n_satellites = 1;

    struct  timeval tVal;

    if (!p)
        p = "";

    /* tokenize */
    while (*p) {
        char*   end;
        double  val = strtod( p, &end );

        if (end == p) {
            control_write( client, "KO: argument '%s' is not a number\n", p );
            return -1;
        }

        params[++top_param] = val;
        if (top_param + 1 == NUM_GEO_PARAMS)
            break;

        p = end;
        while (*p && (p[0] == ' ' || p[0] == '\t'))
            p += 1;
    }

    /* sanity check */
    if (top_param < GEO_LAT) {
        control_write( client, "KO: not enough arguments: see 'help geo fix' for details\r\n" );
        return -1;
    }

    /* check longitude and latitude */
    {
        double longitude = params[0];
        if (longitude < -180.0 || longitude > 180.0) {
            control_write(client, "KO: invalid longitude value. Should be in [-180,+180] range\r\n");
            return -1;
        }
        double latitude = params[1];
        if (latitude < -90.0 || latitude > 90.0) {
            control_write(client, "KO: invalid latitude value. Should be in [-90,+90] range\r\n");
            return -1;
        }
    }

    /* check number of satellites, must be integer between 1 and 12 */
    if (top_param >= GEO_SAT) {
        int sat_index = (top_param >= GEO_SAT2) ? GEO_SAT2 : GEO_SAT;
        n_satellites = (int) params[sat_index];
        if (n_satellites != params[sat_index]
            || n_satellites < 1 || n_satellites > 12) {
            control_write( client, "KO: invalid number of satellites. Must be an integer between 1 and 12\r\n");
            return -1;
        }
    }

    altitude = 0.0;
    if (top_param < GEO_ALT) {
        altitude = params[GEO_ALT];
    }

    memset(&tVal, 0, sizeof(tVal));
    gettimeofday(&tVal, NULL);

    client->global->location_agent->gpsSendLoc(params[GEO_LAT], params[GEO_LONG],
                                               altitude, n_satellites, &tVal);
    return 0;
}

static const CommandDefRec  geo_commands[] =
{
    { "nmea", "send a GPS NMEA sentence",
    "'geo nema <sentence>' sends an NMEA 0183 sentence to the emulated device, as\r\n"
    "if it came from an emulated GPS modem. <sentence> must begin with '$GP'. Only\r\n"
    "'$GPGGA' and '$GPRMC' sentences are supported at the moment.\r\n",
    NULL, do_geo_nmea, NULL },

    { "fix", "send a simple GPS fix",
    "'geo fix <longitude> <latitude> [<altitude> [<satellites>]]'\r\n"
    " allows you to send a simple GPS fix to the emulated system.\r\n"
    " The parameters are:\r\n\r\n"
    "  <longitude>   longitude, in decimal degrees\r\n"
    "  <latitude>    latitude, in decimal degrees\r\n"
    "  <altitude>    optional altitude in meters\r\n"
    "  <satellites>  number of satellites being tracked (1-12)\r\n"
    "\r\n",
    NULL, do_geo_fix, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};


/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                        S E N S O R S  C O M M A N D S                           ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

/* For sensors user prompt string size.*/
#define SENSORS_INFO_SIZE 150

/* Get sensor data - (a,b,c) from sensor name */
static int
do_sensors_get( ControlClient client, char* args )
{
    if (! args) {
        control_write( client, "KO: Usage: \"get <sensorname>\"\n" );
        return -1;
    }

    int status = SENSOR_STATUS_UNKNOWN;
    char sensor[strlen(args) + 1];
    if (1 != sscanf( args, "%s", &sensor[0] ))
        goto SENSOR_STATUS_ERROR;

    int sensor_id = android_sensors_get_id_from_name( sensor );
    char buffer[SENSORS_INFO_SIZE] = { 0 };
    float a, b, c;

    if (sensor_id < 0) {
        status = sensor_id;
        goto SENSOR_STATUS_ERROR;
    } else {
        status = android_sensors_get( sensor_id, &a, &b, &c );
        if (status != SENSOR_STATUS_OK)
            goto SENSOR_STATUS_ERROR;
        snprintf( buffer, sizeof(buffer),
                "%s = %g:%g:%g\r\n", sensor, a, b, c );
        do_control_write( client, buffer );
        return 0;
    }

SENSOR_STATUS_ERROR:
    switch(status) {
    case SENSOR_STATUS_NO_SERVICE:
        snprintf( buffer, sizeof(buffer), "KO: No sensor service found!\r\n" );
        break;
    case SENSOR_STATUS_DISABLED:
        snprintf( buffer, sizeof(buffer), "KO: '%s' sensor is disabled.\r\n", sensor );
        break;
    case SENSOR_STATUS_UNKNOWN:
        snprintf( buffer, sizeof(buffer),
                "KO: unknown sensor name: %s, run 'sensor status' to get available sensors.\r\n", sensor );
        break;
    default:
        snprintf( buffer, sizeof(buffer), "KO: '%s' sensor: exception happens.\r\n", sensor );
    }
    do_control_write( client, buffer );
    return -1;
}

/* set sensor data - (a,b,c) from sensor name */
static int
do_sensors_set( ControlClient client, char* args )
{
    if (! args) {
        control_write( client, "KO: Usage: \"set <sensorname> <value-a>[:<value-b>[:<value-c>]]\"\n" );
        return -1;
    }

    int status;
    char* sensor;
    char* value;
    char* args_dup = strdup( args );
    if (args_dup == NULL) {
        control_write( client, "KO: Memory allocation failed.\n" );
        return -1;
    }
    char* p = args_dup;

    /* Parsing the args to get sensor name string */
    while (*p && isspace(*p)) p++;
    if (*p == 0)
        goto INPUT_ERROR;
    sensor = p;

    /* Parsing the args to get value string */
    while (*p && (! isspace(*p))) p++;
    if (*p == 0 || *(p + 1) == 0/* make sure value isn't NULL */)
        goto INPUT_ERROR;
    *p = 0;
    value = p + 1;

    if (! (strlen(sensor) && strlen(value)))
        goto INPUT_ERROR;

    int sensor_id = android_sensors_get_id_from_name( sensor );
    char buffer[SENSORS_INFO_SIZE] = { 0 };

    if (sensor_id < 0) {
        status = sensor_id;
        goto SENSOR_STATUS_ERROR;
    } else {
        float fvalues[3];
        status = android_sensors_get( sensor_id, &fvalues[0], &fvalues[1], &fvalues[2] );
        if (status != SENSOR_STATUS_OK)
            goto SENSOR_STATUS_ERROR;

        /* Parsing the value part to get the sensor values(a, b, c) */
        int i;
        char* pnext;
        char* pend = value + strlen(value);
        for (i = 0; i < 3; i++, value = pnext + 1) {
            pnext=strchr( value, ':' );
            if (pnext) {
                *pnext = 0;
            } else {
                pnext = pend;
            }

            if (pnext > value) {
                if (1 != sscanf( value,"%g", &fvalues[i] ))
                    goto INPUT_ERROR;
            }
        }

        status = android_sensors_set( sensor_id, fvalues[0], fvalues[1], fvalues[2] );
        if (status != SENSOR_STATUS_OK)
            goto SENSOR_STATUS_ERROR;

        free( args_dup );
        return 0;
    }

SENSOR_STATUS_ERROR:
    switch(status) {
    case SENSOR_STATUS_NO_SERVICE:
        snprintf( buffer, sizeof(buffer), "KO: No sensor service found!\r\n" );
        break;
    case SENSOR_STATUS_DISABLED:
        snprintf( buffer, sizeof(buffer), "KO: '%s' sensor is disabled.\r\n", sensor );
        break;
    case SENSOR_STATUS_UNKNOWN:
        snprintf( buffer, sizeof(buffer),
                "KO: unknown sensor name: %s, run 'sensor status' to get available sensors.\r\n", sensor );
        break;
    default:
        snprintf( buffer, sizeof(buffer), "KO: '%s' sensor: exception happens.\r\n", sensor );
    }
    do_control_write( client, buffer );
    free( args_dup );
    return -1;

INPUT_ERROR:
    control_write( client, "KO: Usage: \"set <sensorname> <value-a>[:<value-b>[:<value-c>]]\"\n" );
    free( args_dup );
    return -1;
}

/* get all available sensor names and enable status respectively. */
static int
do_sensors_status( ControlClient client, char* args )
{
    uint8_t id, status;
    char buffer[SENSORS_INFO_SIZE] = { 0 };

    for(id = 0; id < MAX_SENSORS; id++) {
        status = android_sensors_get_sensor_status( id );
        snprintf( buffer, sizeof(buffer), "%s: %s\n",
                android_sensors_get_name_from_id(id), (status ? "enabled.":"disabled.") );
        control_write( client, buffer );
    }

    return 0;
}

/* Sensor commands for get/set sensor values and get available sensor names. */
static const CommandDefRec sensor_commands[] =
{
    { "status", "list all sensors and their status.",
      "'status': list all sensors and their status.\r\n",
      NULL, do_sensors_status, NULL },

    { "get", "get sensor values",
      "'get <sensorname>' returns the values of a given sensor.\r\n",
      NULL, do_sensors_get, NULL },

    { "set", "set sensor values",
      "'set <sensorname> <value-a>[:<value-b>[:<value-c>]]' set the values of a given sensor.\r\n",
      NULL, do_sensors_set, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};

/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                        F I N G E R P R I N T  C O M M A N D S                   ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static int
do_fingerprint_touch(ControlClient client, char* args )
{
    if (args) {
        char *endptr;
        int fingerid = strtol(args, &endptr, 0);
        if (endptr != args) {
            client->global->finger_agent->setTouch(true, fingerid);
            return 0;
        }
        control_write(client, "KO: invalid fingerid\r\n");
        return -1;
    }
    control_write(client, "KO: missing fingerid\r\n");
    return -1;
}

static int
do_fingerprint_remove(ControlClient client, char* args )
{
    client->global->finger_agent->setTouch(false, -1);
    return 0;
}

static const CommandDefRec fingerprint_commands[] =
{
    { "touch", "touch finger print sensor with <fingerid>",
      "'touch <fingerid>' touch finger print sensor with <fingerid>.\r\n",
      NULL, do_fingerprint_touch, NULL },
    { "remove", "remove finger from the fingerprint sensor",
      "'remove' remove finger from the fingerprint sensor.\r\n",
      NULL, do_fingerprint_remove, NULL },
    { NULL, NULL, NULL, NULL, NULL, NULL }
};

/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                           Q E M U   C O M M A N D S                             ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static int
do_qemu_monitor( ControlClient client, char* args )
{
    control_write(client, "KO: QEMU support no longer available\r\n");
    return -1;
}

static const CommandDefRec  qemu_commands[] =
{
    { "monitor", "enter QEMU monitor",
    "Enter the QEMU virtual machine monitor\r\n",
    NULL, do_qemu_monitor, NULL },

    { NULL, NULL, NULL, NULL, NULL, NULL }
};

#define HELP_COMMAND \
    { "help|h|?", "print a list of commands", NULL, NULL, do_help, NULL }

#define HELP_VERBOSE_COMMAND \
    {"help-verbose", "print a list of commands with descriptions", \
     NULL, NULL, do_help_verbose, NULL}

#define PING_COMMAND \
    { "ping", "check if the emulator is alive", NULL, NULL, do_ping, NULL }

#define QUIT_COMMAND \
    { "quit|exit", "quit control session", NULL, NULL, do_quit, NULL }

#define AVD_COMMAND(sub_commands)                                           \
    {                                                                       \
        "avd", "control virtual device execution",                          \
                "allows you to control (e.g. start/stop) the execution of " \
                "the virtual device\r\n",                                   \
                NULL, NULL, sub_commands                                    \
    }

/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                           M A I N   C O M M A N D S                             ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static void __attribute__((noreturn)) crash() {
    volatile int * ptr = NULL;
    *ptr+=1;
    abort();    // just to make the compiler happy
}

static int
do_crash( ControlClient  client, char*  args )
{
    control_write( client, "OK: crashing emulator, bye bye\r\n" );
    crash();
}

static int
do_crash_on_exit( ControlClient  client, char*  args )
{
    control_write( client, "OK: crashing emulator on exit, bye bye\r\n" );
    crashhandler_exitmode("console-simulated crash on exit");
    crash();
}

static int
do_kill( ControlClient  client, char*  args )
{
    control_write( client, "OK: killing emulator, bye bye\r\n" );
    DINIT("Emulator killed by console kill command.\n");
    fflush(stdout);
    client->global->libui_agent->requestExit(0, "Killed by console command");
    return 0;
}

static int do_debug(ControlClient client, char* args) {
    if (!args) {
        control_write(client, "KO: argument missing, try 'debug <tags>'\r\n");
        return -1;
    }
    if (!android_parse_debug_tags_option(args, /*parse_as_suffix*/false)) {
        control_write(client, "KO: bad tags, see command line help for "
                              """-debug"" option for a list of valid ones\r\n");
        return -1;
    }
    return 0;
}

static int do_rotate_90_clockwise(ControlClient client, char* args) {
    return (int)client->global->emu_agent->rotate90Clockwise();
}

/* NOTE: The names of all commands are listed when the 'help' command
 *       is received.
 *       Android Studio uses the 'help' command and requires that the
 *       response be less than 1024 characters.
 *       Do not expand the list to the point of breaking Android Studio.
 */
static const CommandDefRec main_commands[] = {
        HELP_COMMAND,

        HELP_VERBOSE_COMMAND,

        PING_COMMAND,

        {"event", "simulate hardware events",
         "allows you to send fake hardware events to the kernel\r\n", NULL,
         NULL, event_commands},

        {"geo", "Geo-location commands",
         "allows you to change Geo-related settings, or to send GPS NMEA "
         "sentences\r\n",
         NULL, NULL, geo_commands},

        {"gsm", "GSM related commands",
         "allows you to change GSM-related settings, or to make a new inbound "
         "phone call\r\n",
         NULL, NULL, gsm_commands},

        {"cdma", "CDMA related commands",
         "allows you to change CDMA-related settings\r\n", NULL, NULL,
         cdma_commands},

        {"crash", "crash the emulator instance", NULL, NULL, do_crash, NULL},

        {"crash-on-exit", "simulate crash on exit for the emulator instance",
         NULL, NULL,
         do_crash_on_exit},

        {"kill", "kill the emulator instance", NULL, NULL, do_kill, NULL},

        {"network", "manage network settings",
         "allows you to manage the settings related to the network data "
         "connection "
         "of the\r\n"
         "emulated device.\r\n",
         NULL, NULL, network_commands},

        {"power", "power related commands",
         "allows to change battery and AC power status\r\n", NULL, NULL,
         power_commands},

        QUIT_COMMAND,

        {"redir", "manage port redirections",
         "allows you to add, list and remove UDP and/or PORT redirection from "
         "the "
         "host to the device\r\n"
         "as an example, 'redir  tcp:5000:6000' will route any packet sent to "
         "the "
         "host's TCP port 5000\r\n"
         "to TCP port 6000 of the emulated device\r\n",
         NULL, NULL, redir_commands},

        {"sms", "SMS related commands",
         "allows you to simulate an inbound SMS\r\n", NULL, NULL, sms_commands},

        AVD_COMMAND(vm_commands),

        {"qemu", "QEMU-specific commands",
         "allows to connect to the QEMU virtual machine monitor\r\n", NULL,
         NULL, qemu_commands},

        {"sensor", "manage emulator sensors",
         "allows you to request the emulator sensors\r\n", NULL, NULL,
         sensor_commands},

        {"finger", "manage emulator finger print",
         "allows you to touch the emulator finger print sensor\r\n", NULL, NULL,
         fingerprint_commands},

        {"debug", "control the emulator debug output tags", NULL, NULL,
         do_debug},

        {"rotate", "rotate the screen clockwise by 90 degrees", NULL, NULL,
         do_rotate_90_clockwise, NULL},

        {NULL, NULL, NULL, NULL, NULL, NULL}};

/********************************************************************************************/
/********************************************************************************************/
/*****                                                                                 ******/
/*****                           A U T H   C O M M A N D S                             ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

static int do_auth(ControlClient client, char* args) {
    if (!args) {
        control_write(client, "KO: missing authentication token\r\n");
        return -1;
    }

    char* auth_token = android_console_auth_get_token_dup();
    if (!auth_token) {
        control_write(client,
                      "KO: unable to read ~/.emulator_console_auth_token\r\n");
        return -1;
    }

    if (0 != strcmp(auth_token, args)) {
        free(auth_token);
        control_write(client,
                      "KO: authentication token does not match "
                      "~/.emulator_console_auth_token\r\n");
        return -1;
    }
    free(auth_token);

    control_send_help_prompt(client);
    client->commands = main_commands;
    return 0;
}

static const CommandDefRec vm_commands_preauth[] = {
        {"name",
         "query virtual device name",
         "'avd name' will return the name of this virtual device\r\n",
         NULL,
         do_avd_name,
         NULL},

        {NULL, NULL, NULL, NULL, NULL, NULL}};

/* "preauth" commands are the set of commands that are legal before
 * authentication.  "avd name is special cased here because it is needed by
 * older versions of Android Studio */
static const CommandDefRec main_commands_preauth[] = {
    HELP_COMMAND,

    HELP_VERBOSE_COMMAND,

    PING_COMMAND,

    AVD_COMMAND(vm_commands_preauth),

    {"auth",
     "user authentication for the emulator console",
     "use 'auth <auth_token>' to get extended console functionality\r\n",
     NULL,
     do_auth,
     NULL},

    QUIT_COMMAND,

    {NULL, NULL, NULL, NULL, NULL, NULL}};

const char* android_console_auth_banner_get() {
    static char s_android_monitor_auth_banner[1024];
    static bool s_ready = false;

    if (!s_ready) {
        // in QEMU2, banners get \n appended when they are output so no trailing
        // \n here
        // Note that Studio looks for the first line of this message /exactly/
        char* emulator_console_auth_token_path =
                android_console_auth_token_path_dup();
        snprintf(s_android_monitor_auth_banner,
                 sizeof(s_android_monitor_auth_banner),
                 "Android Console: Authentication required\n"
                 "Android Console: type 'auth <auth_token>' to authenticate\n"
                 "Android Console: you can find your <auth_token> in "
                 "\n'%s'\nOK",
                 emulator_console_auth_token_path);
        free(emulator_console_auth_token_path);

        s_ready = true;
    }

    return s_android_monitor_auth_banner;
}

const char* android_console_help_banner_get() {
    // in QEMU2, banners get \n appended when they are output so no trailing \n
    // here
    return "Android Console: type 'help' for a list of commands"
           "\nOK";
}

/********************************************************************************************/
/********************************************************************************************/
/***** TESTING                                                                         ******/
/*****  The follow functions should only be used in testing.                           ******/
/*****                                                                                 ******/
/********************************************************************************************/
/********************************************************************************************/

void* test_control_client_create(Socket socket)
{
    ControlClient  client = calloc( sizeof(*client), 1 );

    if (client) {
        socket_set_nodelay( socket );
        socket_set_nonblock( socket );
        // Skip the preauth
        client->commands = main_commands;
        client->finished = 0;
        client->sock    = socket;
    }
    return client;
}

void test_control_client_close(void* opaque)
{
    free(opaque);
}

void send_test_string(void* opaque, const char* the_string)
{
    ControlClient c = (ControlClient)opaque;
    strcpy(c->buff, the_string);
    control_client_do_command(c);
}
