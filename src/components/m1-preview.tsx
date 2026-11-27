"use client"
import { useState } from "react"

type Values = { enabled: boolean; weapons: boolean; advanced: boolean; tier: number; mode: string; capacity: number }
const defaults: Values = { enabled: true, weapons: true, advanced: false, tier: 3, mode: "Disassemble", capacity: 500 }
const tabs = ["General", "Weapons", "Inventory"] as const
export function M1Preview() {
  const [tab, setTab] = useState<(typeof tabs)[number]>("General")
  const [v, setV] = useState<Values>({ ...defaults })
  function update<K extends keyof Values>(key: K, value: Values[K]) { setV(previous => ({ ...previous, [key]: value })) }
  const switchRow = (label: string, key: "enabled" | "weapons" | "advanced", disabled = false) =>
    <label className="flex items-center justify-between gap-4 border-b border-current/20 py-3"><span>{label}</span><button type="button" role="switch" aria-label={label} aria-checked={v[key]} disabled={disabled} className="min-w-20 border border-current px-3 py-2 font-bold disabled:opacity-40" style={{background: v[key] ? "#f2b229" : "#333831",color:v[key] ? "#171813" : "#eae4c8"}} onClick={() => update(key, !v[key])}>{v[key] ? "ON" : "OFF"}</button></label>
  return <div className="not-prose my-6 overflow-hidden border border-[#777568] bg-[#eae4c8] text-[#171813]">
    <div className="flex items-center justify-between bg-[#151613] px-4 py-3 text-[#eae4c8] font-bold"><span>M1 ARC <span className="text-[#f2b229]">//</span> CONTROL PANEL</span><span className="text-xs">DEMO</span></div>
    <div className="p-4">
      <div className="mb-3 border border-[#a39b7b] p-3"><strong>Feature Showcase</strong><div className="text-sm opacity-75">Example settings · browser only</div></div>
      <div role="tablist" aria-label="Example settings pages" className="flex flex-wrap">
        {tabs.map(name => <button type="button" role="tab" key={name} aria-selected={tab === name} onClick={() => setTab(name)} className="flex-1 border border-[#777568] px-2 py-2 text-sm font-bold" style={{background:tab === name ? "#f2b229" : "#ded7b7"}}>{name.toUpperCase()}</button>)}
      </div>
      <div role="tabpanel" aria-label={tab} className="mt-4 min-h-48">
        <div className="border-t border-[#171813]"><span className="inline-block bg-[#171813] px-3 py-1 font-bold text-[#eae4c8]"><span className="text-[#f2b229]">//</span> {tab.toUpperCase()}</span></div>
        {tab === "General" && switchRow("Enable example", "enabled")}
        {tab === "Weapons" && <>{switchRow("Process weapons", "weapons", !v.enabled)}
          <fieldset disabled={!v.enabled || !v.weapons} className="disabled:opacity-40">
            <label className="flex items-center justify-between gap-3 py-3"><span>Weapon action</span><select className="border bg-transparent p-2" value={v.mode} onChange={e => update("mode",e.target.value)}><option>Disassemble</option><option>Sell</option></select></label>
            {switchRow("Advanced filters", "advanced", !v.enabled || !v.weapons)}
            <label className="block py-3">Maximum tier: {v.tier}<input aria-label="Maximum tier" className="block w-full accent-[#8b6200]" type="range" min={1} max={5} value={v.tier} disabled={!v.advanced || !v.enabled || !v.weapons} onChange={e => update("tier",Number(e.target.value))} /></label>
          </fieldset></>}
        {tab === "Inventory" && <fieldset disabled={!v.enabled} className="disabled:opacity-40"><label className="block py-3">Carry capacity: {v.capacity}<input aria-label="Carry capacity" type="range" min={50} max={1000} step={10} className="block w-full accent-[#8b6200]" value={v.capacity} onChange={e=>update("capacity",Number(e.target.value))} /></label></fieldset>}
      </div>
    </div>
    <div className="flex justify-end bg-[#151613] p-3"><button type="button" className="border border-[#eae4c8] px-3 py-2 text-sm text-[#eae4c8]" onClick={()=>setV({...defaults})}>DEFAULTS</button></div>
  </div>
}
