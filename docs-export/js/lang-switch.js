// Adds a language selector dropdown to the mdBook menu bar.
//
// Layout assumptions:
//   * The English release book is built at the site root (e.g. /).
//   * The Chinese book is built as a sibling directory (e.g. /zh-CN/).
//   * Versioned English books live one level deeper (e.g. /dev/, /v3.0.0/).
// Those versioned books don't have their own translation, so their selector
// points at the Chinese release book at the site root instead.
(function () {
    'use strict';

    // mdBook copies this file into "<book root>/js/" and appends a content hash
    // to the file name (e.g. "js/lang-switch-5785180a.js"), so match it loosely.
    var SCRIPT_PATTERN = /^(.*\/)js\/lang-switch(?:-[0-9a-f]+)?\.js(?:\?.*)?$/;
    var ZH_DIR = 'zh-CN/';

    var LANGUAGES = [
        { code: 'zh-CN', label: '简体中文' },
        { code: 'en', label: 'English' }
    ];

    // Returns the absolute URL of the book root (always ends with "/").
    function findBookRoot() {
        var scripts = document.getElementsByTagName('script');
        for (var i = 0; i < scripts.length; i++) {
            var match = SCRIPT_PATTERN.exec(scripts[i].src || '');
            if (match) {
                return match[1];
            }
        }
        return new URL('./', document.baseURI).href;
    }

    // Removes a trailing version segment ("dev", "release", "v3.0.0", ...)
    // so that we end up at the directory that contains every book.
    function siteRootPath(rootPath) {
        var segments = rootPath.split('/').filter(function (segment) {
            return segment.length > 0;
        });
        while (segments.length > 0 && /^(dev|release|v\d.*)$/i.test(segments[segments.length - 1])) {
            segments.pop();
        }
        return '/' + segments.map(function (segment) {
            return segment + '/';
        }).join('');
    }

    function resolveUrls() {
        var bookRootPath = new URL(findBookRoot()).pathname;
        var path = window.location.pathname;
        // Path of the current page relative to the book root, e.g. "lua-api.html"
        var pagePath = path.indexOf(bookRootPath) === 0 ? path.slice(bookRootPath.length) : '';
        var isChinese = (document.documentElement.lang || '').toLowerCase().indexOf('zh') === 0;
        var sitePath = isChinese ? bookRootPath.replace(/zh-CN\/$/, '') : siteRootPath(bookRootPath);
        var suffix = window.location.search + window.location.hash;

        return {
            current: isChinese ? 'zh-CN' : 'en',
            urls: {
                'en': sitePath + pagePath + suffix,
                'zh-CN': sitePath + ZH_DIR + pagePath + suffix
            }
        };
    }

    function createSelector(resolved) {
        var wrapper = document.createElement('div');
        wrapper.id = 'lang-switch';
        wrapper.className = 'lang-switch';

        var details = document.createElement('details');
        var summary = document.createElement('summary');
        var currentLabel = '';
        LANGUAGES.forEach(function (language) {
            if (language.code === resolved.current) {
                currentLabel = language.label;
            }
        });
        summary.title = '切换语言 / Switch language';
        summary.setAttribute('aria-label', summary.title);
        summary.appendChild(document.createTextNode('🌐 '));
        summary.appendChild(document.createTextNode(currentLabel));
        summary.appendChild(document.createTextNode(' ▾'));
        details.appendChild(summary);

        var list = document.createElement('ul');
        list.className = 'lang-menu';
        list.setAttribute('role', 'menu');

        LANGUAGES.forEach(function (language) {
            var item = document.createElement('li');
            var link = document.createElement('a');
            item.setAttribute('role', 'none');
            link.setAttribute('role', 'menuitem');
            link.lang = language.code;
            link.hreflang = language.code;
            link.href = language.code === resolved.current
                ? window.location.href
                : resolved.urls[language.code];
            link.textContent = language.label;
            if (language.code === resolved.current) {
                link.setAttribute('aria-current', 'page');
            }
            item.appendChild(link);
            list.appendChild(item);
        });

        details.appendChild(list);
        wrapper.appendChild(details);

        // Close the dropdown when clicking elsewhere or pressing Escape.
        document.addEventListener('click', function (event) {
            if (details.open && !wrapper.contains(event.target)) {
                details.open = false;
            }
        });
        document.addEventListener('keydown', function (event) {
            if (event.key === 'Escape') {
                details.open = false;
            }
        });

        return wrapper;
    }

    var resolved = resolveUrls();
    var container = document.querySelector('.right-buttons') ||
        document.getElementById('mdbook-menu-bar') ||
        document.getElementById('menu-bar');
    var selector = createSelector(resolved);

    if (container) {
        container.appendChild(selector);
    } else {
        selector.classList.add('lang-switch-fallback');
        document.body.appendChild(selector);
    }

    // Help search engines discover the translated page.
    LANGUAGES.forEach(function (language) {
        if (language.code === resolved.current) {
            return;
        }
        var alternate = document.createElement('link');
        alternate.rel = 'alternate';
        alternate.hreflang = language.code;
        alternate.href = resolved.urls[language.code];
        document.head.appendChild(alternate);
    });
}());
