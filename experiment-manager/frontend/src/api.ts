import type { Check, Experiment, Platform, Run, RunTrafficUE, TrafficJob, UE, VoiceGuardStatus } from './types'

async function request<T>(path: string, options?: RequestInit): Promise<T> {
  const response = await fetch(path, {
    ...options,
    headers: { 'Content-Type': 'application/json', ...options?.headers },
  })
  if (!response.ok) {
    let message = `${response.status} ${response.statusText}`
    try {
      const body = await response.json()
      message = typeof body.detail === 'string' ? body.detail : JSON.stringify(body.detail ?? body)
    } catch { /* response was not JSON */ }
    throw new Error(message)
  }
  if (response.status === 204) return undefined as T
  return response.json() as Promise<T>
}

export const api = {
  status: () => request<Platform>('/api/platform/status'),
  preflight: () => request<{ ok: boolean; checks: Check[] }>('/api/platform/preflight'),
  experiments: () => request<Experiment[]>('/api/experiments'),
  runs: () => request<Run[]>('/api/runs'),
  validate: (id: string) => request<{ ok: boolean; checks: Check[]; platform: { checks?: Check[] } }>(`/api/experiments/${id}/validate`, { method: 'POST' }),
  start: (id: string) => request<Run>(`/api/experiments/${id}/runs`, { method: 'POST' }),
  stop: (id: string) => request<Run>(`/api/runs/${id}/stop`, { method: 'POST' }),
  updateUE: (experimentId: string, ueId: string, body: Partial<UE>) => request<UE>(`/api/experiments/${experimentId}/ues/${ueId}`, { method: 'PATCH', body: JSON.stringify(body) }),
  traffic: (runId: string) => request<TrafficJob[]>(`/api/runs/${runId}/traffic`),
  trafficConfig: (runId: string) => request<{ run_id: string; ues: RunTrafficUE[] }>(`/api/runs/${runId}/traffic/config`),
  startTrafficBatch: (runId: string, ues: string[]) => request<TrafficJob[]>(`/api/runs/${runId}/traffic/batch`, { method: 'POST', body: JSON.stringify({ ues }) }),
  startTraffic: (runId: string, body: { ue: string; protocol: string; direction: string; duration: number; bitrate: string }) => request<TrafficJob>(`/api/runs/${runId}/traffic`, { method: 'POST', body: JSON.stringify(body) }),
  stopTraffic: (runId: string, jobId: string) => request<TrafficJob>(`/api/runs/${runId}/traffic/${jobId}`, { method: 'DELETE' }),
  stopAllTraffic: (runId: string) => request<TrafficJob[]>(`/api/runs/${runId}/traffic`, { method: 'DELETE' }),
  voiceGuard: (runId: string) => request<VoiceGuardStatus>(`/api/runs/${runId}/xapps/voiceguard`),
  startVoiceGuard: (
    runId: string,
    mode: 'observe_only' | 'closed_loop' = 'closed_loop',
    config: Record<string, number | string> = {},
  ) => request<VoiceGuardStatus>(`/api/runs/${runId}/xapps/voiceguard/start`, {
    method: 'POST',
    body: JSON.stringify({ mode, config }),
  }),
  stopVoiceGuard: (runId: string) => request<VoiceGuardStatus>(`/api/runs/${runId}/xapps/voiceguard/stop`, { method: 'POST' }),
  metric: (runId: string, metric: string) => request<{ data: { result: Array<{ metric: { ue?: string }; value: [number, string] }> } }>(`/api/runs/${runId}/metrics/query?metric=${encodeURIComponent(metric)}`),
  ueConfig: (experimentId: string, ueId: string) => request<{ experiment_id: string; ue_id: string; ue: string; path: string; custom: boolean; redacted: boolean; content: string; applies: string }>(`/api/experiments/${experimentId}/ues/${ueId}/config`),
  saveUEConfig: (experimentId: string, ueId: string, content: string) => request<{ experiment_id: string; ue_id: string; ue: string; path: string; custom: boolean; redacted: boolean; content: string; applies: string; revision: number }>(`/api/experiments/${experimentId}/ues/${ueId}/config`, { method: 'PUT', body: JSON.stringify({ content }) }),
  gnbConfig: (experimentId: string) => request<{ experiment_id: string; component: 'gnb'; path: string; custom: boolean; redacted: boolean; content: string; applies: string }>(`/api/experiments/${experimentId}/gnb/config`),
  saveGNBConfig: (experimentId: string, content: string) => request<{ experiment_id: string; component: 'gnb'; path: string; custom: boolean; redacted: boolean; content: string; applies: string; revision: number }>(`/api/experiments/${experimentId}/gnb/config`, { method: 'PUT', body: JSON.stringify({ content }) }),
}
