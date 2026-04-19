
function compile_preview() {
    fetch(window.location.pathname, {
        method: "FRAG",
        body: "preview " + document.getElementById('code').innerText
    })
    .then(r => r.text())
    .then(html => {
        document.getElementById('preview').innerHTML = html;
    });
}

function getCursorOffset(el) {
    let offset = 0;
    const sel = window.getSelection();
    if(sel.rangeCount === 0) return 0;
    const range = sel.getRangeAt(0);
    const preRange = range.cloneRange();
    preRange.selectNodeContents(el);
    preRange.setEnd(range.endContainer, range.endOffset);
    offset = preRange.toString().length;
    return offset;
}

function setCursorOffset(el, offset) {
    const sel = window.getSelection();
    const range = document.createRange();
    let currentOffset = 0;
    let found = false;
    
    function walk(node) {
        if(found) return;
        if(node.nodeType === 3) {
            const len = node.textContent.length;
            if(currentOffset + len >= offset) {
                range.setStart(node, offset - currentOffset);
                range.collapse(true);
                found = true;
            } else {
                currentOffset += len;
            }
        } else {
            for(const child of node.childNodes) walk(child);
        }
    }
    
    walk(el);
    if(found) {
        sel.removeAllRanges();
        sel.addRange(range);
    }
}

let debounce_timer = null;
let sent_length = 0;
let abort_controller = null;

const el = document.getElementById('code');

el.addEventListener('keydown', function(e) {
    if(e.key === 'Tab') {
        e.preventDefault();
        const sel = window.getSelection();
        const range = sel.getRangeAt(0);
        const tab = document.createTextNode('    ');
        range.insertNode(tab);
        range.setStartAfter(tab);
        range.collapse(true);
        sel.removeAllRanges();
        sel.addRange(range);
        el.dispatchEvent(new Event('input'));
    }
});

el.addEventListener('input', function() {
    const saved_cursor = getCursorOffset(el);
    const saved_length = el.textContent.length;
    clearTimeout(debounce_timer);
    debounce_timer = setTimeout(function() {

        if(abort_controller) abort_controller.abort();
        abort_controller = new AbortController();

        const text = el.innerText;
        const saved_cursor = getCursorOffset(el);
        sent_length = text.length;
        fetch(window.location.pathname, {
            method: "FRAG",
            body: "code " + text
        })
        .then(r => r.text())
        .then(html => {
            const diff = el.textContent.length - saved_length;
            el.innerHTML = html;
            setCursorOffset(el, saved_cursor + diff);
        });
    }, 150);
});