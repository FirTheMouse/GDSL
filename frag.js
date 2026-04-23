function getel(id) { return document.getElementById(id); }

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
        body: label + " " + col + " " + row + " " + input.value + " " + target
    }).then(() => frag(target));
}

function postForm(fields) {
    const body = Object.entries(fields)
        .map(([k,v]) => k + '=' + encodeURIComponent(v))
        .join('&');
    fetch(window.location.pathname, {
        method: "POST",
        body: body
    })
    .then(res => res.text())
    .then(role => {
        if(role === 'invalid') {
            // show error message on the login page
        } else {
            window.location.href = '/' + role;
        }
    })
}