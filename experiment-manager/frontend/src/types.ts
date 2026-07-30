export type ProcessInfo = { pid: number; running: boolean; command: string }

export type Platform = {
  state: 'RUNNING' | 'STOPPED'
  components: Record<string, ProcessInfo>
  services: Record<string, boolean>
  mongodb: boolean
  prometheus: boolean
  grafana: boolean
  ports: Record<string, boolean>
}

export type Check = { id: string; status: 'pass' | 'fail'; message: string }

export type Channel = {
  enabled: boolean
  awgn_enabled: boolean
  awgn_snr: number
  awgn_signal_power: number
  fading_enabled: boolean
  fading_model: string
  delay_enabled: boolean
  delay_minimum_us: number
  delay_maximum_us: number
  delay_period_s: number
  delay_init_time_s: number
  rlf_enabled: boolean
  rlf_t_on_ms: number
  rlf_t_off_ms: number
  hst_enabled: boolean
  hst_fd_hz: number
  hst_period_s: number
  hst_init_time_s: number
}

export type UE = {
  id: string
  experiment_id: string
  slot: number
  display_name: string
  enabled: boolean
  imsi: string
  imei: string
  credential_profile: string
  apn: string
  sst: number
  sd: string
  rx_port: number
  tx_port: number
  namespace: string
  path_loss_db: number
  channel: Channel
  traffic_defaults: TrafficDefaults | Record<string, unknown>
}

export type TrafficType = 'none' | 'ping' | 'iperf' | 'http' | 'short_video' | 'social' | 'navigation' | 'rtp_voice'

export type TrafficFlow = {
  type: TrafficType
  application_protocol: string
  transport: 'none' | 'icmp' | 'tcp' | 'udp'
  direction: 'UL' | 'DL' | 'BOTH'
  run_mode: 'duration' | 'continuous'
  duration_seconds: number | null
  params: Record<string, string | number | boolean>
}

export type TrafficDefaults = { version: 2; flows: TrafficFlow[] }

export type RunTrafficUE = {
  ue: string
  display_name: string
  enabled: boolean
  traffic: TrafficDefaults
}

export type Experiment = {
  id: string
  name: string
  description: string
  expected_ue_count: number
  broker_capacity: number
  monitoring_enabled: boolean
  scenario: string
  revision: number
  created_at: string
  updated_at: string
  ues: UE[]
}

export type Run = {
  id: string
  experiment_id: string
  experiment_revision: number
  state: string
  operation: string | null
  started_at: string | null
  stopped_at: string | null
  snapshot_path: string | null
  allocated_ues: Record<string, unknown>
  result_summary: Record<string, unknown>
}

export type TrafficJob = {
  id: string
  run_id: string
  ue: string
  batch_id: string | null
  traffic_type: TrafficType
  application_protocol: string
  transport: string
  protocol: 'ping' | 'tcp' | 'udp'
  direction: 'UL' | 'DL' | 'BOTH'
  target: string
  port: number
  duration: number
  duration_seconds: number | null
  run_mode: 'duration' | 'continuous'
  bitrate: string
  parameters: Record<string, string | number | boolean>
  status: string
  created_at: string
  started_at: string | null
  finished_at: string | null
  result: Record<string, unknown>
}

export type VoiceGuardUE = {
  offered_bps?: number
  delivered_bps?: number
  latency_ms?: number | null
  loss_percent?: number | null
  jitter_ms?: number | null
  delivery_ratio?: number | null
}

export type VoiceGuardStatus = {
  run_id: string
  pid?: number
  running: boolean
  state: 'OFF' | 'STARTING' | 'OBSERVING' | 'WOULD_PROTECT' | 'PROTECTING' | 'COOLDOWN' | 'STOPPING' | 'ERROR'
  mode: 'observe_only' | 'closed_loop'
  e2_adapter?: string
  e2_connected?: boolean
  native_control: boolean
  current_policy?: string
  last_decision?: string
  last_sample_at?: number | null
  voice_active?: boolean
  total_video_offered_bps?: number
  total_video_delivered_bps?: number
  traffic_shaping_factor?: number
  actuator?: string
  consecutive_bad_samples?: number
  last_error?: string | null
  last_rc?: {
    success: boolean
    timestamp: number
    duration_ms?: number
    error?: string | null
    policies?: Array<{ ue_id: number; minimum: number; maximum: number; dedicated: number }>
  }
  ue_mapping?: Record<string, number>
  ues?: Record<string, VoiceGuardUE>
  events?: Array<{ timestamp: number; type: string; message: string }>
}
