import { Terminal } from 'xterm';
import { FitAddon } from 'xterm-addon-fit';
import { WebLinksAddon } from 'xterm-addon-web-links';

fire_script().then(Module => {
    const term_container = document.getElementById('terminal');

    const term = new Terminal({
        convertEol: true,
        rightClickSelectsWord: false
    });
    term.open(term_container);

    const fitAddon = new FitAddon();
    term.loadAddon(fitAddon);
    fitAddon.fit();
    window.onresize = function () { fitAddon.fit(); };

    term.loadAddon(new WebLinksAddon());

    const prog = new Module.FireScript();
    prog.set_term_output_cb(data => term.write(data));
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
