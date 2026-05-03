<script setup lang="ts">
const props = defineProps<{
  headingDeg: number
}>()

function norm360(v: number) {
  let x = v % 360
  if (x < 0) x += 360
  return x
}
</script>

<template>
  <svg viewBox="0 0 220 220" class="compass-svg" aria-label="compass">
    <defs>
      <linearGradient id="ring" x1="0%" y1="0%" x2="100%" y2="100%">
        <stop offset="0%" stop-color="#263238" />
        <stop offset="100%" stop-color="#102027" />
      </linearGradient>
    </defs>
    <circle cx="110" cy="110" r="104" fill="url(#ring)" stroke="#4FC3F7" stroke-width="2" />
    <g :transform="`rotate(${-norm360(props.headingDeg)} 110 110)`">
      <text
        v-for="(deg, i) in [0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330]"
        :key="i"
        :x="110 + 88 * Math.sin((deg * Math.PI) / 180)"
        :y="110 - 88 * Math.cos((deg * Math.PI) / 180) + 5"
        text-anchor="middle"
        fill="#B0BEC5"
        font-size="11"
        class="compass-label"
      >
        {{
          deg === 0
            ? 'N'
            : deg === 90
              ? 'E'
              : deg === 180
                ? 'S'
                : deg === 270
                  ? 'W'
                  : deg
        }}
      </text>
    </g>
    <polygon points="110,18 104,42 116,42" fill="#FF7043" stroke="#BF360C" stroke-width="1" />
    <circle cx="110" cy="110" r="6" fill="#37474F" stroke="#90A4AE" stroke-width="1" />
    <text x="110" y="200" text-anchor="middle" fill="#ECEFF1" font-size="14" font-weight="600">
      {{ norm360(props.headingDeg).toFixed(1) }}°
    </text>
  </svg>
</template>

<style scoped>
.compass-svg {
  width: min(320px, 90vw);
  height: auto;
  display: block;
  margin: 0 auto;
}
.compass-label {
  font-family: system-ui, sans-serif;
}
</style>
