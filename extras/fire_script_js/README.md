Fire Script JavaScript App
==========================

A web app which uses the Emscripten build of `tools/fire_script`.

## Requirements

* Emscripten
* Node.js, npm

## How to build

Make sure Emscripten build works, see [README](../../README.adoc#emscripten).

Install Xterm.js and Webpack:
```bash
npm install
````

Then run the build:
```bash
npm run build
````

This will build the Fire script tool for Emscripten and then run Webpack on `main.js`.

The webapp is now ready in `dist` directory.

Test it locally:
```bash
emrun --browser firefox dist/index.html
````

## Public URL

It runs here: https://fire.xci.cz
