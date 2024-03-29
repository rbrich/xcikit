/*
Language: Fire Script
Author: Radek Brich <radek.brich@devl.cz>
Website: https://xci.cz/
*/

module.exports = function (hljs) {
  const KEYWORDS = {
    keyword: 'catch else fun if import instance in match module then try type with',
    literal: 'void false true',
    type: 'Void Bool Byte Char Int Int32 Int64 Float Float32 Float64 String',
    built_in: 'stdin stdout stderr null'
  }
  const TYPE_RE = /\b[A-Z][A-Za-z0-9_]*\b/
  const VAR_RE = /\b[a-z][A-Za-z0-9_]*\b/
  const TYPE = {
    className: 'type',
    begin: TYPE_RE,
    relevance: 0
  }
  const DEFINITION = {
    variants: [
      {
        begin: [
          VAR_RE,
          /\s*:\s*(?=([A-Za-z_()|,\s]|->)+\s*=)/,  // lookahead, e.g. " : Int ="
        ],
        contains: [
          TYPE,
        ],
        end: /=/,
      },
      {
        begin: [
          VAR_RE,
          /\s*=/,
        ],
      }
    ],
    beginScope: {
      1: "title.function"
    },
  }
  const CLASS_DEF = {
    begin: [
      VAR_RE,
      /\s*:/
    ],
    beginScope: {
      1: "title.function"
    },
    contains: [
      TYPE,
    ],
    end: /($|;)/,
  }
  const CLASS = {
    begin: [
      /\bclass\b/,
      /\s+/,
      TYPE_RE,
      /\s+(?=([A-Za-z_()|,\s]|->)+\{)/,  // lookahead, e.g. " T (Eq T) {"
    ],
    beginScope: {
      1: "keyword",
      3: "title.class",
    },
    end: /\}/,
    contains: [
      CLASS_DEF,
      TYPE,
    ],
  }
  const BUILTINS = {
    className: 'built_in',
    begin: '\\b__[A-Za-z_]+\\b'
  }
  const STRING = {
    className: 'string',
    contains: [hljs.BACKSLASH_ESCAPE],
    variants: [
      {
        begin: '"', end: '"',
        illegal: '\\n',
        contains: [hljs.BACKSLASH_ESCAPE],
      },
      {
        begin: '(b)?\'(' + hljs.BACKSLASH_ESCAPE + "|.)",
        end: '\'',
        illegal: '.'
      },
      {
        begin: '"""', end: '"""',
        contains: [hljs.BACKSLASH_ESCAPE],
      },
    ]
  }
  const NUMBERS = {
    className: 'number',
    variants: [
      { begin: '(-?)(\\b([\\d]+)([eE][-+]?[\\d]+)?)(f|F)' },  // float
      { begin: '(-?)(\\b(\\b[\\d]+\\.[\\d]*)([eE][-+]?[\\d]+)?)(f|F)?' },  // float
      { begin: '\\b(0b[01]+)((l|L)(u|U)?|(u|U)(l|L)?|b|B)?' },  // bin
      { begin: '(-?)\\b(0o[0-7]+|0x[a-fA-F0-9]+)((l|L)(u|U)?|(u|U)(l|L)?|b|B)?' },
      { begin: '(-?)\\b([\\d]+)((l|L)(u|U)?|(u|U)(l|L)?|b|B)?' }  // dec
    ],
    relevance: 0
  }
  const META_STATEMENT = {
    className: 'meta',
    begin: '#',
    end: '$'
  }
  const PROMPT = {
    className: 'meta',
    begin: /^_[0-9]+ \?/,
    relevance: 10
  }
  const PROMPT_VARIABLES = {
    className: 'variable',
    begin: '\\b_[0-9]*\\b'
  }

  return {
    name: 'Fire',
    keywords: KEYWORDS,
    contains: [
      hljs.C_LINE_COMMENT_MODE,
      hljs.C_BLOCK_COMMENT_MODE,
      DEFINITION,
      CLASS,
      TYPE,
      BUILTINS,
      NUMBERS,
      STRING,
      META_STATEMENT,
      PROMPT,
      PROMPT_VARIABLES
    ],
    unicodeRegex: true,
    disableAutodetect: true
  }
}
