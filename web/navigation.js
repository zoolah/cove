const pageSwitcher = document.querySelector(".page-switcher");
const pageSwitcherToggle = document.querySelector("#page-switcher-toggle");
const pageSwitcherMenu = document.querySelector("#page-switcher-menu");

function closePageSwitcher() {
  pageSwitcherMenu.hidden = true;
  pageSwitcherToggle.setAttribute("aria-expanded", "false");
  pageSwitcher.classList.remove("is-open");
}

pageSwitcherToggle.addEventListener("click", () => {
  pageSwitcherMenu.hidden = !pageSwitcherMenu.hidden;
  pageSwitcherToggle.setAttribute("aria-expanded", String(!pageSwitcherMenu.hidden));
  pageSwitcher.classList.toggle("is-open", !pageSwitcherMenu.hidden);
});

function sizePageSwitcher() {
  const canvas = document.createElement("canvas");
  const context = canvas.getContext("2d");
  const buttonStyle = getComputedStyle(pageSwitcherToggle);
  const option = pageSwitcherMenu.querySelector("a");
  const arrowWidth = pageSwitcherToggle.querySelector("svg").getBoundingClientRect().width;
  const horizontalPadding = parseFloat(buttonStyle.paddingLeft) + parseFloat(buttonStyle.paddingRight);

  context.font = buttonStyle.font;
  const labelWidth = Math.max(
    context.measureText(pageSwitcherToggle.querySelector("span").textContent).width,
    context.measureText(option.textContent).width
  );
  const contentWidth = Math.ceil(labelWidth + arrowWidth + parseFloat(buttonStyle.gap) + horizontalPadding);
  pageSwitcher.style.setProperty("--page-switcher-width", `${contentWidth}px`);
}

sizePageSwitcher();
document.fonts?.ready.then(sizePageSwitcher);
window.addEventListener("resize", sizePageSwitcher, { passive: true });

document.addEventListener("click", event => {
  if (!pageSwitcher.contains(event.target)) closePageSwitcher();
});

document.addEventListener("keydown", event => {
  if (event.key === "Escape") {
    closePageSwitcher();
    pageSwitcherToggle.focus();
  }
});