import { useEffect, useMemo, useState } from 'react';
import {
  Activity, AlertTriangle, ArrowDownRight, ArrowRight, ArrowUpRight,
  BadgeCheck, BarChart3, Bell, Blocks, Check, ChevronDown, CircleHelp,
  Clock3, Cpu, Database, FileText, Gauge, HardDrive, Home, Layers3,
  ListTodo, LockKeyhole, Menu, Play, RefreshCw, Search, Settings2,
  ShieldCheck, Terminal, Users, X, Zap, Maximize2, Minimize2
} from 'lucide-react';

const implemented = {
  jobs: { label: 'Jobs & Processes', group: 'Core OS', icon: Layers3, status: 'Implemented', description: 'Create validated jobs and track their lifecycle.' },
  scheduling: { label: 'CPU Scheduling', group: 'Core OS', icon: Cpu, status: 'Implemented', description: 'Compare FCFS, Round Robin and non-preemptive Priority.' },
  producer: { label: 'Producer–Consumer', group: 'Concurrency', icon: Activity, status: 'Implemented', description: 'Run the POSIX thread queue and inspect event logs.' },
  ipc: { label: 'Shared Memory IPC', group: 'Concurrency', icon: Database, status: 'Implemented', description: 'Exchange messages through POSIX shared memory and semaphore synchronization.' },
  banker: { label: "Banker's Algorithm", group: 'Resource Management', icon: ShieldCheck, status: 'Implemented', description: 'Evaluate resource allocation safety and safe sequences.' },
  worker: { label: 'Worker Failure', group: 'Resource Management', icon: Users, status: 'Implemented', description: 'Observe child-process completion and controlled failure.' },
  limits: { label: 'Resource Limits', group: 'Resource Management', icon: Gauge, status: 'Implemented', description: 'Demonstrate supported OS resource limits and cleanup.' },
  terminal: { label: 'Terminal / CLI', group: 'Workspace', icon: Terminal, status: 'CLI available', description: 'Run documented engine commands in your local Linux terminal.' }
};

const navGroups = [
  { title: 'OVERVIEW', items: [{ id: 'overview', label: 'Overview', icon: Home }] },
  { title: 'CORE OS', items: [{ id: 'jobs', ...implemented.jobs }, { id: 'scheduling', ...implemented.scheduling }] },
  { title: 'CONCURRENCY', items: [{ id: 'producer', ...implemented.producer }, { id: 'ipc', ...implemented.ipc }] },
  { title: 'RESOURCE MANAGEMENT', items: [{ id: 'banker', ...implemented.banker }, { id: 'worker', ...implemented.worker }, { id: 'limits', ...implemented.limits }] },
  { title: 'WORKSPACE', items: [{ id: 'terminal', ...implemented.terminal }] }
];

const defaults = [];

function Badge({ children, tone = 'neutral' }) {
  return <span className={`badge badge--${tone}`}><span className="badge-dot" />{children}</span>;
}
function PanelTitle({ eyebrow, title, subtitle, action }) {
  return <div className="panel-title"><div><div className="eyebrow">{eyebrow}</div><h2>{title}</h2>{subtitle && <p>{subtitle}</p>}</div>{action}</div>;
}


function Field({ label, children }) {
  return <label className="field"><span>{label}</span>{children}</label>;
}
function UnavailablePage({ item }) {
  const Icon = item?.icon || Settings2;
  return <div className="page-enter">
    <PageHeading eyebrow={item?.group || 'WORKSPACE'} title={item?.label || 'Module'} subtitle={item?.description || 'This module is not available yet.'}/>
    <section className="panel unavailable-panel">
      <div className="unavailable-illustration"><div className="unavailable-orbit"/><div className="unavailable-orbit unavailable-orbit--two"/><div className="unavailable-center"><Icon size={26}/></div></div>
      <Badge tone="neutral">Not implemented</Badge>
      <h2>This module is not wired up yet</h2>
      <p>The navigation entry is present, but the implementation and tests for this module still need to be completed in the CampusCompute C engine.</p>
      <div className="unavailable-checklist">
        <div><Check size={15}/> Define the module input and output contract</div>
        <div><Check size={15}/> Implement and test the C engine</div>
        <div><Check size={15}/> Connect the frontend to real engine output</div>
      </div>
    </section>
  </div>;
}

function PageHeading({ eyebrow, title, subtitle }) {
  return <div className="page-heading">
    <div className="eyebrow">{eyebrow}</div>
    <h1>{title}<span className="title-period">.</span></h1>
    {subtitle && <p className="page-lead">{subtitle}</p>}
  </div>;
}
function CodeLine({ children, copy }) {
  return <div className="code-line">
    <code>{children}</code>
    <button type="button" aria-label="Copy command" title="Copy command" onClick={() => {
      if (copy) copy();
      else if (navigator.clipboard?.writeText) navigator.clipboard.writeText(String(children));
    }}>Copy</button>
  </div>;
}

function StatCard({ icon: Icon, label, value, foot, tone = 'indigo', trend }) {
  return <article className="stat-card"><div className="stat-top"><div className={`stat-icon stat-icon--${tone}`}><Icon size={19}/></div>{trend && <span className="stat-trend"><ArrowUpRight size={14}/>{trend}</span>}</div><div className="stat-label">{label}</div><div className="stat-value">{value}</div><div className="stat-foot">{foot}</div></article>;
}
function EmptyState({ icon: Icon = Database, title, text }) {
  return <div className="empty-state"><div className="empty-icon"><Icon size={22}/></div><h3>{title}</h3><p>{text}</p></div>;
}

export default function App() {

  const [authReady, setAuthReady] = useState(false);
  const [student, setStudent] = useState(null);
  const [authMode, setAuthMode] = useState('login');
  const [authError, setAuthError] = useState('');
  const [authBusy, setAuthBusy] = useState(false);
  const [authToken, setAuthToken] = useState(() => localStorage.getItem('campuscompute_token') || '');
  const [active, setActive] = useState('overview');
  const [mobileOpen, setMobileOpen] = useState(false);
  const [notice, setNotice] = useState('');
  const [jobs, setJobs] = useState(defaults);
  const [jobForm, setJobForm] = useState({ id: '', arrival: '0', burst: '1', priority: '1' });
  const [policy, setPolicy] = useState('FCFS');
  const [quantum, setQuantum] = useState('2');
  const [schedule, setSchedule] = useState(null);
  const [pcCount, setPcCount] = useState('12');
  const [bankerResult, setBankerResult] = useState(null);
  const [available, setAvailable] = useState('');
  const [alloc, setAlloc] = useState('');
  const [max, setMax] = useState('');
  const [search, setSearch] = useState('');
  const [commandCopied, setCommandCopied] = useState(false);
  const [apiBase, setApiBase] = useState(import.meta.env.VITE_API_BASE_URL || 'http://127.0.0.1:8000');
  const [apiStatus, setApiStatus] = useState('checking');
  const [apiError, setApiError] = useState('');
  const [profileMenuOpen, setProfileMenuOpen] = useState(false);
  const [apiEvents, setApiEvents] = useState([]);
  const [apiSummary, setApiSummary] = useState('');
  const [apiBusy, setApiBusy] = useState(false);
  const [ipcMessage, setIpcMessage] = useState('Hello from CampusCompute');
  const [ipcResult, setIpcResult] = useState(null);
  const [ipcBusy, setIpcBusy] = useState(false);
  const [workerResult, setWorkerResult] = useState(null);
  const [limitsResult, setLimitsResult] = useState(null);
  const [resourceBusy, setResourceBusy] = useState(false);


  useEffect(() => {
    const token = localStorage.getItem('campuscompute_token');
    if (!token) { setAuthReady(true); return; }
    fetch(apiBase.replace(/\/$/, '') + '/api/auth/me', { headers: { Authorization: 'Bearer ' + token } })
      .then(response => response.ok ? response.json() : Promise.reject(new Error('Session expired')))
      .then(data => setStudent(data.student))
      .catch(() => { localStorage.removeItem('campuscompute_token'); setAuthToken(''); setStudent(null); })
      .finally(() => setAuthReady(true));
  }, []);
  useEffect(() => {
    const handler = (event) => { if (event.detail) setActive(event.detail); };
    window.addEventListener('campuscompute-navigate', handler);
    return () => window.removeEventListener('campuscompute-navigate', handler);
  }, []);
  useEffect(() => {
    let cancelled = false;
    const controller = new AbortController();
    const timeout = window.setTimeout(() => controller.abort(), 4500);
    const base = apiBase.replace(/\/$/, '');
    setApiStatus('checking');
    setApiError('');
    fetch(base + '/health', { signal: controller.signal })
      .then(async response => {
        if (!response.ok) throw new Error('API returned HTTP ' + response.status);
        return response.json();
      })
      .then(data => {
        if (cancelled) return;
        if (data.status === 'ok') { setApiStatus('connected'); setApiError(''); }
        else { setApiStatus('unavailable'); setApiError('Health endpoint responded, but reported an unhealthy status.'); }
      })
      .catch(error => {
        if (cancelled) return;
        setApiStatus('disconnected');
        setApiError(error.name === 'AbortError'
          ? 'Connection timed out. Start FastAPI on ' + base + '.'
          : 'Cannot reach ' + base + '. Start FastAPI in Ubuntu and check the port/CORS settings.');
      })
      .finally(() => window.clearTimeout(timeout));
    return () => { cancelled = true; controller.abort(); window.clearTimeout(timeout); };
  }, [apiBase]);
  const setPage = (id) => {
    setActive(id);
    setMobileOpen(false);
    setNotice('');
    window.scrollTo({ top: 0, behavior: 'smooth' });
  };
  const notify = (message) => { setNotice(message); window.setTimeout(() => setNotice(''), 5200); };
  const moduleList = Object.entries(implemented).map(([id, item]) => ({ id, ...item }));
  const filteredNav = useMemo(() => navGroups.map(group => ({
    ...group,
    items: group.items.filter(item => item.label.toLowerCase().includes(search.toLowerCase()))
  })).filter(group => group.items.length), [search]);

  function addJob(event) {
    event.preventDefault();
    const id = jobForm.id.trim();
    const arrival = Number(jobForm.arrival), burst = Number(jobForm.burst), priority = Number(jobForm.priority);
    if (!/^[A-Za-z][A-Za-z0-9_-]{0,11}$/.test(id)) return notify('Use a job ID starting with a letter (max 12 characters).');
    if (jobs.some(job => job.id === id)) return notify('Job IDs must be unique.');
    if (![arrival, burst, priority].every(Number.isSafeInteger) || arrival < 0 || burst <= 0 || priority < 0) return notify('Arrival must be ≥ 0, burst must be > 0, and priority must be ≥ 0.');
    if (jobs.length >= 20) return notify('This job workspace supports up to 20 jobs at a time.');
    setJobs([...jobs, { id, arrival, burst, priority }]);
    setJobForm({ id: '', arrival: '0', burst: '1', priority: '1' });
    setSchedule(null);
    notify(`Job ${id} added. Open CPU Scheduling to calculate with the current job set.`);
  }
  function runSchedule() {
    const sorted = [...jobs].sort((a, b) => a.arrival - b.arrival || a.id.localeCompare(b.id));
    if (!sorted.length) return notify('Add at least one job before scheduling.');
    if (policy === 'Round Robin' && (!Number.isInteger(Number(quantum)) || Number(quantum) <= 0 || Number(quantum) > 100000)) return notify('Round Robin quantum must be a positive integer.');
    let time = 0, remaining = Object.fromEntries(sorted.map(j => [j.id, j.burst])), done = new Set(), rows = [], timeline = [];
    const q = Number(quantum);
    if (policy === 'FCFS' || policy === 'Priority') {
      let pending = [...sorted];
      while (pending.length) {
        const ready = pending.filter(j => j.arrival <= time);
        if (!ready.length) { time = pending[0].arrival; continue; }
        ready.sort((a,b) => policy === 'Priority' ? a.priority-b.priority || a.arrival-b.arrival || a.id.localeCompare(b.id) : a.arrival-b.arrival || a.id.localeCompare(b.id));
        const job = ready[0];
        const start = time; time += job.burst;
        timeline.push({ id: job.id, start, end: time });
        rows.push({ ...job, start, completion: time, turnaround: time - job.arrival, waiting: time - job.arrival - job.burst });
        pending = pending.filter(j => j.id !== job.id);
      }
    } else {
      let queue = [], pending = [...sorted], guard = 0;
      while (rows.length < sorted.length && guard++ < 100000) {
        while (pending.length && pending[0].arrival <= time) queue.push(pending.shift());
        if (!queue.length) { if (pending.length) { time = pending[0].arrival; continue; } break; }
        const job = queue.shift(), run = Math.min(q, remaining[job.id]), start = time;
        time += run; remaining[job.id] -= run; timeline.push({ id: job.id, start, end: time });
        while (pending.length && pending[0].arrival <= time) queue.push(pending.shift());
        if (remaining[job.id] > 0) queue.push(job);
        else rows.push({ ...job, completion: time, turnaround: time - job.arrival, waiting: time - job.arrival - job.burst });
      }
    }
    rows.sort((a,b) => a.id.localeCompare(b.id));
    setSchedule({ policy, rows, timeline, avgWaiting: rows.reduce((s,j)=>s+j.waiting,0)/rows.length, avgTurnaround: rows.reduce((s,j)=>s+j.turnaround,0)/rows.length });
    notify('Scheduling calculated from the jobs currently entered in this interactive workspace.');
  }
  function runBanker(event) {
    event.preventDefault();
    const parseVector = raw => raw.trim().split(/\s+/).map(Number);
    const parseMatrix = raw => raw.trim().split(';').map(parseVector);
    const av = parseVector(available), al = parseMatrix(alloc), mx = parseMatrix(max);
    if (!available.trim() || !alloc.trim() || !max.trim()) return notify('Enter Available, Allocation and Maximum values before checking safety.');
    if (!av.length || av.some(n => !Number.isSafeInteger(n) || n < 0)) return notify('Available must be a space-separated list of non-negative integers.');
    if (!al.length || al.length !== mx.length || al.some(r=>r.length!==av.length) || mx.some(r=>r.length!==av.length)) return notify('Allocation and Maximum need the same process/resource dimensions as Available. Separate rows with semicolons.');
    if (al.some((r,i)=>r.some((v,j)=>!Number.isSafeInteger(v)||v<0||!Number.isSafeInteger(mx[i][j])||mx[i][j]<v))) return notify('Matrix values must be non-negative integers, and Maximum must be ≥ Allocation.');
    const need = mx.map((r,i)=>r.map((v,j)=>v-al[i][j]));
    const work = [...av], finished = al.map(()=>false), seq = [];
    let changed = true;
    while(changed) { changed=false; for(let i=0;i<al.length;i++) if(!finished[i]&&need[i].every((v,j)=>v<=work[j])) { work.forEach((_,j)=>work[j]+=al[i][j]); finished[i]=true;seq.push(i);changed=true; } }
    setBankerResult({ safe: finished.every(Boolean), seq, need });
    notify("Banker's safety check calculated from the entered matrices. Use the Ubuntu C executable for authoritative assessment evidence.");
  }


  async function submitAuth(form) {
    setAuthBusy(true); setAuthError('');
    try {
      const endpoint = authMode === 'register' ? '/api/auth/register' : '/api/auth/login';
      const response = await fetch(apiBase.replace(/\/$/, '') + endpoint, { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify(form) });
      const data = await response.json();
      if (!response.ok) throw new Error(data.detail || 'Authentication failed.');
      localStorage.setItem('campuscompute_token', data.token); setAuthToken(data.token); setStudent(data.student);
    } catch (error) { setAuthError(error.message); }
    finally { setAuthBusy(false); }
  }
  async function logout() {
    try { if (authToken) await fetch(apiBase.replace(/\/$/, '') + '/api/auth/logout', { method: 'POST', headers: { Authorization: 'Bearer ' + authToken } }); } catch {}
    localStorage.removeItem('campuscompute_token'); setAuthToken(''); setStudent(null); setActive('overview'); setProfileMenuOpen(false);
  }
  if (!authReady) return <div className="auth-loading">Loading CampusCompute…</div>;
  if (!student) return <AuthScreen mode={authMode} setMode={mode=>{setAuthMode(mode);setAuthError('');}} onSubmit={submitAuth} busy={authBusy} error={authError} />;
  const activeItem = active === 'overview' ? { label: 'Overview', description: 'A clear view of the operating-systems lab and its modules.' } : implemented[active];
  const currentStatus = active === 'overview' ? 'Workspace snapshot' : activeItem?.status;
  const supported = ['jobs','scheduling','producer','ipc','banker','worker','limits'].includes(active);

  return <div className="app-shell">
    {mobileOpen && <button className="mobile-scrim" aria-label="Close navigation" onClick={()=>setMobileOpen(false)} />}
    <aside className={`sidebar ${mobileOpen?'sidebar--open':''}`}>
      <div className="brand"><div className="brand-mark"><Blocks size={22}/></div><div><div className="brand-name">CampusCompute</div><div className="brand-subtitle">OPERATING SYSTEMS LAB</div></div><button className="icon-button sidebar-close" aria-label="Close navigation" onClick={()=>setMobileOpen(false)}><X size={18}/></button></div>
      <div className="side-search"><Search size={16}/><input aria-label="Search navigation" placeholder="Search modules..." value={search} onChange={e=>setSearch(e.target.value)}/><kbd>/</kbd></div>
      <nav className="side-nav" aria-label="Main navigation">
        {filteredNav.map(group=><div className="nav-group" key={group.title}><div className="nav-heading">{group.title}</div>{group.items.map(item=>{ const Icon=item.icon || Home; return <button key={item.id} onClick={()=>setPage(item.id)} className={`nav-item ${active===item.id?'nav-item--active':''}`} aria-current={active===item.id?'page':undefined}><Icon size={17}/><span>{item.label}</span>{item.status==='Implemented'&&<span className="nav-check"><Check size={12}/></span>}{item.status==='Not implemented'&&<span className="nav-lock"><LockKeyhole size={12}/></span>}</button>})}</div>)}
        {filteredNav.length===0&&<p className="no-results">No matching modules.</p>}
      </nav>
      <div className="sidebar-bottom"><div className="environment-card"><div className="env-top"><span className="env-light"/><span>Environment</span><Badge tone="neutral">Local</Badge></div><p>Linux / Ubuntu / WSL</p><div className="env-note"><Terminal size={13}/> C engine runs locally</div></div><button className="profile-row" onClick={()=>notify('CampusCompute OS laboratory workspace. Project status percentages are not tracked yet.')}><div className="avatar">CC</div><div className="profile-text"><strong>OS Laboratory</strong><span>Student workspace</span></div><ChevronDown size={16}/></button></div>
    </aside>
    <main className="main-area">
      <header className="topbar"><div className="topbar-left"><button className="icon-button mobile-menu" aria-label="Open navigation" onClick={()=>setMobileOpen(true)}><Menu size={20}/></button><div className="breadcrumb">Workspace <span>/</span> <strong>{activeItem?.label || 'Overview'}</strong></div></div><div className="topbar-actions"><div className="profile-menu-wrap"><button className={`student-profile-trigger ${profileMenuOpen ? 'student-profile-trigger--open' : ''}`} aria-haspopup="menu" aria-expanded={profileMenuOpen} onClick={()=>setProfileMenuOpen(open=>!open)}><span className="student-avatar">{(student.name || 'S').trim().charAt(0).toUpperCase()}</span><span className="student-profile-copy"><strong>{student.name}</strong><small>{student.srn}</small></span><ChevronDown size={15}/></button>{profileMenuOpen&&<><button className="profile-menu-backdrop" aria-label="Close profile menu" onClick={()=>setProfileMenuOpen(false)}/><div className="student-profile-menu" role="menu"><div className="profile-menu-heading"><strong>{student.name}</strong><span>{student.srn}</span></div><div className="profile-menu-divider"/><button role="menuitem" onClick={()=>{setProfileMenuOpen(false);logout();}}><ArrowRight size={15}/> Log out</button></div></>}</div><button type="button" className={`connection-pill connection-pill--${apiStatus}`} title={apiError || 'FastAPI health endpoint: ' + apiBase + '/health'} onClick={()=>{setApiStatus('checking');setApiError('Checking ' + apiBase + '/health…');window.location.reload();}}><span className={`muted-dot ${apiStatus === "connected" ? "muted-dot--connected" : ""}`}/>{apiStatus === "connected" ? "Local API connected" : apiStatus === "checking" ? "Checking local API…" : "Engine API not connected"}</button><button className="icon-button" title="Notifications" aria-label="Notifications" onClick={()=>notify('No notifications. The local engine/API is not connected yet.')}><Bell size={18}/></button><button className="help-button" onClick={()=>notify('Use each module screen to view its status, required inputs and the local run command.')}><CircleHelp size={17}/> Help</button></div></header>
      <div className="page-wrap">
        {notice&&<div className="notice" role="status"><span><BadgeCheck size={17}/></span>{notice}<button aria-label="Dismiss notice" onClick={()=>setNotice('')}><X size={15}/></button></div>}
        {active==='overview' ? <Overview setPage={setPage} modules={moduleList} apiStatus={apiStatus} jobs={jobs} /> :
          active==='jobs' ? <JobsPage jobs={jobs} setPage={setPage} form={jobForm} setForm={setJobForm} addJob={addJob} /> :
          active==='scheduling' ? <SchedulingPage jobs={jobs} policy={policy} setPolicy={setPolicy} quantum={quantum} setQuantum={setQuantum} schedule={schedule} runSchedule={runSchedule} /> :
          active==='banker' ? <BankerPage available={available} setAvailable={setAvailable} alloc={alloc} setAlloc={setAlloc} max={max} setMax={setMax} result={bankerResult} run={runBanker}/> :
          active==='ipc' ? <SharedMemoryPage message={ipcMessage} setMessage={setIpcMessage} result={ipcResult} busy={ipcBusy} apiStatus={apiStatus} runApi={async () => {
            setIpcBusy(true); setIpcResult(null);
            try {
              const response = await fetch(apiBase.replace(/\/$/, '') + '/api/shared-memory', { method: 'POST', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ message: ipcMessage }) });
              const data = await response.json();
              if (!response.ok) throw new Error(data.detail || 'Shared-memory exchange failed');
              setIpcResult(data); notify('Shared-memory exchange completed and reader acknowledgement received.');
            } catch (error) { notify('Shared-memory API failed: ' + error.message); }
            finally { setIpcBusy(false); }
          }}/> :
          active==='producer' ? <ProducerPage count={pcCount} setCount={setPcCount} notify={notify} apiBase={apiBase} setApiBase={setApiBase} apiStatus={apiStatus} events={apiEvents} summary={apiSummary} busy={apiBusy} runApi={async () => {
            setApiBusy(true); setApiEvents([]); setApiSummary('');
            try {
              const base = apiBase.replace(/\/$/, '');
              const response = await fetch(base + '/api/producer-consumer?event_count=' + encodeURIComponent(pcCount), { method: 'POST' });
              const data = await response.json();
              if (!response.ok) throw new Error(data.detail || 'API request failed');
              setApiEvents(data.events || []); setApiSummary(data.summary || '');
              notify('Real C engine run complete: ' + data.events_produced + ' events.');
            } catch (error) {
              notify('API run failed: ' + error.message + '. Check the local API and input.');
            } finally { setApiBusy(false); }
          }}/> :
          active==='worker' ? <WorkerFailurePage apiBase={apiBase} apiStatus={apiStatus} result={workerResult} busy={resourceBusy} run={async scenario => {
            setResourceBusy(true); setWorkerResult(null);
            try { const response=await fetch(apiBase.replace(/\/$/,'')+'/api/worker-failure?scenario='+scenario,{method:'POST'}); const data=await response.json(); if(!response.ok) throw new Error(data.detail||'Worker experiment failed'); setWorkerResult(data); notify('C engine completed the worker-process experiment.'); }
            catch(error){notify('Worker failure run failed: '+error.message);}
            finally{setResourceBusy(false);}
          }}/> :
          active==='limits' ? <ResourceLimitsPage apiBase={apiBase} apiStatus={apiStatus} result={limitsResult} busy={resourceBusy} run={async () => {
            setResourceBusy(true); setLimitsResult(null);
            try { const response=await fetch(apiBase.replace(/\/$/,'')+'/api/resource-limits',{method:'POST'}); const data=await response.json(); if(!response.ok) throw new Error(data.detail||'Resource-limit experiment failed'); setLimitsResult(data); notify('C engine verified descriptor exhaustion and cleanup.'); }
            catch(error){notify('Resource-limits run failed: '+error.message);}
            finally{setResourceBusy(false);}
          }}/> :
          active==='terminal' ? <TerminalPage notify={notify}/> :
          <UnavailablePage item={activeItem} />}
        <footer className="footer"><span>CampusCompute <span className="footer-sep">·</span> OS Laboratory</span><span><span className="footer-dot"/>Core C modules are developed independently</span></footer>
      </div>
    </main>
  </div>;
}

function Overview({ setPage, modules, apiStatus, jobs }) {
  const ready = modules.filter(m=>m.status==='Implemented').length;
  return <div className="page-enter">
    <div className="welcome-row"><div><div className="eyebrow">WEDNESDAY, OCTOBER 28, 2026 <span className="eyebrow-sep">/</span> OS LABORATORY</div><h1>System overview<span className="title-period">.</span></h1><p className="page-lead">Explore your operating-systems experiments from one workspace.</p></div><button className="button button-primary" onClick={()=>setPage('terminal')}><Terminal size={16}/> Open terminal guide <ArrowRight size={15}/></button></div>
    <div className="status-banner"><div className="status-banner-icon"><Activity size={19}/></div><div className="status-banner-copy"><strong>{apiStatus === "connected" ? "Local API connected" : "Local engine connection needed"}</strong><span>{apiStatus === "connected" ? "FR3 Producer–Consumer can run against the actual C engine. Other modules are enabled only where the API reports support." : "Start the optional localhost API to run the supported FR3 operation. Other modules remain CLI-only or unavailable."}</span></div><Badge tone="warning">Not connected</Badge></div>
    <div className="stats-grid">
      <StatCard icon={Layers3} label="Jobs entered" value="3" foot="Sample UI state · not engine data" tone="indigo" />
      <StatCard icon={Cpu} label="Scheduling policies" value="3" foot="C CLI available · API not connected" tone="teal" />
      <StatCard icon={Activity} label="Concurrency module" value="Ready" foot="C producer–consumer logger" tone="blue" />
      <StatCard icon={ShieldCheck} label="Resource safety" value="Ready" foot="Banker's Algorithm module" tone="purple" />
    </div>
    <div className="content-grid overview-grid">
      <section className="panel"><PanelTitle eyebrow="YOUR WORKSPACE" title="OS experiment modules" subtitle="Choose a module to configure inputs, run experiments and inspect results." action={<span className="small-count">{modules.length} modules</span>}/><div className="module-list">
        {modules.map(m=>{const Icon=m.icon;const available=m.status==='Implemented'||m.status==='CLI available';return <button key={m.id} className="module-row" onClick={()=>setPage(m.id)}><div className={`module-icon module-icon--${available?'ready':'muted'}`}><Icon size={18}/></div><div className="module-info"><strong>{m.label}</strong><span>{m.description}</span></div><Badge tone={m.status==='Implemented'?'success':m.status==='CLI available'?'info':'neutral'}>{m.status}</Badge><ArrowRight className="module-arrow" size={16}/></button>})}
      </div></section>
      <div className="right-stack"><section className="panel progress-panel"><PanelTitle eyebrow="IMPLEMENTATION" title="Project progress" subtitle="Tracked implementation status"/><div className="progress-placeholder"><div className="progress-ring"><span>—</span></div><div><strong>Not tracked yet</strong><p>No repository-backed project status feed is connected. No overall percentage is inferred.</p></div></div><div className="module-progress-list">{modules.map(m=><div className="module-progress" key={m.id}><span>{m.label}</span><Badge tone={m.status==='Implemented'?'success':m.status==='CLI available'?'info':'neutral'}>{m.status}</Badge></div>)}</div></section>
      <section className="tip-card"><div className="tip-icon"><Zap size={18}/></div><div><strong>Keep results trustworthy</strong><p>Browser calculations are interactive learning tools. Use the Ubuntu C executable for authoritative assessment evidence.</p></div></section></div>
    </div>
  </div>;
}
function JobsPage({jobs,setPage,form,setForm,addJob}) {
  const set=(key,value)=>setForm(prev=>({...prev,[key]:value}));
  return <div className="page-enter"><PageHeading eyebrow="CORE OS / FR1" title="Jobs & Processes" subtitle="Create jobs, validate fields and inspect the process lifecycle data model."/><div className="stats-grid stats-grid--three"><StatCard icon={Layers3} label="Jobs in workspace" value={jobs.length} foot="Current editable workspace" tone="indigo"/><StatCard icon={Clock3} label="Total CPU burst" value={jobs.reduce((s,j)=>s+j.burst,0)} foot="Time units from entered jobs" tone="teal"/><StatCard icon={ListTodo} label="Lifecycle source" value="C engine" foot="Authoritative lifecycle uses FR1" tone="purple"/></div><div className="content-grid"><section className="panel"><PanelTitle eyebrow="JOB REGISTER" title="Job records" subtitle="Add jobs here, then use CPU Scheduling to run an interactive calculation."/><div className="table-scroll"><table><thead><tr><th>Job ID</th><th>Arrival</th><th>Burst</th><th>Priority</th><th>State</th></tr></thead><tbody>{jobs.map(j=><tr key={j.id}><td><span className="job-id">{j.id}</span></td><td>{j.arrival}</td><td>{j.burst}</td><td>{j.priority}</td><td><Badge tone="info">Workspace</Badge></td></tr>)}</tbody></table></div></section><section className="panel"><PanelTitle eyebrow="NEW RECORD" title="Add a job" subtitle="Enter process details to add a job to the current workspace."/><form className="form-stack" onSubmit={addJob}><Field label="Job ID"><input required maxLength="12" placeholder="e.g. P4" value={form.id} onChange={e=>set('id',e.target.value)}/></Field><div className="field-grid"><Field label="Arrival time"><input type="number" min="0" step="1" required value={form.arrival} onChange={e=>set('arrival',e.target.value)}/></Field><Field label="CPU burst"><input type="number" min="1" step="1" required value={form.burst} onChange={e=>set('burst',e.target.value)}/></Field></div><Field label="Priority (lower number = higher priority)"><input type="number" min="0" step="1" required value={form.priority} onChange={e=>set('priority',e.target.value)}/></Field><button className="button button-primary button-full" type="submit"><span>+</span> Add job</button></form><div className="info-note"><CircleHelp size={15}/> Use <code>make -C engine/src/job_manager clean test</code> to test the actual FR1 C module.</div></section></div><section className="panel lifecycle-panel"><PanelTitle eyebrow="PROCESS LIFECYCLE" title="Supported transition model"/><div className="lifecycle"><div className="life-state"><span>01</span><strong>CREATED</strong><small>Job validated</small></div><ArrowRight/><div className="life-state"><span>02</span><strong>READY</strong><small>Waiting for CPU</small></div><ArrowRight/><div className="life-state"><span>03</span><strong>RUNNING</strong><small>Being scheduled</small></div><ArrowRight/><div className="life-state life-state--done"><span>04</span><strong>COMPLETED</strong><small>Execution finished</small></div></div></section></div>;
}
function SchedulingPage({jobs,policy,setPolicy,quantum,setQuantum,schedule,runSchedule}) {
  return <div className="page-enter"><PageHeading eyebrow="CORE OS / FR2" title="CPU Scheduling" subtitle="Compare the three supported policies with the same job set."/><div className="stats-grid stats-grid--three"><StatCard icon={Cpu} label="Available policies" value="3" foot="FCFS · Round Robin · Priority" tone="indigo"/><StatCard icon={Layers3} label="Jobs in workspace" value={jobs.length} foot="Editable job records" tone="teal"/><StatCard icon={Clock3} label="Quantum" value={policy==='Round Robin'?quantum:'—'} foot="Used only for Round Robin" tone="purple"/></div><div className="content-grid"><section className="panel"><PanelTitle eyebrow="RUN CONFIGURATION" title="Scheduler controls" subtitle="Choose a policy and calculate using the jobs entered in this workspace."/><div className="form-stack"><Field label="Scheduling policy"><select value={policy} onChange={e=>setPolicy(e.target.value)}><option>FCFS</option><option>Round Robin</option><option>Priority</option></select></Field>{policy==='Round Robin'&&<Field label="Time quantum"><input type="number" min="1" step="1" value={quantum} onChange={e=>setQuantum(e.target.value)}/></Field>}<div className="policy-help"><Badge tone="info">{policy}</Badge><p>{policy==='FCFS'?'Jobs execute in arrival order.':policy==='Round Robin'?'Ready jobs receive a fixed time slice in queue order.':'The lowest numeric priority is selected first, without preemption.'}</p></div><button className="button button-primary button-full" onClick={runSchedule}><Play size={16}/> Run calculation</button></div></section><section className="panel"><PanelTitle eyebrow="INPUT JOB SET" title="Current jobs" subtitle="Edit these on Jobs & Processes."/><div className="table-scroll"><table><thead><tr><th>Job</th><th>Arrival</th><th>Burst</th><th>Priority</th></tr></thead><tbody>{jobs.map(j=><tr key={j.id}><td><span className="job-id">{j.id}</span></td><td>{j.arrival}</td><td>{j.burst}</td><td>{j.priority}</td></tr>)}</tbody></table></div><button className="button button-secondary button-full margin-top" onClick={()=>window.dispatchEvent(new CustomEvent('campuscompute-navigate',{detail:'jobs'}))}>Job set is editable on the Jobs screen <ArrowRight size={15}/></button></section></div>{schedule?<section className="panel result-panel"><PanelTitle eyebrow="SCHEDULING RESULT" title={schedule.policy+" result"} subtitle="Calculated from the current job records in this workspace." action={<Badge tone="info">Interactive calculation</Badge>}/><div className="mini-stats"><div><span>Average waiting</span><strong>{schedule.avgWaiting.toFixed(2)}</strong></div><div><span>Average turnaround</span><strong>{schedule.avgTurnaround.toFixed(2)}</strong></div><div><span>Makespan</span><strong>{Math.max(...schedule.rows.map(r=>r.completion))}</strong></div></div><div className="gantt"><div className="gantt-label">Execution timeline</div><div className="gantt-track">{schedule.timeline.map((t,i)=><div key={i} className={`gantt-piece gantt-piece--${i%5}`} style={{flexGrow:t.end-t.start,flexBasis:0}} title={`${t.id}: ${t.start}–${t.end}`}><strong>{t.id}</strong><small>{t.start}–{t.end}</small></div>)}</div></div><div className="table-scroll"><table><thead><tr><th>Job</th><th>Completion</th><th>Turnaround</th><th>Waiting</th></tr></thead><tbody>{schedule.rows.map(j=><tr key={j.id}><td><span className="job-id">{j.id}</span></td><td>{j.completion}</td><td>{j.turnaround}</td><td>{j.waiting}</td></tr>)}</tbody></table></div><p className="footnote">For verified results and assessment evidence, run <code>make -C engine/src/scheduler clean test</code> then <code>./engine/src/scheduler/cpu_scheduler</code> in Linux/Ubuntu.</p></section>:<section className="panel"><EmptyState icon={BarChart3} title="Ready for your first run" text="Add process records on Jobs & Processes, choose a scheduling policy and run the calculation."/></section>}</div>;
}
function ResourceAllocationGraph({allocation,need}) {
  const processCount=allocation.length, resourceCount=allocation[0]?.length||0;
  const height=Math.max(190,Math.max(processCount,resourceCount)*76+35);
  const processY=i=>45+i*(height-70)/Math.max(1,processCount-1);
  const resourceY=i=>45+i*(height-70)/Math.max(1,resourceCount-1);
  return <section className="panel rag-panel">
    <div className="banker-section-heading"><div><span className="eyebrow">RESOURCE FLOW VISUALIZATION</span><h2>Resource Allocation Graph</h2><p>Allocation arrows point Resource → Process; remaining-need arrows point Process → Resource.</p></div><span className="banker-step">RAG</span></div>
    <div className="rag-legend"><span><i className="rag-legend-allocation"/>Allocated resources</span><span><i className="rag-legend-need"/>Remaining resource need</span></div>
    <div className="rag-scroll"><svg className="rag-svg" viewBox={`0 0 680 ${height}`} role="img" aria-label="Resource allocation graph showing process and resource nodes with allocation and remaining-need arrows">
      <defs><marker id="rag-allocation-arrow" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M0 0 L8 4 L0 8 z" fill="#0f766e"/></marker><marker id="rag-need-arrow" markerWidth="8" markerHeight="8" refX="7" refY="4" orient="auto"><path d="M0 0 L8 4 L0 8 z" fill="#6658d3"/></marker></defs>
      {allocation.flatMap((row,p)=>row.map((value,r)=>value>0?<g key={`a-${p}-${r}`}><path d={`M 510 ${resourceY(r)} C 400 ${resourceY(r)}, 280 ${processY(p)}, 170 ${processY(p)}`} fill="none" stroke="#0f766e" strokeWidth="2.2" markerEnd="url(#rag-allocation-arrow)"/><text x="340" y={(resourceY(r)+processY(p))/2-6} className="rag-edge-label rag-edge-label--allocation">{value}</text></g>:null))}
      {need.flatMap((row,p)=>row.map((value,r)=>value>0?<g key={`n-${p}-${r}`}><path d={`M 170 ${processY(p)+5} C 280 ${processY(p)+5}, 400 ${resourceY(r)+5}, 510 ${resourceY(r)+5}`} fill="none" stroke="#6658d3" strokeWidth="1.8" strokeDasharray="5 4" markerEnd="url(#rag-need-arrow)"/><text x="340" y={(resourceY(r)+processY(p))/2+12} className="rag-edge-label rag-edge-label--need">{value}</text></g>:null))}
      {allocation.map((_,i)=><g key={`p-${i}`}><rect x="65" y={processY(i)-19} width="105" height="38" rx="9" fill="#eef2ff" stroke="#c9c5ff"/><text x="117" y={processY(i)+4} textAnchor="middle" className="rag-node-label">P{i}</text></g>)}
      {Array.from({length:resourceCount},(_,i)=><g key={`r-${i}`}><rect x="510" y={resourceY(i)-19} width="105" height="38" rx="9" fill="#e4f6f0" stroke="#b8e6d6"/><text x="562" y={resourceY(i)+4} textAnchor="middle" className="rag-node-label">R{i}</text></g>)}
    </svg></div>
    <p className="rag-caption">Numbers beside arrows show the allocated amount or remaining need. A remaining-need edge represents a process claim, not a currently blocked request.</p>
  </section>;
}
function BankerPage({available,setAvailable,alloc,setAlloc,max,setMax,result,run}) {
  const parseVector=raw=>raw.trim()?raw.trim().split(/\s+/):[];
  const parseMatrix=raw=>raw.trim()?raw.trim().split(/[;\n]+/).filter(row=>row.trim()).map(row=>row.trim().split(/\s+/)): [];
  const vector=parseVector(available);
  const allocation=parseMatrix(alloc);
  const maximum=parseMatrix(max);
  const resources=Math.max(1,vector.length,allocation[0]?.length||0,maximum[0]?.length||0);
  const processes=Math.max(1,allocation.length,maximum.length);
  const normalizeRows=(rows,count)=>Array.from({length:count},(_,i)=>Array.from({length:resources},(_,j)=>rows[i]?.[j]??'0'));
  const allocationRows=normalizeRows(allocation,processes);
  const maximumRows=normalizeRows(maximum,processes);
  const updateVector=(index,value)=>{const next=Array.from({length:resources},(_,i)=>vector[i]??'0');next[index]=value;setAvailable(next.join(' '));};
  const updateMatrix=(setter,rows,p,r,value)=>{const next=normalizeRows(rows,processes).map(row=>[...row]);next[p][r]=value;setter(next.map(row=>row.join(' ')).join('; '));};
  const addResource=()=>{if(resources>=32)return;setAvailable([...Array.from({length:resources},(_,i)=>vector[i]??'0'),'0'].join(' '));setAlloc(allocationRows.map(row=>[...row,'0'].join(' ')).join('; '));setMax(maximumRows.map(row=>[...row,'0'].join(' ')).join('; '));};
  const removeResource=()=>{if(resources<=1)return;setAvailable(vector.slice(0,-1).join(' '));setAlloc(allocationRows.map(row=>row.slice(0,-1).join(' ')).join('; '));setMax(maximumRows.map(row=>row.slice(0,-1).join(' ')).join('; '));};
  const addProcess=()=>{if(processes>=64)return;setAlloc([...allocationRows.map(row=>row.join(' ')),Array(resources).fill('0').join(' ')].join('; '));setMax([...maximumRows.map(row=>row.join(' ')),Array(resources).fill('0').join(' ')].join('; '));};
  const removeProcess=()=>{if(processes<=1)return;setAlloc(allocationRows.slice(0,-1).map(row=>row.join(' ')).join('; '));setMax(maximumRows.slice(0,-1).map(row=>row.join(' ')).join('; '));};
  const sample=()=>{setAvailable('3 3 2');setAlloc('0 1 0; 2 0 0; 3 0 2; 2 1 1; 0 0 2');setMax('7 5 3; 3 2 2; 9 0 2; 2 2 2; 4 3 3');};
  const clear=()=>{setAvailable('0');setAlloc('0');setMax('0');};
  return <div className="page-enter banker-page">
    <PageHeading eyebrow="RESOURCE MANAGEMENT / FR5" title="Banker's Algorithm" subtitle="Build the resource tables visually, calculate remaining needs and inspect the resource-allocation graph."/>
    <div className="banker-intro"><div className="banker-intro-icon"><ShieldCheck size={22}/></div><div><strong>Deadlock avoidance workspace</strong><p>Add process rows and resource columns with buttons. Enter Available, Allocation and Maximum values, then run the safety algorithm.</p></div><Badge tone="success">FR5 implemented</Badge></div>
    <form className="banker-workspace" onSubmit={run}>
      <section className="panel banker-input-panel">
        <div className="banker-section-heading"><div><span className="eyebrow">STEP 01 · SYSTEM RESOURCES</span><h2>Available resources</h2><p>Enter the free instances for each resource type. Use the controls to add or remove resource columns.</p></div><span className="banker-step">01</span></div>
        <div className="banker-grid-controls"><span>{resources} resource types</span><button type="button" className="button button-secondary" onClick={addResource} disabled={resources>=32}>+ Add resource</button><button type="button" className="button button-secondary" onClick={removeResource} disabled={resources<=1}>− Remove resource</button></div>
        <div className="banker-vector-grid">{Array.from({length:resources},(_,r)=><label className="banker-cell" key={r}><span>Available R{r}</span><input type="number" min="0" step="1" value={vector[r]??'0'} onChange={e=>updateVector(r,e.target.value)}/></label>)}</div>
        <div className="banker-hint">Available is the number of currently free instances of each resource type.</div>
      </section>
      <div className="banker-matrix-grid">
        {[{title:'Allocation matrix',eyebrow:'STEP 02 · CURRENT STATE',description:'Resources currently assigned to each process.',rows:allocationRows,setter:setAlloc},{title:'Maximum matrix',eyebrow:'STEP 03 · CLAIMED RESOURCES',description:'Maximum resource demand for each process.',rows:maximumRows,setter:setMax}].map(matrix=><section className="panel banker-matrix-panel" key={matrix.title}>
          <div className="banker-section-heading"><div><span className="eyebrow">{matrix.eyebrow}</span><h2>{matrix.title}</h2><p>{matrix.description}</p></div><span className="banker-step">{matrix.eyebrow.slice(5,7)}</span></div>
          <div className="banker-table-wrap"><table className="banker-entry-table"><thead><tr><th>Process</th>{Array.from({length:resources},(_,r)=><th key={r}>R{r}</th>)}</tr></thead><tbody>{matrix.rows.map((row,p)=><tr key={p}><td><span className="job-id">P{p}</span></td>{row.map((value,r)=><td key={r}><input aria-label={`${matrix.title} P${p} R${r}`} type="number" min="0" step="1" value={value} onChange={e=>updateMatrix(matrix.setter,matrix.rows,p,r,e.target.value)}/></td>)}</tr>)}</tbody></table></div>
        </section>)}
      </div>
      <div className="banker-actions"><div className="banker-input-summary"><span>{resources} resource types</span><span>{processes} processes</span><button type="button" className="button button-secondary" onClick={addProcess} disabled={processes>=64}>+ Add process</button><button type="button" className="button button-secondary" onClick={removeProcess} disabled={processes<=1}>− Remove process</button></div><div className="form-actions"><button type="button" className="button button-secondary" onClick={clear}>Clear inputs</button><button type="button" className="button button-secondary" onClick={sample}>Load example</button><button className="button button-primary" type="submit"><ShieldCheck size={16}/> Run safety algorithm</button></div></div>
    </form>
    {result ? <>
      <section className="panel banker-result-panel">
        <div className="banker-result-heading"><div><span className="eyebrow">STEP 04 · ALGORITHM OUTPUT</span><h2>Safety analysis</h2><p>Need = Maximum − Allocation</p></div><Badge tone={result.safe?'success':'danger'}>{result.safe?'SAFE STATE':'UNSAFE STATE'}</Badge></div>
        <div className={`banker-verdict ${result.safe?'banker-verdict--safe':'banker-verdict--unsafe'}`}><div className="banker-verdict-mark">{result.safe?<Check size={23}/>:<AlertTriangle size={23}/>}</div><div><strong>{result.safe?'A safe sequence exists':'No safe sequence exists'}</strong><p>{result.safe?'The system can complete every process in a safe order without entering deadlock.':'The current available resources are not sufficient to find a complete safe sequence for all processes.'}</p></div></div>
        {result.safe&&<div className="banker-sequence-block"><h3>Safe execution sequence</h3><div className="banker-sequence">{result.seq.map((p,i)=><div className="banker-sequence-item" key={p}><span>P{p}</span>{i<result.seq.length-1&&<ArrowRight size={16}/>}</div>)}</div></div>}
        <div className="banker-need-heading"><div><h3>Need matrix</h3><p>Remaining resources each process may still request.</p></div><span>Need = Max − Allocation</span></div>
        <div className="table-scroll banker-need-table"><table><thead><tr><th>Process</th>{result.need[0].map((_,j)=><th key={j}>Resource R{j}</th>)}</tr></thead><tbody>{result.need.map((row,i)=><tr key={i}><td><span className="job-id">P{i}</span></td>{row.map((v,j)=><td key={j}>{v}</td>)}</tr>)}</tbody></table></div>
      </section>
      <ResourceAllocationGraph allocation={allocationRows.map(row=>row.map(Number))} need={result.need}/>
      <p className="banker-source-note">The dashboard graph uses Resource → Process for allocated instances and Process → Resource for remaining need claims. Verify the same state in the Ubuntu C CLI with <code>./engine/src/banker/banker</code>.</p>
    </> : <section className="panel banker-empty"><div className="banker-empty-icon"><BarChart3 size={22}/></div><div><strong>Your safety analysis and graph will appear here</strong><p>Enter the resource tables and select <b>Run safety algorithm</b>.</p></div></section>}
  </div>;
}


function WorkerFailurePage({apiBase,apiStatus,result,busy,run}) {
  return <div className="page-enter">
    <PageHeading eyebrow="RESOURCE MANAGEMENT / FR6" title="Worker Failure" subtitle="Spawn a child worker, observe normal completion or a controlled failure, and verify that the parent reaps the child."/>
    <div className="status-banner status-banner--teal"><div className="status-banner-icon"><Users size={19}/></div><div className="status-banner-copy"><strong>Real fork() + waitpid() experiment</strong><span>The dashboard invokes the C process-manager executable. The failure case exits with code 42 intentionally; it is not a crash of the dashboard or API.</span></div><Badge tone={apiStatus==='connected'?'success':'danger'}>{apiStatus==='connected'?'API connected':'API disconnected'}</Badge></div>
    <div className="content-grid">
      <section className="panel"><PanelTitle eyebrow="RUN CONFIGURATION" title="Worker scenarios" subtitle="Choose how the child process should finish."/>
        <div className="form-stack"><button className="button button-primary button-full" disabled={busy||apiStatus!=='connected'} onClick={()=>run('normal')}>{busy?<RefreshCw size={16}/>:<Play size={16}/>} Run normal completion</button>
        <button className="button button-secondary button-full" disabled={busy||apiStatus!=='connected'} onClick={()=>run('failure')}>{busy?<RefreshCw size={16}/>:<AlertTriangle size={16}/>} Simulate controlled worker failure</button>
        <p className="footnote">API: <code>{apiBase}/api/worker-failure</code></p>
        {apiStatus!=='connected'&&<p className="footnote">Start FastAPI in Ubuntu to enable execution.</p>}</div>
      </section>
      <section className="panel"><PanelTitle eyebrow="PROCESS REPORT" title="Child lifecycle" subtitle={result?'Returned by the C process manager':'Run a scenario to inspect the child process report.'} action={result?<Badge tone="success">C engine</Badge>:null}/>
        {result ? <><div className="stats-grid stats-grid--three"><StatCard icon={Users} label="Child reaped" value={result.child_reaped ? "Yes" : "No"} foot="waitpid() confirmation" tone="teal"/><StatCard icon={AlertTriangle} label="Exit code" value={result.child_exit_code ?? "—"} foot={result.scenario === "failure" ? "Expected controlled failure" : "Normal completion"} tone="purple"/><StatCard icon={BadgeCheck} label="Scenario" value={result.scenario === "failure" ? "Failure" : "Normal"} foot={result.outcome.replaceAll("_", " ")} tone="indigo"/></div><pre className="terminal-line terminal-line--output">{result.output}</pre></> : <EmptyState icon={Users} title="No worker run yet" text="Run normal completion and controlled failure to compare child exit status and parent cleanup."/>}
      </section>
    </div>
    <section className="panel"><PanelTitle eyebrow="BUILD & TEST" title="Ubuntu CLI" subtitle="Build and test the same C module independently."/><CodeLine>make -C engine/src/process_manager clean test all</CodeLine><CodeLine>./engine/src/process_manager/worker_failure_demo normal</CodeLine><CodeLine>./engine/src/process_manager/worker_failure_demo failure</CodeLine></section>
  </div>;
}

function ResourceLimitsPage({apiBase,apiStatus,result,busy,run}) {
  return <div className="page-enter">
    <PageHeading eyebrow="RESOURCE MANAGEMENT / FR7" title="Resource Limits" subtitle="Apply RLIMIT_NOFILE in an isolated child, trigger file-descriptor exhaustion, and verify safe cleanup."/>
    <div className="status-banner status-banner--teal"><div className="status-banner-icon"><Gauge size={19}/></div><div className="status-banner-copy"><strong>Real getrlimit() + setrlimit() experiment</strong><span>The C engine lowers the child process soft descriptor limit, opens /dev/null until EMFILE, closes every descriptor, and leaves the parent limits unchanged.</span></div><Badge tone={apiStatus==='connected'?'success':'danger'}>{apiStatus==='connected'?'API connected':'API disconnected'}</Badge></div>
    <div className="content-grid">
      <section className="panel"><PanelTitle eyebrow="RUN CONFIGURATION" title="Descriptor limit experiment" subtitle="The demonstration runs in a child process so the dashboard/API process limits are not modified."/>
        <div className="form-stack"><button className="button button-primary button-full" disabled={busy||apiStatus!=='connected'} onClick={run}>{busy?<RefreshCw size={16}/>:<Play size={16}/>} Run resource-limit experiment</button><p className="footnote">API: <code>{apiBase}/api/resource-limits</code></p>{apiStatus!=='connected'&&<p className="footnote">Start FastAPI in Ubuntu to enable execution.</p>}</div>
      </section>
      <section className="panel"><PanelTitle eyebrow="ENGINE REPORT" title="Limit and cleanup results" subtitle={result?'Verified values returned by the C executable':'No resource-limit run yet.'} action={result?<Badge tone="success">C engine</Badge>:null}/>
        {result?<><div className="stats-grid stats-grid--three"><StatCard icon={Gauge} label="Applied soft limit" value={result.applied_soft_limit} foot="RLIMIT_NOFILE" tone="indigo"/><StatCard icon={HardDrive} label="Descriptors opened" value={result.descriptors_opened} foot="Until EMFILE" tone="teal"/><StatCard icon={Check} label="Descriptors closed" value={result.descriptors_closed} foot="All opened descriptors" tone="purple"/></div><div className="summary-strip"><Check size={16}/><code>EMFILE detected · cleanup successful · parent limits unchanged</code></div><pre className="terminal-line terminal-line--output">{result.output}</pre></>:<EmptyState icon={Gauge} title="No limit experiment yet" text="Run the experiment to see original/applied limits, exhaustion detection and cleanup counts."/></>}
      </section>
    </div>
    <section className="panel"><PanelTitle eyebrow="BUILD & TEST" title="Ubuntu CLI" subtitle="Run the FR7 executable and unit tests directly."/><CodeLine>make -C engine/src/resource_limits clean test all</CodeLine><CodeLine>./engine/src/resource_limits/resource_limits_demo</CodeLine></section>
  </div>;
}

function SharedMemoryPage({message,setMessage,result,busy,apiStatus,runApi}) {
  const valid = message.trim().length > 0 && new TextEncoder().encode(message).length <= 255;
  return <div className="page-enter">
    <PageHeading eyebrow="CONCURRENCY / FR4" title="Shared Memory IPC" subtitle="Send a message between two independent C processes using POSIX shared memory and named semaphores."/>
    <div className="status-banner status-banner--teal"><div className="status-banner-icon"><Database size={19}/></div><div className="status-banner-copy"><strong>Real POSIX shared-memory implementation</strong><span>The API launches the C writer and reader processes. The reader receives the message and posts an acknowledgement semaphore.</span></div><Badge tone={apiStatus==='connected'?'success':apiStatus==='checking'?'warning':'danger'}>{apiStatus==='connected'?'API connected':apiStatus==='checking'?'Checking API':'API disconnected'}</Badge></div>
    <div className="content-grid">
      <section className="panel"><PanelTitle eyebrow="MESSAGE INPUT" title="Publish to shared memory" subtitle="Enter up to 255 UTF-8 bytes. The C writer publishes this message for the reader process."/>
        <div className="form-stack"><Field label="Message to send"><textarea rows="4" maxLength={255} value={message} onChange={e=>setMessage(e.target.value)} placeholder="Enter a message for the reader process…"/></Field>
          <div className="form-actions"><span className="footnote">{new TextEncoder().encode(message).length}/255 bytes</span><button className="button button-primary" disabled={!valid||apiStatus!=='connected'||busy} onClick={runApi}>{busy?<RefreshCw size={16}/>:<Play size={16}/>} {busy?'Running IPC exchange…':'Run real C IPC exchange'}</button></div>
          {apiStatus!=='connected'&&<p className="footnote">Start FastAPI in Ubuntu: <code>python3 -m uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000</code></p>}
        </div><div className="info-note"><CircleHelp size={15}/> The API serializes exchanges because this C demonstration uses fixed POSIX object names. No browser-side simulation is used.</div>
      </section>
      <section className="panel"><PanelTitle eyebrow="PROCESS EXCHANGE" title="Writer → Shared Memory → Reader" subtitle="Two separate C processes synchronize through ready and acknowledgement semaphores."/>
        <div className="safe-sequence"><span className="sequence-step"><span>Writer</span><ArrowRight size={15}/></span><span className="sequence-step"><span>Shared memory</span><ArrowRight size={15}/></span><span className="sequence-step"><span>Reader</span></span></div>
        {result?<><div className="summary-strip"><Check size={16}/><code>Reader acknowledged sequence {result.sequence}</code></div><div className="form-stack"><Field label="Message sent by writer"><textarea rows="2" readOnly value={result.message_sent}/></Field><Field label="Message received by reader"><textarea rows="2" readOnly value={result.message_received}/></Field></div><Badge tone="success">Acknowledgement received</Badge><h3 className="subheading">Writer output</h3><pre className="terminal-line terminal-line--output">{result.writer_output}</pre><h3 className="subheading">Reader output</h3><pre className="terminal-line terminal-line--output">{result.reader_output}</pre></>:<EmptyState icon={Database} title="No IPC exchange yet" text="Send a message to launch the writer and reader processes and display their actual C output here."/>}
      </section>
    </div>
    <section className="panel"><PanelTitle eyebrow="BUILD & TEST" title="Run independently from Ubuntu" subtitle="You can also test the C module without the dashboard."/><CodeLine>make -C engine/src/shared_memory clean test</CodeLine><CodeLine>./engine/src/shared_memory/shared_memory writer "Hello from CampusCompute"</CodeLine><p className="footnote">For the standalone reader command, run it in a second terminal while the writer is waiting for acknowledgement.</p></section>
  </div>;
}

function ProducerPage({count,setCount,notify,apiBase,setApiBase,apiStatus,events,summary,busy,runApi}) {
  const validCount = Number.isInteger(Number(count)) && Number(count) >= 1 && Number(count) <= 100000;
  return <div className="page-enter"><PageHeading eyebrow="CONCURRENCY / FR3" title="Producer–Consumer" subtitle="Execute the real POSIX-threaded C logger through the localhost API and inspect its event log."/><div className="status-banner status-banner--teal"><div className="status-banner-icon"><Activity size={19}/></div><div className="status-banner-copy"><strong>POSIX producer–consumer module is implemented</strong><span>The adapter invokes the fixed C executable, parses the temporary TSV log, and returns those real events.</span></div><Badge tone={apiStatus==='connected'?'success':apiStatus==='checking'?'warning':'danger'}>{apiStatus==='connected'?'API connected':apiStatus==='checking'?'Checking API':'API disconnected'}</Badge></div><div className="content-grid"><section className="panel"><PanelTitle eyebrow="LOCAL RUN CONFIGURATION" title="Run C engine" subtitle="Requests are sent only to the configured local API origin."/><div className="form-stack"><Field label="API base URL"><input value={apiBase} onChange={e=>setApiBase(e.target.value.trim())} placeholder="http://127.0.0.1:8000" /></Field><Field label="Number of events (1–100000)"><input type="number" min="1" max="100000" step="1" value={count} onChange={e=>setCount(e.target.value)}/></Field><button className="button button-primary button-full" disabled={!validCount||apiStatus!=='connected'||busy} onClick={runApi}>{busy?<RefreshCw size={16}/>:<Play size={16}/>} {busy?'Running C module…':'Run real C module'}</button>{apiStatus!=='connected'&&<p className="footnote">Start the adapter from the repository root in Ubuntu/WSL: <code>python3 -m uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000</code></p>}</div><div className="info-note"><CircleHelp size={15}/> The API validates this count and calls the fixed executable with shell execution disabled. It does not run browser-supplied commands.</div></section><section className="panel"><PanelTitle eyebrow="REAL ENGINE OUTPUT" title="Runtime summary" subtitle={events.length?'Returned by the C executable':'No event log loaded yet.'} action={events.length?<Badge tone="success">C engine</Badge>:null}/>{summary&&<div className="summary-strip"><Check size={16}/><code>{summary}</code></div>}{events.length?<div className="table-scroll"><table><thead><tr><th>Time</th><th>Sequence</th><th>Message</th></tr></thead><tbody>{events.map(event=><tr key={event.sequence}><td>{event.timestamp}</td><td>{event.sequence}</td><td>{event.message}</td></tr>)}</tbody></table></div>:<EmptyState icon={FileText} title="No runtime events loaded" text="When the API is connected, run a batch to display real event-log rows here."/>}</section></div><section className="panel"><PanelTitle eyebrow="BUILD & TEST" title="Direct CLI fallback" subtitle="The engine stays usable when the dashboard or API is stopped."/><CodeLine>make -C engine/src/producer_consumer clean test</CodeLine><CodeLine>mkdir -p logs</CodeLine><CodeLine>./engine/src/producer_consumer/producer_consumer {count} logs/activity.log</CodeLine></section></div>;
}
function TerminalPage({notify}) {
  const [input, setInput] = useState('');
  const [terminalExpanded, setTerminalExpanded] = useState(false);
  const [history, setHistory] = useState([]);
  const [historyIndex, setHistoryIndex] = useState(-1);
  const [lines, setLines] = useState([
    {type:'system', text:'CampusCompute terminal · Ubuntu interactive backend'},
    {type:'muted', text:'Connecting to the local command runner at http://127.0.0.1:8000 …'},
    {type:'success', text:'Run ./engine/src/scheduler/cpu_scheduler to start an interactive session. Enter sends input to the running program.'}
  ]);
  const [cwd, setCwd] = useState('.');
  const [busy, setBusy] = useState(false);
  const [connected, setConnected] = useState(false);
  const [sessionId, setSessionId] = useState('');
  const apiBase = 'http://127.0.0.1:8000';
  const quickCommands = ['help', 'project', 'modules', 'pwd', 'ls', 'git status', 'build scheduler', 'test scheduler'];

  useEffect(() => {
    let cancelled = false;
    fetch(apiBase + '/health')
      .then(response => response.ok ? response.json() : Promise.reject(new Error('API unavailable')))
      .then(() => {
        if (!cancelled) {
          setConnected(true);
          setLines(prev => [...prev, {type:'success', text:'Local API connected. Interactive programs can receive keyboard input.'}]);
        }
      })
      .catch(() => {
        if (!cancelled) {
          setConnected(false);
          setLines(prev => [...prev, {type:'warning', text:'Local API is not running. Start it from Ubuntu/WSL, then try again.'}]);
        }
      });
    return () => { cancelled = true; };
  }, []);

  useEffect(() => {
    if (!sessionId) return undefined;
    let cancelled = false;
    const poll = async () => {
      try {
        const response = await fetch(apiBase + '/api/terminal/session/' + sessionId);
        const data = await response.json().catch(() => ({}));
        if (!response.ok) throw new Error(data.detail || 'Interactive session polling failed');
        if (cancelled) return;
        if (data.output) setLines(prev => [...prev, {type:'output', text:data.output}]);
        if (!data.running) {
          setLines(prev => [...prev, {type:'success', text:'Program exited with code ' + data.exit_code}]);
          setSessionId('');
        }
      } catch (error) {
        if (!cancelled) {
          setLines(prev => [...prev, {type:'error', text:error.message}]);
          setSessionId('');
        }
      }
    };
    const timer = window.setInterval(poll, 250);
    poll();
    return () => { cancelled = true; window.clearInterval(timer); };
  }, [sessionId]);

  function appendCommand(cmd, output, type='output') {
    setLines(prev => [
      ...prev,
      {type:'prompt', text:'student@campuscompute:' + (cwd === '.' ? '~' : '~/' + cwd) + '$ ' + cmd},
      ...(output ? [{type, text:output}] : [])
    ]);
  }

  async function sendInteractiveInput(raw) {
    const text = raw + '\n';
    setLines(prev => [...prev, {type:'prompt', text:raw}]);
    setInput('');
    setHistory(prev => [raw, ...prev].slice(0, 100));
    setHistoryIndex(-1);
    try {
      const response = await fetch(apiBase + '/api/terminal/session/input', {
        method:'POST', headers:{'Content-Type':'application/json'},
        body:JSON.stringify({session_id:sessionId, text})
      });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) throw new Error(data.detail || 'Could not send input');
    } catch (error) {
      setLines(prev => [...prev, {type:'error', text:error.message}]);
    }
  }

  async function runCommand(raw) {
    const cmd = raw.trim();
    if (!cmd || busy) return;
    if (sessionId) {
      // Provide a terminal-level escape hatch even if the running program is stuck
      // at a different prompt than expected. "exit"/"quit" stops the child session.
      if (['exit', 'quit'].includes(raw.trim().toLowerCase())) {
        await stopInteractive();
        setInput('');
        return;
      }
      await sendInteractiveInput(raw);
      return;
    }
    setHistory(prev => [cmd, ...prev].slice(0, 100));
    setHistoryIndex(-1);
    setInput('');
    if (cmd === 'clear' || cmd === 'cls') { setLines([]); return; }
    if (cmd === 'help') {
      appendCommand(cmd, 'CampusCompute terminal\\nInteractive: ./engine/src/scheduler/cpu_scheduler (type answers at each prompt)\\nNavigation: pwd | ls [path] | cd <directory> | tree [path]\\nFiles: cat <file> | head [-n N] <file> | tail [-n N] <file> | grep <text> <file> | find [path] [-name pattern] | wc <file>\\nProject: build [scheduler|producer] | test [backend|scheduler|producer|all] | run producer [count]\\nEnter sends input to an interactive program. Select 0 in the scheduler menu to exit.');
      return;
    }
    setBusy(true);
    try {
      if (cmd.startsWith('./')) {
        const response = await fetch(apiBase + '/api/terminal/session/start', {
          method:'POST', headers:{'Content-Type':'application/json'},
          body:JSON.stringify({command:cmd, cwd})
        });
        const data = await response.json().catch(() => ({}));
        if (!response.ok) throw new Error(data.detail || 'Could not start interactive program');
        setConnected(true);
        setSessionId(data.session_id);
        appendCommand(cmd, 'Interactive session started. Output will stream below; type answers and press Enter.');
        return;
      }
      const response = await fetch(apiBase + '/api/terminal/execute', {
        method: 'POST',
        headers: {'Content-Type':'application/json'},
        body: JSON.stringify({command:cmd, cwd})
      });
      const data = await response.json().catch(() => ({}));
      if (!response.ok) throw new Error(data.detail || 'Terminal request failed (' + response.status + ')');
      setConnected(true);
      if (data.cwd !== undefined) setCwd(data.cwd);
      const output = [data.stdout || '', data.stderr || ''].filter(Boolean).join(data.stdout && data.stderr ? '\\n' : '');
      appendCommand(cmd, output || (data.exit_code === 0 ? '(completed with no output)' : 'Command failed with exit code ' + data.exit_code), data.exit_code === 0 ? 'output' : 'error');
      if (data.exit_code !== 0) setLines(prev => [...prev, {type:'warning', text:'Exit code: ' + data.exit_code}]);
    } catch (error) {
      setConnected(false);
      appendCommand(cmd, error.message + '\\nMake sure the Ubuntu backend is running and the command is supported.', 'error');
    } finally {
      setBusy(false);
    }
  }

  async function stopInteractive() {
    if (!sessionId) return;
    try { await fetch(apiBase + '/api/terminal/session/' + sessionId, {method:'DELETE'}); }
    finally {
      setLines(prev => [...prev, {type:'warning', text:'Interactive session stopped.'}]);
      setSessionId('');
    }
  }

  function onKeyDown(event) {
    if (event.key === 'Enter') { event.preventDefault(); runCommand(input); }
    else if (!sessionId && event.key === 'ArrowUp') {
      event.preventDefault();
      if (history.length) {
        const next = Math.min(historyIndex + 1, history.length - 1);
        setHistoryIndex(next); setInput(history[next]);
      }
    } else if (!sessionId && event.key === 'ArrowDown') {
      event.preventDefault();
      if (historyIndex > 0) {
        const next = historyIndex - 1; setHistoryIndex(next); setInput(history[next]);
      } else { setHistoryIndex(-1); setInput(''); }
    } else if (event.ctrlKey && event.key.toLowerCase() === 'l') {
      event.preventDefault(); setLines([]);
    }
  }

  return <div className="page-enter terminal-page">
    <PageHeading eyebrow="WORKSPACE / CLI" title="Terminal" subtitle="Run commands and interact with C programs directly in Ubuntu."/>
    <div className={connected ? 'terminal-notice terminal-notice--connected' : 'terminal-notice'}>
      {connected ? <Check size={17}/> : <AlertTriangle size={17}/>}
      <div><strong>{connected ? (sessionId ? 'Interactive program running' : 'Ubuntu terminal backend connected') : 'Local backend required'}</strong>
        <p>{connected ? 'Interactive sessions keep the process alive and send each Enter key input to the running program.' : <>Start the API from the CampusCompute repository root in Ubuntu: <code>python3 -m uvicorn app.main:app --app-dir backend --host 127.0.0.1 --port 8000</code></>}</p>
        <p>Interactive execution is limited to executable files inside the CampusCompute repository. Use 0 in the scheduler menu to exit normally.</p>
      </div>
    </div>
    <section className={terminalExpanded ? "terminal-window terminal-window--expanded" : "terminal-window"} aria-label="Interactive Ubuntu terminal">
      <div className="terminal-titlebar"><div className="terminal-lights"><span/><span/><span/></div><div className="terminal-title"><Terminal size={14}/> campuscompute — interactive terminal</div><button type="button" className="terminal-clear terminal-expand" onClick={()=>setTerminalExpanded(value=>!value)} title={terminalExpanded ? "Restore terminal size" : "Enlarge terminal"} aria-label={terminalExpanded ? "Restore terminal size" : "Enlarge terminal"}>{terminalExpanded?<Minimize2 size={13}/>:<Maximize2 size={13}/>}<span>{terminalExpanded ? "Restore" : "Enlarge"}</span></button>{sessionId&&<button className="terminal-clear" onClick={stopInteractive} title="Stop interactive program">Stop program</button>}<button className="terminal-clear" onClick={()=>setLines([])} title="Clear terminal">Clear</button></div>
      <div className="terminal-screen" onClick={()=>document.getElementById('terminal-command')?.focus()}>
        {lines.map((line,index)=><pre className={'terminal-line terminal-line--' + line.type} key={index}>{line.text}</pre>)}
        <form className="terminal-input-row" onSubmit={event=>{event.preventDefault();runCommand(input);}}>
          <span className="terminal-prompt">{sessionId ? 'program input ›' : 'student@campuscompute:' + (cwd === '.' ? '~' : '~/' + cwd) + '$'}</span>
          <input id="terminal-command" aria-label={sessionId?'Input for running program':'Terminal command'} autoComplete="off" spellCheck="false" value={input} onChange={event=>setInput(event.target.value)} onKeyDown={onKeyDown} placeholder={sessionId ? 'type program input and press Enter…' : 'type a command…'} disabled={busy}/>
          <button type="submit" className="terminal-run" disabled={!input.trim()||busy}>{busy?<RefreshCw size={14}/>:<Play size={14}/>} {sessionId?'Send input':busy?'Running':'Run'}</button>
        </form>
      </div>
      <div className="terminal-footer"><span><i className={connected?'':'terminal-dot--offline'}/>{sessionId?'interactive session · input enabled':connected?'connected · local execution':'waiting for backend'}</span><span>↑/↓ history · Enter run/send · Ctrl+L clear</span></div>
    </section>
    <section className="panel terminal-quick-panel"><PanelTitle eyebrow="QUICK COMMANDS" title="Try a command" subtitle="Shortcuts execute through the Ubuntu backend."/><div className="terminal-quick-actions">{quickCommands.map(cmd=><button key={cmd} className="terminal-chip" onClick={()=>runCommand(cmd)} disabled={busy||!!sessionId}><code>{cmd}</code><ArrowRight size={13}/></button>)}</div></section>
    <section className="panel"><PanelTitle eyebrow="START THE BACKEND" title="Ubuntu setup" subtitle="Run these commands from the CampusCompute repository root in Ubuntu and keep the server open."/></section>
  </div>;
}


function AuthScreen({ mode, setMode, onSubmit, busy, error }) {
  const [form, setForm] = useState({ name: '', srn: '', password: '', confirmPassword: '' });
  const register = mode === 'register';
  function submit(event) {
    event.preventDefault();
    if (register && form.password !== form.confirmPassword) return;
    onSubmit(register ? { name: form.name.trim(), srn: form.srn.trim(), password: form.password } : { srn: form.srn.trim(), password: form.password });
  }
  const inputStyle = { width: '100%', boxSizing: 'border-box', padding: '13px 14px', border: '1px solid #d9deea', borderRadius: 10, background: '#fff', color: '#182033', fontSize: 14 };
  const labelStyle = { display: 'grid', gap: 7, color: '#35405a', fontSize: 13, fontWeight: 650 };
  return <main className="auth-screen">
    <section className="auth-card">
      <div className="auth-brand"><div className="auth-brand-mark"><Blocks size={23}/></div><div><strong>CampusCompute</strong><span>OPERATING SYSTEMS LAB</span></div></div>
      <div className="auth-heading"><div className="eyebrow">STUDENT WORKSPACE</div><h1>{register ? 'Create your account' : 'Welcome back'}<span className="title-period">.</span></h1><p>{register ? 'Register with your student details to access your OS laboratory dashboard.' : 'Sign in with your SRN to continue to your CampusCompute workspace.'}</p></div>
      <div className="auth-tabs"><button className={!register?'auth-tab auth-tab--active':'auth-tab'} onClick={()=>setMode('login')} type="button">Login</button><button className={register?'auth-tab auth-tab--active':'auth-tab'} onClick={()=>setMode('register')} type="button">Register</button></div>
      <form className="auth-form" onSubmit={submit}>
        {register && <label style={labelStyle}>Full name<input style={inputStyle} autoComplete="name" placeholder="Enter your full name" value={form.name} onChange={e=>setForm({...form,name:e.target.value})} minLength={2} maxLength={80} required/></label>}
        <label style={labelStyle}>Student SRN<input style={inputStyle} autoComplete="username" placeholder="Enter your SRN" value={form.srn} onChange={e=>setForm({...form,srn:e.target.value.toUpperCase()})} minLength={4} maxLength={32} required/></label>
        <label style={labelStyle}>Password<input style={inputStyle} type="password" autoComplete={register?'new-password':'current-password'} placeholder="At least 8 characters" value={form.password} onChange={e=>setForm({...form,password:e.target.value})} minLength={8} maxLength={128} required/></label>
        {register && <label style={labelStyle}>Confirm password<input style={inputStyle} type="password" autoComplete="new-password" placeholder="Re-enter your password" value={form.confirmPassword} onChange={e=>setForm({...form,confirmPassword:e.target.value})} minLength={8} maxLength={128} required/></label>}
        {register && form.confirmPassword && form.password !== form.confirmPassword && <p className="auth-error">Passwords do not match.</p>}
        {error && <p className="auth-error" role="alert">{error}</p>}
        <button className="button button-primary button-full auth-submit" type="submit" disabled={busy || (register && form.password !== form.confirmPassword)}>{busy?'Please wait…':register?'Create account':'Login to dashboard'}<ArrowRight size={16}/></button>
      </form>
      <p className="auth-footnote"><LockKeyhole size={14}/> Passwords are securely hashed in the local SQLite database.</p>
    </section>
    <div className="auth-side-note"><span>CAMPUSCOMPUTE / OS LAB</span><strong>Intelligent Job Scheduling<br/>and Resource Management</strong><p>A unified workspace for operating-systems experiments.</p></div>
  </main>;
}
