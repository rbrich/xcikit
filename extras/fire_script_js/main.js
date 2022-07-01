import { Terminal } from 'xterm';
import { FitAddon } from 'xterm-addon-fit';
import { WebLinksAddon } from 'xterm-addon-web-links';

const termContainer = document.getElementById('terminal');

const term = new Terminal({
    convertEol: true,
    rightClickSelectsWord: false,
    theme: {
        foreground: '#999',
    }
});
term.open(termContainer);

// fit terminal size to window size
const termFit = new FitAddon();
term.loadAddon(termFit);
termFit.fit();
window.onresize = function () {
    termFit.fit();
};

term.loadAddon(new WebLinksAddon());

term.write("Loading...\n");

createModule({}).then(Module => {
    term.write("\r\x1b[A\x1b[2J");  // clear viewport
    const url_args = new URLSearchParams(window.location.search);
    const a_debug = Boolean(parseInt(url_args.get('debug')));
    const a_input = url_args.get('input');
    const prog = new Module.FireScript(a_debug);
    prog.set_term_out_cb(data => term.write(data));
    prog.set_quit_cb(function() {
        // try to actually quit, it may work in some circumstances
        window.close();
        // inform it didn't work otherwise
        term.write("Cannot quit the web app.\nHint: Try to close the window instead.\n");
    });
    prog.set_sync_history_cb(Module.syncFS);
    prog.repl_init();
    prog.repl_prompt();
    if (a_input) {
        prog.repl_step(a_input);
    }
    term.onData(data => prog.repl_step(data));
});
