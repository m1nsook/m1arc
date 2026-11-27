// Build the exact NextGuide source used to generate this site; no moving alpha tag.
import { existsSync, mkdirSync, mkdtempSync, readFileSync, readdirSync, writeFileSync, cpSync, rmSync, renameSync } from "node:fs"
import { tmpdir } from "node:os"
import { join, resolve } from "node:path"
import { spawnSync } from "node:child_process"
import { fileURLToPath } from "node:url"
const root = fileURLToPath(new URL("../", import.meta.url))
const pin = "b5240cad0d9880f98b56bd4f27e8c8d772264d40"
const cache = join(root, ".nextguide-cache"), archive = join(cache, "nextguide.tgz"), marker = join(cache, "source-revision")
const run = (command, args, cwd) => {
  const result = spawnSync(command, args, { cwd, stdio: "inherit", shell: false })
  if (result.error) throw result.error
  if (result.status !== 0) throw new Error(command + " failed: " + result.status)
}
mkdirSync(cache, { recursive: true })
if (!existsSync(archive) || !existsSync(marker) || readFileSync(marker, "utf8").trim() !== pin) {
  const temp = mkdtempSync(join(tmpdir(), "m1arc-nextguide-"))
  try {
    const sourceArchive = join(temp, "source.tar.gz")
    const response = await fetch("https://codeload.github.com/m1nsook/NextGuide/tar.gz/" + pin, { signal: AbortSignal.timeout(120000) })
    if (!response.ok) throw new Error("NextGuide source download failed: " + response.status)
    writeFileSync(sourceArchive, Buffer.from(await response.arrayBuffer()))
    run("tar", ["-xzf", sourceArchive, "-C", temp], temp)
    const extracted = readdirSync(temp).find(name => name.toLowerCase() === ("NextGuide-" + pin).toLowerCase())
    if (!extracted) throw new Error("Pinned NextGuide source tree not found")
    const pkg = join(temp, "package-build")
    cpSync(join(temp, extracted, "packages", "nextguide"), pkg, { recursive: true })
    run("bun", ["install"], pkg)
    run("bun", ["run", "build"], pkg)
    // Bun packs the already compiled package; no prepack/rebuild hooks.
    run("npm", ["pack", "--ignore-scripts", "--pack-destination", temp], pkg)
    const packageFile = readdirSync(temp).find(name => /^nextguide-.*\.tgz$/.test(name))
    if (!packageFile) throw new Error("NextGuide package not created")
    const next = join(cache, "nextguide-new.tgz")
    cpSync(join(temp, packageFile), next)
    rmSync(archive, { force: true }); renameSync(next, archive)
    writeFileSync(marker, pin + "\n")
  } finally { rmSync(temp, { recursive: true, force: true, maxRetries: 3 }) }
}
run("bun", ["install"], root)
