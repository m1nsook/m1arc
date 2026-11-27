// Compile the private NextGuide source locally. No npm publishing or credentials.
import { existsSync, mkdirSync, mkdtempSync, readFileSync, readdirSync, cpSync, rmSync, renameSync } from "node:fs"
import { tmpdir } from "node:os"
import { basename, join, resolve } from "node:path"
import { spawnSync } from "node:child_process"
import { fileURLToPath } from "node:url"

const root = fileURLToPath(new URL("../", import.meta.url))
const source = resolve(process.env.NEXTGUIDE_SOURCE || join(root, "..", "NextGuide"))
const packageDir = join(source, "packages", "nextguide")
if (!existsSync(join(packageDir, "package.json"))) {
  console.error("NextGuide checkout not found: " + packageDir)
  console.error("Clone m1nsook/NextGuide next to m1arc with GitHub Desktop, or set NEXTGUIDE_SOURCE to its path.")
  process.exit(1)
}
const metadata = JSON.parse(readFileSync(join(packageDir, "package.json"), "utf8"))
if (metadata.name !== "nextguide" || metadata.version !== "0.0.1-alpha.2") {
  throw new Error("Expected NextGuide source version 0.0.1-alpha.2; found " + metadata.name + " " + metadata.version)
}
const cache = join(root, ".nextguide-cache")
mkdirSync(cache, { recursive: true })
const temp = mkdtempSync(join(tmpdir(), "m1arc-nextguide-"))
const run = (command, args, cwd) => {
  const result = spawnSync(command, args, { cwd, stdio: "inherit", shell: process.platform === "win32" && command === "npm" })
  if (result.error) throw result.error
  if (result.status !== 0) throw new Error(command + " failed: " + result.status)
}
try {
  const pkg = join(temp, "nextguide")
  cpSync(packageDir, pkg, {
    recursive: true,
    filter: path => !["node_modules", "dist", ".git", ".next", ".npmrc", "bun.lock", "bun.lockb", "package-lock.json"].includes(basename(path))
      && !basename(path).startsWith(".env")
  })
  run("bun", ["install"], pkg)
  run("bun", ["run", "build"], pkg)
  run("npm", ["pack", "--ignore-scripts"], pkg)
  const file = readdirSync(pkg).find(name => /^nextguide-.*\.tgz$/.test(name))
  if (!file) throw new Error("NextGuide package was not generated")
  const next = join(cache, "nextguide-new.tgz"), archive = join(cache, "nextguide.tgz")
  cpSync(join(pkg, file), next)
  rmSync(archive, { force: true })
  renameSync(next, archive)
} finally {
  rmSync(temp, { recursive: true, force: true, maxRetries: 3 })
}
// Force Bun to resolve the newly packed local tarball even if its version is unchanged.
run("bun", ["install", "--force"], root)
