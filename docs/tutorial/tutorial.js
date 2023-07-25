console.log("tutorial.js load @ " + performance.now());

import { CodeJar } from "../assets/codejar.min.js";

const highlight = (editor) => {
  editor.textContent = editor.textContent
  hljs.highlightElement(editor)
}

//------------------------------------------------------------------------------

class LeftPanel {
  constructor(pane_div) {
    this.mod           = null;
    this.panel_div     = pane_div;
    this.source_header = pane_div.querySelector(".header_bar");
    this.source_pane   = pane_div.querySelector(".code_jar");
    this.source_jar    = new CodeJar(this.source_pane, highlight, {tab:"  "});
  }

  set_mod(mod) {
    this.mod = mod;
  }

  reload() {
    try {
      let filename = this.source_header.innerText;
      let src_contents = new TextDecoder().decode(this.mod.FS.readFile(filename));
      this.source_jar.updateCode(src_contents);
    } catch {}
  }

};

//------------------------------------------------------------------------------

class RightPanel {
  constructor(panel) {
    this.mod           = null;
    this.panel_div     = panel;
    this.input_header  = panel.querySelector(".input_header");
    this.input_pane    = panel.querySelector(".input_pane");
    this.output_header = panel.querySelector(".output_header");
    this.output_pane   = panel.querySelector(".output_pane");
    this.fitAddon      = null;
  }

  set_mod(mod) {
    this.mod = mod;
  }

  reload() {
    try {
      let filename = this.input_header.innerText;
      let input_contents = new TextDecoder().decode(this.mod.FS.readFile(filename));
      this.input_pane.innerText = input_contents;
    } catch {}
  }
};

//------------------------------------------------------------------------------

window.tutorials = [];

class Tutorial {
  constructor(div) {
    this.mod = null;
    this.div = div;
    this.stdout = "";
    this.stderr = "";
    this.ansi_up = new AnsiUp;

    this.left_panel  = new LeftPanel(div.querySelector(".left_panel"));
    this.right_panel = new RightPanel(div.querySelector(".right_panel"));
    this.compile_timeout = null;
    window.tutorials.push(this);
  }

  set_mod(mod) {
    this.mod = mod;

    this.left_panel.set_mod(mod);
    this.right_panel.set_mod(mod);

    this.right_panel.input_pane.oninput = ()=> {
      let filename = this.right_panel.input_header.innerText;
      let contents = this.right_panel.input_pane.innerText;
      this.mod.FS.writeFile(filename, contents);
      this.convert();
    };

    this.left_panel.reload();
    this.right_panel.reload();

    this.convert();
  }

  convert() {
    this.stdout = "";
    this.stderr = "";

    let args = [this.right_panel.input_header.innerText];
    let ret = this.mod.callMain(args);

    this.right_panel.output_header.style.backgroundColor = ret == 0 ? "#353" : "#533";
    this.right_panel.output_pane.innerHTML = this.ansi_up.ansi_to_html(this.stdout);
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

//------------------------------------------------------------------------------

load_tutorial("json_tut0a");
load_tutorial("json_tut1a");
load_tutorial("json_tut1b");
load_tutorial("json_tut1c");
load_tutorial("json_tut2a");
load_tutorial("json_tut2b");

//------------------------------------------------------------------------------

let code_boxes = document.querySelectorAll(".code_jar");
let code_box_jars = [];

console.log(code_boxes);

for(let c of code_boxes) {
  let jar = new CodeJar(c, highlight, {tab:"  "});
  code_box_jars.push(jar);
}

//------------------------------------------------------------------------------
