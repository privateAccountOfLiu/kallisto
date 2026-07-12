#ifndef KALLISTO_GETOPT_PORT_H
#define KALLISTO_GETOPT_PORT_H

// Minimal GNU-style getopt_long for MSVC
//
// Supports: short options, long options, -- termination, optional_argument,
// flag pattern, and GNU-style argv permutation (non-option arguments moved
// to the end so optind points past all options).

#include <cstdio>
#include <cstring>

#define no_argument 0
#define required_argument 1
#define optional_argument 2

static char* optarg = nullptr;
static int optind = 1;
static int opterr = 1;
static int optopt = 0;

struct option {
    const char* name;
    int has_arg;
    int* flag;
    int val;
};

namespace {

int _getopt_isdash(const char* s) {
    return s && s[0] == '-' && s[1] != '\0';
}

int _getopt_is_doubledash(const char* s) {
    return s && s[0] == '-' && s[1] == '-' && s[2] == '\0';
}

// Swap two argv entries
void _getopt_swap(char** a, char** b) {
    char* tmp = *a;
    *a = *b;
    *b = tmp;
}

// Permute argv: move non-option at position 'pos' to the end of argv,
// shifting everything else left. Returns new pos (same index, now has the
// element that was at pos+1).
int _getopt_permute(int argc, char** argv, int pos) {
    char* nonopt = argv[pos];
    for (int i = pos; i < argc - 1; i++) {
        argv[i] = argv[i + 1];
    }
    argv[argc - 1] = nonopt;
    return pos; // same index now holds what used to be at pos+1
}

const option* _getopt_find_long(const char* name, const option* longopts) {
    for (const option* o = longopts; o->name != nullptr; o++) {
        const char* n = name;
        const char* on = o->name;
        while (*n && *on && *n == *on) { n++; on++; }
        if (*n == '\0' && *on == '\0') return o;
        if (*n == '=' && *on == '\0') return o;
    }
    return nullptr;
}

} // anonymous namespace

inline int getopt_long(
    int argc, char* const argv[],
    const char* optstring,
    const struct option* longopts,
    int* longindex)
{
    // GNU getopt permutes argv in place. main() always passes mutable pointers.
    char** nargv = const_cast<char**>(argv);

    if (optind >= argc) return -1;

    // Skip permuted non-options at the end
    const char* arg = nargv[optind];
    if (!_getopt_isdash(arg)) {
        // Non-option: permute it to the end
        // First find the real end (before previously permuted non-options)
        int first_nonopt = optind;
        while (first_nonopt < argc && nargv[first_nonopt] != nullptr) {
            first_nonopt++;
        }
        // If all remaining are non-options, we're done
        if (optind >= argc) return -1;
        // Check if there are any options left
        int has_options = 0;
        for (int i = optind + 1; i < argc; i++) {
            if (nargv[i] && _getopt_isdash(nargv[i])) { has_options = 1; break; }
        }
        if (!has_options) return -1;
        _getopt_permute(argc, nargv, optind);
        arg = nargv[optind];
        if (!_getopt_isdash(arg)) return -1;
    }

    if (_getopt_is_doubledash(arg)) {
        optind++;
        return -1;
    }

    // Long option: --foo or --foo=bar
    if (arg[0] == '-' && arg[1] == '-') {
        const char* name = arg + 2;
        const char* eq = strchr(name, '=');

        const option* o = _getopt_find_long(name, longopts);
        if (!o) {
            if (opterr) {
                fprintf(stderr, "Error: unrecognized option '--%s'\n", name);
            }
            optopt = 0;
            optind++;
            return '?';
        }

        if (longindex) *longindex = (int)(o - longopts);

        if (o->has_arg == required_argument) {
            if (eq) {
                optarg = (char*)(eq + 1);
            } else if (optind + 1 < argc) {
                optarg = nargv[++optind];
            } else {
                if (opterr) {
                    fprintf(stderr, "Error: option '--%s' requires an argument\n", o->name);
                }
                optopt = o->val;
                optind++;
                return (optstring[0] == ':') ? ':' : '?';
            }
        } else if (o->has_arg == optional_argument) {
            if (eq) {
                optarg = (char*)(eq + 1);
            } else {
                optarg = nullptr;
            }
        } else {
            if (eq) {
                if (opterr) {
                    fprintf(stderr, "Error: option '--%s' does not take an argument\n", o->name);
                }
                optind++;
                return '?';
            }
            optarg = nullptr;
        }

        if (o->flag) {
            *o->flag = o->val;
            optind++;
            return 0;
        }

        optind++;
        return o->val;
    }

    // Short option(s): -x or -xfoo
    const char* p = arg + 1;
    char c = *p++;

    const char* s = strchr(optstring, c);
    if (!s) {
        if (opterr) {
            fprintf(stderr, "Error: unrecognized option '-%c'\n", c);
        }
        optopt = c;
        // Skip the rest of combined short options after the bad one
        if (*p) optind++; // was combined like -xyz where x is bad; skip whole group
        else optind++;
        return '?';
    }

    optopt = c;

    if (s[1] == ':') {
        if (*p) {
            // -xARG where x takes an argument
            optarg = (char*)p;
        } else if (optind + 1 < argc) {
            // -x ARG
            optarg = nargv[++optind];
        } else {
            if (opterr) {
                fprintf(stderr, "Error: option '-%c' requires an argument\n", c);
            }
            optind++;
            return (optstring[0] == ':') ? ':' : '?';
        }
        optind++;
    } else {
        // No argument
        optarg = nullptr;
        if (*p) {
            // Combined short options like -xyz; push back remaining -yz as the
            // next argument to process by advancing optind to the re-inserted arg
            // Actually, GNU getopt handles this by leaving optind and resetting
            // an internal pointer. Simple approach: create a new argv entry.
            // But we can't modify argv easily. Instead, prepend a '-' to the
            // remaining chars and process them next time.
            //
            // Simplest approach that works: advance optind past current arg,
            // and on next call, process combined chars by shifting optind back.
            // OR: just don't support combined short options that well.
            //
            // Actually, the cleanest approach: decrement optind so that next
            // call re-processes with the remaining characters as a new arg.
            // We do this by replacing argv[optind] with "-remaining" and NOT
            // advancing optind.
            // But argv is char* const*, meaning we can modify the strings.
            // We need to build a new string. For simplicity, allocate from a
            // static buffer.
            static char combined_buf[256];
            combined_buf[0] = '-';
            size_t rem_len = strlen(p);
            if (rem_len > 253) rem_len = 253;
            memcpy(combined_buf + 1, p, rem_len);
            combined_buf[rem_len + 1] = '\0';
            nargv[optind] = combined_buf;
            // Don't advance optind; next call processes the remaining chars
        } else {
            optind++;
        }
    }

    return c;
}

#endif // KALLISTO_GETOPT_PORT_H
