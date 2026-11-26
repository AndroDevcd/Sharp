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
