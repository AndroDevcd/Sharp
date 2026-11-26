// colour Sharp source: comments, text, keywords, numbers
(function () {
  var words = "extern mod import def class interface component inject single override return if else for while new as base private static const true false null self enum is";
  var rule = new RegExp("(//[^\\n]*)|(\"(?:[^\"\\\\]|\\\\.)*\")|\\b(" + words.split(" ").join("|") + ")\\b|\\b(\\d+(?:\\.\\d+)?)\\b", "g");
  function safe(t) { return t.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;"); }
  document.querySelectorAll("pre.sharp").forEach(function (pre) {
    var text = pre.textContent, out = "", last = 0, m;
    while ((m = rule.exec(text)) !== null) {
      out += safe(text.slice(last, m.index));
      var kind = m[1] ? "c" : m[2] ? "s" : m[3] ? "k" : "n";
      out += '<span class="' + kind + '">' + safe(m[0]) + "</span>";
      last = m.index + m[0].length;
    }
    pre.innerHTML = out + safe(text.slice(last));
  });
})();

// the example tabs in the hero
document.querySelectorAll("#tour .bar button").forEach(function (tab) {
  tab.addEventListener("click", function () {
    var name = tab.dataset.show;
    document.querySelectorAll("#tour .bar button").forEach(function (t) { t.setAttribute("aria-selected", t === tab ? "true" : "false"); });
    document.querySelectorAll("#tour [data-sample]").forEach(function (p) { p.hidden = p.dataset.sample !== name; });
    document.querySelectorAll("#tour [data-out]").forEach(function (p) { p.hidden = p.dataset.out !== name; });
  });
});

// the download button: ask GitHub for the newest release, and offer its zip and its version.
// When GitHub does not answer, the button stays as it was written: a link to the newest release's page.
(function () {
  var button = document.getElementById("get-sharp");
  if (!button || !window.fetch) { return; }
  var asked = button.href.replace("https://github.com/", "https://api.github.com/repos/");
  fetch(asked).then(function (answer) { return answer.ok ? answer.json() : null; }).then(function (release) {
    if (!release) { return; }
    var zip = (release.assets || []).filter(function (a) { return /\.zip$/.test(a.name); })[0];
    if (!zip) { return; }
    button.href = zip.browser_download_url;
    var version = String(release.tag_name || "").replace(/^v/, "");
    if (!version) { return; }
    document.querySelectorAll("[data-newest-version]").forEach(function (place) { place.textContent = version; });
  }).catch(function () { });
})();

// the library's search box: every name is in std/search.js (name, module, page, what it does)
(function () {
  var box = document.getElementById("lib-search"), found = document.getElementById("lib-found");
  if (!box || !found || typeof SHARP_INDEX === "undefined") { return; }
  function safe(t) { return t.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;"); }
  box.addEventListener("input", function () {
    var asked = box.value.trim().toLowerCase(), rows = [];
    if (asked.length >= 2) {
      // names that begin with what was typed come first, then names that hold it
      [0, 1].forEach(function (pass) {
        for (var i = 0; i < SHARP_INDEX.length && rows.length < 40; i++) {
          var at = SHARP_INDEX[i][0].toLowerCase().indexOf(asked);
          var last = SHARP_INDEX[i][0].toLowerCase().split(".").pop().indexOf(asked);
          if ((pass === 0 && (at === 0 || last === 0)) || (pass === 1 && at > 0 && last !== 0)) { rows.push(SHARP_INDEX[i]); }
        }
      });
    }
    found.hidden = rows.length === 0;
    found.innerHTML = rows.map(function (r) {
      return '<li><a href="' + r[2] + '">' + safe(r[0]) + "<span>" + safe(r[1]) + (r[3] ? " - " + safe(r[3]) : "") + "</span></a></li>";
    }).join("");
  });
})();
