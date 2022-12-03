# -*- coding: utf-8 -*-
"""
    Pygments lexer for Fire Script
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Based on: pygments.lexers.c_like

    https://pygments.org/docs/lexerdevelopment/
    https://pygments.org/docs/tokens/

    :copyright: Copyright 2006-2019 by the Pygments team, see AUTHORS.
    :license: BSD, see LICENSE for details.
"""

from pygments.lexer import RegexLexer, include, bygroups, inherit, words, \
    default
from pygments.token import Text, Comment, Operator, Keyword, Name, String, \
    Number, Punctuation, Error

from pygments.lexers.c_cpp import CFamilyLexer

import re

__all__ = ['FireLexer']


class FireLexer(CFamilyLexer):
    """
    For `Fire Script <https://github.com/rbrich/xcikit>`_.

    .. versionadded:: 2.0
    """
    name = 'Fire'
    aliases = ('fire',)
    filenames = ('*.fire',)
    mimetypes = ('text/x-fire-script',)

    flags = re.MULTILINE | re.UNICODE

    tokens = {
        'keywords': [
            (words(('void', 'false', 'true'), suffix=r'\b'),
             Keyword.Constant),
            (words(('stdin', 'stdout', 'stderr', 'null'), suffix=r'\b'),
             Name.Builtin),
            (words((
                'catch', 'class', 'else', 'fun', 'if', 'import',
                'instance', 'in', 'match', 'module', 'then', 'try', 'type', 'with',
            ), suffix=r'\b'),
             Keyword.Reserved)
        ],
        'types': [
            (words(('Void', 'Bool', 'Byte', 'Char', 'Int', 'Int32', 'Int64',
                    'Float', 'Float32', 'Float64', 'String'), suffix=r'\b'),
             Keyword.Type)
        ],
        'raw_string': [
            (r'"""', String.Double, '#pop'),
            (r'\\"""+', String.Escape),
            (r'\\', String.Double),  # backslash must be parsed one at a time
            (r'[^\\"]+', String.Double),  # other characters, including newlines
        ],
        'statements': [
            include('keywords'),
            include('types'),
            (r'(b?)(""")', bygroups(String.Affix, String.Double), 'raw_string'),
            (r'(b?)(")', bygroups(String.Affix, String.Double), 'string'),
            (r"(b?)(')(\\.|\\[0-7]{1,3}|\\x[a-fA-F0-9]{1,2}|[^\\\'\n])(')",
             bygroups(String.Affix, String.Char, String.Char, String.Char)),
            (r'(\d+\.\d*|\.\d+|\d+)[eE][+-]?\d+[LlUu]*', Number.Float),
            (r'(\d+\.\d*|\.\d+|\d+[fF])[fF]?', Number.Float),
            (r'0x[0-9a-fA-F]+[LlUuBb]*', Number.Hex),
            (r'0o[0-7]+[LlUuBb]*', Number.Oct),
            (r'0b[01]+[LlUuBb]*', Number.Bin),
            (r'\d+[LlUuBb]*', Number.Integer),
            (r'\*/', Error),
            (r'[~!%^&*+=|?:<>/-]', Operator),
            (r'[()\[\],.]', Punctuation),
            (r'([a-zA-Z_]\w*)(\s*)(:)(?!:)', bygroups(Name.Label, Text, Punctuation)),
            (r'[a-z_]\w*', Name),
            (r'[A-Z]\w*', Name.Class),
        ],
    }
