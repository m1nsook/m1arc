// Verify real hydrated browser navigation, not just hrefs in exported HTML.
// Uses Chrome DevTools protocol and Node's native WebSocket; no new dependency.
import assert from "node:assert/strict"
import { spawn } from "node:child_process"
import { existsSync, mkdtempSync, readFileSync, rmSync } from "node:fs"
import { tmpdir } from "node:os"
import { join } from "node:path"

const base = new URL(process.argv[2] ?? "http://127.0.0.1:8765/m1arc/")
const chrome = process.env.CHROME_PATH || ["google-chrome","chromium","chromium-browser"].map(name=>"/usr/bin/"+name).find(existsSync)
assert.ok(chrome,"Chrome or Chromium must be installed")
const profile = mkdtempSync(join(tmpdir(),"m1arc-browser-"))
const pause = ms=>new Promise(r=>setTimeout(r,ms))
async function until(fn,label,timeout=20000){
  const end=Date.now()+timeout
  let last
  while(Date.now()<end){try{const result=await fn();if(result)return result}catch(e){last=e}await pause(100)}
  throw Error("Timed out: "+label+(last?": "+last.message:""))
}
let child,socket
try{
  await until(async()=> (await fetch(base)).ok,"static HTTP server")
  child=spawn(chrome,["--headless=new","--no-sandbox","--disable-dev-shm-usage","--disable-gpu","--no-first-run","--window-size=1440,1000","--remote-debugging-port=0","--user-data-dir="+profile,"about:blank"],{stdio:"ignore"})
  const port=await until(()=>{const f=join(profile,"DevToolsActivePort");return existsSync(f)&&Number(readFileSync(f,"utf8").split("\n")[0])},"Chrome port")
  const target=(await fetch("http://127.0.0.1:"+port+"/json/list").then(r=>r.json())).find(t=>t.type==="page")
  socket=new WebSocket(target.webSocketDebuggerUrl)
  await new Promise((resolve,reject)=>{socket.addEventListener("open",resolve,{once:true});socket.addEventListener("error",reject,{once:true})})
  let id=0
  const pending=new Map()
  socket.addEventListener("message",e=>{const m=JSON.parse(e.data);if(m.method==="Runtime.exceptionThrown")console.error("Browser exception:",JSON.stringify(m.params.exceptionDetails));if(m.method==="Log.entryAdded"&&m.params.entry.level==="error")console.error("Browser error:",m.params.entry.text);if(pending.has(m.id)){const {resolve,reject,timer}=pending.get(m.id);clearTimeout(timer);pending.delete(m.id);m.error?reject(Error(JSON.stringify(m.error))):resolve(m.result)}})
  function send(method,params={}){
    return new Promise((resolve,reject)=>{const n=++id;const timer=setTimeout(()=>{pending.delete(n);reject(Error("CDP timeout: "+method))},10000);pending.set(n,{resolve,reject,timer});socket.send(JSON.stringify({id:n,method,params}))})
  }
  async function ev(expression){const r=await send("Runtime.evaluate",{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw Error("Browser evaluation: "+r.exceptionDetails.text);return r.result?.value}
  const norm=s=>s.replace(/\/+$/,"")||"/"
  async function ready(path){await until(async()=>norm(await ev("location.pathname"))===norm(path)&&await ev('document.readyState==="complete" && !!document.querySelector("[data-slot=docs-nav]") && !!document.querySelector("h1")'),"page "+path);await pause(250)}
  await send("Page.enable")
  await send("Runtime.enable")
  await send("Log.enable")
  await send("Page.navigate",{url:base.href})
  await ready(base.pathname)
  await pause(750)
  const links=await ev('Array.from(document.querySelectorAll("[data-slot=docs-nav] a[href]")).map(a=>({path:new URL(a.href).pathname,label:a.textContent.trim()}))')
  const unique=[...new Map(links.map(x=>[x.path,x])).values()]
  assert.ok(unique.length>=14,"Missing documentation navigation: "+JSON.stringify(unique))
  for(const link of unique){
    assert.ok(link.path.startsWith(base.pathname),"Unprefixed sidebar URL: "+link.path)
    if(norm(link.path)===norm(await ev("location.pathname")))continue
    const clicked=await ev('(function(){const a=[...document.querySelectorAll("[data-slot=docs-nav] a[href]")].find(a=>new URL(a.href).pathname==='+JSON.stringify(link.path)+');if(!a)return false;a.click();return true})()')
    assert.ok(clicked,"Sidebar link missing: "+link.path)
    await ready(link.path)
    const title=await ev('document.querySelector("h1").innerText')
    assert.ok(title&&!/404|not found/i.test(title),"Invalid document: "+link.path)
    console.log("PASS sidebar click: "+link.path+" — "+title)
  }
  await send("Page.navigate",{url:base.href})
  await ready(base.pathname)
  await pause(500)
  const card=await ev('(function(){const a=[...document.querySelectorAll("main a[href], [data-ng-docs-content] a[href]")].find(a=>new URL(a.href).pathname==="/m1arc/quickstart/");if(!a)return false;a.click();return true})()')
  assert.ok(card,"Homepage installation card missing")
  await ready("/m1arc/quickstart/")
  console.log("PASS homepage card")
  await ev("history.back()")
  await ready(base.pathname)
  console.log("PASS browser Back")
  // Test the actual demo, including hydrated NextGuide controls.
  await until(()=>ev('Boolean(document.querySelector(".arc-preview[data-ready=true]"))'),"interactive preview hydration")
  assert.ok(await ev('(function(){const b=document.querySelector(".arc-preview button[aria-label=\\"Toggle preview theme\\"]");if(!b)return false;b.click();return true})()'),"Theme control missing")
  await until(()=>ev('document.querySelector(".arc-preview").dataset.arcTheme==="dark"'),"preview theme switch")
  assert.ok(await ev('(function(){const b=[...document.querySelectorAll(".arc-tabs button")].find(b=>b.textContent.includes("WEAPONS"));if(!b)return false;b.click();return true})()'),"Weapons tab missing")
  await until(()=>ev('document.querySelector(".arc-detail-scroll").innerText.includes("Process weapons")'),"weapons tab content")
  console.log("PASS interactive demo theme and tabs")
  console.log("Browser checks completed for "+unique.length+" documentation destinations.")
}finally{
  try{socket?.close()}catch{}
  try{child?.kill()}catch{}
  await pause(500)
  try { rmSync(profile,{recursive:true,force:true,maxRetries:5,retryDelay:200}) } catch (e) { console.warn("Chrome profile cleanup:",e.message) }
}
