"""Checks that every error message in the compiler and runtime is tested.

Each message format (see messages.py) must match the text of at least one
expected-error file (tests/*/*.err, examples/*/*.err) or a CLI check in
tests/run.sh. Messages that can only happen when the system itself fails
(out of memory, a process that can't start) are listed in EXEMPT with the
reason. Prints one line per untested message and exits 1 if there are any.
"""
import glob, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from messages import ROOT, messages

# Messages that need the operating system to fail, so no test can produce them.
EXEMPT = {
    'EasyScript ran out of memory.': 'needs memory to run out',
    'The program ran out of memory.': 'needs memory to run out',
    "I couldn't make a temporary folder in %s: %s.": 'needs the temp directory to be unwritable',
    "I couldn't write the C file: %s.": 'needs the temp directory to fail mid-run',
    "I couldn't start %s: %s.": 'needs fork() to fail',
    "I couldn't wait for %s: %s.": 'needs waitpid() to fail',
    "I couldn't run %s: %s.": 'needs exec to fail (no cc, or a deleted compiler)',
    "The C compiler couldn't build the program. This is a bug in EasyScript; please report it.": 'generated C always compiles (checked by fuzzing with -Werror)',
    "I couldn't set up the shell's answers file: %s.": 'needs the temp directory to fail',
}

def pattern(fmt):
    out, i = '', 0
    for m in re.finditer(r'%[-+ #0-9.*]*(?:zu|ld|lld|[sdxXuc])', fmt):
        out += re.escape(fmt[i:m.start()]) + '.+?'
        i = m.end()
    return re.compile(out + re.escape(fmt[i:]), re.S)

def tested_text():
    text = ''
    for path in glob.glob(os.path.join(ROOT, 'tests', '*', '*.err')) + glob.glob(os.path.join(ROOT, 'examples', '*', '*.err')):
        text += open(path, encoding='utf-8', errors='replace').read() + '\n'
    run_sh = open(os.path.join(ROOT, 'tests', 'run.sh'), encoding='utf-8').read()
    # CLI checks: the expected first line of stderr is a quoted argument of expect_failure
    for m in re.finditer(r'expect_failure "[^"]*" \d+ "((?:[^"\\]|\\.)*)"', run_sh):
        text += m.group(1).replace('\\"', '"') + '\n'
    return text

def main():
    text = tested_text()
    missing = 0
    for path, line, fmt in messages():
        if fmt in EXEMPT:
            continue
        if not pattern(fmt).search(text):
            print(f'untested: {path}:{line}: {fmt}')
            missing += 1
    return 1 if missing else 0

if __name__ == '__main__':
    sys.exit(main())
