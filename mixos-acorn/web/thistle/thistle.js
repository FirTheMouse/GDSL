function post(body) {
    fetch(window.location.pathname, {
        method: "POST",
        body: body
    })
}

function run(...args) {
    const body = args.join('').replace(/&quot;/g, '"')
        .replace(/&amp;/g, '&')
        .replace(/&lt;/g, '<')
        .replace(/&gt;/g, '>')
        .replace(/&#39;/g, "'")
        .replace(/&apos;/g, "'");
    fetch(window.location.pathname, {
        method: "RUN",
        body: body
    })
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

document.addEventListener('mouseover', function(e) {
    const th = e.target.closest('th');
    if(th) {
        const popup = th.querySelector('.popup');
        if(popup) {
            const rect = th.getBoundingClientRect();
            popup.style.display = 'block';
            popup.style.position = 'fixed';
            popup.style.top = (rect.bottom) + 'px';
            popup.style.left = (rect.left) + 'px';
        }
    }
});
document.addEventListener('mouseout', function(e) {
    const th = e.target.closest('th');
    if(th) {
        const popup = th.querySelector('.popup');
        if(popup) popup.style.display = 'none';
    }
});

document.addEventListener('mouseover', function(e) {
    const td = e.target.closest('td');
    if(td) {
        const popup = td.querySelector('.popup');
        if(popup) {
            const rect = td.getBoundingClientRect();
            popup.style.display = 'block';
            popup.style.position = 'fixed';
            popup.style.top = (rect.bottom) + 'px';
            popup.style.left = (rect.left) + 'px';
        }
    }
});
document.addEventListener('mouseout', function(e) {
    const td = e.target.closest('td');
    if(td) {
        const popup = td.querySelector('.popup');
        if(popup && !popup.contains(e.relatedTarget)) {
            popup.style.display = 'none';
        }
    }
});

document.addEventListener('contextmenu', function(e) {
    const td = e.target.closest('td');
    if(td) {
        e.preventDefault();
        const menu = td.querySelector('.context_menu');
        if(menu) {
            document.querySelectorAll('.context_menu').forEach(m => m.style.display = 'none');
            menu.style.display = 'block';
            menu.style.position = 'fixed';
            menu.style.top = e.clientY + 'px';
            menu.style.left = e.clientX + 'px';
            menu.style.zIndex = '1000';
        }
    }
});
document.addEventListener('click', function(e) {
    if(!e.target.closest('.context_menu')) {
        document.querySelectorAll('.context_menu').forEach(m => m.style.display = 'none');
    }
});