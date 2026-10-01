const sampleProgram = `
function twice(x) 
  return x * 2; 
end

class Box { 
  num value; 
  Box(x) 
    this::value = x; 
  end 
  get() 
    return this::value; 
  end 
}
new Box(2) box;

tbl demo = { 
  fn = twice; 
  text = "Cove"; 
}

num n = demo.fn(box::get());

n = ++n + 5 % 2 - 1 / 1;
print(n++); 
print(--n); 
print(box::value);

if n ~= 0 and n > 1 or n == 0 then
 print(demo.text .. n); 
end

while n > 0 do 
  n--; 
end

for i = 0, i < 1, i++ do 
  print(i); 
end

n = "done"; 
print(n);
`;

const sourceEditor = document.querySelector("#source");
const lineNumbers = document.querySelector("#line-numbers");
const outputPanel = document.querySelector("#console-output");
const runButton = document.querySelector("#run-code");
let coveModule;

function updateLineNumbers() {
  const lineCount = sourceEditor.value.split("\n").length;
  lineNumbers.textContent = Array.from({ length: lineCount }, (_, index) => index + 1).join("\n");
  lineNumbers.scrollTop = sourceEditor.scrollTop;
}

function updateCursorPosition() {
  const beforeCursor = sourceEditor.value.slice(0, sourceEditor.selectionStart);
  const lines = beforeCursor.split("\n");
  document.querySelector("#cursor-position").textContent = `Ln ${lines.length}, Col ${lines.at(-1).length + 1}`;
}

function showOutput(text, isError = false) {
  outputPanel.replaceChildren();
  const output = document.createElement("p");
  output.textContent = text;
  if (isError) output.className = "console-error";
  outputPanel.append(output);
}

async function runProgram() {
  if (!coveModule || runButton.disabled) return;
  runButton.disabled = true;
  outputPanel.replaceChildren();

  try {
    const result = coveModule.runCove(sourceEditor.value);
    showOutput(result || "Program finished with no output.");
  } catch (error) {
    showOutput(error?.message || String(error), true);
  } finally {
    runButton.disabled = false;
  }
}

sourceEditor.value = sampleProgram;
updateLineNumbers();
updateCursorPosition();

sourceEditor.addEventListener("input", () => {
  updateLineNumbers();
  updateCursorPosition();
});
sourceEditor.addEventListener("scroll", () => { lineNumbers.scrollTop = sourceEditor.scrollTop; });
sourceEditor.addEventListener("click", updateCursorPosition);
sourceEditor.addEventListener("keyup", updateCursorPosition);

document.querySelector("#run-code").addEventListener("click", runProgram);
document.querySelector("#clear-output").addEventListener("click", () => {
  outputPanel.replaceChildren();
  const placeholder = document.createElement("p");
  placeholder.className = "console-placeholder";
  placeholder.textContent = "Console cleared.";
  outputPanel.append(placeholder);
});
document.addEventListener("keydown", event => {
  if ((event.ctrlKey || event.metaKey) && event.key === "Enter") {
    event.preventDefault();
    runProgram();
  }
});

if (typeof createCoveModule === "function") {
  createCoveModule({ locateFile: path => new URL(path, window.location.href).href })
    .then(module => {
      coveModule = module;
      runButton.disabled = false;
    })
    .catch(error => {
      showOutput(error?.message || String(error), true);
    });
} else {
  showOutput("Cove's WebAssembly files are not here yet. Run build-wasm.bat in this folder, then reload the page.");
}