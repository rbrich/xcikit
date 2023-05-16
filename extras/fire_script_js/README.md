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
```

Then run the build:
```bash
export CONAN_DEFAULT_PROFILE=emscripten
npm run build
```

This will build the Fire script tool with Emscripten and then run Webpack on `main.js`.

The web app is now ready in `dist` directory.

Test it locally:
```bash
emrun --browser firefox dist/v*/index.html
```

## Serving with NGINX

To serve the app, copy the content of `dist` to a web server and point some URL to it.
It's a client-side web application, so the server can be completely static.

Example NGINX config:

```
server {
    listen 80;
    listen 443 ssl;
    server_name fire.xci.cz;

    # redirect / -> /latest/
    location = / {
        return 301 $scheme://$host/latest/;
    }

    # redirect /latest -> /latest/, /v1.0.0 -> /v1.0.0/
    location ~ ^/([^/]+)$ {
        return 301 $scheme://$host$uri/;
    }

    # lookup files in root
    location / {
        root /var/www/html/fire.xci.cz;
        gzip_static on;
        disable_symlinks if_not_owner;
    }
}
```

This expects that you have a symlink named `latest` which points to latest `vX.Y.Z` directory.

GZIP files should be generated statically:
```bash
gzip -k9 -r *
```

## URL parameters

* `debug=1` Enable debug logging.
* `input=<line>` Feed this input to Fire script. Can contain a newline (`%0a`) to immediately execute the code.
