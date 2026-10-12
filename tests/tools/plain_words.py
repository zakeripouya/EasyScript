"""Checks that messages beginners see are plain English (docs/voice.md).

- No banned programmer word (docs/english-pass.md's list) outside quotes in
  any expected .err file's messages and notes, or in any message string in
  src/ and runtime/ (the formats messages.py finds, plus every diag_note).
  EasyScript's own phrases for functions kept as values are allowed.
- Each error in an expected .err file (its message and notes together,
  without the EasyScript in quotes) reads at a Flesch-Kincaid grade of 8
  or below.
- The beginner docs get a reading-level report that doesn't fail.

Exits 1 and lists every problem if any is found.
"""
import glob, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import messages

ROOT = messages.ROOT
MAX_GRADE = 8.0
BANNED = ('variable function parameter argument return string boolean integer float array index null exception '
          'syntax compile runtime type object instance iterate initialize declare scope').split()
BANNED_RE = re.compile(r"\b(" + '|'.join(w + r"(?:s|es|d|ed|ing|r|rs)?" for w in BANNED) + r")\b", re.I)
# EasyScript's own words for functions kept as values (`a function taking x that gives back ...`).
ALLOWED = [re.compile(p, re.I) for p in (
    r"\b(?:a|the)? ?functions? (?:taking|giving back|made)\b",
    r"\ba function\b(?=\s*(?:[.,;:)]|$|or nothing))",
)]
BEGINNER_DOCS = ['README.md', 'docs/language-guide.md', 'docs/voice.md']


def unquote(text):
    """The text without the EasyScript inside quotes. Quotes can nest ("run "git" with ...")."""
    out, depth, i = [], 0, 0
    while i < len(text):
        c = text[i]
        if c != '"':
            if depth == 0:
                out.append(c)
            i += 1
            continue
        prev = text[i - 1] if i else ' '
        nxt = text[i + 1] if i + 1 < len(text) else ' '
        if nxt == '"':  # "" (empty text)
            out.append(' it ' if depth == 0 else '')
            i += 2
            continue
        opening = depth == 0 or not (prev.isalnum() or prev in ".!?)]%'")
        if depth > 0 and not nxt.isalnum() and nxt not in '(["':
            opening = False
        if opening:
            if depth == 0:
                out.append(' it ')
            depth += 1
        else:
            depth = max(0, depth - 1)
        i += 1
    return ''.join(out)


def banned_words(text):
    plain = unquote(text)
    for allowed in ALLOWED:
        plain = allowed.sub(' ', plain)
    return sorted({m.group(1).lower() for m in BANNED_RE.finditer(plain)})


def syllables(word):
    w = re.sub(r'[^a-z]', '', word.lower())
    if not w:
        return 0
    if w == 'easyscript':
        return 3
    if len(w) <= 3:
        return 1
    w = re.sub(r'(?:[^laeiouy]es|ed|[^laeiouy]e)$', '', w)
    w = re.sub(r'^y', '', w)
    return max(1, len(re.findall(r'[aeiouy]{1,2}', w)))


def grade(text):
    """Flesch-Kincaid grade level of the text, without the EasyScript in quotes."""
    t = unquote(text)
    sentences = max(1, len(re.findall(r'[.!?:;]+(?:\s|$)', t)))
    words = re.findall(r"[A-Za-z][A-Za-z']*", t)
    if not words:
        return 0.0
    syl = sum(syllables(w) for w in words)
    return 0.39 * len(words) / sentences + 11.8 * syl / len(words) - 15.59


HEADER = re.compile(r'^Line \d+(?: of \S+)?(?: \(a warning\))?: ')


def errors(path):
    """The errors in an expected .err file: each one's message and notes, without source lines, carets and call chains."""
    found, cur = [], []
    for line in open(path, encoding='utf-8', errors='replace'):
        line = line.rstrip('\n')
        if HEADER.match(line) or line.startswith('Memory check:'):
            if cur:
                found.append(cur)
            cur = [HEADER.sub('', line)]
        elif cur and line.strip() and not line.startswith('    ') and set(line.strip()) - set('^') and \
                not re.match(r'(while working out|called from|\.\.\.it called)', line):
            cur.append(line)
    if cur:
        found.append(cur)
    return found


def source_strings():
    """Every message string in the source: the formats messages.py finds, and every note and hint."""
    found = [(f'{path}:{line}', fmt) for path, line, fmt in messages.messages()]
    for folder in messages.SOURCES:
        for path in glob.glob(os.path.join(ROOT, folder, '**', '*.[ch]'), recursive=True):
            text = open(path, encoding='utf-8').read()
            for m in re.finditer(r'\b(?:diag_note|es_fail|fail)\(', text):
                args = messages.c_string_args(text, m.end())
                for arg in args:
                    value = messages.literal(arg)
                    if value:
                        line = text.count('\n', 0, m.start()) + 1
                        found.append((f'{os.path.relpath(path, ROOT)}:{line}', value))
            for m in re.finditer(r'fputs\(\s*((?:"(?:[^"\\]|\\.)*"\s*)+),\s*stderr', text):
                value = messages.literal(m.group(1))
                if value:
                    found.append((f'{os.path.relpath(path, ROOT)}:{text.count(chr(10), 0, m.start()) + 1}', value))
    return found


def doc_prose(path):
    """A doc's prose: no code blocks, inline code, tables or headings."""
    text, prose, code = open(path, encoding='utf-8').read(), [], False
    for line in text.split('\n'):
        if line.startswith('```'):
            code = not code
            continue
        if code or line.startswith(('|', '#')) or not line.strip():
            continue
        prose.append(re.sub(r'`[^`]*`', 'it', re.sub(r'\[([^\]]*)\]\([^)]*\)', r'\1', line)))
    return ' '.join(prose)


def main():
    problems = []
    seen = set()
    for path in sorted(glob.glob(os.path.join(ROOT, 'tests', '**', '*.err'), recursive=True) +
                       glob.glob(os.path.join(ROOT, 'examples', '**', '*.err'), recursive=True)):
        for error in errors(path):
            text = ' '.join(error)
            if text in seen:
                continue
            seen.add(text)
            where = os.path.relpath(path, ROOT)
            for line in error:
                words = banned_words(line)
                if words:
                    problems.append(f'{where}: uses {", ".join(words)}: {line}')
            g = grade(text)
            if g > MAX_GRADE:
                problems.append(f'{where}: grade {g:.1f} (over {MAX_GRADE:.0f}): {text}')
    for where, value in source_strings():
        words = banned_words(re.sub(r'%[-0-9.*]*(?:zu|s|d|c|g|f)', 'x', value))
        if words:
            problems.append(f'{where}: uses {", ".join(words)}: {value}')
    for doc in BEGINNER_DOCS:
        print(f'reading level of {doc}: grade {grade(doc_prose(os.path.join(ROOT, doc))):.1f} (reported, not checked)')
    for problem in problems:
        print(problem)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
