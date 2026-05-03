<script setup lang="ts">
import { ref } from 'vue'

const busy = ref(false)
const message = ref<string | null>(null)
const error = ref<string | null>(null)

async function post(path: string, label: string) {
  busy.value = true
  message.value = null
  error.value = null
  try {
    const r = await fetch(path, { method: 'POST' })
    if (!r.ok) throw new Error(await r.text())
    const j = (await r.json()) as { queued?: string }
    message.value = `Calibration request queued (${j.queued ?? label}). The controller clears the command slot after handling.`
  } catch (e) {
    error.value = e instanceof Error ? e.message : String(e)
  } finally {
    busy.value = false
  }
}
</script>

<template>
  <div>
    <h1 class="text-h4 font-weight-light text-primary mb-2">Calibrate</h1>
    <p class="text-body-2 text-medium-emphasis mb-6">
      When the firmware supports it, these actions write a command byte into mmap file
      <code class="text-primary">FILE_ID_COMMAND</code> (15). The controller loop consumes
      it and returns the slot to idle.
    </p>
    <v-alert v-if="message" type="success" class="mb-4" border="start" closable @click:close="message = null">
      {{ message }}
    </v-alert>
    <v-alert v-if="error" type="error" class="mb-4" border="start">{{ error }}</v-alert>
    <v-row>
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>IMU</v-card-title>
          <v-card-text>
            Gyro / accelerometer bias calibration (firmware-dependent).
            <v-btn
              class="mt-4"
              color="primary"
              block
              :loading="busy"
              prepend-icon="mdi-axis-arrow"
              @click="post('/api/v1/calibrate/imu', 'imu')"
            >
              Start IMU calibration
            </v-btn>
          </v-card-text>
        </v-card>
      </v-col>
      <v-col cols="12" md="6">
        <v-card>
          <v-card-title>Magnetometer</v-card-title>
          <v-card-text>
            Compass / soft-iron calibration (firmware-dependent).
            <v-btn
              class="mt-4"
              color="secondary"
              block
              :loading="busy"
              prepend-icon="mdi-compass-outline"
              @click="post('/api/v1/calibrate/compass', 'compass')"
            >
              Start compass calibration
            </v-btn>
          </v-card-text>
        </v-card>
      </v-col>
    </v-row>
  </div>
</template>
