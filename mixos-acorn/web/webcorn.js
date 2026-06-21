function getel(id) { return document.getElementById(id); }

function fragthree(target, instruction, content) {
    fetch(window.location.pathname, {
        method: "FRAG",
        body: target+" "+instruction+" "+content
    })
    .then(r => r.text())
    .then(html => {
        document.getElementById(target).outerHTML = html;
    });
}

function frag(target, instruction = "") {
    fetch(window.location.pathname, {
        method: "FRAG",
        body: target + (instruction ? " " + instruction : "")
    })
    .then(r => r.text())
    .then(html => {
        document.getElementById(target).outerHTML = html;
    });
}

function post(body) {
    fetch(window.location.pathname, {
        method: "POST",
        body: body
    })
}

function cell_post(input, label, col, row, target) {
    fetch(window.location.pathname, {
        method: "POST",
        body: label + " " + col + " " + row + " " + target + " " + input.value
    }).then(() => frag(target));
}

function postForm(fields) {
    const form = document.createElement('form');
    form.method = 'POST';
    form.action = window.location.pathname;
    Object.entries(fields).forEach(([k,v]) => {
        const input = document.createElement('input');
        input.name = k; input.value = v;
        form.appendChild(input);
    });
    document.body.appendChild(form);
    form.submit();
}