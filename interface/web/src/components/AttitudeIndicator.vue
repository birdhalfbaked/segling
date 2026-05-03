<script setup lang="ts">
const props = defineProps<{
  pitchDeg: number
  rollDeg: number
}>()

const cx = 100
const cy = 100
const pitchScale = 2.2
</script>

<template>
  <svg viewBox="0 0 200 200" class="ai-svg" aria-label="attitude indicator">
    <defs>
      <clipPath id="ai-clip">
        <circle :cx="cx" :cy="cy" r="92" />
      </clipPath>
    </defs>
    <circle :cx="cx" :cy="cy" r="96" fill="#0d1117" stroke="#546E7A" stroke-width="3" />
    <g :clip-path="'url(#ai-clip)'">
      <g
        :transform="`translate(${cx} ${cy}) rotate(${-props.rollDeg}) translate(${-cx} ${-cy}) translate(0 ${props.pitchDeg * pitchScale})`"
      >
        <rect x="-200" y="-400" width="600" height="400" fill="#1565C0" />
        <rect x="-200" y="0" width="600" height="400" fill="#5D4037" />
        <line x1="-200" y1="0" x2="400" y2="0" stroke="#ECEFF1" stroke-width="3" />
        <line x1="-200" y1="-40" x2="400" y2="-40" stroke="#B0BEC5" stroke-width="1" opacity="0.6" />
        <line x1="-200" y1="40" x2="400" y2="40" stroke="#B0BEC5" stroke-width="1" opacity="0.6" />
      </g>
    </g>
    <g pointer-events="none">
      <path
        d="M 100 12 L 108 28 L 100 24 L 92 28 Z"
        fill="none"
        stroke="#ECEFF1"
        stroke-width="1.5"
      />
      <line x1="20" y1="100" x2="36" y2="100" stroke="#ECEFF1" stroke-width="2" />
      <line x1="164" y1="100" x2="180" y2="100" stroke="#ECEFF1" stroke-width="2" />
    </g>
    <circle :cx="cx" :cy="cy" r="92" fill="none" stroke="#263238" stroke-width="6" opacity="0.35" />
  </svg>
</template>

<style scoped>
.ai-svg {
  width: min(320px, 90vw);
  height: auto;
  display: block;
  margin: 0 auto;
}
</style>
