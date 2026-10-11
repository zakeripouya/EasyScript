"""Checks that every C file uses standard names only with their header included.

macOS's headers include each other more than glibc's do (<stdlib.h> brings in
SIZE_MAX there), so a missing #include compiles on a Mac, even with Homebrew's
GCC, which uses the same headers, and only fails on Linux. For each file in
src/, runtime/, tools/ and tests/unit/, this follows its #include "..." chain
and checks that every name in NAMES it uses comes with one of the headers
that declare it. Functions count only when called (followed by "("), so a
field named `floor` is fine. Prints each miss and exits 1 if there are any.
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

TYPES = {  # types and macros: any use
    'SIZE_MAX': 'stdint.h', 'INT64_MAX': 'stdint.h', 'INT64_MIN': 'stdint.h', 'UINT64_MAX': 'stdint.h',
    'INT32_MAX': 'stdint.h', 'UINT32_MAX': 'stdint.h', 'UINT8_MAX': 'stdint.h', 'uint64_t': 'stdint.h inttypes.h',
    'int64_t': 'stdint.h inttypes.h', 'uint32_t': 'stdint.h inttypes.h', 'int32_t': 'stdint.h inttypes.h',
    'uint16_t': 'stdint.h inttypes.h', 'uint8_t': 'stdint.h inttypes.h', 'uintptr_t': 'stdint.h inttypes.h',
    'PRIu64': 'inttypes.h', 'PRId64': 'inttypes.h',
    'bool': 'stdbool.h', 'true': 'stdbool.h', 'false': 'stdbool.h',
    'size_t': 'stddef.h stdio.h stdlib.h string.h time.h wchar.h', 'NULL': 'stddef.h stdio.h stdlib.h string.h time.h',
    'ptrdiff_t': 'stddef.h', 'offsetof': 'stddef.h', 'max_align_t': 'stddef.h',
    'INT_MAX': 'limits.h', 'INT_MIN': 'limits.h', 'LONG_MAX': 'limits.h', 'LLONG_MAX': 'limits.h',
    'LLONG_MIN': 'limits.h', 'CHAR_BIT': 'limits.h', 'PATH_MAX': 'limits.h', 'UINT_MAX': 'limits.h',
    'DBL_MAX': 'float.h', 'DBL_EPSILON': 'float.h', 'DBL_DIG': 'float.h',
    'va_list': 'stdarg.h', 'errno': 'errno.h', 'ENOENT': 'errno.h', 'EEXIST': 'errno.h', 'EACCES': 'errno.h',
    'FILE': 'stdio.h', 'EOF': 'stdio.h', 'stdout': 'stdio.h', 'stderr': 'stdio.h', 'stdin': 'stdio.h',
    'EXIT_SUCCESS': 'stdlib.h', 'EXIT_FAILURE': 'stdlib.h',
    'NAN': 'math.h', 'INFINITY': 'math.h', 'HUGE_VAL': 'math.h',
    'time_t': 'time.h sys/types.h sys/stat.h', 'clock_t': 'time.h',
    'ssize_t': 'sys/types.h unistd.h stdio.h', 'pid_t': 'sys/types.h unistd.h',
}
CALLS = {  # functions and function-like macros: only when called
    'va_start va_end va_arg va_copy': 'stdarg.h',
    'memcpy memmove memset memcmp memchr strlen strcmp strncmp strchr strrchr strstr strerror strcpy strncpy': 'string.h',
    'malloc calloc realloc free exit abort getenv strtod strtol strtoll strtoul atoi qsort atexit mkdtemp': 'stdlib.h',
    'printf fprintf sprintf snprintf vsnprintf vfprintf fopen fclose fread fwrite fflush fputs fputc fgetc '
    'getchar putchar fgets perror remove rename setvbuf': 'stdio.h',
    'fabs floor ceil round trunc sqrt pow fmod fmax fmin log exp isnan isinf isfinite signbit nextafter': 'math.h',
    'time localtime gmtime mktime strftime difftime clock_gettime nanosleep': 'time.h',
    'isalpha isdigit isspace isalnum isupper islower tolower toupper isprint': 'ctype.h',
    'assert': 'assert.h',
}
NAMES = {name: headers.split() for name, headers in TYPES.items()}
FUNCTIONS = {name: headers.split() for names, headers in CALLS.items() for name in names.split()}


def strip(code):
    code = re.sub(r'/\*.*?\*/', ' ', code, flags=re.S)
    code = re.sub(r'//[^\n]*', ' ', code)
    code = re.sub(r'"(\\.|[^"\\\n])*"', '""', code)
    return re.sub(r"'(\\.|[^'\\\n])*'", "''", code)


def includes(path):
    text = open(path, encoding='utf-8').read()
    system = re.findall(r'#\s*include\s*<([^>]+)>', text)
    local = []
    for name in re.findall(r'#\s*include\s*"([^"]+)"', text):
        for base in (os.path.dirname(path), os.path.join(ROOT, 'src'), os.path.join(ROOT, 'runtime')):
            candidate = os.path.normpath(os.path.join(base, name))
            if os.path.exists(candidate):
                local.append(candidate)
                break
    return system, local


def reachable(path, seen):
    if path in seen:
        return set()
    seen.add(path)
    system, local = includes(path)
    found = set(system)
    for other in local:
        found |= reachable(other, seen)
    return found


def main():
    files = []
    for pattern in ('src/**/*.[ch]', 'runtime/*.h', 'tools/*.c', 'tests/unit/*.[ch]'):
        files += glob.glob(os.path.join(ROOT, pattern), recursive=True)
    missing = []
    for path in sorted(files):
        have = reachable(path, set())
        code = strip(open(path, encoding='utf-8').read())
        for table, after in ((NAMES, ''), (FUNCTIONS, r'\s*\(')):
            for name, headers in table.items():
                if any(h in have for h in headers):
                    continue
                if re.search(r'(?<![\w.>])' + re.escape(name) + r'\b' + after, code):
                    missing.append(f'{os.path.relpath(path, ROOT)}: uses {name} without <{headers[0]}>')
    for line in missing:
        print(line)
    return 1 if missing else 0


if __name__ == '__main__':
    sys.exit(main())
