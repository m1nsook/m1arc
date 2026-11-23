<script setup lang="ts">
import { ref, computed } from 'vue'

interface Setting {
  t: 'sw' | 'int' | 'flt' | 'ls' | 'kb' | 'btn'
  l: string
  v: any
  d?: any
  a?: number
  b?: number
  u?: string
  o?: string[]
  btnLabel?: string
}

interface ModItem {
  n: string
  s: Setting[]
}

const initialMods: ModItem[] = [
  {
    n: 'Let There Be Flight',
    s: [
      { t: 'sw', l: 'Enabled', v: 1 },
      { t: 'int', l: 'Cruise speed', v: 120, a: 20, b: 400, u: ' km/h' },
      { t: 'flt', l: 'Gravity scale', v: 0.35, a: 0, b: 1 },
      { t: 'ls', l: 'Flight model', v: 1, o: ['Arcade', 'Assisted', 'Simulation'] },
      { t: 'kb', l: 'Toggle flight', v: 'F6' },
      { t: 'btn', l: 'Vehicle pose', v: 'RESET', btnLabel: 'RESET' }
    ]
  },
  {
    n: 'In-World Navigation',
    s: [
      { t: 'sw', l: 'Enabled', v: 0 },
      { t: 'ls', l: 'Display mode', v: 0, o: ['Always', 'In vehicle', 'On foot'] },
      { t: 'int', l: 'Arrow spacing', v: 10, a: 2, b: 40 },
      { t: 'int', l: 'Max number of arrows', v: 200, a: 20, b: 500 },
      { t: 'flt', l: 'Fade distance', v: 25, a: 5, b: 100, u: ' m' }
    ]
  },
  {
    n: 'Speedometer Plus',
    s: [
      { t: 'sw', l: 'Enabled', v: 1 },
      { t: 'ls', l: 'Units', v: 0, o: ['km/h', 'mph'] },
      { t: 'kb', l: 'Hide overlay', v: 'Ctrl+H' }
    ]
  },
  {
    n: 'Cyberware Overhaul',
    s: [
      { t: 'sw', l: 'Enabled', v: 1 },
      { t: 'flt', l: 'Global Stat Multiplier', v: 1.5, a: 0.1, b: 5.0, u: 'x' },
      { t: 'ls', l: 'Balance Preset', v: 1, o: ['Vanilla', 'Hardcore', 'Cyberpsycho', 'Custom'] },
      { t: 'btn', l: 'Purge Data', v: 'PURGE', btnLabel: 'PURGE' }
    ]
  }
]

initialMods.forEach((m) => m.s.forEach((i) => (i.d = i.v)))

const mods = ref<ModItem[]>(initialMods)
const cur = ref(0)
const listeningIndex = ref<number | null>(null)
const doneIndex = ref<number | null>(null)

const currentMod = computed(() => mods.value[cur.value])
const isModOff = (m: ModItem) => m.s[0].v === 0
const onlineCount = computed(() => mods.value.filter((m) => !isModOff(m)).length)

function formatVal(i: Setting) {
  return i.t === 'flt' ? Number(i.v.toFixed(2)) + (i.u || '') : i.v + (i.u || '')
}

function updateSlider(i: Setting, e: PointerEvent) {
  const el = e.currentTarget as HTMLElement
  const rect = el.getBoundingClientRect()
  const a = i.a ?? 0
  const b = i.b ?? 100
  const raw = a + ((e.clientX - rect.left) / rect.width) * (b - a)
  const clamped = Math.min(b, Math.max(a, raw))
  i.v = i.t === 'int' ? Math.round(clamped) : Math.round(clamped * 100) / 100
}

function onPointerDown(i: Setting, e: PointerEvent) {
  const el = e.currentTarget as HTMLElement
  el.setPointerCapture(e.pointerId)
  updateSlider(i, e)
  el.onpointermove = (ev) => updateSlider(i, ev)
  el.onpointerup = () => {
    el.onpointermove = null
  }
}

function cycleList(i: Setting, dir: number) {
  if (!i.o) return
  i.v = (i.v + dir + i.o.length) % i.o.length
}

function captureKey(i: Setting, idx: number) {
  listeningIndex.value = idx
  const handler = (e: KeyboardEvent) => {
    e.preventDefault()
    e.stopPropagation()
    if (['Control', 'Shift', 'Alt', 'Meta'].includes(e.key)) return
    if (e.key === 'Backspace') i.v = 'UNBOUND'
    else if (e.key !== 'Escape') {
      i.v = [
        e.ctrlKey && 'Ctrl',
        e.shiftKey && 'Shift',
        e.altKey && 'Alt',
        e.key.length === 1 ? e.key.toUpperCase() : e.key
      ]
        .filter(Boolean)
        .join('+')
    }
    listeningIndex.value = null
    window.removeEventListener('keydown', handler, true)
  }
  window.addEventListener('keydown', handler, true)
}

function triggerBtn(idx: number) {
  doneIndex.value = idx
  setTimeout(() => {
    if (doneIndex.value === idx) doneIndex.value = null
  }, 800)
}

function resetDefaults() {
  currentMod.value.s.forEach((i) => {
    if (i.t !== 'btn') i.v = i.d
  })
}
</script>

<template>
  <div class="m1-win">
    <div class="m1-top">
      <span class="m1-title">CONTROL PANEL <b>//</b> M1 ARC</span>
      <span class="m1-status">{{ onlineCount }}/{{ mods.length }} ONLINE</span>
    </div>

    <div class="m1-body">
      <div class="m1-mods" role="tablist" aria-label="Mods">
        <button
          v-for="(m, idx) in mods"
          :key="m.n"
          class="m1-mod"
          :class="{ off: isModOff(m) }"
          :aria-selected="idx === cur"
          @click="cur = idx"
        >
          <span>{{ m.n }}</span>
          <i class="m1-dot" />
        </button>
      </div>

      <div class="m1-set">
        <div>
          <div
            v-for="(item, idx) in currentMod.s"
            :key="item.l"
            class="m1-row"
            :class="{ dis: idx > 0 && isModOff(currentMod) }"
          >
            <span>{{ item.l }}</span>

            <!-- Switch -->
            <button
              v-if="item.t === 'sw'"
              class="m1-box m1-sw"
              role="switch"
              :aria-checked="!!item.v"
              @click="item.v = item.v ? 0 : 1"
            >
              <span class="off">OFF</span>
              <span class="on">ON</span>
            </button>

            <!-- Slider -->
            <div
              v-else-if="item.t === 'int' || item.t === 'flt'"
              class="m1-box m1-sl"
              tabindex="0"
              role="slider"
              :style="{ '--p': ((item.v - (item.a ?? 0)) / ((item.b ?? 100) - (item.a ?? 0))) * 100 + '%' }"
              @pointerdown="onPointerDown(item, $event)"
            >
              <div class="fill" />
              <div class="v">{{ formatVal(item) }}</div>
            </div>

            <!-- List Picker with Phosphor Carets -->
            <div v-else-if="item.t === 'ls'" class="m1-pick">
              <button class="m1-box" aria-label="Previous" @click="cycleList(item, -1)">
                <i class="ph ph-caret-left" />
              </button>
              <div class="m1-box">{{ item.o?.[item.v] }}</div>
              <button class="m1-box" aria-label="Next" @click="cycleList(item, 1)">
                <i class="ph ph-caret-right" />
              </button>
            </div>

            <!-- Keybind -->
            <button
              v-else-if="item.t === 'kb'"
              class="m1-box m1-key"
              :class="{ on: listeningIndex === idx }"
              @click="captureKey(item, idx)"
            >
              <i class="ph ph-keyboard" />
              <span>{{ listeningIndex === idx ? 'PRESS A KEY' : item.v }}</span>
            </button>

            <!-- Action Button -->
            <button
              v-else-if="item.t === 'btn'"
              class="m1-box m1-btn"
              @click="triggerBtn(idx)"
            >
              <i v-if="doneIndex === idx" class="ph ph-check" />
              <span>{{ doneIndex === idx ? 'DONE' : item.btnLabel }}</span>
            </button>
          </div>
        </div>

        <div class="m1-foot">
          <small>2026 © M1NSOOK // RED4EXT</small>
          <button class="m1-box m1-btn" @click="resetDefaults">
            <i class="ph ph-arrow-counter-clockwise" />
            <span>DEFAULTS</span>
          </button>
        </div>
      </div>
    </div>
  </div>
</template>

<style scoped>
.m1-win {
  width: 100%;
  background: var(--m1-paper);
  border: 2px solid var(--m1-ink);
  color: var(--m1-ink);
  font-family: var(--vp-font-family-base);
  font-size: 15px;
  display: flex;
  flex-direction: column;
  margin: 24px 0 36px;
}

.m1-top {
  background: var(--m1-bar-bg);
  color: var(--m1-bar-text);
  padding: 8px 16px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-bottom: 2px solid var(--m1-ink);
}

/* Chakra Petch for panel title */
.m1-title {
  font-family: var(--m1-font-display);
  font-weight: 700;
  letter-spacing: 0.06em;
}

.m1-title b {
  color: var(--m1-amber);
}

.m1-status {
  font-size: 13px;
  font-weight: 700;
  letter-spacing: 0.06em;
}

.m1-body {
  display: grid;
  grid-template-columns: 280px 1fr;
  min-height: 360px;
}

.m1-mods {
  border-right: 2px solid var(--m1-ink);
  display: flex;
  flex-direction: column;
  background: var(--m1-paper);
}

.m1-mod {
  all: unset;
  box-sizing: border-box;
  cursor: pointer;
  padding: 12px 16px;
  border-bottom: 2px solid var(--m1-ink);
  display: flex;
  justify-content: space-between;
  align-items: center;
  font-weight: 600;
}

.m1-dot {
  width: 10px;
  height: 10px;
  background: var(--m1-amber);
  border: 2px solid var(--m1-ink);
}

.m1-mod.off .m1-dot {
  background: transparent;
}

.m1-mod[aria-selected='true'] {
  background: var(--m1-ink);
  color: var(--m1-paper);
}

.m1-mod[aria-selected='true'] .m1-dot {
  border-color: var(--m1-paper);
}

.m1-set {
  padding: 16px 24px;
  display: flex;
  flex-direction: column;
  justify-content: space-between;
}

.m1-row {
  display: grid;
  grid-template-columns: 1fr 220px;
  align-items: center;
  gap: 16px;
  min-height: 40px;
  font-weight: 500;
}

.m1-row.dis {
  opacity: 0.4;
  pointer-events: none;
}

.m1-box {
  height: 32px;
  border: 2px solid var(--m1-ink);
  background: var(--m1-fill);
  color: var(--m1-ink);
  display: inline-flex;
  align-items: center;
  justify-content: center;
  gap: 6px;
  padding: 0 10px;
  cursor: pointer;
  font: inherit;
  font-weight: 600;
}

.m1-box:hover {
  background: var(--m1-paper);
}

.m1-sw {
  width: 110px;
  justify-self: end;
  padding: 0;
  gap: 0;
}

.m1-sw span {
  flex: 1;
  text-align: center;
  line-height: 28px;
  font-size: 13px;
  font-weight: 700;
}

.m1-sw[aria-checked='true'] .on {
  background: var(--m1-amber);
  color: #1a1b18;
}

.m1-sw[aria-checked='false'] .off {
  background: var(--m1-ink);
  color: var(--m1-paper);
}

.m1-sl {
  position: relative;
  padding: 0;
  justify-content: flex-end;
  overflow: hidden;
  touch-action: none;
  cursor: ew-resize;
}

.m1-sl .fill {
  position: absolute;
  left: 0;
  top: 0;
  bottom: 0;
  background: var(--m1-amber);
  width: var(--p);
}

.m1-sl .v {
  position: relative;
  padding: 0 8px;
  font-weight: 700;
}

.m1-pick {
  display: grid;
  grid-template-columns: 32px 1fr 32px;
  border: 2px solid var(--m1-ink);
}

.m1-pick .m1-box {
  border: 0;
  height: 28px;
  padding: 0;
}

.m1-pick .m1-box:nth-child(2) {
  border-left: 2px solid var(--m1-ink);
  border-right: 2px solid var(--m1-ink);
  cursor: default;
}

.m1-key {
  min-width: 150px;
  justify-self: end;
}

.m1-key.on {
  background: var(--m1-amber);
  color: #1a1b18;
}

.m1-btn {
  justify-self: end;
  background: var(--m1-ink);
  color: var(--m1-paper);
  padding: 0 18px;
  font-weight: 700;
  letter-spacing: 0.04em;
}

.m1-btn:hover {
  background: var(--m1-amber);
  color: #1a1b18;
}

.m1-foot {
  margin-top: 20px;
  padding-top: 14px;
  display: flex;
  justify-content: space-between;
  align-items: center;
  border-top: 2px solid var(--m1-ink);
}

.m1-foot small {
  color: var(--m1-mute);
  letter-spacing: 0.06em;
  font-weight: 700;
}

@media (max-width: 768px) {
  .m1-body {
    grid-template-columns: 1fr;
  }
  .m1-mods {
    border-right: 0;
    border-bottom: 2px solid var(--m1-ink);
    flex-direction: row;
    overflow-x: auto;
  }
  .m1-mod {
    border-bottom: 0;
    border-right: 2px solid var(--m1-ink);
    white-space: nowrap;
    gap: 10px;
  }
  .m1-row {
    grid-template-columns: 1fr;
    padding: 8px 0;
  }
  .m1-sw,
  .m1-key,
  .m1-btn {
    justify-self: start;
  }
}
</style>