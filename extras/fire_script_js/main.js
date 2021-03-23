import { Terminal } from 'xterm';
import { FitAddon } from 'xterm-addon-fit';
import { WebLinksAddon } from 'xterm-addon-web-links';

fire_script().then(Module => {
    const termContainer = document.getElementById('terminal');
    const termLogContainer = document.getElementById('terminal_log');

    const term = new Terminal({
        convertEol: true,
        rightClickSelectsWord: false,
        theme: {
            foreground: '#999',
        }
    });
    term.open(termContainer);

    const termLog = new Terminal({
        disableStdin: true,
        convertEol: true,
        rightClickSelectsWord: false,
        theme: {
            background: '#111',
            cursor: '#111',
        }
    });
    termLog.open(termLogContainer);

    const termFit = new FitAddon();
    term.loadAddon(termFit);
    termFit.fit();

    const termLogFit = new FitAddon();
    termLog.loadAddon(termLogFit);
    termLogFit.fit();

    window.onresize = function () {
        termFit.fit();
        termLogFit.fit();
    };

    term.loadAddon(new WebLinksAddon());

    const url_args = new URLSearchParams(window.location.search);
    const debug = Boolean(parseInt(url_args.get('debug')));
    const prog = new Module.FireScript(debug);
    prog.set_term_out_cb(data => term.write(data));
    prog.set_term_err_cb(data => termLog.write(data));
    prog.set_quit_cb(function() {
        // try to actually quit, it may work in some circumstances
        window.close();
        // inform it didn't work otherwise
        term.write("Cannot quit the web app.\nHint: Try to close the window instead.\n");
    });
    prog.repl_init();
    prog.repl_prompt();
    term.onData(data => prog.repl_step(data));
});
