console.log("tutorial.js load @ " + performance.now());

import { CodeJar } from "../assets/codejar.min.js";

const highlight = (editor) => {
  editor.textContent = editor.textContent
  hljs.highlightElement(editor)
}

//------------------------------------------------------------------------------

class SourcePane {
  constructor(pane_div) {
    this.mod        = null;
    this.pane_div   = pane_div;
    this.header_bar = pane_div.querySelector(".header_bar");
    this.body_div   = pane_div.querySelector(".code_jar");
    this.code_jar   = null;

    if (this.body_div) {
      this.code_jar = new CodeJar(this.body_div, highlight, {tab:"  "});
    }
  }

  set_mod(mod) {
    this.mod = mod;
  }

  reload() {
    try {
      let filename = this.header_bar.innerText;
      let src_contents = new TextDecoder().decode(this.mod.FS.readFile(filename));
      this.code_jar.updateCode(src_contents);
    } catch {}
  }

};

//------------------------------------------------------------------------------

class TerminalPane {
  constructor(pane_div) {
    this.mod        = null;
    this.pane_div   = pane_div;
    this.header_bar = pane_div.querySelector(".header_bar");
    this.body_div   = pane_div.querySelector(".terminal");
    this.terminal   = null;

    if (this.body_div) {
      this.terminal = new Terminal({
        fontSize: 14,
        fontFamily: "monospace",
        convertEol: true,
        theme: {
          background: '#1E1E1E'
        },
        scrollback: 9999,
      });

      const fitAddon = new FitAddon.FitAddon();
      this.terminal.loadAddon(fitAddon);
      this.terminal.open(this.body_div);
      fitAddon.fit();
    }
  }

  set_mod(mod) {
    this.mod = mod;
  }

  reload() {
  }
};

//------------------------------------------------------------------------------

class Tutorial {
  constructor(div) {
    this.mod = null;
    this.div = div;
    this.stdout = "";
    this.stderr = "";

    this.source_pane = new SourcePane(div.querySelector("#source"));
    this.input_pane  = new SourcePane(div.querySelector("#input"));
    this.output_pane = new TerminalPane(div.querySelector("#output"));
    this.compile_timeout = null;

    //this.input_pane.code_jar.updateCode("-12345.6789e10 Hello World");
  }

  set_mod(mod) {
    this.mod = mod;

    this.source_pane.set_mod(mod);
    this.input_pane.set_mod(mod);
    this.output_pane.set_mod(mod);

    this.input_pane.code_jar.onUpdate(() => this.convert());

    this.source_pane.reload();
    this.input_pane.reload();
    this.output_pane.reload();

    this.convert();
  }

  convert() {
    //console.log("convert");
    this.stdout = "";
    this.stderr = "";

    let args = [this.input_pane.header_bar.innerText];
    let ret = this.mod.callMain(args);

    this.output_pane.header_bar.style.backgroundColor = ret == 0 ? "#353" : "#533";
    this.output_pane.terminal.clear();
    this.output_pane.terminal.write(this.stdout);
  }
}

//------------------------------------------------------------------------------

function load_tutorial(name) {
  let tutorial = new Tutorial(document.querySelector("#" + name));

  console.log("Importing " + name + " @ " + performance.now());
  import("./" + name + ".js").then((tutorial_mod) => {
    console.log("Initializing " + name + "_mod @ " + performance.now());
    var mod_options = {};
    mod_options.noInitialRun = true;
    mod_options.thisProgram = name;
    mod_options.print =    (text) => { tutorial.stdout = tutorial.stdout + text + "\n"; }
    mod_options.printErr = (text) => { tutorial.stderr = tutorial.stderr + text + "\n"; }
    mod_options.onRuntimeInitialized = function() {
      console.log(name + ".onRuntimeInitialized " + performance.now());
    }
    tutorial_mod.default(mod_options).then((mod) => {
      console.log("Initializing " + name + " @ " + performance.now());
      tutorial.set_mod(mod);
    });
  });
}

/*
const FS_global = FS;
const Module_cat = {
    preRun: () => {
        const FS = Module_cat.FS;
        FS.writeFile('test.txt', 'test');
        FS.mkdir('/global');
        FS.mount(FS_global, {}, '/global');
    },
};

cat(Module_cat).then(Module_ =>
{
    Module_.callMain([arg]);
});
*/



load_tutorial("json_tut1a");
load_tutorial("json_tut1b");
load_tutorial("json_tut1c");

//new Tutorial(document.querySelector("#json_tut1b"));

//new Tutorial(document.querySelector("#json_tut1c"));

//------------------------------------------------------------------------------
