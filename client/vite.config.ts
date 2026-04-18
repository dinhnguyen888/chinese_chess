import { defineConfig, loadEnv } from 'vite'
import react from '@vitejs/plugin-react'

// https://vite.dev/config/
export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '')
  
  const backendProxy = {
    '/api': {
      target: env.VITE_API_URL || 'http://127.0.0.1:8081',
      changeOrigin: true,
      rewrite: (path: string) => path.replace(/^\/api/, '') || '/',
    },
    '/ws': {
      target: env.VITE_WS_URL || 'ws://127.0.0.1:8080',
      ws: true,
      changeOrigin: true,
    },
  } as const

  return {
    plugins: [react()],
    server: { proxy: { ...backendProxy } },
    preview: { proxy: { ...backendProxy } },
  }
})
