Fire Script syntax for Highlight.js
===================================

## Build requirements

* Node.js, npm

## How to build

See [Highlight.js 3rd-party quickstart](https://github.com/highlightjs/highlight.js/blob/main/extra/3RD_PARTY_QUICK_START.md)

* Clone https://github.com/highlightjs/highlight.js
* Copy `src`, `test` dirs into `highlight.js/extra/fire`
* Run `npm install` (in highlight.js)
* Run `node tools/build.js -t cdn` (still in highlight.js)
* Pick `extra/fire/dist/fire.min.js` (a copy of it is committed here in dist)

Embed fire.js in the highlight.js dist, together with some core languages:
```shell
node tools/build.js -t cdn fire cpp haskell rust
```

A copy of highlight.js with embedded `fire` language support is hosted at:
https://doc.xci.cz/lib/highlightjs/highlight.min.js

## Development

As above, but run tests instead of cdn build:

```shell
node ./tools/build.js -t node
env ONLY_EXTRA=true npm run test-markup
```

Reference:
* https://highlightjs.readthedocs.io/en/latest/language-guide.html
* https://highlightjs.readthedocs.io/en/latest/css-classes-reference.html
