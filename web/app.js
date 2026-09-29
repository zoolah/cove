const sampleProgram = `str greeting = "Hello from Cove!";
print(greeting);

num total = 0;
for i = 1, i < 6, i++ do
    total = total + i;
end
print("sum: " .. total);`;

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