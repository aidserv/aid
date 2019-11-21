#include "android/cmdline-option.h"
#include "android/constants.h"
#include "android/utils/debug.h"
#include "android/utils/misc.h"
#include "android/utils/system.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

const AndroidOptions* android_cmdLineOptions = NULL;

#define  _VERBOSE_TAG(x,y)   { #x, VERBOSE_##x, y },
static const struct { const char*  name; int  flag; const char*  text; }
debug_tags[] = {
    VERBOSE_TAG_LIST
    { 0, 0, 0 }
};

void  parse_env_debug_tags( void );

enum {
    OPTION_IS_FLAG = 0,
    OPTION_IS_PARAM,
    OPTION_IS_LIST,
};

typedef struct {
    const char*  name;
    int          var_offset;
    int          var_type;
    int          var_is_config;
} OptionInfo;

#define  OPTION(_name,_type,_config)  \
    { #_name, offsetof(AndroidOptions,_name), _type, _config },


static const OptionInfo  option_keys[] = {
#define  OPT_FLAG(_name,_descr)             OPTION(_name,OPTION_IS_FLAG,0)
#define  OPT_PARAM(_name,_template,_descr)  OPTION(_name,OPTION_IS_PARAM,0)
#define  OPT_LIST(_name,_template,_descr)   OPTION(_name,OPTION_IS_LIST,0)
#define  CFG_FLAG(_name,_descr)             OPTION(_name,OPTION_IS_FLAG,1)
#define  CFG_PARAM(_name,_template,_descr)  OPTION(_name,OPTION_IS_PARAM,1)
#include "android/cmdline-options.h"
    { NULL, 0, 0, 0 }
};

int
android_parse_options( int  *pargc, char**  *pargv, AndroidOptions*  opt )
{
    int     nargs = *pargc-1;
    char**  aread = *pargv+1;
    char**  awrite = aread;

    memset( opt, 0, sizeof *opt );

    while (nargs > 0) {
        char*  arg;
        char   arg2_tab[64], *arg2 = arg2_tab;

        /* process @<name> as a special exception meaning
         * '-avd <name>'
         */
        if (aread[0][0] == '@') {
            opt->avd = aread[0] + 1;
            nargs--;
            aread++;
            continue;
        }

        /* anything that isn't an option past this points
         * exits the loop
         */
        if (aread[0][0] != '-') {
            break;
        }

        arg = aread[0]+1;

        /* an option cannot contain an underscore */
        if (strchr(arg, '_') != NULL) {
            break;
        }

        nargs--;
        aread++;

        /* for backwards compatibility with previous versions */
        if (!strcmp(arg, "verbose")) {
            arg = "debug-init";
        }

        /* special handing for -debug <tags> and -debug-<tags> */
        if (!strcmp(arg, "debug")) {
            if (nargs == 0) {
                derror( "-debug must be followed by tags (see -help-verbose)\n");
                return -1;
            }
            nargs--;
            android_parse_debug_tags_option(*aread++, /*parse_as_suffix*/false);
            continue;
        } else if (!strncmp(arg, "debug-", sizeof("debug-") - 1)) {
            // For a "-debug-<tag>" option we only allow a single tag,
            // otherwise it may look strange to someone.
            android_parse_debug_tags_option(arg + sizeof("debug-") - 1, /*parse_as_suffix*/true);
            continue;
        }

        /* NOTE: variable tables map option names to values
         * (e.g. field offsets into the AndroidOptions structure).
         *
         * however, the names stored in the table used underscores
         * instead of dashes. this means that the command-line option
         * '-foo-bar' will be associated to the name 'foo_bar' in
         * this table, and will point to the field 'foo_bar' or
         * AndroidOptions.
         *
         * as such, before comparing the current option to the
         * content of the table, we're going to translate dashes
         * into underscores.
         */
        arg2 = arg2_tab;
        buffer_translate_char( arg2_tab, sizeof(arg2_tab),
                               arg, '-', '_');

        /* look into our table of options
         *
         */
        {
            const OptionInfo*  oo = option_keys;

            for ( ; oo->name; oo++ ) {
                if ( !strcmp( oo->name, arg2 ) ) {
                    void*  field = (char*)opt + oo->var_offset;

                    if (oo->var_type != OPTION_IS_FLAG) {
                        /* parameter/list option */
                        if (nargs == 0) {
                            derror( "-%s must be followed by parameter (see -help-%s)",
                                    arg, arg );
                            exit(1);
                        }
                        nargs--;

                        if (oo->var_type == OPTION_IS_PARAM)
                        {
                            ((char**)field)[0] = strdup(*aread++);
                        }
                        else if (oo->var_type == OPTION_IS_LIST)
                        {
                            ParamList**  head = (ParamList**)field;
                            ParamList*   pl;
                            ANEW0(pl);
                            /* note: store list items in reverse order here
                             *       the list is reversed later in this function.
                             */
                            pl->param = strdup(*aread++);
                            pl->next  = *head;
                            *head     = pl;
                        }
                    } else {
                        /* flag option */
                        ((int*)field)[0] = 1;
                    }
                    break;
                }
            }

            if (oo->name == NULL) {  /* unknown option ? */
                nargs++;
                aread--;
                break;
            }
        }
    }

    /* copy remaining parameters, if any, to command line */
    *pargc = nargs + 1;

    while (nargs > 0) {
        awrite[0] = aread[0];
        awrite ++;
        aread  ++;
        nargs  --;
    }

    awrite[0] = NULL;

    /* reverse any parameter list before exit.
     */
    {
        const OptionInfo*  oo = option_keys;

        for ( ; oo->name; oo++ ) {
            if ( oo->var_type == OPTION_IS_LIST ) {
                ParamList**  head = (ParamList**)((char*)opt + oo->var_offset);
                ParamList*   prev = NULL;
                ParamList*   cur  = *head;

                while (cur != NULL) {
                    ParamList*  next = cur->next;
                    cur->next = prev;
                    prev      = cur;
                    cur       = next;
                }
                *head = prev;
            }
        }
    }

    return 0;
}



/* special handling of -debug option and tags */
#define  ENV_DEBUG   "ANDROID_DEBUG"

// Checks if a character range [tagStart, tagEnd) represents a debug tag |to|.
// |tagStart| to |tagEnd| is assumed to be a substring of a zero-terminated C
//  string, with no \0 characters in the middle.
static bool is_tag_equal(const char* tagStart, const char* tagEnd,
                         const char* to) {
    const size_t tagLen = tagEnd - tagStart;
    if (strncmp(to, tagStart, tagLen)) {
        return false;
    }

    // Can't do this before the strncmp() call as |to| might be not long enough.
    if (to[tagLen] != 0) {
        return false;
    }
    return true;
}

bool android_parse_debug_tags_option(const char* opt, bool parse_as_suffix) {
    if (opt == NULL)
        return false;

    bool result = false;
    const char* x = opt;
    while (*x) {
        const char* y = parse_as_suffix ? (x + strlen(x)) : strchr(x, ',');
        if (y == NULL)
            y = x + strlen(x);

        if (y > x+1) {
            int remove = 0;
            const char* x0 = x; // in case we need to print it out

            if (!parse_as_suffix && x[0] == '-') {
                remove = 1;
                x += 1;
            } else if (!strncmp(x, "no-", 3)) {
                remove = 1;
                x += 3;
            }

            if (is_tag_equal(x, y, "all")) {
                result = true;
                remove
                        ? base_disable_verbose_logs()
                        : base_enable_verbose_logs();
            } else {
                char  temp[32];
                buffer_translate_char_with_len(temp, sizeof temp,
                                               x, y - x, '-', '_');

                int nn;
                uint64_t mask = 0;
                for (nn = 0; debug_tags[nn].name != NULL; nn++) {
                    if (is_tag_equal(temp, temp + (y - x), debug_tags[nn].name)) {
                        mask |= (1ULL << debug_tags[nn].flag);
                        break;
                    }
                }

                if (mask == 0) {
                    dprint("ignoring unknown debug item '%.*s'",
                           (int)(y - x0), x0);
                } else {
                    result = true;
                    if (remove)
                        android_verbose &= ~mask;
                    else
                        android_verbose |= mask;
                }
            }
        }
        // Skip the comma, but don't jump over the terminating zero.
        x = *y ? y + 1 : y;
    }

    return result;
}

void
parse_env_debug_tags( void )
{
    const char*  env = getenv( ENV_DEBUG );
    android_parse_debug_tags_option(env, /*parse_as_suffix*/false);
}

bool android_validate_ports(int console_port, int adb_port) {
    bool result = true;

    int lower_bound = ANDROID_CONSOLE_BASEPORT  + 1;
    int upper_bound = lower_bound + (MAX_ANDROID_EMULATORS - 1) * 2 + 1;
    if (adb_port < lower_bound || adb_port > upper_bound) {
        dwarning("Requested adb port (%d) is outside the recommended range "
                 "[%d,%d]. ADB may not function properly for the emulator. See "
                 "-help-port for details.",
                 adb_port, lower_bound, upper_bound);
        result = false;
    } else if (adb_port % 2 == 0) {
        dwarning("Requested adb port (%d) is an even number. ADB may not "
                 "function properly for the emulator. See -help-port for "
                 "details.", adb_port);
        result = false;
    }

    return result;
}

bool android_parse_port_option(const char* port_string,
                               int* console_port,
                               int* adb_port) {
    if (port_string == NULL) {
        return false;
    }

    char* end;
    errno = 0;
    int port = strtol(port_string, &end, 0);
    if (end == NULL || *end || errno || port < 1 || port > UINT16_MAX) {
        derror("option -port must be followed by an integer. "
               "'%s' is not a valid input.", port_string);
        return false;
    }

    *console_port = port;
    *adb_port = *console_port + 1;
    dprint("Requested console port %d: Inferring adb port %d.",
           *console_port, *adb_port);
    return true;
}


bool android_parse_ports_option(const char* ports_string,
                                int* console_port,
                                int* adb_port) {
    if (ports_string == NULL) {
        return false;
    }

    char* comma_location;
    char* end;
    int first_port = strtol(ports_string, &comma_location, 0);
    if (comma_location == NULL || *comma_location != ',' ||
        first_port < 1 || first_port > UINT16_MAX) {
        derror("Failed to parse option: |%s| (Could not parse first port). "
               "See -help-ports.", ports_string);
        return false;
    }

    int second_port = strtol(comma_location+1, &end, 0);
    if (end == NULL || *end || second_port < 1 || second_port > UINT16_MAX) {
        derror("Failed to parse option: |%s|. (Could not parse second port). "
               "See -help-ports.", ports_string);
        return false;
    }
    if (first_port == second_port) {
        derror("Failed to parse option: |%s|. (Both ports are identical). "
               "See -help-ports.", ports_string);
        return false;
    }

    *console_port = first_port;
    *adb_port = second_port;
    return true;
}
