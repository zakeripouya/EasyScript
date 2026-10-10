"""Lists every error message format in the compiler and runtime source.

Used by error_coverage.py. A message is the format string passed to one of
the error-reporting functions (diag_error, parser_error, es_fail, fail,
fprintf(stderr, "..."), usage_error). Adjacent C string literals are joined.
"""
import re, os, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SOURCES = ['src', 'runtime']
CALLS = {
    'diag_error': 2,    # diag_error(diag, span, FMT, ...)
    'parser_error': 2,  # parser_error(p, span, FMT, ...)
    'es_fail': 2,       # es_fail(line, hint, FMT, ...)
    'fail': 3,          # consteval fail(ev, span, hint, FMT, ...)
    'usage_error': 0,   # usage_error(FMT)
}

def c_string_args(text, start):
    """Splits the call's arguments starting after '(' at `start`; returns a list of raw argument texts."""
    args, depth, cur, i, in_str = [], 0, '', start, False
    while i < len(text):
        ch = text[i]
        if in_str:
            cur += ch
            if ch == '\\': cur += text[i + 1]; i += 1
            elif ch == '"': in_str = False
        elif ch == '"': in_str = True; cur += ch
        elif ch in '([{': depth += 1; cur += ch
        elif ch in ')]}':
            if depth == 0: args.append(cur.strip()); return args
            depth -= 1; cur += ch
        elif ch == ',' and depth == 0: args.append(cur.strip()); cur = ''
        else: cur += ch
        i += 1
    return args

def literal(arg):
    """The joined value of adjacent string literals, or None if the argument isn't only literals."""
    parts = re.findall(r'"((?:[^"\\]|\\.)*)"', arg)
    rest = re.sub(r'"((?:[^"\\]|\\.)*)"', '', arg).strip()
    if not parts or rest: return None
    return bytes(''.join(parts), 'utf-8').decode('unicode_escape')

def messages():
    found = []
    for folder in SOURCES:
        for dirpath, _, files in os.walk(os.path.join(ROOT, folder)):
            for f in files:
                if not f.endswith(('.c', '.h')): continue
                path = os.path.join(dirpath, f)
                text = open(path, encoding='utf-8').read()
                for name, index in CALLS.items():
                    for m in re.finditer(r'\b' + name + r'\(', text):
                        if text[max(0, m.start() - 7):m.start()].endswith(('void ', 'bool ')):  # a definition
                            continue
                        args = c_string_args(text, m.end())
                        if len(args) > index:
                            fmt = literal(args[index])
                            if fmt and fmt != '%s':
                                line = text.count('\n', 0, m.start()) + 1
                                found.append((os.path.relpath(path, ROOT), line, fmt))
                for m in re.finditer(r'fprintf\(stderr,\s*("(?:[^"\\]|\\.)*")', text):
                    fmt = literal(m.group(1)).rstrip('\n')
                    if fmt in ('%s', ''):
                        continue
                    line = text.count('\n', 0, m.start()) + 1
                    found.append((os.path.relpath(path, ROOT), line, fmt))
    return found

if __name__ == '__main__':
    for path, line, fmt in messages():
        print(f'{path}:{line}: {fmt}')
