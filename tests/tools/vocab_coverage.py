"""Checks that every word and phrase the parser accepts is in docs/vocabulary.md.

Phrases are collected from the parser's source: the statement table
(parse_stmt.c forms[]), the operator phrase tables (parse_expr.c), and the
words and phrases the parser looks for (parser_at_word, parser_match_words,
parser_expect_phrase, and NULL-terminated word lists). A phrase counts as
documented if it appears, as whole words, in a `code span` of the vocabulary
(ignoring case). Prints each
missing phrase and exits 1 if there are any.
"""
import glob, os, re, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

# Words the parser checks for only to explain a mistake, not to accept them.
ERROR_ONLY = {'greater', 'more', 'bigger', 'above', 'over', 'less', 'smaller', 'below', 'under'}

def phrases():
    found = set()
    for path in glob.glob(os.path.join(ROOT, 'src', 'front', 'parse_*.c')):
        text = open(path, encoding='utf-8').read()
        found.update(re.findall(r'\{"([^"]+)", parse_\w+,', text))                          # statement table
        found.update(re.findall(r'\{"[^"]+", "([^"]+)", (?:BINARY|CONVERT)_\w+', text))  # phrase tables (display)
        found.update(re.findall(r'parser_at_word\(p, \d+, "([^"]+)"\)', text))
        found.update(re.findall(r'parser_match_words\(p, "([^"]+)"\)', text))
        found.update(re.findall(r'parser_expect_phrase\(p, "[^"]+", "([^"]+)"', text))
        for words in re.findall(r'const char \*const \w+\[\] = \{([^}]*NULL)\}', text):
            found.update(re.findall(r'"([^"]+)"', words))
        for words in re.findall(r'operator_words\[\] = \{([^}]*)\}', text):
            found.update(re.findall(r'"([^"]+)"', words))
    return sorted(p for p in found if p not in ERROR_ONLY)

def documented(phrase, code_spans):
    """True if the phrase appears, as whole words, inside some `code span`."""
    words = r'\s+'.join(re.escape(w) for w in phrase.lower().split())
    pattern = re.compile(r'(?<![\w\'])' + words + r'(?![\w\'])')
    return any(pattern.search(span) for span in code_spans)

def main():
    vocabulary = open(os.path.join(ROOT, 'docs', 'vocabulary.md'), encoding='utf-8').read().lower()
    code_spans = re.findall(r'`([^`]+)`', vocabulary)
    missing = [p for p in phrases() if not documented(p, code_spans)]
    for p in missing:
        print(f'not in docs/vocabulary.md: "{p}"')
    return 1 if missing else 0

if __name__ == '__main__':
    sys.exit(main())
