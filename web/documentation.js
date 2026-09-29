const documentation = document.querySelector("#readme-content");
const sidebar = document.querySelector("#docs-sidebar");
const tableOfContents = document.querySelector("#docs-toc-links");

function buildTableOfContents() {
  const headings = [...documentation.querySelectorAll("h1, h2, h3")];
  const usedIds = new Set();
  const headingLinks = new Map();

  for (const heading of headings) {
    const baseId = heading.textContent
      .toLowerCase()
      .trim()
      .replace(/[^a-z0-9 -]/g, "")
      .replace(/\s+/g, "-");
    let id = baseId;
    let suffix = 2;
    while (usedIds.has(id)) id = `${baseId}-${suffix++}`;
    usedIds.add(id);
    heading.id = id;

    const link = document.createElement("a");
    link.href = `#${id}`;
    link.textContent = heading.textContent;
    link.className = `toc-level-${heading.tagName.slice(1)}`;
    link.addEventListener("click", () => setActiveHeading(id));
    tableOfContents.append(link);
    headingLinks.set(id, link);
  }

  if (!headings.length) {
    sidebar.hidden = true;
    return;
  }

  function setActiveHeading(id) {
    for (const [headingId, link] of headingLinks) {
      if (headingId === id) link.setAttribute("aria-current", "location");
      else link.removeAttribute("aria-current");
    }
  }

  function updateActiveHeading() {
    const readingLine = 96;
    let activeHeading = headings[0];
    let nearestDistance = Infinity;

    for (const heading of headings) {
      const distance = Math.abs(heading.getBoundingClientRect().top - readingLine);
      if (distance < nearestDistance) {
        activeHeading = heading;
        nearestDistance = distance;
      }
    }

    setActiveHeading(activeHeading.id);
  }

  document.addEventListener("scroll", updateActiveHeading, { capture: true, passive: true });
  window.addEventListener("hashchange", updateActiveHeading);
  window.addEventListener("resize", updateActiveHeading, { passive: true });

  updateActiveHeading();
}

async function loadDocumentation() {
  try {
    const response = await fetch("../README.md");
    if (!response.ok) throw new Error(`README.md returned ${response.status}`);
    if (!window.marked?.parse) throw new Error("The Markdown renderer did not load.");

    documentation.innerHTML = window.marked.parse(await response.text());
    buildTableOfContents();
  } catch (error) {
    documentation.textContent = `Could not load the repository README: ${error.message}`;
    documentation.classList.add("docs-error");
    sidebar.hidden = true;
  }
}

loadDocumentation();