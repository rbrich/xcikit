import { Terminal } from 'xterm';
import { FitAddon } from 'xterm-addon-fit';

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

    const prog = new Module.FireScript();
    prog.set_term_output_cb(data => term.write(data));
    prog.repl_init();
    prog.repl_prompt();
    term.onData(data => prog.repl_step(data));
});
