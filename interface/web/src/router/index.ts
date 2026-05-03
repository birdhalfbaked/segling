import { createRouter, createWebHistory } from 'vue-router'
import DashboardView from '@/views/DashboardView.vue'
import SensorsView from '@/views/SensorsView.vue'
import CalibrateView from '@/views/CalibrateView.vue'

export default createRouter({
  history: createWebHistory(import.meta.env.BASE_URL),
  routes: [
    { path: '/', name: 'dashboard', component: DashboardView },
    { path: '/sensors', name: 'sensors', component: SensorsView },
    { path: '/calibrate', name: 'calibrate', component: CalibrateView },
  ],
})
