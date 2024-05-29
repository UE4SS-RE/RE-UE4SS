var redirectLink = document.querySelector("#version-warning > #redirect-url")
if(redirectLink !== null) {
    redirectLink.href = new URL(window.location.pathname, redirectLink.href)
}
