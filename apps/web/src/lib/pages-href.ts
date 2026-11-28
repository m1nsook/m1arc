// GitHub Pages exports plain documents under /m1arc/. Do not pass these
// public URLs to Next Router, which adds its own basePath.
export function pagesHref(href: string): string {
  if (!href.startsWith("/") || href.startsWith("//")) return href
  const [, path, suffix] = href.match(/^([^?#]*)(.*)$/) ?? ["",href,""]
  const relative = path === "/m1arc" ? "/" : path.startsWith("/m1arc/") ? path.slice(6) : path
  const canonical = relative.endsWith("/") || /\.[a-z0-9]+$/i.test(relative) ? relative : relative + "/"
  return "/m1arc" + canonical + suffix
}
