export interface Snapshot {
  imuState: number
  magnetometerState: number
  headingDeg: number
  pitchDeg: number
  rollDeg: number
  yawDeg: number
  rotationRateX: number
  rotationRateY: number
  rotationRateZ: number
  imuStateLabel: string
  magnetometerStateLabel: string
}
