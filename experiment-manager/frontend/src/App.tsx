import { useCallback, useEffect, useMemo, useState } from 'react'
import {
  Activity, Antenna, Check, ChevronRight, CircleStop, Database, FlaskConical,
  FileText, Gauge, Network, Play, Radio, RefreshCw, Server, Settings2, ShieldCheck,
  SlidersHorizontal, Wifi, X, Zap,
} from 'lucide-react'
import { api } from './api'
import type { Check as CheckType, Experiment, Platform, Run, RunTrafficUE, TrafficDefaults, TrafficFlow, TrafficJob, TrafficType, UE, VoiceGuardStatus } from './types'

type Page = 'overview' | 'experiments' | 'live'
type VoiceHistory = {
  offered: number[]
  received: number[]
  delivery: number[]
  loss: number[]
  jitter: number[]
  rtt: number[]
  states: string[]
}
type ConfigView = {
  experiment_id: string
  component: 'gnb' | 'ue'
  ue_id?: string
  label: string
  path: string
  content: string
  custom: boolean
  redacted: boolean
}

const componentNames: Record<string, string> = {
  'nearRT-RIC': 'Near-RT RIC', gnb: 'gNB', broker: 'Radio Broker',
}

function StateDot({ up }: { up: boolean }) {
  return <span className={`state-dot ${up ? 'up' : 'down'}`}><span /></span>
}

function Pill({ up, children }: { up: boolean; children: React.ReactNode }) {
  return <span className={`pill ${up ? 'good' : 'muted'}`}><StateDot up={up} />{children}</span>
}

const trafficLabels: Record<TrafficType, string> = {
  none: '無流量', ping: 'PING', iperf: '容量測試', http: 'HTTP 傳輸',
  short_video: '短影片', social: '社群瀏覽', navigation: '導航', rtp_voice: '語音通話',
}

const profileDefaults: Record<TrafficType, TrafficFlow> = {
  none: { type: 'none', application_protocol: 'none', transport: 'none', direction: 'UL', run_mode: 'duration', duration_seconds: 60, params: {} },
  ping: { type: 'ping', application_protocol: 'ping', transport: 'icmp', direction: 'UL', run_mode: 'duration', duration_seconds: 60, params: { interval_ms: 1000, packet_size: 56 } },
  iperf: { type: 'iperf', application_protocol: 'iperf3', transport: 'udp', direction: 'UL', run_mode: 'duration', duration_seconds: 60, params: { bitrate: '750K' } },
  http: { type: 'http', application_protocol: 'http', transport: 'tcp', direction: 'DL', run_mode: 'duration', duration_seconds: 60, params: { object_size_kb: 256, interval_ms: 1000 } },
  short_video: {
    type: 'short_video', application_protocol: 'http', transport: 'tcp', direction: 'DL',
    run_mode: 'continuous', duration_seconds: null,
    params: {
      offered_load_mbps: 0.8, traffic_pattern: 'wave', variation_percent: 30,
      peak_limit_mbps: 1.2, random_seed: 1234, pattern_period_seconds: 20,
      segment_interval_ms: 1000,
    },
  },
  social: { type: 'social', application_protocol: 'http', transport: 'tcp', direction: 'DL', run_mode: 'continuous', duration_seconds: null, params: { object_size_kb: 180, objects_per_cycle: 4, cycle_interval_ms: 4000 } },
  navigation: { type: 'navigation', application_protocol: 'http', transport: 'tcp', direction: 'BOTH', run_mode: 'continuous', duration_seconds: null, params: { tile_size_kb: 40, tiles_per_cycle: 6, update_interval_ms: 5000 } },
  rtp_voice: { type: 'rtp_voice', application_protocol: 'rtp-like', transport: 'udp', direction: 'BOTH', run_mode: 'duration', duration_seconds: 300, params: { packet_interval_ms: 20, bitrate_kbps: 64 } },
}

function trafficFlowOf(raw: TrafficDefaults | Record<string, unknown>): TrafficFlow {
  const flows = (raw as TrafficDefaults)?.flows
  if (Array.isArray(flows) && flows[0]) return { ...flows[0], params: { ...flows[0].params } }
  const legacy = raw as Record<string, unknown>
  const protocol = String(legacy.protocol ?? 'udp')
  const type: TrafficType = protocol === 'ping' ? 'ping' : 'iperf'
  return {
    ...profileDefaults[type],
    transport: protocol === 'tcp' ? 'tcp' : profileDefaults[type].transport,
    direction: String(legacy.direction ?? 'UL') as TrafficFlow['direction'],
    duration_seconds: Number(legacy.duration ?? 60),
    params: { ...profileDefaults[type].params, bitrate: String(legacy.bitrate ?? '750K') },
  }
}

function trafficSummary(flow: TrafficFlow) {
  const protocol = flow.type === 'iperf' ? `iperf3/${flow.transport.toUpperCase()}` : `${flow.application_protocol.toUpperCase()}/${flow.transport.toUpperCase()}`
  const duration = flow.run_mode === 'continuous' ? '持續執行' : `${flow.duration_seconds ?? 0}s`
  return `${trafficLabels[flow.type]} · ${protocol} · ${flow.direction} · ${duration}`
}

function parseDraftNumber(value: string) {
  if (!value.trim()) return null
  const parsed = Number(value)
  return Number.isFinite(parsed) ? parsed : null
}

function App() {
  const [page, setPage] = useState<Page>('overview')
  const [platform, setPlatform] = useState<Platform | null>(null)
  const [experiments, setExperiments] = useState<Experiment[]>([])
  const [runs, setRuns] = useState<Run[]>([])
  const [selectedId, setSelectedId] = useState<string>('')
  const [checks, setChecks] = useState<CheckType[] | null>(null)
  const [busy, setBusy] = useState('')
  const [error, setError] = useState('')
  const [lastUpdate, setLastUpdate] = useState<Date | null>(null)
  const [configView, setConfigView] = useState<ConfigView | null>(null)
  const [configNotice, setConfigNotice] = useState('')

  const refresh = useCallback(async (quiet = false) => {
    try {
      const [nextPlatform, nextExperiments, nextRuns] = await Promise.all([
        api.status(), api.experiments(), api.runs(),
      ])
      setPlatform(nextPlatform)
      setExperiments(nextExperiments)
      setRuns(nextRuns)
      if (!selectedId && nextExperiments[0]) {
        const running = nextRuns.find(run => ['STARTING', 'RUNNING', 'STOPPING', 'DEGRADED'].includes(run.state))
        setSelectedId(running?.experiment_id ?? nextExperiments[0].id)
      }
      setLastUpdate(new Date())
      if (!quiet) setError('')
    } catch (reason) {
      if (!quiet) setError(reason instanceof Error ? reason.message : String(reason))
    }
  }, [selectedId])

  useEffect(() => {
    void refresh()
    const timer = window.setInterval(() => void refresh(true), 3000)
    return () => window.clearInterval(timer)
  }, [refresh])

  const selected = experiments.find(item => item.id === selectedId) ?? experiments[0]
  const activeRun = runs.find(run => ['STARTING', 'RUNNING', 'STOPPING', 'DEGRADED'].includes(run.state))
  const activeExperiment = experiments.find(experiment => experiment.id === activeRun?.experiment_id)

  async function action(name: string, job: () => Promise<unknown>) {
    setBusy(name); setError('')
    try { await job(); await refresh() }
    catch (reason) { setError(reason instanceof Error ? reason.message : String(reason)) }
    finally { setBusy('') }
  }

  async function openConfig(experimentId: string | undefined, ue: UE | undefined) {
    if (!experimentId || !ue) { setError('找不到這台 UE 的 Config'); return }
    setBusy(`config-${ue.id}`); setConfigNotice('')
    try {
      const config = await api.ueConfig(experimentId, ue.id)
      setConfigView({ ...config, component: 'ue', label: config.ue, redacted: true })
    }
    catch (reason) { setError(reason instanceof Error ? reason.message : String(reason)) }
    finally { setBusy('') }
  }

  async function openGNBConfig(experimentId: string | undefined) {
    if (!experimentId) { setError('找不到這份 Experiment 的 gNB Config'); return }
    setBusy('config-gnb'); setConfigNotice('')
    try {
      const config = await api.gnbConfig(experimentId)
      setConfigView({ ...config, label: 'gNB' })
    }
    catch (reason) { setError(reason instanceof Error ? reason.message : String(reason)) }
    finally { setBusy('') }
  }

  async function saveConfig() {
    if (!configView) return
    setBusy('save-config'); setConfigNotice('')
    try {
      const saved = configView.component === 'gnb'
        ? await api.saveGNBConfig(configView.experiment_id, configView.content)
        : await api.saveUEConfig(configView.experiment_id, configView.ue_id!, configView.content)
      setConfigView({ ...configView, ...saved })
      setConfigNotice(`已儲存為 Experiment revision ${saved.revision}，下次啟動生效`)
      await refresh(true)
    } catch (reason) { setConfigNotice(reason instanceof Error ? reason.message : String(reason)) }
    finally { setBusy('') }
  }

  function openRunConfig(run: Run | undefined, ueName: string) {
    const experiment = experiments.find(item => item.id === run?.experiment_id) ?? selected
    const slot = Number(ueName.replace('ue', ''))
    void openConfig(experiment?.id, experiment?.ues.find(ue => ue.slot === slot))
  }

  return <div className="shell">
    <aside className="sidebar">
      <div className="brand"><div className="brand-mark"><Radio size={22} /></div><div><b>O-RAN</b><span>Experiment Manager</span></div></div>
      <nav>
        <button className={page === 'overview' ? 'active' : ''} onClick={() => setPage('overview')}><Gauge />平台總覽</button>
        <button className={page === 'experiments' ? 'active' : ''} onClick={() => setPage('experiments')}><FlaskConical />實驗設定</button>
        <button className={page === 'live' ? 'active' : ''} onClick={() => setPage('live')}><Activity />即時執行</button>
      </nav>
      <div className="sidebar-bottom">
        <div className="platform-mini"><StateDot up={platform?.state === 'RUNNING'} /><div><span>Radio Stack</span><b>{platform?.state ?? '讀取中'}</b></div></div>
        <div className="version">CONTROL PLANE · v0.1</div>
      </div>
    </aside>

    <main>
      <header>
        <div><span className="eyebrow">LOCAL O-RAN LAB</span><h1>{page === 'overview' ? '平台總覽' : page === 'experiments' ? '實驗設定' : '即時執行'}</h1></div>
        <div className="header-actions">{activeRun && <button className="danger header-stop" disabled={busy === 'stop'} onClick={() => action('stop', () => api.stop(activeRun.id))}><CircleStop />{busy === 'stop' ? '停止中…' : '停止實驗'}</button>}<span className="updated">{lastUpdate ? `${lastUpdate.toLocaleTimeString()} 更新` : '同步中'}</span><button className="icon-button" onClick={() => void refresh()}><RefreshCw size={17} /></button></div>
      </header>

      {error && <div className="alert"><X size={18} /><span>{error}</span><button onClick={() => setError('')}><X size={15} /></button></div>}
      {page === 'overview' && <Overview platform={platform} activeRun={activeRun} ueCount={activeExperiment?.expected_ue_count ?? 3} checks={checks} busy={busy} onPreflight={() => action('preflight', async () => setChecks((await api.preflight()).checks))} onNavigate={setPage} />}
      {page === 'experiments' && <Experiments experiments={experiments} selectedId={selected?.id ?? ''} onSelect={setSelectedId} selected={selected} checks={checks} busy={busy} onValidate={() => selected && action('validate', async () => setChecks((await api.validate(selected.id)).checks))} onStart={() => selected && action('start', () => api.start(selected.id))} onUpdateUE={(ue, body) => selected && action(`ue-${ue.id}`, () => api.updateUE(selected.id, ue.id, body))} onBatchUpdate={(ues, buildBody) => selected && action('ue-batch', () => Promise.all(ues.map(ue => api.updateUE(selected.id, ue.id, buildBody(ue)))))} onViewConfig={(experimentId, ue) => void openConfig(experimentId, ue)} onViewGNBConfig={experimentId => void openGNBConfig(experimentId)} />}
      {page === 'live' && <Live platform={platform} activeRun={activeRun} latestRun={runs[0]} busy={busy} onStop={() => activeRun && action('stop', () => api.stop(activeRun.id))} onViewConfig={openRunConfig} />}
    </main>
    {configView && <div className="modal-backdrop" onMouseDown={() => setConfigView(null)}><section className="config-modal" onMouseDown={event => event.stopPropagation()}><div className="config-modal-head"><div><span className="eyebrow">EDITABLE CONFIG{configView.redacted ? ' · SECRETS REDACTED' : ' · YAML VALIDATED'}</span><h2>{configView.label.toUpperCase()} Config</h2><p>{configView.path}</p></div><button className="icon-button" onClick={() => setConfigView(null)}><X size={17} /></button></div><div className="config-warning">{configView.component === 'gnb' ? '直接修改 gNB YAML。後端會驗證 YAML、必要區段與受管 ZMQ endpoints；儲存後於下次啟動生效。' : '直接修改 srsUE config。儲存後會用於下次啟動；目前正在運行的 UE 不會即時重載。K、OPc、PIN 由後端保留。'}</div><textarea spellCheck={false} value={configView.content} onChange={event => setConfigView({ ...configView, content: event.target.value })} /><div className="config-modal-actions"><span>{configNotice}</span><button className="secondary" onClick={() => setConfigView(null)}>取消</button><button className="primary" disabled={busy === 'save-config'} onClick={() => void saveConfig()}>{busy === 'save-config' ? '儲存中…' : '儲存 Config'}</button></div></section></div>}
  </div>
}

function Overview({ platform, activeRun, ueCount, checks, busy, onPreflight, onNavigate }: { platform: Platform | null; activeRun?: Run; ueCount: number; checks: CheckType[] | null; busy: string; onPreflight: () => void; onNavigate: (page: Page) => void }) {
  const services = [
    ['Open5GS Core', !!platform && Object.values(platform.services).every(Boolean), Server],
    ['MongoDB', !!platform?.mongodb, Database], ['Prometheus', !!platform?.prometheus, Activity],
    ['Grafana', !!platform?.grafana, Gauge],
  ] as const
  return <div className="content">
    <section className="hero">
      <div><div className="hero-kicker"><Zap size={14} /> EXPERIMENT CONTROL</div><h2>{platform?.state === 'RUNNING' ? '實驗正在運行' : '平台已就緒，可以開始實驗'}</h2><p>從環境預檢、Radio Stack 啟停，到每台 UE 的 channel 與流量控制，都集中在同一個工作區。</p><div className="hero-actions"><button className="primary" onClick={() => onNavigate(activeRun ? 'live' : 'experiments')}>{activeRun ? <Activity /> : <Play />}{activeRun ? '查看目前執行' : '設定並啟動實驗'}</button><button className="secondary" disabled={busy === 'preflight'} onClick={onPreflight}><ShieldCheck />{busy === 'preflight' ? '檢查中…' : '執行環境預檢'}</button></div></div>
      <div className="signal-art"><div className="orb"><Antenna /></div><i /><i /><i /></div>
    </section>

    <div className="section-title"><div><h3>常駐服務</h3><p>實驗以外持續運行的基礎設施</p></div><Pill up={services.every(item => item[1])}>{services.filter(item => item[1]).length} / {services.length} 正常</Pill></div>
    <section className="service-grid">{services.map(([name, up, Icon]) => <article className="service-card" key={name}><div className="service-icon"><Icon /></div><div><span>{name}</span><b>{up ? '正常運行' : '未連線'}</b></div><StateDot up={up} /></article>)}</section>

    <div className="section-title"><div><h3>Radio Stack</h3><p>目前受 Experiment Manager 管理的元件</p></div></div>
    <section className="topology">
      {['nearRT-RIC', 'gnb', 'broker', ...Array.from({ length: ueCount }, (_, index) => `ue${index + 1}`)].map(name => <div className="topology-item" key={name}><div className="node"><StateDot up={!!platform?.components[name]?.running} /><span>{componentNames[name] ?? name.toUpperCase()}</span><small>{platform?.components[name]?.running ? `PID ${platform.components[name].pid}` : 'stopped'}</small></div></div>)}
    </section>
    {checks && <CheckPanel checks={checks} />}
  </div>
}

function CheckPanel({ checks }: { checks: CheckType[] }) {
  const passed = checks.filter(item => item.status === 'pass').length
  return <section className="panel checks"><div className="panel-head"><div><h3>檢查結果</h3><p>{passed} / {checks.length} 通過</p></div><Pill up={passed === checks.length}>{passed === checks.length ? '可啟動' : '需要處理'}</Pill></div><div className="check-list">{checks.map(item => <div key={item.id}><span className={item.status}><Check size={14} /></span><b>{item.id}</b><small>{item.message}</small></div>)}</div></section>
}

function Experiments({ experiments, selectedId, onSelect, selected, checks, busy, onValidate, onStart, onUpdateUE, onBatchUpdate, onViewConfig, onViewGNBConfig }: { experiments: Experiment[]; selectedId: string; onSelect: (id: string) => void; selected?: Experiment; checks: CheckType[] | null; busy: string; onValidate: () => void; onStart: () => void; onUpdateUE: (ue: UE, body: Partial<UE>) => void; onBatchUpdate: (ues: UE[], buildBody: (ue: UE) => Partial<UE>) => void; onViewConfig: (experimentId: string, ue: UE) => void; onViewGNBConfig: (experimentId: string) => void }) {
  const [editing, setEditing] = useState<string>('')
  const groupedScenario = (selected?.ues.length ?? 0) >= 10
  const videoUEs = selected?.ues.filter(ue => ue.slot <= 8) ?? []
  const voiceUEs = selected?.ues.filter(ue => ue.slot > 8) ?? []
  const totalOfferedLoad = selected?.ues.reduce((total, ue) => {
    const flow = trafficFlowOf(ue.traffic_defaults)
    if (!ue.enabled) return total
    if (flow.type === 'short_video') return total + Number(flow.params.offered_load_mbps ?? 0)
    if (flow.type === 'rtp_voice') return total + Number(flow.params.bitrate_kbps ?? 0) / 1000
    return total
  }, 0) ?? 0
  return <div className="content experiment-workspace">
    <div className="editor-stack">
      {selected ? <>
        <section className="panel definition"><div className="panel-head"><div><span className="eyebrow">REVISION {selected.revision}</span><h2>{selected.name}</h2><p>{selected.description}</p></div><div><select aria-label="選擇實驗" value={selectedId} disabled={!!busy} onChange={event => onSelect(event.target.value)}>{experiments.map(experiment => <option key={experiment.id} value={experiment.id}>{experiment.name} · {experiment.expected_ue_count} UE</option>)}</select><Pill up={selected.monitoring_enabled}>監控啟用</Pill></div></div><div className="definition-grid"><div><span>預期 UE</span><b>{selected.expected_ue_count}</b></div><div><span>總 Target Offered Load</span><b>{totalOfferedLoad.toFixed(2)} Mbps</b></div><div><span>Scenario</span><b>{selected.scenario}</b></div><div><span>拓撲</span><b>FDD · ZMQ</b></div></div><div className="button-row"><button className="secondary" disabled={!!busy} onClick={() => onViewGNBConfig(selected.id)}><FileText />gNB Config</button><button className="secondary" disabled={!!busy} onClick={onValidate}><ShieldCheck />驗證設定</button><button className="primary" disabled={!!busy} onClick={onStart}><Play />{busy === 'start' ? '啟動中…' : '啟動實驗'}</button></div></section>
        <div className="section-title"><div><h3>UE 流量角色與 Channel</h3><p>可先批次設定整組流量，再展開單台 UE 微調；變更會用於下一次 Run</p></div></div>
        {groupedScenario ? <>
          <UEBatchPanel kind="video" ues={videoUEs} saving={busy === 'ue-batch'} onApply={buildBody => onBatchUpdate(videoUEs, buildBody)} />
          <div className="ue-group-list">
            {videoUEs.map(ue => <UECard key={ue.id} role="影片" ue={ue} experimentId={selected.id} open={editing === ue.id} setOpen={() => setEditing(editing === ue.id ? '' : ue.id)} saving={busy === `ue-${ue.id}`} onSave={body => onUpdateUE(ue, body)} onViewConfig={onViewConfig} />)}
          </div>
          <UEBatchPanel kind="voice" ues={voiceUEs} saving={busy === 'ue-batch'} onApply={buildBody => onBatchUpdate(voiceUEs, buildBody)} />
          <div className="ue-group-list">
            {voiceUEs.map(ue => <UECard key={ue.id} role="通話" ue={ue} experimentId={selected.id} open={editing === ue.id} setOpen={() => setEditing(editing === ue.id ? '' : ue.id)} saving={busy === `ue-${ue.id}`} onSave={body => onUpdateUE(ue, body)} onViewConfig={onViewConfig} />)}
          </div>
        </> : selected.ues.map(ue => <UECard key={ue.id} ue={ue} experimentId={selected.id} open={editing === ue.id} setOpen={() => setEditing(editing === ue.id ? '' : ue.id)} saving={busy === `ue-${ue.id}`} onSave={body => onUpdateUE(ue, body)} onViewConfig={onViewConfig} />)}
        {checks && <CheckPanel checks={checks} />}
      </> : <section className="panel empty">尚未建立實驗設定</section>}
    </div>
  </div>
}

function UEBatchPanel({ kind, ues, saving, onApply }: {
  kind: 'video' | 'voice'
  ues: UE[]
  saving: boolean
  onApply: (buildBody: (ue: UE) => Partial<UE>) => void
}) {
  const firstFlow = trafficFlowOf(ues[0]?.traffic_defaults ?? profileDefaults[kind === 'video' ? 'short_video' : 'rtp_voice'])
  const sourceSignature = JSON.stringify(ues.map(ue => ue.traffic_defaults))
  const [offered, setOffered] = useState(Number(firstFlow.params.offered_load_mbps ?? 1))
  const [variation, setVariation] = useState(Number(firstFlow.params.variation_percent ?? 35))
  const [peak, setPeak] = useState(Number(firstFlow.params.peak_limit_mbps ?? 1.35))
  const [pattern, setPattern] = useState(String(firstFlow.params.traffic_pattern ?? 'wave'))
  const [voiceBitrate, setVoiceBitrate] = useState(Number(firstFlow.params.bitrate_kbps ?? 96))
  const [packetInterval, setPacketInterval] = useState(Number(firstFlow.params.packet_interval_ms ?? 20))
  const [voiceRunMode, setVoiceRunMode] = useState<TrafficFlow['run_mode']>(firstFlow.run_mode)
  const [voiceDuration, setVoiceDuration] = useState(Number(firstFlow.duration_seconds ?? 300))
  const [draftError, setDraftError] = useState('')

  useEffect(() => {
    if (!ues[0]) return
    const flow = trafficFlowOf(ues[0].traffic_defaults)
    setOffered(Number(flow.params.offered_load_mbps ?? 1))
    setVariation(Number(flow.params.variation_percent ?? 35))
    setPeak(Number(flow.params.peak_limit_mbps ?? 1.35))
    setPattern(String(flow.params.traffic_pattern ?? 'wave'))
    setVoiceBitrate(Number(flow.params.bitrate_kbps ?? 96))
    setPacketInterval(Number(flow.params.packet_interval_ms ?? 20))
    setVoiceRunMode(flow.run_mode)
    setVoiceDuration(Number(flow.duration_seconds ?? 300))
    setDraftError('')
  }, [sourceSignature])

  const apply = () => {
    if (kind === 'video') {
      if (peak < offered) {
        setDraftError('Peak Limit 必須大於或等於每台 Offered Load')
        return
      }
      onApply(ue => {
        const current = trafficFlowOf(ue.traffic_defaults)
        const base = current.type === 'short_video' ? current : profileDefaults.short_video
        return {
          traffic_defaults: {
            version: 2,
            flows: [{
              ...base,
              type: 'short_video',
              application_protocol: 'http',
              transport: 'tcp',
              direction: 'DL',
              run_mode: 'continuous',
              duration_seconds: null,
              params: {
                ...base.params,
                offered_load_mbps: offered,
                variation_percent: variation,
                peak_limit_mbps: peak,
                traffic_pattern: pattern,
              },
            }],
          },
        }
      })
    } else {
      onApply(ue => {
        const current = trafficFlowOf(ue.traffic_defaults)
        const base = current.type === 'rtp_voice' ? current : profileDefaults.rtp_voice
        return {
          traffic_defaults: {
            version: 2,
            flows: [{
              ...base,
              type: 'rtp_voice',
              application_protocol: 'rtp-like',
              transport: 'udp',
              direction: 'BOTH',
              run_mode: voiceRunMode,
              duration_seconds: voiceRunMode === 'continuous' ? null : voiceDuration,
              params: {
                ...base.params,
                bitrate_kbps: voiceBitrate,
                packet_interval_ms: packetInterval,
              },
            }],
          },
        }
      })
    }
    setDraftError('')
  }

  return <section className={`panel ue-batch-panel ${kind}`}>
    <div className="ue-batch-head">
      <div className="ue-group-icon">{kind === 'video' ? <Activity /> : <Radio />}</div>
      <div><span className="eyebrow">{kind === 'video' ? 'EMBB VIDEO POOL' : 'VOICE QOS POOL'}</span><h3>{kind === 'video' ? 'UE1–UE8 · 短影片背景流量' : 'UE9–UE10 · 語音通話'}</h3><p>{kind === 'video' ? '8 台 UE 各自產生有波動的 HTTP/TCP 下行流量' : '兩台 UE 可各自啟動 RTP-like/UDP 雙向通話，供 VoiceGuard 保護'}</p></div>
      <span className="ue-count">{ues.length} UE</span>
    </div>
    <div className={`ue-batch-fields ${kind}`}>
      {kind === 'video' ? <>
        <label><span>每台 Offered (Mbps)</span><CommitNumberInput value={offered} min={0.01} max={100} step={0.05} onCommit={setOffered} /></label>
        <label><span>波動幅度 (%)</span><CommitNumberInput value={variation} min={0} max={100} step={1} integer onCommit={setVariation} /></label>
        <label><span>每台 Peak (Mbps)</span><CommitNumberInput value={peak} min={0.01} max={100} step={0.05} onCommit={setPeak} /></label>
        <label><span>波動模式</span><select value={pattern} onChange={event => setPattern(event.target.value)}><option value="fixed">Fixed</option><option value="wave">Wave</option><option value="random_burst">Random Burst</option><option value="adaptive">Adaptive Video</option></select></label>
      </> : <>
        <label><span>每台 Offered (Kbps)</span><CommitNumberInput value={voiceBitrate} min={8} max={5000} step={8} integer onCommit={setVoiceBitrate} /></label>
        <label><span>封包間隔 (ms)</span><CommitNumberInput value={packetInterval} min={5} max={1000} step={1} integer onCommit={setPacketInterval} /></label>
        <label><span>執行方式</span><select value={voiceRunMode} onChange={event => setVoiceRunMode(event.target.value as TrafficFlow['run_mode'])}><option value="continuous">持續到手動停止</option><option value="duration">指定時間</option></select></label>
        <label><span>Duration (s)</span><CommitNumberInput value={voiceDuration} min={1} max={86400} step={1} integer disabled={voiceRunMode === 'continuous'} onCommit={setVoiceDuration} /></label>
      </>}
      <button className="primary batch-apply" disabled={saving || !ues.length} onClick={apply}>{saving ? '套用中…' : `套用到 ${ues.length} 台 UE`}</button>
    </div>
    {draftError && <div className="traffic-error"><X size={13} />{draftError}</div>}
    <div className="ue-batch-note"><RefreshCw size={12} />批次套用只修改 Traffic，不會覆蓋各 UE 的 IMSI、Channel 與影片 Random Seed。</div>
  </section>
}

function UECard({ ue, experimentId, role, open, setOpen, saving, onSave, onViewConfig }: { ue: UE; experimentId: string; role?: string; open: boolean; setOpen: () => void; saving: boolean; onSave: (body: Partial<UE>) => void; onViewConfig: (experimentId: string, ue: UE) => void }) {
  const [pathLoss, setPathLoss] = useState(String(ue.path_loss_db))
  const [channel, setChannel] = useState(ue.channel)
  const [channelNumbers, setChannelNumbers] = useState(() => ({
    awgn_snr: String(ue.channel.awgn_snr),
    delay_maximum_us: String(ue.channel.delay_maximum_us),
    rlf_t_off_ms: String(ue.channel.rlf_t_off_ms),
    hst_fd_hz: String(ue.channel.hst_fd_hz),
  }))
  const [traffic, setTraffic] = useState<TrafficFlow>(() => trafficFlowOf(ue.traffic_defaults))
  const [trafficDuration, setTrafficDuration] = useState(() => String(trafficFlowOf(ue.traffic_defaults).duration_seconds ?? 60))
  const [draftError, setDraftError] = useState('')
  const sourceSignature = JSON.stringify([ue.path_loss_db, ue.channel, ue.traffic_defaults])

  useEffect(() => {
    setPathLoss(String(ue.path_loss_db))
    setChannel(ue.channel)
    setChannelNumbers({
      awgn_snr: String(ue.channel.awgn_snr),
      delay_maximum_us: String(ue.channel.delay_maximum_us),
      rlf_t_off_ms: String(ue.channel.rlf_t_off_ms),
      hst_fd_hz: String(ue.channel.hst_fd_hz),
    })
    const nextTraffic = trafficFlowOf(ue.traffic_defaults)
    setTraffic(nextTraffic)
    setTrafficDuration(String(nextTraffic.duration_seconds ?? 60))
    setDraftError('')
  }, [ue.id, sourceSignature])

  const toggle = (key: 'awgn_enabled' | 'fading_enabled' | 'delay_enabled' | 'rlf_enabled' | 'hst_enabled') => {
    setChannel(previous => ({ ...previous, [key]: !previous[key] }))
  }

  const updateChannelNumber = (key: keyof typeof channelNumbers, value: string) => {
    setChannelNumbers(previous => ({ ...previous, [key]: value }))
    setDraftError('')
  }

  const changeTrafficType = (type: TrafficType) => {
    const next = profileDefaults[type]
    setTraffic({ ...next, params: { ...next.params } })
    setTrafficDuration(String(next.duration_seconds ?? 60))
    setDraftError('')
  }

  const updateTrafficParam = (key: string, value: string) => {
    setTraffic(previous => ({ ...previous, params: { ...previous.params, [key]: value } }))
    setDraftError('')
  }

  const saveUE = () => {
    const parsedPathLoss = parseDraftNumber(pathLoss)
    const parsedAwgnSnr = parseDraftNumber(channelNumbers.awgn_snr)
    const parsedDelayMaximum = parseDraftNumber(channelNumbers.delay_maximum_us)
    const parsedRlfOffTime = parseDraftNumber(channelNumbers.rlf_t_off_ms)
    const parsedHstFd = parseDraftNumber(channelNumbers.hst_fd_hz)
    if ([parsedPathLoss, parsedAwgnSnr, parsedDelayMaximum, parsedRlfOffTime, parsedHstFd].some(value => value === null)) {
      setDraftError('請將所有數字欄位填寫完整')
      return
    }
    if (parsedPathLoss! < 0 || parsedPathLoss! > 200) {
      setDraftError('Path loss 必須介於 0 到 200 dB')
      return
    }
    if (!Number.isInteger(parsedRlfOffTime) || parsedRlfOffTime! < 0) {
      setDraftError('RLF Off time 必須是大於或等於 0 的整數')
      return
    }
    const parsedDuration = traffic.run_mode === 'duration' ? parseDraftNumber(trafficDuration) : null
    if (traffic.run_mode === 'duration' && (parsedDuration === null || !Number.isInteger(parsedDuration) || parsedDuration < 1 || parsedDuration > 86400)) {
      setDraftError('Traffic 執行時間必須是 1 到 86400 秒的整數')
      return
    }
    const requiredTrafficNumbers = Object.entries(traffic.params).filter(([, value]) => typeof value === 'string' && !value.trim())
    if (requiredTrafficNumbers.length) {
      setDraftError('請將 Traffic 參數填寫完整')
      return
    }
    if (traffic.type === 'short_video') {
      const target = Number(traffic.params.offered_load_mbps)
      const peak = Number(traffic.params.peak_limit_mbps)
      const variation = Number(traffic.params.variation_percent)
      if (!Number.isFinite(target) || target < 0.01 || target > 100) {
        setDraftError('Target Offered Load 必須介於 0.01 到 100 Mbps')
        return
      }
      if (!Number.isFinite(peak) || peak < target || peak > 100) {
        setDraftError('Peak Limit 必須大於或等於 Target，且不可超過 100 Mbps')
        return
      }
      if (!Number.isInteger(variation) || variation < 0 || variation > 100) {
        setDraftError('Variation 必須是 0 到 100 的整數百分比')
        return
      }
    }
    setDraftError('')
    onSave({
      path_loss_db: parsedPathLoss!,
      channel: {
        ...channel,
        awgn_snr: parsedAwgnSnr!,
        delay_maximum_us: parsedDelayMaximum!,
        rlf_t_off_ms: parsedRlfOffTime!,
        hst_fd_hz: parsedHstFd!,
      },
      traffic_defaults: {
        version: 2,
        flows: [{
          ...traffic,
          duration_seconds: traffic.run_mode === 'continuous' ? null : parsedDuration!,
        }],
      },
    })
  }

  return <section className={`panel ue-card ${open ? 'open' : ''}`}>
    <button className="ue-summary" onClick={setOpen}><div className="ue-index">{ue.slot}</div><div><div className="ue-name-line"><b>{ue.display_name}</b>{role && <i className={`ue-role ${role === '通話' ? 'voice' : 'video'}`}>{role}</i>}</div><span>{ue.namespace} · RX {ue.rx_port} / TX {ue.tx_port}</span><small>{trafficSummary(traffic)}</small></div><Pill up={ue.enabled}>{ue.enabled ? 'Enabled' : 'Disabled'}</Pill><SlidersHorizontal /></button>
    {open && <div className="ue-editor"><div className="field-row"><label><span>IMSI</span><input value={ue.imsi} disabled /></label><label><span>APN / DNN</span><input value={ue.apn} disabled /></label><label><span>Path loss (dB)</span><input type="number" min="0" max="200" step="any" value={pathLoss} onChange={event => { setPathLoss(event.target.value); setDraftError('') }} /></label></div>
      <div className="editor-section-title"><div><b>Channel 設定</b><span>儲存後於下一次 Radio Stack 啟動套用</span></div></div>
      <div className="channel-grid">
        <ChannelBox label="AWGN" enabled={channel.awgn_enabled} onToggle={() => toggle('awgn_enabled')}><label>SNR (dB)<input type="number" step="any" value={channelNumbers.awgn_snr} onChange={e => updateChannelNumber('awgn_snr', e.target.value)} /></label></ChannelBox>
        <ChannelBox label="Fading" enabled={channel.fading_enabled} onToggle={() => toggle('fading_enabled')}><label>Model<select value={channel.fading_model} onChange={e => setChannel({ ...channel, fading_model: e.target.value })}><option>none</option><option>epa5</option><option>eva70</option><option>etu300</option></select></label></ChannelBox>
        <ChannelBox label="Delay" enabled={channel.delay_enabled} onToggle={() => toggle('delay_enabled')}><label>Maximum μs<input type="number" step="any" value={channelNumbers.delay_maximum_us} onChange={e => updateChannelNumber('delay_maximum_us', e.target.value)} /></label></ChannelBox>
        <ChannelBox label="RLF" enabled={channel.rlf_enabled} onToggle={() => toggle('rlf_enabled')}><label>Off time ms<input type="number" min="0" step="1" value={channelNumbers.rlf_t_off_ms} onChange={e => updateChannelNumber('rlf_t_off_ms', e.target.value)} /></label></ChannelBox>
        <ChannelBox label="HST Doppler" enabled={channel.hst_enabled} onToggle={() => toggle('hst_enabled')}><label>fd (Hz)<input type="number" step="any" value={channelNumbers.hst_fd_hz} onChange={e => updateChannelNumber('hst_fd_hz', e.target.value)} /></label></ChannelBox>
      </div>
      <div className="editor-section-title traffic-section-title"><div><b>Traffic 設定</b><span>執行頁會直接使用這份設定，不需要再次選擇協定與時間</span></div><div className="traffic-tags"><i>{traffic.application_protocol.toUpperCase()}</i><i>{traffic.transport.toUpperCase()}</i></div></div>
      <div className="traffic-config-grid">
        <label><span>Traffic Profile</span><select value={traffic.type} onChange={e => changeTrafficType(e.target.value as TrafficType)}>{(Object.keys(trafficLabels) as TrafficType[]).map(type => <option value={type} key={type}>{trafficLabels[type]}</option>)}</select></label>
        <label><span>Direction</span><select value={traffic.direction} disabled={!['iperf', 'http'].includes(traffic.type)} onChange={e => setTraffic({ ...traffic, direction: e.target.value as TrafficFlow['direction'] })}><option value="UL">UL</option><option value="DL">DL</option>{!['iperf', 'http'].includes(traffic.type) && <option value="BOTH">雙向</option>}</select></label>
        <label><span>執行方式</span><select value={traffic.run_mode} disabled={traffic.type === 'none'} onChange={e => setTraffic({ ...traffic, run_mode: e.target.value as TrafficFlow['run_mode'], duration_seconds: e.target.value === 'continuous' ? null : Number(trafficDuration) })}><option value="duration">指定時間</option><option value="continuous">持續到手動停止</option></select></label>
        <label><span>Duration (s)</span><input type="number" min="1" max="86400" step="1" disabled={traffic.type === 'none' || traffic.run_mode === 'continuous'} value={trafficDuration} onChange={e => { setTrafficDuration(e.target.value); setDraftError('') }} /></label>
      </div>
      {traffic.type !== 'none' && <div className="traffic-params-grid">
        {traffic.type === 'iperf' && <><label><span>Transport</span><select value={traffic.transport} onChange={e => setTraffic({ ...traffic, transport: e.target.value as 'tcp' | 'udp' })}><option value="udp">UDP</option><option value="tcp">TCP</option></select></label><TrafficNumber label="Bitrate / UE" value={traffic.params.bitrate} onChange={value => updateTrafficParam('bitrate', value)} /></>}
        {traffic.type === 'ping' && <><TrafficNumber label="Interval (ms)" value={traffic.params.interval_ms} onChange={value => updateTrafficParam('interval_ms', value)} /><TrafficNumber label="Packet size (bytes)" value={traffic.params.packet_size} onChange={value => updateTrafficParam('packet_size', value)} /></>}
        {traffic.type === 'http' && <><TrafficNumber label="Object size (KB)" value={traffic.params.object_size_kb} onChange={value => updateTrafficParam('object_size_kb', value)} /><TrafficNumber label="Request interval (ms)" value={traffic.params.interval_ms} onChange={value => updateTrafficParam('interval_ms', value)} /></>}
        {traffic.type === 'short_video' && <>
          <TrafficNumber label="Target Offered Load (Mbps)" value={traffic.params.offered_load_mbps} onChange={value => updateTrafficParam('offered_load_mbps', value)} />
          <label><span>Traffic Pattern</span><select value={String(traffic.params.traffic_pattern ?? 'wave')} onChange={e => updateTrafficParam('traffic_pattern', e.target.value)}><option value="fixed">Fixed</option><option value="wave">Wave</option><option value="random_burst">Random Burst</option><option value="adaptive">Adaptive Video</option></select></label>
          <TrafficNumber label="Variation (%)" value={traffic.params.variation_percent} onChange={value => updateTrafficParam('variation_percent', value)} />
          <TrafficNumber label="Peak Limit (Mbps)" value={traffic.params.peak_limit_mbps} onChange={value => updateTrafficParam('peak_limit_mbps', value)} />
          <TrafficNumber label="Random Seed" value={traffic.params.random_seed} onChange={value => updateTrafficParam('random_seed', value)} />
          <TrafficNumber label="Wave Period (s)" value={traffic.params.pattern_period_seconds} onChange={value => updateTrafficParam('pattern_period_seconds', value)} />
          <TrafficNumber label="Pacing Interval (ms)" value={traffic.params.segment_interval_ms} onChange={value => updateTrafficParam('segment_interval_ms', value)} />
        </>}
        {traffic.type === 'social' && <><TrafficNumber label="Image size (KB)" value={traffic.params.object_size_kb} onChange={value => updateTrafficParam('object_size_kb', value)} /><TrafficNumber label="Objects / cycle" value={traffic.params.objects_per_cycle} onChange={value => updateTrafficParam('objects_per_cycle', value)} /><TrafficNumber label="Stay time (ms)" value={traffic.params.cycle_interval_ms} onChange={value => updateTrafficParam('cycle_interval_ms', value)} /></>}
        {traffic.type === 'navigation' && <><TrafficNumber label="Tile size (KB)" value={traffic.params.tile_size_kb} onChange={value => updateTrafficParam('tile_size_kb', value)} /><TrafficNumber label="Tiles / update" value={traffic.params.tiles_per_cycle} onChange={value => updateTrafficParam('tiles_per_cycle', value)} /><TrafficNumber label="Update interval (ms)" value={traffic.params.update_interval_ms} onChange={value => updateTrafficParam('update_interval_ms', value)} /></>}
        {traffic.type === 'rtp_voice' && <><TrafficNumber label="Packet interval (ms)" value={traffic.params.packet_interval_ms} onChange={value => updateTrafficParam('packet_interval_ms', value)} /><TrafficNumber label="Bitrate (Kbps)" value={traffic.params.bitrate_kbps} onChange={value => updateTrafficParam('bitrate_kbps', value)} /></>}
      </div>}
      <div className="button-row end"><span className={`restart-note ${draftError ? 'input-error' : ''}`}>{draftError ? <><X size={13} />{draftError}</> : <><RefreshCw size={13} />Channel 下次啟動套用；Traffic 會保存到 Run snapshot</>}</span><button className="secondary small" onClick={() => onViewConfig(experimentId, ue)}><FileText />Config</button><button className="primary small" disabled={saving} onClick={saveUE}>{saving ? '儲存中…' : '儲存 UE 設定'}</button></div>
    </div>}
  </section>
}

function TrafficNumber({ label, value, onChange }: { label: string; value: string | number | boolean | undefined; onChange: (value: string) => void }) {
  return <label><span>{label}</span><input value={String(value ?? '')} onChange={e => onChange(e.target.value)} /></label>
}

function CommitNumberInput({ value, min, max, step, integer = false, disabled, onCommit }: {
  value: number
  min: number
  max: number
  step: number
  integer?: boolean
  disabled?: boolean
  onCommit: (value: number) => void
}) {
  const [draft, setDraft] = useState(String(value))
  useEffect(() => setDraft(String(value)), [value])
  const commit = () => {
    const parsed = Number(draft)
    if (!draft.trim() || !Number.isFinite(parsed)) {
      setDraft(String(value))
      return
    }
    const normalized = integer ? Math.round(parsed) : parsed
    const next = Math.max(min, Math.min(max, normalized))
    setDraft(String(next))
    if (next !== value) onCommit(next)
  }
  return <input
    type="number"
    inputMode="decimal"
    min={min}
    max={max}
    step={step}
    value={draft}
    disabled={disabled}
    onChange={event => setDraft(event.target.value)}
    onBlur={commit}
    onKeyDown={event => {
      if (event.key === 'Enter') event.currentTarget.blur()
      if (event.key === 'Escape') {
        setDraft(String(value))
        event.currentTarget.blur()
      }
    }}
  />
}

function ChannelBox({ label, enabled, onToggle, children }: { label: string; enabled: boolean; onToggle: () => void; children: React.ReactNode }) {
  return <div className={`channel-box ${enabled ? 'enabled' : ''}`}><div><b>{label}</b><button className={`switch ${enabled ? 'on' : ''}`} onClick={onToggle}><span /></button></div>{children}</div>
}

function trafficProgress(job: TrafficJob | undefined): Record<string, unknown> {
  const progress = job?.result?.progress
  return progress && typeof progress === 'object' ? progress as Record<string, unknown> : {}
}

function Live({ platform, activeRun, latestRun, busy, onStop, onViewConfig }: { platform: Platform | null; activeRun?: Run; latestRun?: Run; busy: string; onStop: () => void; onViewConfig: (run: Run | undefined, ue: string) => void }) {
  const run = activeRun ?? latestRun
  const uptime = useMemo(() => run?.started_at ? new Date(run.started_at).toLocaleString() : '—', [run?.started_at])
  const [jobs, setJobs] = useState<TrafficJob[]>([])
  const [configuredUEs, setConfiguredUEs] = useState<RunTrafficUE[]>([])
  const [trafficBusy, setTrafficBusy] = useState('')
  const [trafficError, setTrafficError] = useState('')
  const [voiceGuard, setVoiceGuard] = useState<VoiceGuardStatus | null>(null)
  const [voiceGuardMode, setVoiceGuardMode] = useState<'observe_only' | 'closed_loop'>('closed_loop')
  const [voiceGuardAlgorithm, setVoiceGuardAlgorithm] = useState<'rules' | 'random_forest'>('random_forest')
  const [voiceGuardPolicy, setVoiceGuardPolicy] = useState({ videoScale: 60, threshold: 1.2 })
  const [radioMetrics, setRadioMetrics] = useState<Record<string, { rx: number; tx: number; latency: number; loss: number }>>({})
  const [throughputHistory, setThroughputHistory] = useState<Record<string, number[]>>({})
  const [offeredHistory, setOfferedHistory] = useState<Record<string, number[]>>({})
  const [voiceHistory, setVoiceHistory] = useState<VoiceHistory>({ offered: [], received: [], delivery: [], loss: [], jitter: [], rtt: [], states: [] })
  useEffect(() => {
    setThroughputHistory({})
    setOfferedHistory({})
    setVoiceHistory({ offered: [], received: [], delivery: [], loss: [], jitter: [], rtt: [], states: [] })
  }, [run?.id])
  const activeJobs = useMemo(() => jobs.filter(job => ['QUEUED', 'RUNNING', 'STOP_REQUESTED'].includes(job.status)), [jobs])
  const activeTrafficUEs = useMemo(() => new Set(activeJobs.map(job => job.ue)), [activeJobs])
  const rfScenario = configuredUEs.some(item => ['ue9', 'ue10'].includes(item.ue))
  const voiceUENames = rfScenario ? ['ue9', 'ue10'] : ['ue3']
  useEffect(() => {
    if (configuredUEs.length) setVoiceGuardAlgorithm(rfScenario ? 'random_forest' : 'rules')
  }, [run?.id, configuredUEs.length, rfScenario])
  const refreshTraffic = useCallback(async () => {
    if (!run) { setJobs([]); setConfiguredUEs([]); return }
    try {
      const [nextJobs, configuration, nextVoiceGuard] = await Promise.all([
        api.traffic(run.id),
        api.trafficConfig(run.id),
        api.voiceGuard(run.id).catch(() => null),
      ])
      setJobs(nextJobs); setConfiguredUEs(configuration.ues); setVoiceGuard(nextVoiceGuard)
      const ueNames = configuration.ues.map(item => item.ue)
      setOfferedHistory(previous => Object.fromEntries(ueNames.map(ue => {
        const job = nextJobs.find(item => item.ue === ue && ['QUEUED', 'RUNNING', 'STOP_REQUESTED'].includes(item.status))
        const progress = trafficProgress(job)
        const configured = configuration.ues.find(item => item.ue === ue)?.traffic.flows[0]
        const fallback = configured?.type === 'short_video'
          ? Number(configured.params.offered_load_mbps ?? 0) * 1e6
          : configured?.type === 'rtp_voice' ? Number(configured.params.bitrate_kbps ?? 0) * 1000 : 0
        return [ue, [...(previous[ue] ?? []), Number(progress.offered_bps ?? (job ? fallback : 0))].slice(-40)]
      })))
      const nextRfScenario = configuration.ues.some(item => ['ue9', 'ue10'].includes(item.ue))
      const voiceTargets = nextRfScenario ? ['ue9', 'ue10'] : ['ue3']
      const voiceJobs = nextJobs.filter(item => voiceTargets.includes(item.ue) && item.traffic_type === 'rtp_voice' && ['QUEUED', 'RUNNING', 'STOP_REQUESTED'].includes(item.status))
      const voiceProgresses = voiceJobs.map(trafficProgress)
      const minimum = (key: string, fallback = 0) => voiceProgresses.length ? Math.min(...voiceProgresses.map(item => Number(item[key] ?? fallback))) : fallback
      const maximum = (keys: string[], fallback = 0) => voiceProgresses.length ? Math.max(...voiceProgresses.map(item => Number(keys.map(key => item[key]).find(value => value != null) ?? fallback))) : fallback
      setVoiceHistory(previous => ({
        offered: [...previous.offered, voiceProgresses.reduce((total, item) => total + Number(item.offered_bps ?? 0), 0)].slice(-40),
        received: [...previous.received, voiceProgresses.reduce((total, item) => total + Number(item.received_bps ?? 0), 0)].slice(-40),
        delivery: [...previous.delivery, minimum('delivery_ratio')].slice(-40),
        loss: [...previous.loss, maximum(['loss_percent'])].slice(-40),
        jitter: [...previous.jitter, maximum(['jitter_rolling_ms', 'jitter_ms'])].slice(-40),
        rtt: [...previous.rtt, maximum(['rtt_p95_rolling_ms', 'rtt_p95_ms'])].slice(-40),
        states: [...previous.states, nextVoiceGuard?.state ?? 'OFF'].slice(-40),
      }))
    } catch { /* run may change during polling */ }
  }, [run?.id])
  useEffect(() => { void refreshTraffic(); const timer = window.setInterval(() => void refreshTraffic(), 2000); return () => window.clearInterval(timer) }, [refreshTraffic])
  const refreshMetrics = useCallback(async () => {
    if (!activeRun) return
    try {
      const names = ['ue_rx_bps', 'ue_tx_bps', 'ue_ping_latency', 'ue_ping_loss']
      const responses = await Promise.all(names.map(name => api.metric(activeRun.id, name)))
      const mapped = responses.map(response => Object.fromEntries(response.data.result.map(item => [item.metric.ue ?? '', Number(item.value[1])])))
      const ueNames = configuredUEs.map(item => item.ue)
      const next = Object.fromEntries(ueNames.map(ue => [ue, { rx: mapped[0][ue] ?? 0, tx: mapped[1][ue] ?? 0, latency: mapped[2][ue] ?? -1, loss: mapped[3][ue] ?? 100 }]))
      setRadioMetrics(next)
      setThroughputHistory(previous => Object.fromEntries(ueNames.map(ue => [ue, [...(previous[ue] ?? []), next[ue].rx + next[ue].tx].slice(-40)])))
    } catch { /* Prometheus may be between scrapes */ }
  }, [activeRun?.id, configuredUEs])
  useEffect(() => { void refreshMetrics(); const timer = window.setInterval(() => void refreshMetrics(), 2000); return () => window.clearInterval(timer) }, [refreshMetrics])
  async function startTraffic(target: string) {
    if (!activeRun) return
    const ues = target === 'all'
      ? configuredUEs.filter(item => item.traffic.flows.some(flow => flow.type !== 'none')).map(item => item.ue)
      : [target]
    if (!ues.length) { setTrafficError('沒有已設定 Traffic 的 UE'); return }
    setTrafficBusy(target); setTrafficError('')
    try {
      await api.startTrafficBatch(activeRun.id, ues)
      await refreshTraffic()
    }
    catch (reason) { setTrafficError(reason instanceof Error ? reason.message : String(reason)) }
    finally { setTrafficBusy('') }
  }

  async function stopTraffic(target: string) {
    if (!activeRun) return
    setTrafficBusy(`stop-${target}`); setTrafficError('')
    try {
      if (target === 'all') await api.stopAllTraffic(activeRun.id)
      else await Promise.all(activeJobs.filter(job => job.ue === target).map(job => api.stopTraffic(activeRun.id, job.id)))
      await refreshTraffic()
    } catch (reason) { setTrafficError(reason instanceof Error ? reason.message : String(reason)) }
    finally { setTrafficBusy('') }
  }
  async function toggleVoiceGuard() {
    if (!activeRun) return
    setTrafficBusy('voiceguard'); setTrafficError('')
    try {
      setVoiceGuard(voiceGuard?.running
        ? await api.stopVoiceGuard(activeRun.id)
        : await api.startVoiceGuard(activeRun.id, voiceGuardMode, {
            algorithm: voiceGuardAlgorithm,
            video_offered_scale_percent: voiceGuardPolicy.videoScale,
            congestion_threshold_mbps: voiceGuardPolicy.threshold,
          }))
      window.setTimeout(() => void refreshTraffic(), 500)
    } catch (reason) { setTrafficError(reason instanceof Error ? reason.message : String(reason)) }
    finally { setTrafficBusy('') }
  }
  return <div className="content">
    <section className="run-header"><div><div className="hero-kicker"><Activity size={14} /> LIVE EXPERIMENT</div><h2>{run ? `Run ${run.id.slice(0, 8)}` : '目前沒有執行紀錄'}</h2><p>{run ? `開始時間 ${uptime} · revision ${run.experiment_revision}` : '請從實驗設定頁選擇 baseline 並啟動。'}</p></div><div className="run-actions"><span className={`run-state ${run?.state?.toLowerCase()}`}>{run?.state ?? 'IDLE'}</span>{activeRun && <button className="danger" disabled={!!busy} onClick={onStop}><CircleStop />停止實驗</button>}</div></section>
    <section className="metric-grid"><Metric label="Radio Stack" value={platform?.state ?? '—'} hint={`${Object.values(platform?.components ?? {}).filter(item => item.running).length} components`} icon={<Radio />} /><Metric label="UE Attached" value={`${configuredUEs.filter(item => platform?.components[item.ue]?.running).length} / ${configuredUEs.length || 0}`} hint="PDU sessions" icon={<Wifi />} /><Metric label="ZMQ Endpoints" value={`${Object.values(platform?.ports ?? {}).filter(Boolean).length} / ${2 + configuredUEs.length * 2}`} hint={`gNB 2 + ${configuredUEs.length} UE × 2`} icon={<Network />} /><Metric label="Monitoring" value={platform?.prometheus ? 'ONLINE' : 'OFFLINE'} hint="Prometheus :9095" icon={<Activity />} /></section>
    <VoiceGuardPanel status={voiceGuard} activeRun={!!activeRun} rfScenario={rfScenario} busy={trafficBusy === 'voiceguard'} mode={voiceGuardMode} algorithm={voiceGuardAlgorithm} policy={voiceGuardPolicy} onMode={setVoiceGuardMode} onAlgorithm={setVoiceGuardAlgorithm} onToggle={() => void toggleVoiceGuard()} />
    <section className="panel traffic-panel"><div className="panel-head"><div><h3>UE Traffic Controller</h3><p>流量設定來自這次 Run 的 Experiment snapshot；可單獨執行或同時啟動全部 UE</p></div><div className="traffic-batch-actions"><button className="secondary small" disabled={!activeRun || !activeJobs.length || !!trafficBusy} onClick={() => void stopTraffic('all')}><CircleStop />停止全部</button><button className="primary small" disabled={!activeRun || !!trafficBusy || activeTrafficUEs.size > 0} onClick={() => void startTraffic('all')}><Zap />{trafficBusy === 'all' ? '啟動中…' : '執行全部'}</button></div></div>
      <TrafficPlanTable configuredUEs={configuredUEs} jobs={jobs} activeRun={!!activeRun} busy={trafficBusy} onStart={ue => void startTraffic(ue)} onStop={ue => void stopTraffic(ue)} />
      {trafficError && <div className="traffic-error">{trafficError}</div>}
      <div className="traffic-results-title"><b>執行結果</b><span>{jobs.length} 筆紀錄</span></div>
      <TrafficTable jobs={jobs} />
    </section>
    <section className="panel chart-panel"><div className="panel-head"><div><h3>Offered Load vs Delivered Throughput</h3><p>虛線是流量產生器需求量；實線是 Prometheus 實際 RX + TX，兩者分開才能看出資源競爭</p></div><div className="config-shortcuts">{configuredUEs.map(item => item.ue).map(ue => <button key={ue} onClick={() => onViewConfig(run, ue)}><FileText />{ue.toUpperCase()} Config</button>)}<a href={`${window.location.protocol}//${window.location.hostname}:3001`} target="_blank">Grafana <ChevronRight size={14} /></a></div></div><ThroughputChart ues={configuredUEs.map(item => item.ue)} history={throughputHistory} offeredHistory={offeredHistory} metrics={radioMetrics} /></section>
    <section className="panel voice-quality-panel"><div className="panel-head"><div><h3>{voiceUENames.map(ue => ue.toUpperCase()).join(' / ')} Voice Quality</h3><p>取目前通話 UE 的最差值 · RTP-like 最近 3 秒 rolling</p></div><span className={`xapp-state ${voiceGuard?.state?.toLowerCase() ?? 'off'}`}>{voiceGuard?.state ?? 'XAPP OFF'}</span></div><VoiceQualityChart history={voiceHistory} voiceGuard={voiceGuard} ueCount={configuredUEs.length || 3} /></section>
  </div>
}

function VoiceGuardPanel({ status, activeRun, rfScenario, busy, mode, algorithm, policy, onMode, onAlgorithm, onToggle }: {
  status: VoiceGuardStatus | null
  activeRun: boolean
  rfScenario: boolean
  busy: boolean
  mode: 'observe_only' | 'closed_loop'
  algorithm: 'rules' | 'random_forest'
  policy: { videoScale: number; threshold: number }
  onMode: (mode: 'observe_only' | 'closed_loop') => void
  onAlgorithm: (algorithm: 'rules' | 'random_forest') => void
  onToggle: () => void
}) {
  const running = !!status?.running
  const videoOffered = Number(status?.total_video_offered_bps ?? 0) / 1e6
  const videoDelivered = Number(status?.total_video_delivered_bps ?? 0) / 1e6
  const lastEvent = status?.events?.at(-1)
  return <section className={`panel voiceguard-panel ${status?.state?.toLowerCase() ?? 'off'}`}>
    <div className="panel-head">
      <div><div className="xapp-title"><ShieldCheck /><span>{rfScenario ? 'RANDOM FOREST XAPP + NATIVE E2SM-RC' : 'RULE XAPP + NATIVE E2SM-RC'}</span></div><h3>{rfScenario ? 'VoiceGuard RF V2' : 'VoiceGuard Rule V1 · 3 UE'}</h3><p>{rfScenario ? '8 台動態影片＋UE9/UE10 隨機通話 · RF 選擇最少必要的保護比例' : 'UE1／UE2 背景流量＋UE3 語音通話 · 規則式 QoS 保護'}</p></div>
      <div className="voiceguard-actions"><span className={`xapp-state ${status?.state?.toLowerCase() ?? 'off'}`}>{status?.state ?? 'OFF'}</span><button className={running ? 'danger small' : 'primary small'} disabled={!activeRun || busy} onClick={onToggle}>{running ? <CircleStop /> : <Play />}{busy ? '處理中…' : running ? '關閉並恢復基線' : '啟動 xApp'}</button></div>
    </div>
    <div className="voiceguard-config">
      <label><span>執行模式</span><select value={running ? status?.mode ?? mode : mode} disabled={running || busy} onChange={event => onMode(event.target.value as 'observe_only' | 'closed_loop')}><option value="closed_loop">Closed Loop（實際控制）</option><option value="observe_only">Observe Only（只觀察）</option></select></label>
      <label><span>策略引擎</span><select value={running ? status?.algorithm ?? algorithm : algorithm} disabled={running || busy} onChange={event => onAlgorithm(event.target.value as 'rules' | 'random_forest')}>{rfScenario && <option value="random_forest">Random Forest V2</option>}<option value="rules">Rule V1（3 UE）</option></select></label>
      <div className="rc-link online"><span>{rfScenario ? 'RF Model' : 'Policy'}</span><b>{status?.model_name ?? (algorithm === 'random_forest' ? 'voiceguard_rf.joblib' : `Rule · ${policy.videoScale}%`)}</b></div>
      <div className={`rc-link ${status?.e2_connected ? 'online' : ''}`}><span>RC Link</span><b>{status?.e2_connected ? 'ACK / CONNECTED' : running && status?.mode === 'closed_loop' ? 'CONNECTING' : 'STANDBY'}</b></div>
    </div>
    <div className="voiceguard-grid">
      <div><span>模式</span><b>{(status?.mode ?? mode).replace('_', ' ').toUpperCase()}</b><small>{status?.native_control ? 'E2SM-RC safety baseline verified' : 'No RAN control commands'}</small></div>
      <div><span>語音通話</span><b>{status?.voice_active ? 'ACTIVE' : 'WAITING'}</b><small>{status?.active_voice_ues?.map(ue => ue.toUpperCase()).join(' + ') || (rfScenario ? '等待 UE9 / UE10 來電' : '等待 UE3 來電')}</small></div>
      <div><span>影片 Offered</span><b>{videoOffered.toFixed(2)} Mbps</b><small>{rfScenario ? 'UE1–UE8 demand' : 'UE1 / UE2 demand'}</small></div>
      <div><span>{rfScenario ? 'RF 信心／延遲' : '策略輸出'}</span><b>{status?.prediction_confidence != null ? `${(status.prediction_confidence * 100).toFixed(1)}%` : '—'}</b><small>{status?.inference_ms != null ? `${status.inference_ms.toFixed(2)} ms · ${status.predicted_policy ?? ''}` : `${videoDelivered.toFixed(2)} Mbps delivered`}</small></div>
    </div>
    <div className="voiceguard-decision"><i /><div><span>目前決策</span><b>{status?.last_decision ?? '尚未啟動 VoiceGuard'}</b><small>{status?.current_policy ?? (lastEvent ? `${new Date(lastEvent.timestamp * 1000).toLocaleTimeString()} · ${lastEvent.message}` : '安全預設：BASELINE')}</small></div></div>
  </section>
}

function trafficParameterSummary(flow: TrafficFlow) {
  const params = flow.params
  if (flow.type === 'iperf') return `${params.bitrate ?? '750K'} / UE`
  if (flow.type === 'ping') return `${params.packet_size ?? 56}B / ${params.interval_ms ?? 1000}ms`
  if (flow.type === 'http') return `${params.object_size_kb ?? 256}KB / ${params.interval_ms ?? 1000}ms`
  if (flow.type === 'short_video') return `${params.offered_load_mbps ?? 0.8} Mbps · ${String(params.traffic_pattern ?? 'wave')} · ±${params.variation_percent ?? 30}%`
  if (flow.type === 'social') return `${params.objects_per_cycle ?? 4} × ${params.object_size_kb ?? 180}KB`
  if (flow.type === 'navigation') return `${params.tiles_per_cycle ?? 6} × ${params.tile_size_kb ?? 40}KB`
  if (flow.type === 'rtp_voice') return `${params.bitrate_kbps ?? 64}Kbps / ${params.packet_interval_ms ?? 20}ms`
  return '—'
}

function TrafficPlanTable({ configuredUEs, jobs, activeRun, busy, onStart, onStop }: { configuredUEs: RunTrafficUE[]; jobs: TrafficJob[]; activeRun: boolean; busy: string; onStart: (ue: string) => void; onStop: (ue: string) => void }) {
  if (!configuredUEs.length) return <div className="traffic-empty">這次 Run 沒有 Traffic snapshot；請重新啟動 Experiment</div>
  return <div className="traffic-plan-table">
    <div className="traffic-plan-row traffic-head"><span>UE</span><span>場景</span><span>協定</span><span>方向</span><span>主要設定</span><span>執行方式</span><span>狀態</span><span>操作</span></div>
    {configuredUEs.map(item => {
      const flow = item.traffic.flows[0] ?? profileDefaults.none
      const latest = jobs.find(job => job.ue === item.ue)
      const running = jobs.some(job => job.ue === item.ue && ['QUEUED', 'RUNNING', 'STOP_REQUESTED'].includes(job.status))
      const protocol = flow.type === 'iperf' ? `iperf3/${flow.transport}` : `${flow.application_protocol}/${flow.transport}`
      return <div className="traffic-plan-row" key={item.ue}><b>{item.ue.toUpperCase()}</b><span>{trafficLabels[flow.type]}</span><span>{protocol.toUpperCase()}</span><span>{flow.direction}</span><span>{trafficParameterSummary(flow)}</span><span>{flow.run_mode === 'continuous' ? '持續 ∞' : `${flow.duration_seconds}s`}</span><span className={`job-status ${(latest?.status ?? 'ready').toLowerCase()}`}>{running ? latest?.status ?? 'RUNNING' : latest?.status ?? (flow.type === 'none' ? 'DISABLED' : 'READY')}</span><span>{running ? <button className="danger mini" disabled={!!busy} onClick={() => onStop(item.ue)}><CircleStop />停止</button> : <button className="secondary mini" disabled={!activeRun || !!busy || flow.type === 'none'} onClick={() => onStart(item.ue)}><Play />執行</button>}</span></div>
    })}
  </div>
}

function ThroughputChart({ ues, history, offeredHistory, metrics }: { ues: string[]; history: Record<string, number[]>; offeredHistory: Record<string, number[]>; metrics: Record<string, { rx: number; tx: number; latency: number; loss: number }> }) {
  const palette = ['#60e3a4', '#69a7ff', '#edc46b', '#ff8077', '#b78cff', '#55d7e8', '#f49d5b', '#9bd36a', '#f078b8', '#aab7c4']
  const colors = Object.fromEntries(ues.map((ue, index) => [ue, palette[index % palette.length]]))
  const maximum = Math.max(1, ...Object.values(history).flat(), ...Object.values(offeredHistory).flat())
  const points = (values: number[]) => values.map((value, index) => `${(index / 39) * 100},${50 - (value / maximum) * 46}`).join(' ')
  return <div className="real-chart"><div className="chart-legend">{ues.map(ue => <div key={ue}><i style={{ background: colors[ue] }} /><b>{ue.toUpperCase()}</b><span>{(((metrics[ue]?.rx ?? 0) + (metrics[ue]?.tx ?? 0)) / 1e6).toFixed(2)} Mbps</span><small>Offered {((offeredHistory[ue]?.at(-1) ?? 0) / 1e6).toFixed(2)} Mbps · {metrics[ue]?.latency >= 0 ? `${metrics[ue].latency.toFixed(1)} ms · ${metrics[ue].loss.toFixed(0)}% loss` : '等待 UE'}</small></div>)}</div><svg viewBox="0 0 100 52" preserveAspectRatio="none"><defs><pattern id="grid" width="10" height="10" patternUnits="userSpaceOnUse"><path d="M 10 0 L 0 0 0 10" fill="none" stroke="rgba(255,255,255,.05)" strokeWidth=".25" /></pattern></defs><rect width="100" height="52" fill="url(#grid)" />{ues.map(ue => <polyline key={`offered-${ue}`} points={points(offeredHistory[ue] ?? [])} fill="none" stroke={colors[ue]} strokeOpacity=".5" strokeDasharray="2 1.5" strokeWidth=".45" vectorEffect="non-scaling-stroke" />)}{ues.map(ue => <polyline key={ue} points={points(history[ue] ?? [])} fill="none" stroke={colors[ue]} strokeWidth=".65" vectorEffect="non-scaling-stroke" />)}</svg></div>
}

function VoiceQualityChart({ history, voiceGuard, ueCount }: { history: VoiceHistory; voiceGuard: VoiceGuardStatus | null; ueCount: number }) {
  const offered = history.offered.at(-1) ?? 0
  const received = history.received.at(-1) ?? 0
  const loss = history.loss.at(-1) ?? 0
  const jitter = history.jitter.at(-1) ?? 0
  const rtt = history.rtt.at(-1) ?? 0
  const rollingDelivery = history.delivery.at(-1) ?? 0
  const stable = offered > 0 && rollingDelivery >= 0.95 && loss <= 2 && jitter <= 30 && rtt <= 120
  const qualityReason = stable
    ? '最近 3 秒皆在門檻內'
    : [
        rollingDelivery < 0.95 ? `delivery ${(rollingDelivery * 100).toFixed(1)}%` : '',
        loss > 2 ? `loss ${loss.toFixed(1)}%` : '',
        jitter > 30 ? `jitter ${jitter.toFixed(1)} ms` : '',
        rtt > 120 ? `RTT spike ${rtt.toFixed(1)} ms` : '',
      ].filter(Boolean).join(' · ')
  const rateMaximum = Math.max(80000, ...history.offered, ...history.received)
  const points = (values: number[], maximum: number, height = 46) => values.map((value, index) => `${(index / 39) * 100},${50 - Math.min(1, value / Math.max(1, maximum)) * height}`).join(' ')
  const transitions = history.states.map((state, index) => index > 0 && state !== history.states[index - 1] ? index : -1).filter(index => index >= 0)
  const qualitySeries = [
    { label: 'Loss', value: loss, unit: '%', values: history.loss, maximum: Math.max(5, ...history.loss), color: '#ff8077' },
    { label: 'Jitter', value: jitter, unit: 'ms', values: history.jitter, maximum: Math.max(30, ...history.jitter), color: '#69a7ff' },
    { label: 'RTT P95', value: rtt, unit: 'ms', values: history.rtt, maximum: Math.max(120, ...history.rtt), color: '#edc46b' },
  ]
  return <div className="voice-quality">
    <div className="voice-kpis">
      <div><span>Voice State</span><b className={stable ? 'stable' : offered > 0 ? 'unstable' : ''}>{offered > 0 ? stable ? 'STABLE' : 'UNSTABLE' : 'WAITING'}</b><small>{offered > 0 ? qualityReason : voiceGuard?.state === 'PROTECTING' ? 'Traffic protection active' : '等待語音流量'}</small></div>
      <div><span>Offered</span><b>{(offered / 1000).toFixed(1)} Kbps</b><small>固定發送端</small></div>
      <div><span>Received</span><b>{(received / 1000).toFixed(1)} Kbps</b><small>{(rollingDelivery * 100).toFixed(1)}% delivery · 3s</small></div>
      <div><span>Loss</span><b>{loss.toFixed(1)}%</b><small>目標 ≤ 2%</small></div>
      <div><span>Jitter</span><b>{jitter.toFixed(1)} ms</b><small>目標 ≤ 30 ms</small></div>
      <div><span>RTT P95</span><b>{rtt.toFixed(1)} ms</b><small>{ueCount} UE 目標 ≤ 120 ms</small></div>
    </div>
    <div className="voice-rate-chart"><div className="voice-chart-label"><span><i className="offered-line" />Offered</span><span><i className="received-line" />Received</span></div><svg viewBox="0 0 100 52" preserveAspectRatio="none"><rect width="100" height="52" fill="url(#grid)" />{transitions.map(index => <line key={index} x1={(index / 39) * 100} x2={(index / 39) * 100} y1="0" y2="52" stroke="#edc46b" strokeDasharray="1 1" strokeWidth=".35" vectorEffect="non-scaling-stroke" />)}<polyline points={points(history.offered, rateMaximum)} fill="none" stroke="#edc46b" strokeOpacity=".7" strokeDasharray="2 1.5" strokeWidth=".55" vectorEffect="non-scaling-stroke" /><polyline points={points(history.received, rateMaximum)} fill="none" stroke="#60e3a4" strokeWidth=".75" vectorEffect="non-scaling-stroke" /></svg></div>
    <div className="voice-quality-series">{qualitySeries.map(series => <div key={series.label}><div><span>{series.label}</span><b>{series.value.toFixed(1)} {series.unit}</b></div><svg viewBox="0 0 100 28" preserveAspectRatio="none"><polyline points={points(series.values, series.maximum, 24)} fill="none" stroke={series.color} strokeWidth=".7" vectorEffect="non-scaling-stroke" /></svg></div>)}</div>
  </div>
}

function TrafficTable({ jobs }: { jobs: TrafficJob[] }) {
  const [expanded, setExpanded] = useState('')
  const formatResult = (job: TrafficJob) => {
    const receiver = Number(job.result.receiver_bps)
    if (job.traffic_type === 'iperf' && receiver > 0) return `${(receiver / 1e6).toFixed(2)} Mbps`
    const sender = Number(job.result.sender_bps)
    const loss = job.result.loss_percent
    if (job.traffic_type === 'iperf' && sender > 0 && typeof loss === 'number' && Number.isFinite(loss)) return `~${(sender * (1 - loss / 100) / 1e6).toFixed(2)} Mbps`
    if (job.traffic_type === 'ping') return `${Number(job.result.received ?? 0)} / ${Number(job.result.sent ?? 0)} replies`
    if (job.traffic_type === 'short_video' && job.status === 'RUNNING') {
      const progress = trafficProgress(job)
      return `${(Number(progress.offered_bps ?? 0) / 1e6).toFixed(2)} → ${(Number(progress.delivered_bps ?? 0) / 1e6).toFixed(2)} Mbps`
    }
    if (job.traffic_type === 'rtp_voice' && job.status === 'RUNNING') {
      const progress = trafficProgress(job)
      return `${(Number(progress.offered_bps ?? 0) / 1000).toFixed(1)} → ${(Number(progress.received_bps ?? 0) / 1000).toFixed(1)} Kbps`
    }
    if (['http', 'short_video', 'social', 'navigation'].includes(job.traffic_type)) return `${Number(job.result.successful_requests ?? 0)} / ${Number(job.result.requests ?? 0)} requests`
    if (job.traffic_type === 'rtp_voice') return `${Number(job.result.received_packets ?? 0)} / ${Number(job.result.sent_packets ?? 0)} packets`
    return '—'
  }
  const formatQuality = (job: TrafficJob) => {
    if (typeof job.result.loss_percent === 'number') {
      const jitter = typeof job.result.jitter_ms === 'number' ? ` · ${job.result.jitter_ms.toFixed(1)}ms jitter` : ''
      const rtt = typeof job.result.rtt_avg_ms === 'number' ? ` · ${job.result.rtt_avg_ms.toFixed(1)}ms RTT` : ''
      return `${job.result.loss_percent.toFixed(1)}% loss${jitter}${rtt}`
    }
    if (typeof job.result.latency_p95_ms === 'number') return `${job.result.latency_p95_ms.toFixed(1)}ms P95 · ${Number(job.result.failed_requests ?? 0)} failed`
    const progress = trafficProgress(job)
    if (typeof progress.segment_latency_ms === 'number') return `${progress.segment_latency_ms.toFixed(1)}ms segment · ${Number(progress.failed_requests ?? 0)} failed`
    if (job.traffic_type === 'rtp_voice' && typeof progress.loss_percent === 'number') return `${progress.loss_percent.toFixed(1)}% loss · ${Number(progress.jitter_ms ?? 0).toFixed(1)}ms jitter · ${Number(progress.rtt_p95_ms ?? 0).toFixed(1)}ms P95`
    return job.status === 'FAILED' ? '查看錯誤' : '—'
  }
  const failure = (job: TrafficJob) => {
    const error = String(job.result.error ?? '')
    if (job.result.error_code === 'IPERF_TIMEOUT' || error.includes('TimeoutExpired')) return 'IPERF_TIMEOUT：radio/RLC queue 未在期限內排空'
    if (job.result.error_code === 'IPERF_INCOMPLETE') return 'IPERF_INCOMPLETE：沒有取得完整 server report'
    return error || '沒有額外錯誤資訊'
  }
  if (!jobs.length) return <div className="traffic-empty">尚無流量紀錄</div>
  return <div className="traffic-table"><div className="traffic-row traffic-head"><span>UE</span><span>場景</span><span>狀態</span><span>主要結果</span><span>品質</span><span>執行時間</span><span>開始時間</span></div>{jobs.map(job => <div className="traffic-item" key={job.id}><button className="traffic-row" onClick={() => setExpanded(expanded === job.id ? '' : job.id)}><b>{job.ue.toUpperCase()}</b><span>{trafficLabels[job.traffic_type] ?? job.traffic_type}</span><span className={`job-status ${job.status.toLowerCase()}`}>{job.status}</span><span>{formatResult(job)}</span><span>{formatQuality(job)}</span><span>{job.run_mode === 'continuous' && !job.finished_at ? '持續 ∞' : `${Number(job.result.seconds ?? job.duration_seconds ?? 0).toFixed(1)}s`}</span><span>{new Date(job.created_at).toLocaleTimeString()}</span></button>{expanded === job.id && <div className="traffic-detail"><b>{job.status === 'FAILED' ? failure(job) : job.status === 'STOPPED' ? '使用者手動停止' : '完整結果'}</b><pre>{JSON.stringify(job.result, null, 2)}</pre></div>}</div>)}</div>
}

function Metric({ label, value, hint, icon }: { label: string; value: string; hint: string; icon: React.ReactNode }) {
  return <article className="metric"><div className="metric-icon">{icon}</div><span>{label}</span><b>{value}</b><small>{hint}</small></article>
}

export default App
