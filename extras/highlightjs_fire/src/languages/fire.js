/*
Language: Fire Script
Author: Radek Brich <radek.brich@devl.cz>
Website: https://xci.cz/
*/

module.exports = function (hljs) {
  const KEYWORDS = {
    keyword: 'catch class else fun if import instance in match module then try type with',
    literal: 'void false true',
    type: 'Void Bool Byte Char Int Int32 Int64 Float Float32 Float64 String',
    built_in: 'stdin stdout stderr null'
  }
  const DEFINITIONS = {
    begin: [
      /[a-z][a-zA-Z_]*/,
      /\s+/,
      /[=:]/,
    ],
    className: {
      1: "title.function"
    }
  }
  const TYPES = {
    className: 'type',
    begin: '\\b[A-Z][A-Za-z_]*\\b',
    relevance: 0
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
    aliases: ['fire'],
    keywords: KEYWORDS,
    contains: [
      hljs.C_LINE_COMMENT_MODE,
      hljs.C_BLOCK_COMMENT_MODE,
      DEFINITIONS,
      TYPES,
      BUILTINS,
      NUMBERS,
      STRING,
      META_STATEMENT,
      PROMPT,
      PROMPT_VARIABLES
    ],
    disableAutodetect: true
  }
}
