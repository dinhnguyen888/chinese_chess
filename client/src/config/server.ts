/**
 * Khớp server: REST trên 8081 (/login, /register), WebSocket trên 8080 với ?token=jwt.
 *
 * Dev: Vite proxy /api → 127.0.0.1:8081, /ws → 127.0.0.1:8080.
 * Prod: dùng cùng origin (/api, /ws) và để Nginx proxy tới backend để tránh mixed content khi trang là HTTPS.
 */
const apiBase = (import.meta.env.VITE_HTTP_API_BASE as string | undefined)?.replace(/\/$/, '');
const wsBase = (import.meta.env.VITE_WS_BASE as string | undefined)?.replace(/\/$/, '');

export function httpApiUrl(path: string): string {
  const p = path.startsWith('/') ? path : `/${path}`;
  if (apiBase) return `${apiBase}${p}`;
  return `/api${p}`;
}

export function wsUrlWithToken(token: string): string {
  const q = `?token=${encodeURIComponent(token)}`;
  if (wsBase) return `${wsBase}${q}`;
  const proto = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
  return `${proto}//${window.location.host}/ws${q}`;
}
