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

fire_script().then(Module => {
    term.clear();
    term.write("\x1b[J");  // clear screen from cursor down
    const url_args = new URLSearchParams(window.location.search);
    const debug = Boolean(parseInt(url_args.get('debug')));
    const prog = new Module.FireScript(debug);
    prog.set_term_out_cb(data => term.write(data));
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
