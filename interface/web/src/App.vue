<script setup lang="ts">
import { RouterView } from 'vue-router'
</script>

<template>
  <v-app class="seg-app">
    <v-app-bar flat density="comfortable" color="surface">
      <v-toolbar-title class="text-h6 font-weight-medium">Segling</v-toolbar-title>
      <v-spacer />
      <v-btn variant="text" to="/" prepend-icon="mdi-compass">Instruments</v-btn>
      <v-btn variant="text" to="/sensors" prepend-icon="mdi-gauge">Sensors</v-btn>
      <v-btn variant="text" to="/calibrate" prepend-icon="mdi-tune">Calibrate</v-btn>
    </v-app-bar>
    <v-main>
      <v-container fluid class="py-6">
        <RouterView />
      </v-container>
    </v-main>
  </v-app>
</template>

<style>
html,
body,
#app {
  height: 100%;
  margin: 0;
}
.seg-app {
  background: radial-gradient(ellipse at top, #1a2332 0%, #0d1117 55%);
  min-height: 100vh;
}
</style>
