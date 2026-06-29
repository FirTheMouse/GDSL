function post(body) {
    fetch(window.location.pathname, {
        method: "POST",
        body: body
    })
}

function run(ptr, ...captures) {
    const body = [ptr, ...captures].join('@'); //@ is the delmiter we use for runs
    fetch(window.location.pathname, {
        method: 'RUN',
        body: body
    }).then(r => r.text()).then(html => {
        if(!html) return;
        const parser = new DOMParser();
        const doc = parser.parseFromString(html, 'text/html');
        Array.from(doc.body.children).forEach(newEl => {
            if(newEl.id) {
                const targets = Array.from(document.querySelectorAll('#'+newEl.id));
                targets.forEach(el => el.outerHTML = newEl.outerHTML);
            }
        });
    });
    //Single id version
    // }).then(r => r.text()).then(html => {
    //     if(!html) return;
    //     const parser = new DOMParser();
    //     const doc = parser.parseFromString(html, 'text/html');
    //     const newEl = doc.body.firstChild;
    //     if(newEl && newEl.id) {
    //         document.getElementById(newEl.id).outerHTML = newEl.outerHTML;
    //     }
    // });
}

function run_raw(...args) {
    const body = args.join('').replace(/&quot;/g, '"')
        .replace(/&amp;/g, '&')
        .replace(/&lt;/g, '<')
        .replace(/&gt;/g, '>')
        .replace(/&#39;/g, "'")
        .replace(/&apos;/g, "'");
    fetch(window.location.pathname, {
        method: "RUN_RAW",
        body: body
    }).then(r => r.text()).then(html => {
        if(!html) {console.log("Returning, html empty"); return;}
        const parser = new DOMParser();
        const doc = parser.parseFromString(html, 'text/html');
        const newEl = doc.body.firstChild;
        console.log("NEW EL:", newEl);
        console.log("NEW EL ID:", newEl?.id);
        console.log("TARGET:", document.getElementById(newEl?.id));
        if(newEl && newEl.id) {
            const target = document.getElementById(newEl.id);
            if(!target) {console.log("Target not found in DOM"); return;}
            target.outerHTML = newEl.outerHTML;
        }
    });
}

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

function goTo(route) {
    window.location.href = route;
}
