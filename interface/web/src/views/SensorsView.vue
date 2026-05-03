<script setup lang="ts">
import { onMounted, onUnmounted, ref } from 'vue'

type SensorBlock = {
  state: number
  stateLabel: string
  supportsCal: boolean
}

const loading = ref(true)
const error = ref<string | null>(null)
const imu = ref<SensorBlock | null>(null)
const magnetometer = ref<SensorBlock | null>(null)
const headingDeg = ref(0)
const pitchDeg = ref(0)
const rollDeg = ref(0)

async function load() {
  loading.value = true
  error.value = null
  try {
    const r = await fetch('/api/v1/sensors')
    if (!r.ok) throw new Error(await r.text())
    const j = (await r.json()) as {
      imu: SensorBlock
      magnetometer: SensorBlock
      headingDeg: number
      pitchDeg: number
      rollDeg: number
    }
    imu.value = j.imu
    magnetometer.value = j.magnetometer
    headingDeg.value = j.headingDeg
    pitchDeg.value = j.pitchDeg
    rollDeg.value = j.rollDeg
  } catch (e) {
    error.value = e instanceof Error ? e.message : String(e)
  } finally {
    loading.value = false
  }
}

let timer: number | undefined
onMounted(() => {
  void load()
  timer = window.setInterval(() => void load(), 2000)
})
onUnmounted(() => {
  if (timer !== undefined) window.clearInterval(timer)
})
</script>

<template>
  <div>
    <h1 class="text-h4 font-weight-light text-primary mb-2">Sensors</h1>
    <p class="text-body-2 text-medium-emphasis mb-6">
      Status decoded from the same mmap region as the instruments page.
    </p>
    <v-alert v-if="error" type="error" class="mb-4" border="start">{{ error }}</v-alert>
    <v-progress-linear v-if="loading && !imu" indeterminate color="primary" class="mb-4" />
    <v-row v-if="imu && magnetometer">
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>IMU</v-card-title>
          <v-card-text>
            <v-list density="compact">
              <v-list-item title="State code" :subtitle="String(imu.state)" />
              <v-list-item title="State" :subtitle="imu.stateLabel" />
              <v-list-item title="Calibration" :subtitle="imu.supportsCal ? 'Supported' : 'No'" />
            </v-list>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>Magnetometer</v-card-title>
          <v-card-text>
            <v-list density="compact">
              <v-list-item title="State code" :subtitle="String(magnetometer.state)" />
              <v-list-item title="State" :subtitle="magnetometer.stateLabel" />
              <v-list-item
                title="Calibration"
                :subtitle="magnetometer.supportsCal ? 'Supported' : 'No'"
              />
            </v-list>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12">
        <v-card>
          <v-card-title>Fused angles (AHRS public)</v-card-title>
          <v-card-text>
            <v-chip class="me-2 mb-2" variant="tonal">Heading {{ headingDeg.toFixed(1) }}°</v-chip>
            <v-chip class="me-2 mb-2" variant="tonal">Pitch {{ pitchDeg.toFixed(1) }}°</v-chip>
            <v-chip class="mb-2" variant="tonal">Roll {{ rollDeg.toFixed(1) }}°</v-chip>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>
