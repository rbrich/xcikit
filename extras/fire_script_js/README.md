Fire Script JavaScript App
==========================

A web app which uses the Emscripten build of `tools/fire_script`.

It runs here: https://fire.xci.cz

## Build requirements

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

This will build the Fire script tool with Emscripten and then run Webpack on `main.js`.

The web app is now ready in `dist` directory.

Test it locally:
```bash
emrun --browser firefox dist/index.html
````

To serve it, copy the content of `dist` to a web server and point some URL to it.
It's completely client-side web application, so it can be served easily by Nginx, Apache etc.

## URL parameters

* `debug=1` Enable debug logging.
