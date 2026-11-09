"use strict";

const resultBox = document.getElementById("resultBox");
const pathBox = document.getElementById("pathBox");
const statsSection = document.getElementById("statsSection");
const statsBody = document.getElementById("statsBody");
const findButton = document.getElementById("findButton");
let serverOnline = false;

function el(tag, className, text) {
  const node = document.createElement(tag);
  if (className) node.className = className;
  if (text !== undefined) node.textContent = text;
  return node;
}

function icon(name) {
  const svg = document.createElementNS("http://www.w3.org/2000/svg", "svg");
  const use = document.createElementNS("http://www.w3.org/2000/svg", "use");
  use.setAttribute("href", "#icon-" + name);
  svg.appendChild(use);
  return svg;
}

function movieLabel(movie) {
  return movie.year ? movie.title + " (" + movie.year + ")" : movie.title;
}

function actorMeta(actor) {
  const parts = [];
  parts.push(actor.birthYear ? "b. " + actor.birthYear : "birth year unknown");
  if (actor.knownFor) parts.push(actor.knownFor);
  return parts.join(" · ");
}

async function getJson(url) {
  const response = await fetch(url);
  const body = await response.json();
  if (!response.ok) throw new Error(body.error || "Request failed");
  return body;
}

// Chain diagram

function actorNode(label, sub, endpoint) {
  const node = el("div", "node actor" + (endpoint ? " endpoint" : ""));
  const shape = el("div", "shape");
  const circle = el("div", "actor-circle");
  circle.appendChild(icon("person"));
  shape.appendChild(circle);
  node.appendChild(shape);
  const caption = el("div", "caption");
  caption.appendChild(el("div", "label", label));
  if (sub) caption.appendChild(el("div", "sub", sub));
  node.appendChild(caption);
  return node;
}

function movieNode(label) {
  const node = el("div", "node movie");
  const shape = el("div", "shape");
  const box = el("div", "movie-box");
  box.appendChild(icon("clapper"));
  shape.appendChild(box);
  node.appendChild(shape);
  const caption = el("div", "caption");
  if (label) caption.appendChild(el("div", "label", label));
  node.appendChild(caption);
  return node;
}

function chain(items, isResult, broken) {
  const row = el("div", "chain" + (isResult ? " result" : ""));
  items.forEach((item, i) => {
    if (i > 0) row.appendChild(el("div", broken ? "link broken" : "link"));
    row.appendChild(item);
  });
  return row;
}

// Shrinks the chain, then stacks it vertically, if it doesn't fit across.
function fitChain(row) {
  row.classList.remove("compact", "vertical");
  if (row.scrollWidth <= row.clientWidth) return;
  row.classList.add("compact");
  if (row.scrollWidth <= row.clientWidth) return;
  row.classList.add("vertical");
}

window.addEventListener("resize", () => {
  const row = resultBox.querySelector(".chain");
  if (row) fitChain(row);
});

function showEmptyState() {
  resultBox.replaceChildren();
  const message = el("p", "message");
  message.append("Enter two actors and click “");
  message.appendChild(el("strong", "", "Find Connection"));
  message.append("” to see the shortest path between them!");
  resultBox.appendChild(message);
  const row = chain([actorNode("Actor 1"), movieNode(""), actorNode("Actor 2")], false);
  resultBox.appendChild(row);
  fitChain(row);
  pathBox.replaceChildren(el("p", "placeholder", "The steps of the connection will appear here."));
  statsSection.hidden = true;
}

function showMessage(text, isError) {
  resultBox.replaceChildren(el("p", "message" + (isError ? " error" : ""), text));
}

function showLoading() {
  resultBox.replaceChildren(el("div", "spinner"));
  pathBox.replaceChildren(el("p", "placeholder", "Searching..."));
  statsSection.hidden = true;
}

function showResult(data) {
  resultBox.replaceChildren();
  const from = data.from;
  const to = data.to;

  resultBox.appendChild(el("p", "headline", from.name + " → " + to.name));
  let degreesText;
  if (data.degrees < 0) degreesText = "Not connected";
  else if (data.degrees === 1) degreesText = "1 degree of separation";
  else degreesText = data.degrees + " degrees of separation";
  resultBox.appendChild(el("p", "degrees", degreesText));

  const items = [];
  if (data.degrees < 0) {
    items.push(actorNode(from.name, from.birthYear ? "b. " + from.birthYear : "", true));
    items.push(actorNode(to.name, to.birthYear ? "b. " + to.birthYear : "", true));
  } else {
    data.path.forEach((step, i) => {
      const endpoint = i === 0 || i === data.path.length - 1;
      items.push(step.type === "movie" ? movieNode(movieLabel(step)) : actorNode(step.name, "", endpoint));
    });
  }
  const row = chain(items, true, data.degrees < 0);
  resultBox.appendChild(row);
  fitChain(row);

  pathBox.replaceChildren();
  if (data.degrees < 0) {
    pathBox.appendChild(el("p", "placeholder", "No chain of movies connects " + from.name + " and " + to.name + " in this dataset."));
  } else if (data.degrees === 0) {
    pathBox.appendChild(el("p", "placeholder", "That's the same actor twice: 0 degrees."));
  } else {
    const list = el("ol");
    for (let i = 1; i < data.path.length; i += 2) {
      const li = el("li");
      li.appendChild(el("strong", "", data.path[i - 1].name));
      li.append(" and ");
      li.appendChild(el("strong", "", data.path[i + 1].name));
      li.append(" were both in ");
      li.appendChild(el("em", "", movieLabel(data.path[i])));
      list.appendChild(li);
    }
    pathBox.appendChild(list);
  }

  statsBody.replaceChildren();
  (data.stats || []).forEach((s) => {
    const tr = el("tr");
    tr.appendChild(el("td", "", s.algorithm));
    tr.appendChild(el("td", "", s.degrees < 0 ? "—" : String(s.degrees)));
    tr.appendChild(el("td", "", s.actorsVisited.toLocaleString()));
    tr.appendChild(el("td", "", s.moviesVisited.toLocaleString()));
    tr.appendChild(el("td", "", s.ms.toFixed(3) + " ms"));
    statsBody.appendChild(tr);
  });
  statsSection.hidden = !data.stats || data.stats.length === 0;
}

function showChoices(title, actors, onPick) {
  resultBox.replaceChildren(el("p", "message", title));
  const list = el("div", "choices");
  actors.forEach((actor) => {
    const button = el("button");
    button.type = "button";
    button.append(actor.name);
    button.appendChild(el("span", "meta", actorMeta(actor)));
    button.addEventListener("click", () => onPick(actor));
    list.appendChild(button);
  });
  resultBox.appendChild(list);
  pathBox.replaceChildren(el("p", "placeholder", "Pick an actor to continue."));
  statsSection.hidden = true;
}

// Autocomplete

class ActorField {
  constructor(input) {
    this.input = input;
    this.list = input.parentElement.querySelector(".suggestions");
    this.actor = null;
    this.items = [];
    this.active = -1;
    this.timer = null;
    this.requestId = 0;

    input.addEventListener("input", () => {
      this.actor = null;
      clearTimeout(this.timer);
      this.timer = setTimeout(() => this.suggest(), 150);
    });
    input.addEventListener("keydown", (e) => this.onKey(e));
    input.addEventListener("blur", () => setTimeout(() => this.close(), 150));
  }

  async suggest() {
    const query = this.input.value.trim();
    const id = ++this.requestId;
    if (!serverOnline || query.length < 2) {
      this.close();
      return;
    }
    try {
      const data = await getJson("/api/actors?q=" + encodeURIComponent(query));
      if (id !== this.requestId || document.activeElement !== this.input) return;
      this.render(data.actors);
    } catch (e) {
      this.close();
    }
  }

  render(actors) {
    this.items = actors;
    this.active = -1;
    this.list.replaceChildren();
    actors.forEach((actor, i) => {
      const li = el("li");
      li.setAttribute("role", "option");
      li.appendChild(el("div", "name", actor.name));
      li.appendChild(el("div", "meta", actorMeta(actor)));
      li.addEventListener("mousedown", (e) => {
        e.preventDefault();
        this.pick(i);
      });
      this.list.appendChild(li);
    });
  }

  pick(i) {
    this.set(this.items[i]);
    this.close();
  }

  set(actor) {
    this.actor = actor;
    this.input.value = actor.name;
  }

  close() {
    this.items = [];
    this.list.replaceChildren();
  }

  onKey(e) {
    if (this.items.length === 0) return;
    if (e.key === "ArrowDown" || e.key === "ArrowUp") {
      e.preventDefault();
      const step = e.key === "ArrowDown" ? 1 : -1;
      this.active = (this.active + step + this.items.length) % this.items.length;
      [...this.list.children].forEach((li, i) => li.classList.toggle("active", i === this.active));
    } else if (e.key === "Enter" && this.active >= 0) {
      e.preventDefault();
      this.pick(this.active);
    } else if (e.key === "Escape") {
      this.close();
    }
  }

  // Returns the chosen actor, or null after showing a picker or an error.
  async resolve(label) {
    if (this.actor) return this.actor;
    const query = this.input.value.trim();
    if (!query) {
      showMessage("Enter a name for " + label + ".", true);
      return null;
    }
    const data = await getJson("/api/actors?q=" + encodeURIComponent(query));
    const exact = data.actors.slice(0, data.exact);
    if (exact.length === 1) return exact[0];

    const retry = (actor) => {
      this.set(actor);
      findConnection();
    };
    if (exact.length > 1) {
      showChoices(data.exact + " actors are named “" + query + "”. Which one did you mean for " + label + "?", exact, retry);
    } else if (data.actors.length > 0) {
      showChoices("No actor named “" + query + "”. Did you mean one of these?", data.actors, retry);
    } else {
      showMessage("No actor named “" + query + "” in this dataset.", true);
      pathBox.replaceChildren(el("p", "placeholder", "Check the spelling, or try a bigger dataset."));
    }
    return null;
  }
}

const field1 = new ActorField(document.getElementById("actor1"));
const field2 = new ActorField(document.getElementById("actor2"));

async function findConnection() {
  if (!serverOnline) {
    showOffline();
    return;
  }
  findButton.disabled = true;
  try {
    const a = await field1.resolve("Actor 1");
    if (!a) return;
    const b = await field2.resolve("Actor 2");
    if (!b) return;
    showLoading();
    const data = await getJson("/api/search?from=" + encodeURIComponent(a.id) + "&to=" + encodeURIComponent(b.id));
    showResult(data);
  } catch (e) {
    showMessage(e.message, true);
  } finally {
    findButton.disabled = false;
  }
}

document.getElementById("searchForm").addEventListener("submit", (e) => {
  e.preventDefault();
  field1.close();
  field2.close();
  findConnection();
});

// Pages

function showPage() {
  const page = location.hash === "#about" ? "about" : "home";
  document.getElementById("home").hidden = page !== "home";
  document.getElementById("about").hidden = page !== "about";
  document.querySelectorAll("nav a").forEach((a) => a.classList.toggle("active", a.dataset.page === page));
}
window.addEventListener("hashchange", showPage);

// Without the server, show a path exported with --export viz/path.json instead.

function showOffline() {
  showMessage("The search server isn't running. Start it with: six_degrees data/small --serve, then open http://localhost:8080", true);
}

function fromExport(json) {
  const onPath = json.nodes.filter((n) => n.onPath);
  const path = onPath.map((n) => (n.type === "movie" ? { type: "movie", title: n.label } : { type: "actor", name: n.label }));
  return {
    from: { name: json.sourceName },
    to: { name: json.targetName },
    degrees: json.degrees,
    path: path,
    stats: [{ algorithm: json.algorithm, degrees: json.degrees, actorsVisited: json.actorsVisited, moviesVisited: json.moviesVisited, ms: json.ms }],
  };
}

async function start() {
  showPage();
  showEmptyState();
  try {
    const info = await getJson("/api/info");
    serverOnline = true;
    const text = "Dataset “" + info.dataset + "”: " + info.actors.toLocaleString() + " actors, " + info.movies.toLocaleString() + " movies.";
    document.getElementById("datasetInfo").textContent = text;
    document.getElementById("aboutDataset").textContent = "Currently loaded: " + text;
  } catch (e) {
    document.getElementById("datasetInfo").textContent = "Search server offline. Showing the last exported path, if any.";
    try {
      const exported = await getJson("path.json");
      showResult(fromExport(exported));
    } catch (e2) {
      showOffline();
    }
  }
}

start();
