import { readFileSync, existsSync } from "node:fs"
import { createHash } from "node:crypto"
const archive = new URL("../vendor/nextguide-0.0.1-alpha.2.tgz", import.meta.url)
const expected = "GLP3MkoOMGFzUB4SaL4x1HSn4wwAGlayw1l02CLxw7Iy7SwSJVV1vcE0CbloG4+6M+5CohIoMrsyrnI0gIkjuQ=="
if (!existsSync(archive)) throw new Error("Missing pinned NextGuide archive in apps/web/vendor. The npm placeholder is intentionally not used.")
if (createHash("sha512").update(readFileSync(archive)).digest("base64") !== expected) throw new Error("NextGuide archive differs from the tested package.")
console.log("Verified exact NextGuide 0.0.1-alpha.2 package")
