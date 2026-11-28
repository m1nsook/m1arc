"use client"

import { useEffect, useId, useRef, useState, type ReactNode } from "react"
import { Button, Badge, Input, Label, NativeSelect, Checkbox, Switch, Tabs, TabsList, TabsTrigger, TabsContent, Dialog, DialogContent, DialogTitle, DialogDescription, DialogFooter, Table, TableHeader, TableBody, TableHead, TableRow, TableCell, Kbd, ScrollArea, ScrollAreaViewport, ScrollBar, EmptyState } from "nextguide/ui"
import { UIIcon } from "nextguide/icons"
import type { UIIconName } from "nextguide/icons"
import source from "./m1-preview-data.json"
import "./m1-preview.css"

type Value = boolean | string | number | null
type Setting = { id: string; kind: string; label: string; tab?: string; group?: string; description?: string; default?: Value; presentation?: string; controlsGroup?: boolean; enabledWhen?: { setting: string; equals: Value }; choices?: string[]; min?: number; max?: number; step?: number; keywords?: string[]; buttonLabel?: string; button?: { id: string; label: string } }
type Mod = { id: string; name: string; author?: string; version?: string; scope: string; shortDesc: string; description?: string; tabs?: { id: string; label: string }[]; settings: Setting[] }
type Values = Record<string, Record<string, Value>>
type Severity = "DEBUG" | "INFO" | "WARN" | "ERROR"
type Entry = { id: number; time: string; level: Severity; message: string }
const mods = source as Mod[]
const levels: Severity[] = ["DEBUG", "INFO", "WARN", "ERROR"]
const defaults = (): Values => Object.fromEntries(mods.map(m => [m.id, Object.fromEntries(m.settings.filter(s => s.kind !== "action").map(s => [s.id, s.default ?? null]))]))
const initialLogs = (): Record<string, Entry[]> => Object.fromEntries(mods.map((m,i) => [m.id, [
  { id: i*10+2, time: "00:00:01", level: "INFO", message: "Example settings registered." },
  { id: i*10+1, time: "00:00:00", level: "DEBUG", message: "Browser preview initialized. No game connection." },
]]))
const scopeIcons: Record<string, UIIconName> = { "UI/UX": "settings", Crafting: "code", Gameplay: "home" }
function blocked(m: Mod, s: Setting, v: Record<string, Value>, visited = new Set<string>()): string {
  if (visited.has(s.id)) return "Invalid dependency"
  const seen = new Set(visited).add(s.id)
  const master = m.settings.find(x => x.kind === "boolean" && ["enabled","active"].includes(x.id))
  if (master && s.id !== master.id && v[master.id] !== true) return master.label
  const group = m.settings.find(x => x.controlsGroup && x.id !== s.id && x.group === s.group && x.tab === s.tab)
  if (group && (v[group.id] !== true || blocked(m,group,v,seen))) return group.label
  if (s.enabledWhen) {
    const p = m.settings.find(x => x.id === s.enabledWhen?.setting)
    if (!p || v[p.id] !== s.enabledWhen.equals || blocked(m,p,v,seen)) return p?.label ?? s.enabledWhen.setting
  }
  return ""
}
function Icon({name}: {name: UIIconName}) { return <UIIcon name={name} width={16} height={16} aria-hidden="true" /> }
function Heading({children}: {children: ReactNode}) { return <div className="arc-heading"><span><b>//</b> {children}</span></div> }
function keyLabel(v: Value | undefined) { return v === 0 || !v ? "Unassigned" : String(v) }
export function M1Preview() {
  const uid = useId()
  const root = useRef<HTMLDivElement>(null)
  const sequence = useRef(100)
  const [ready,setReady] = useState(false)
  useEffect(()=>{setReady(true)},[])
  const [values,setValues] = useState<Values>(defaults)
  const [selected,setSelected] = useState(mods[0].id)
  const [tabs,setTabs] = useState<Record<string,string>>({})
  const [dark,setDark] = useState(false)
  const [open,setOpen] = useState(true)
  const [mode,setMode] = useState<"settings"|"logs">("settings")
  const [modal,setModal] = useState<"search"|"about"|"key"|"reset"|"clear"|"inspect"|null>(null)
  const [resetGroup,setResetGroup] = useState<string|null>(null)
  const [keyTarget,setKeyTarget] = useState("menu")
  const [keyDraft,setKeyDraft] = useState("")
  const [menuKey,setMenuKey] = useState("F11")
  const [query,setQuery] = useState("")
  const [logQuery,setLogQuery] = useState("")
  const [filter,setFilter] = useState<Severity|null>(null)
  const [logs,setLogs] = useState(initialLogs)
  const [frozen,setFrozen] = useState<Entry[]|null>(null)
  const [inspected,setInspected] = useState<Entry|null>(null)
  const [notice,setNotice] = useState("")
  const [highlight,setHighlight] = useState("")
  const m = mods.find(x=>x.id===selected)!
  const v = values[m.id]
  const tab = tabs[m.id] ?? m.tabs?.[0]?.id ?? ""
  const rows = m.settings.filter(s=>!m.tabs?.length || s.tab===tab)
  const entries = frozen ?? logs[m.id]
  const visible = entries.filter(e=>(!filter||e.level===filter)&&e.message.toLowerCase().includes(logQuery.toLowerCase()))
  const online = mods.filter(x=>values[x.id].enabled!==false&&values[x.id].active!==false).length
  function log(id:string,message:string,level:Severity="INFO") {
    const entry:Entry={id:sequence.current++,time:new Date().toLocaleTimeString("en-GB",{hour12:false}),level,message}
    setLogs(prev=>({...prev,[id]:[entry,...prev[id]].slice(0,100)}))
  }
  function notify(message:string) { setNotice(message) }
  useEffect(()=>{if(!notice)return;const timer=window.setTimeout(()=>setNotice(""),3500);return()=>window.clearTimeout(timer)},[notice])
  useEffect(()=>{if(!highlight)return;const timer=window.setTimeout(()=>{root.current?.querySelector('[data-highlight="true"]')?.scrollIntoView({block:"nearest",behavior:"smooth"})},50);const clear=window.setTimeout(()=>setHighlight(""),2600);return()=>{window.clearTimeout(timer);window.clearTimeout(clear)}},[highlight,selected,tab])
  function selectMod(id:string) {setSelected(id);setFrozen(null);setFilter(null);setLogQuery("");setHighlight("")}
  function update(s:Setting,value:Value) {
    if(blocked(m,s,v))return
    setValues(prev=>({...prev,[m.id]:{...prev[m.id],[s.id]:value}}))
    log(m.id,s.label+": "+String(value),"DEBUG")
  }
  function action(s:Setting) { if(!blocked(m,s,v)){log(m.id,"Example action requested: "+(s.button?.id??s.id));notify("Action requested · simulation only")} }
  function chooseKey(target:string) {setKeyTarget(target);setKeyDraft(target==="menu"?menuKey:keyLabel(v[target]));setModal("key")}
  function reset() {
    setValues(prev=>({...prev,[m.id]:{...prev[m.id],...Object.fromEntries(m.settings.filter(s=>s.kind!=="action"&&(!resetGroup||(s.group===resetGroup&&(!m.tabs||s.tab===tab)))).map(s=>[s.id,s.default??null]))}}))
    log(m.id,resetGroup?"Group defaults restored: "+resetGroup:"All defaults restored")
    notify("Defaults restored");setModal(null)
  }
  async function copy(text:string) {try{await navigator.clipboard.writeText(text);notify("Copied to clipboard")}catch{notify("Clipboard unavailable in this browser")}}
  const searchResults = mods.flatMap(mod=>[
    {mod,setting:null,label:mod.name,path:mod.scope,text:mod.name+" "+mod.shortDesc+" "+mod.author},
    ...mod.settings.map(s=>({mod,setting:s,label:s.label,path:mod.name+" / "+(s.tab??s.group??"General"),text:[s.label,s.description,s.group,s.tab,mod.name,...(s.keywords??[])].join(" ")}))
  ]).filter(x=>query.trim().toLowerCase().split(/\s+/).every(word=>x.text.toLowerCase().includes(word))).slice(0,25)
  function go(result:typeof searchResults[number]) {selectMod(result.mod.id);setMode("settings");if(result.setting?.tab)setTabs(prev=>({...prev,[result.mod.id]:result.setting!.tab!}));setHighlight(result.setting?.id??"");setModal(null)}
  function renderSetting(s:Setting) {
    const reason=blocked(m,s,v), disabled=!!reason, value=v[s.id], id=uid+"-"+m.id+"-"+s.id
    return <div className="arc-setting" key={s.id} data-highlight={highlight===s.id} data-disabled={disabled}>
      <div className="arc-setting-copy"><Label htmlFor={id}>{s.label}</Label>{s.description&&<p>{s.description}</p>}{reason&&<p className="arc-lock">Requires: {reason}</p>}</div>
      <div className="arc-control">
        {s.kind==="boolean" ? s.presentation==="checkbox" ? <Checkbox id={id} checked={value===true} disabled={disabled} onCheckedChange={x=>update(s,x===true)} /> : <div className="arc-switch"><Switch id={id} checked={value===true} disabled={disabled} onCheckedChange={x=>update(s,x)} /><span>{value===true?"ON":"OFF"}</span></div>
        :s.kind==="choice"?<NativeSelect id={id} value={String(value)} disabled={disabled} onChange={e=>update(s,e.target.value)}>{s.choices?.map(c=><option key={c}>{c}</option>)}</NativeSelect>
        :s.kind==="integer"||s.kind==="float"?<div className="arc-slider"><Input id={id} aria-label={s.label} type="range" min={s.min??0} max={s.max??100} step={s.step??(s.kind==="float"?.1:1)} value={Number(value)} disabled={disabled} onChange={e=>update(s,Number(e.target.value))}/><output htmlFor={id}>{s.kind==="float"?Number(value).toFixed(2):String(value)}</output>{s.button&&<Button size="sm" variant="outline" disabled={disabled} onClick={()=>action(s)}>{s.button.label}</Button>}</div>
        :s.kind==="hotkey"?<Button id={id} size="sm" variant="outline" disabled={disabled} onClick={()=>chooseKey(s.id)}>{keyLabel(value)}</Button>
        :s.kind==="action"?<Button id={id} size="sm" variant="outline" disabled={disabled} onClick={()=>action(s)}>{s.buttonLabel??"Request"}</Button>:null}
      </div>
    </div>
  }
  function settingsContent() {
    const groups=[...new Set(rows.map(s=>s.group??"General"))]
    return <><div className="arc-mod-header"><div className="arc-eyebrow">{m.scope.toUpperCase()}</div><h3>{m.name}</h3><p>{m.description??m.shortDesc}</p><div className="arc-meta"><Badge>{m.author??"Unknown author"}</Badge><Badge>v{m.version}</Badge><Badge>Browser example</Badge></div></div>
    {m.tabs?.length?<Tabs value={tab} onValueChange={x=>{setTabs(prev=>({...prev,[m.id]:x}));setHighlight("")}}><TabsList className="arc-tabs">{m.tabs.map(t=><TabsTrigger key={t.id} value={t.id}>{t.label.toUpperCase()}</TabsTrigger>)}</TabsList><TabsContent value={tab} className="arc-tab-content">{groups.map(g=><section key={g}><Heading>{g.toUpperCase()}</Heading>{rows.filter(s=>(s.group??"General")===g).map(renderSetting)}<Button className="arc-group-reset" variant="ghost" size="sm" onClick={()=>{setResetGroup(g);setModal("reset")}}>Reset group</Button></section>)}</TabsContent></Tabs>:groups.map(g=><section key={g}><Heading>{g.toUpperCase()}</Heading>{rows.filter(s=>(s.group??"General")===g).map(renderSetting)}<Button className="arc-group-reset" variant="ghost" size="sm" onClick={()=>{setResetGroup(g);setModal("reset")}}>Reset group</Button></section>)}</>
  }
  return <div className="not-prose arc-preview-wrap" ref={root}>
    <div className="arc-preview-caption"><span>INTERACTIVE DEMO · 1.1.0 DEVELOPMENT PREVIEW</span><Badge>Browser only</Badge></div>
    <div className="arc-preview" data-ready={ready} data-arc-theme={dark?"dark":"light"}>
      <header className="arc-toolbar"><div className="arc-brand"><Icon name="settings"/> M1 ARC <b>//</b> <span>CONTROL PANEL</span></div><div className="arc-toolbar-actions">
        <Button variant="outline" size="sm" title="Toggle theme" aria-label="Toggle preview theme" onClick={()=>setDark(!dark)}><Icon name={dark?"sun":"moon"}/></Button>
        <Button variant="outline" size="sm" title="Change menu shortcut" onClick={()=>chooseKey("menu")}>KEY | {menuKey}</Button>
        <Button variant="outline" size="sm" onClick={()=>{log(m.id,"Simulated manifest refresh; compatible values retained.");notify("Reloaded: "+mods.length+" | Rejected: 0 | Retained: 0")}}>REFRESH</Button>
        <Button variant="outline" size="sm" aria-label="Quick Search" title="Quick Search" onClick={()=>{setQuery("");setModal("search")}}><Icon name="search"/></Button>
        <Button variant="outline" size="sm" aria-pressed={mode==="logs"} onClick={()=>setMode(mode==="logs"?"settings":"logs")}><Icon name={mode==="logs"?"settings":"terminal"}/>{mode==="logs"?"SETTINGS":"LOGS"}</Button>
        <span className="arc-online">{online}/{mods.length} ONLINE</span><Button variant="outline" size="sm" onClick={()=>setOpen(!open)}>{open?"CLOSE":"OPEN"}</Button>
      </div></header>
      {open?<div className="arc-workspace"><aside className="arc-sidebar"><div className="arc-sidebar-heading">REGISTERED MODS <span>{mods.length}</span></div><ScrollArea className="arc-nav-scroll"><ScrollAreaViewport className="arc-nav-viewport">{mods.map(mod=><Button key={mod.id} variant="ghost" className="arc-mod-row" aria-pressed={selected===mod.id} onClick={()=>selectMod(mod.id)}><Icon name={scopeIcons[mod.scope]??"settings"}/><span className="arc-mod-copy"><strong>{mod.name}</strong><small>{mod.author} | v{mod.version}</small><small>{mod.shortDesc}</small></span><span aria-label={values[mod.id].enabled===false?"Offline":"Online"} className="arc-status-square" data-online={values[mod.id].enabled!==false}/></Button>)}</ScrollAreaViewport><ScrollBar orientation="vertical"/></ScrollArea></aside>
      <div className="arc-detail"><div className="arc-detail-scroll">
        {mode==="settings"?settingsContent():<><Heading>DIAGNOSTICS</Heading><h3>{m.name}</h3><p className="arc-subtle">Simulated session · Newest first {frozen?" / PAUSED":""}</p><div className="arc-severity">{levels.map(level=><Button variant="outline" key={level} data-level={level} aria-pressed={filter===level} onClick={()=>setFilter(filter===level?null:level)}><span>{level}</span><strong>{String(entries.filter(e=>e.level===level).length).padStart(2,"0")}</strong></Button>)}</div><div className="arc-log-tools"><Input aria-label="Search log messages" placeholder="Search log messages..." value={logQuery} onChange={e=>setLogQuery(e.target.value)}/><Button variant="outline" size="sm" onClick={()=>setFrozen(frozen?null:[...logs[m.id]])}>{frozen?"Resume":"Pause"}</Button></div><div className="arc-filters">{[null,...levels].map(level=><Button key={level??"ALL"} variant="outline" size="sm" aria-pressed={filter===level} onClick={()=>setFilter(level)}>{level??"ALL"}</Button>)}</div><Table className="arc-log-table"><TableHeader><TableRow><TableHead>TIME</TableHead><TableHead>LEVEL</TableHead><TableHead>MESSAGE</TableHead></TableRow></TableHeader><TableBody>{visible.map(entry=><TableRow key={entry.id}><TableCell>{entry.time}</TableCell><TableCell data-level={entry.level}>{entry.level}</TableCell><TableCell><Button variant="ghost" className="arc-log-message" onClick={()=>{setInspected(entry);setModal("inspect")}}>{entry.message}</Button></TableCell></TableRow>)}</TableBody></Table>{!visible.length&&<EmptyState title="No matching logs" description="Change the filter or update a setting."/>}</>}
      </div><footer className="arc-footer"><Button variant="outline" size="sm" onClick={()=>setModal("about")}><Icon name="info"/> ABOUT</Button><div>{mode==="settings"?<Button variant="outline" size="sm" onClick={()=>{setResetGroup(null);setModal("reset")}}>RESET ALL</Button>:<><Button variant="outline" size="sm" onClick={()=>copy(visible.map(e=>e.time+" "+e.level+" "+e.message).join("\n"))}>COPY LOGS</Button><Button variant="outline" size="sm" onClick={()=>setModal("clear")}>CLEAR</Button></>}<Button variant="outline" size="sm" onClick={()=>copy("M1 ARC // BROWSER DEMO\nMod: "+m.id+"\n"+JSON.stringify(v,null,2))}>COPY DIAGNOSTICS</Button></div></footer></div></div>:<div className="arc-closed"><Button onClick={()=>setOpen(true)}>Open M1 Arc</Button><p>Preview closed. Your example values are retained.</p></div>}
      {notice&&<div className="arc-toast" role="status">{notice}</div>}
    </div>
    <Dialog open={modal!==null} onOpenChange={x=>{if(!x)setModal(null)}}><DialogContent className="arc-dialog">
      <DialogTitle>{modal==="search"?"Quick Search":modal==="key"?"Change shortcut":modal==="reset"?"Restore defaults":modal==="clear"?"Clear simulated logs":modal==="inspect"?"Log details":"About M1 Arc"}</DialogTitle>
      <DialogDescription>{modal==="search"?"Search mods, groups, settings and keywords.":modal==="key"?"Choose a browser demonstration shortcut; it will not affect the game.":modal==="reset"?"This affects only the current browser session.":modal==="clear"?"Remove simulated messages for the selected mod.":modal==="inspect"?"Simulated diagnostic entry.":"Native offline control panel for Cyberpunk 2077."}</DialogDescription>
      {modal==="search"?<><Input autoFocus placeholder="Try backpack, rarity, custom..." aria-label="Quick Search" value={query} onChange={e=>setQuery(e.target.value)}/><div className="arc-search-results">{searchResults.map((result,i)=><Button key={result.mod.id+"-"+(result.setting?.id??i)} variant="ghost" onClick={()=>go(result)}><strong>{result.label}</strong><small>{result.path}</small></Button>)}{!searchResults.length&&<EmptyState title="No matching settings"/>}</div></>:modal==="key"?<><NativeSelect aria-label="Shortcut" value={keyDraft} onChange={e=>setKeyDraft(e.target.value)}>{[...new Set([keyDraft,"Unassigned","F8","F9","F10","F11","F12","Ctrl + K","Ctrl + Shift + K","Ctrl + P"])].map(k=><option key={k}>{k}</option>)}</NativeSelect><p>Emergency recovery: <Kbd>Ctrl</Kbd> + <Kbd>Shift</Kbd> + <Kbd>F11</Kbd></p><Button onClick={()=>{if(keyTarget==="menu")setMenuKey(keyDraft);else {const s=m.settings.find(x=>x.id===keyTarget);if(s)update(s,keyDraft==="Unassigned"?0:keyDraft)}setModal(null);notify("Example shortcut saved")}}>Save</Button></>:modal==="reset"?<Button variant="destructive" onClick={reset}>Restore {resetGroup??"all"} defaults</Button>:modal==="clear"?<Button variant="destructive" onClick={()=>{setLogs(prev=>({...prev,[m.id]:[]}));setFrozen(null);setModal(null);notify("Simulated logs cleared")}}>Clear logs</Button>:modal==="inspect"&&inspected?<><Badge>{inspected.level} · {inspected.time}</Badge><p>{inspected.message}</p><Button variant="outline" onClick={()=>copy(inspected.message)}>Copy message</Button></>:<><p>M1 Arc 1.1.0 development preview. This web implementation recreates the native interface using NextGuide components. No game, RED4ext or installed mods are accessed.</p><Button asChild variant="outline"><a href="https://github.com/m1nsook/m1arc">GitHub repository <Icon name="external-link"/></a></Button></>}
      <DialogFooter><Button variant="outline" onClick={()=>setModal(null)}>Close</Button></DialogFooter>
    </DialogContent></Dialog>
  </div>
}
