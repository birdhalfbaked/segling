<script setup lang="ts">
import { onMounted, onUnmounted, ref, shallowRef } from 'vue'
import type { Snapshot } from '@/api/types'
import CompassRose from '@/components/CompassRose.vue'
import AttitudeIndicator from '@/components/AttitudeIndicator.vue'

const snap = shallowRef<Snapshot | null>(null)
const connected = ref(false)
const err = ref<string | null>(null)
let ws: WebSocket | null = null

function wsURL() {
  const proto = location.protocol === 'https:' ? 'wss:' : 'ws:'
  return `${proto}//${location.host}/api/v1/stream`
}

onMounted(() => {
  ws = new WebSocket(wsURL())
  ws.onopen = () => {
    connected.value = true
    err.value = null
  }
  ws.onclose = () => {
    connected.value = false
  }
  ws.onerror = () => {
    err.value = 'WebSocket error — is the Go server running on :8787 with Vite proxy?'
  }
  ws.onmessage = (ev) => {
    try {
      snap.value = JSON.parse(ev.data as string) as Snapshot
    } catch {
      err.value = 'Invalid telemetry frame'
    }
  }
})

onUnmounted(() => {
  ws?.close()
  ws = null
})
</script>

<template>
  <div>
    <v-row align="center" class="mb-2">
      <v-col cols="12" md="8">
        <h1 class="text-h4 font-weight-light text-primary">Instruments</h1>
        <p class="text-body-2 text-medium-emphasis">
          Live data from read-only mmap <code class="text-primary">state.bin</code> (file slot 1, AHRS public); commands use <code class="text-primary">commands.bin</code>.
        </p>
      </v-col>
      <v-col cols="12" md="4" class="text-md-end">
        <v-chip :color="connected ? 'success' : 'warning'" variant="flat" size="small">
          {{ connected ? 'Live stream' : 'Disconnected' }}
        </v-chip>
      </v-col>
    </v-row>
    <v-alert v-if="err" type="warning" density="compact" class="mb-4" border="start">
      {{ err }}
    </v-alert>
    <v-row v-if="snap">
      <v-col cols="12" md="6" class="d-flex justify-center">
        <v-sheet rounded="lg" class="pa-4 instrument-card" elevation="4">
          <div class="text-subtitle-2 text-center text-medium-emphasis mb-2">Heading</div>
          <CompassRose :heading-deg="snap.headingDeg" />
        </v-sheet>
      </v-col>
      <v-col cols="12" md="6" class="d-flex justify-center">
        <v-sheet rounded="lg" class="pa-4 instrument-card" elevation="4">
          <div class="text-subtitle-2 text-center text-medium-emphasis mb-2">Attitude</div>
          <AttitudeIndicator :pitch-deg="snap.pitchDeg" :roll-deg="snap.rollDeg" />
          <div class="text-caption text-center text-medium-emphasis mt-2">
            Pitch {{ snap.pitchDeg.toFixed(1) }}° · Roll {{ snap.rollDeg.toFixed(1) }}°
          </div>
        </v-sheet>
      </v-col>
    </v-row>
    <v-row v-else>
      <v-col cols="12" class="text-center text-medium-emphasis">Waiting for telemetry…</v-col>
    </v-row>
  </div>
</template>

<style scoped>
.instrument-card {
  background: rgba(22, 32, 48, 0.85) !important;
  border: 1px solid rgba(79, 195, 247, 0.15);
  min-height: 360px;
  align-items: center;
  justify-content: center;
  display: flex;
  flex-direction: column;
}
</style>
