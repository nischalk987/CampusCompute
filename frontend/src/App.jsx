import { useMemo, useState } from 'react';
import {
  Activity, AlertTriangle, ArrowDownRight, ArrowRight, ArrowUpRight,
  BadgeCheck, BarChart3, Bell, Blocks, Check, ChevronDown, CircleHelp,
  Clock3, Cpu, Database, FileText, Gauge, HardDrive, Home, Layers3,
  ListTodo, LockKeyhole, Menu, Play, RefreshCw, Search, Settings2,
  ShieldCheck, Terminal, Users, X, Zap
} from 'lucide-react';

const implemented = {
  jobs: { label: 'Jobs & Processes', group: 'Core OS', icon: Layers3, status: 'Implemented', description: 'Create validated jobs and track their lifecycle.' },
  scheduling: { label: 'CPU Scheduling', group: 'Core OS', icon: Cpu, status: 'Implemented', description: 'Compare FCFS, Round Robin and non-preemptive Priority.' },
  producer: { label: 'Producer–Consumer', group: 'Concurrency', icon: Activity, status: 'Implemented', description: 'Run the POSIX thread queue and inspect event logs.' },
  ipc: { label: 'Shared Memory IPC', group: 'Concurrency', icon: Database, status: 'Not implemented', description: 'POSIX shared-memory exchange and synchronization.' },
  banker: { label: "Banker's Algorithm", group: 'Resource Management', icon: ShieldCheck, status: 'Implemented', description: 'Evaluate resource allocation safety and safe sequences.' },
  worker: { label: 'Worker Failure', group: 'Resource Management', icon: Users, status: 'Not implemented', description: 'Observe child-process completion and controlled failure.' },
  limits: { label: 'Resource Limits', group: 'Resource Management', icon: Gauge, status: 'Not implemented', description: 'Demonstrate supported OS resource limits and cleanup.' },
  terminal: { label: 'Terminal / CLI', group: 'Workspace', icon: Terminal, status: 'CLI available', description: 'Run documented engine commands in your local Linux terminal.' }
};

const navGroups = [
  { title: 'OVERVIEW', items: [{ id: 'overview', label: 'Overview', icon: Home }] },
  { title: 'CORE OS', items: [{ id: 'jobs', ...implemented.jobs }, { id: 'scheduling', ...implemented.scheduling }] },
  { title: 'CONCURRENCY', items: [{ id: 'producer', ...implemented.producer }, { id: 'ipc', ...implemented.ipc }] },
  { title: 'RESOURCE MANAGEMENT', items: [{ id: 'banker', ...implemented.banker }, { id: 'worker', ...implemented.worker }, { id: 'limits', ...implemented.limits }] },
  { title: 'WORKSPACE', items: [{ id: 'terminal', ...implemented.terminal }] }
];

const defaults = [
  { id: 'P1', arrival: 0, burst: 5, priority: 2 },
  { id: 'P2', arrival: 1, burst: 3, priority: 1 },
  { id: 'P3', arrival: 2, burst: 7, priority: 3 }
];

function Badge({ children, tone = 'neutral' }) {
  return <span className={`badge badge--${tone}`}><span className="badge-dot" />{children}</span>;
}
function PanelTitle({ eyebrow, title, subtitle, action }) {
  return <div className="panel-title"><div><div className="eyebrow">{eyebrow}</div><h2>{title}</h2>{subtitle && <p>{subtitle}</p>}</div>{action}</div>;
}
function StatCard({ icon: Icon, label, value, foot, tone = 'indigo', trend }) {
  return <article className="stat-card"><div className="stat-top"><div className={`stat-icon stat-icon--${tone}`}><Icon size={19}/></div>{trend && <span className="stat-trend"><ArrowUpRight size={14}/>{trend}</span>}</div><div className="stat-label">{label}</div><div className="stat-value">{value}</div><div className="stat-foot">{foot}</div></article>;
}
function EmptyState({ icon: Icon = Database, title, text }) {
  return <div className="empty-state"><div className="empty-icon"><Icon size={22}/></div><h3>{title}</h3><p>{text}</p></div>;
}

export default function App() {
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
  const [available, setAvailable] = useState('3 3 2');
  const [alloc, setAlloc] = useState('0 1 0; 2 0 0; 3 0 2; 2 1 1; 0 0 2');
  const [max, setMax] = useState('7 5 3; 3 2 2; 9 0 2; 2 2 2; 4 3 3');
  const [search, setSearch] = useState('');

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
    if (jobs.length >= 20) return notify('This dashboard preview supports up to 20 jobs at a time.');
    setJobs([...jobs, { id, arrival, burst, priority }]);
    setJobForm({ id: '', arrival: '0', burst: '1', priority: '1' });
    setSchedule(null);
    notify(`Job ${id} added to the local dashboard preview. Run the C module for authoritative engine results.`);
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
    notify('Calculated in the browser preview from the current form data—not from the C engine.');
  }
  function runBanker(event) {
    event.preventDefault();
    const parseVector = raw => raw.trim().split(/\s+/).map(Number);
    const parseMatrix = raw => raw.trim().split(';').map(parseVector);
    const av = parseVector(available), al = parseMatrix(alloc), mx = parseMatrix(max);
    if (!av.length || av.some(n => !Number.isSafeInteger(n) || n < 0)) return notify('Available must be a space-separated list of non-negative integers.');
    if (!al.length || al.length !== mx.length || al.some(r=>r.length!==av.length) || mx.some(r=>r.length!==av.length)) return notify('Allocation and Maximum need the same process/resource dimensions as Available. Separate rows with semicolons.');
    if (al.some((r,i)=>r.some((v,j)=>!Number.isSafeInteger(v)||v<0||!Number.isSafeInteger(mx[i][j])||mx[i][j]<v))) return notify('Matrix values must be non-negative integers, and Maximum must be ≥ Allocation.');
    const need = mx.map((r,i)=>r.map((v,j)=>v-al[i][j]));
    const work = [...av], finished = al.map(()=>false), seq = [];
    let changed = true;
    while(changed) { changed=false; for(let i=0;i<al.length;i++) if(!finished[i]&&need[i].every((v,j)=>v<=work[j])) { work.forEach((_,j)=>work[j]+=al[i][j]); finished[i]=true;seq.push(i);changed=true; } }
    setBankerResult({ safe: finished.every(Boolean), seq, need });
    notify('Calculated in the browser preview. Verify results against the C Banker module for authoritative OS-project output.');
  }

  const activeItem = active === 'overview' ? { label: 'Overview', description: 'A clear view of the operating-systems lab and its modules.' } : implemented[active];
  const currentStatus = active === 'overview' ? 'Workspace snapshot' : activeItem?.status;
  const supported = ['jobs','scheduling','producer','banker'].includes(active);

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
      <header className="topbar"><div className="topbar-left"><button className="icon-button mobile-menu" aria-label="Open navigation" onClick={()=>setMobileOpen(true)}><Menu size={20}/></button><div className="breadcrumb">Workspace <span>/</span> <strong>{activeItem?.label || 'Overview'}</strong></div></div><div className="topbar-actions"><div className="connection-pill"><span className="muted-dot"/>Engine API not connected</div><button className="icon-button" title="Notifications" aria-label="Notifications" onClick={()=>notify('No notifications. The local engine/API is not connected yet.')}><Bell size={18}/></button><button className="help-button" onClick={()=>notify('Use each module screen to view its status, required inputs and the local run command.')}><CircleHelp size={17}/> Help</button></div></header>
      <div className="page-wrap">
        {notice&&<div className="notice" role="status"><span><BadgeCheck size={17}/></span>{notice}<button aria-label="Dismiss notice" onClick={()=>setNotice('')}><X size={15}/></button></div>}
        {active==='overview' ? <Overview setPage={setPage} modules={moduleList} /> :
          active==='jobs' ? <JobsPage jobs={jobs} setPage={setPage} form={jobForm} setForm={setJobForm} addJob={addJob} /> :
          active==='scheduling' ? <SchedulingPage jobs={jobs} policy={policy} setPolicy={setPolicy} quantum={quantum} setQuantum={setQuantum} schedule={schedule} runSchedule={runSchedule} /> :
          active==='banker' ? <BankerPage available={available} setAvailable={setAvailable} alloc={alloc} setAlloc={setAlloc} max={max} setMax={setMax} result={bankerResult} run={runBanker}/> :
          active==='producer' ? <ProducerPage count={pcCount} setCount={setPcCount} notify={notify}/> :
          active==='terminal' ? <TerminalPage notify={notify}/> :
          <UnavailablePage item={activeItem} />}
        <footer className="footer"><span>CampusCompute <span className="footer-sep">·</span> OS Laboratory</span><span><span className="footer-dot"/>Core C modules are developed independently</span></footer>
      </div>
    </main>
  </div>;
}

function Overview({ setPage, modules }) {
  const ready = modules.filter(m=>m.status==='Implemented').length;
  return <div className="page-enter">
    <div className="welcome-row"><div><div className="eyebrow">WEDNESDAY, OCTOBER 28, 2026 <span className="eyebrow-sep">/</span> OS LABORATORY</div><h1>System overview<span className="title-period">.</span></h1><p className="page-lead">Explore your operating-systems experiments from one workspace.</p></div><button className="button button-primary" onClick={()=>setPage('terminal')}><Terminal size={16}/> Open terminal guide <ArrowRight size={15}/></button></div>
    <div className="status-banner"><div className="status-banner-icon"><Activity size={19}/></div><div className="status-banner-copy"><strong>Local engine connection needed</strong><span>Module UIs are available; connect the C engine/API to execute authoritative runs from this dashboard.</span></div><Badge tone="warning">Not connected</Badge></div>
    <div className="stats-grid">
      <StatCard icon={Layers3} label="Jobs in preview" value="3" foot="Local example data · editable" tone="indigo" />
      <StatCard icon={Cpu} label="Scheduling policies" value="3" foot="FCFS, Round Robin, Priority" tone="teal" />
      <StatCard icon={Activity} label="Concurrency module" value="Ready" foot="C producer–consumer logger" tone="blue" />
      <StatCard icon={ShieldCheck} label="Resource safety" value="Ready" foot="Banker's Algorithm module" tone="purple" />
    </div>
    <div className="content-grid overview-grid">
      <section className="panel"><PanelTitle eyebrow="YOUR WORKSPACE" title="OS experiment modules" subtitle="Choose a module to inspect its status or run a local preview." action={<span className="small-count">{modules.length} modules</span>}/><div className="module-list">
        {modules.map(m=>{const Icon=m.icon;const available=m.status==='Implemented'||m.status==='CLI available';return <button key={m.id} className="module-row" onClick={()=>setPage(m.id)}><div className={`module-icon module-icon--${available?'ready':'muted'}`}><Icon size={18}/></div><div className="module-info"><strong>{m.label}</strong><span>{m.description}</span></div><Badge tone={m.status==='Implemented'?'success':m.status==='CLI available'?'info':'neutral'}>{m.status}</Badge><ArrowRight className="module-arrow" size={16}/></button>})}
      </div></section>
      <div className="right-stack"><section className="panel progress-panel"><PanelTitle eyebrow="IMPLEMENTATION" title="Project progress" subtitle="Tracked implementation status"/><div className="progress-placeholder"><div className="progress-ring"><span>—</span></div><div><strong>Not tracked yet</strong><p>No repository-backed project status feed is connected. No overall percentage is inferred.</p></div></div><div className="module-progress-list">{modules.map(m=><div className="module-progress" key={m.id}><span>{m.label}</span><Badge tone={m.status==='Implemented'?'success':m.status==='CLI available'?'info':'neutral'}>{m.status}</Badge></div>)}</div></section>
      <section className="tip-card"><div className="tip-icon"><Zap size={18}/></div><div><strong>Keep results trustworthy</strong><p>Preview calculations are labeled. For submission evidence, build and run the C engine in Ubuntu/WSL and capture real output.</p></div></section></div>
    </div>
  </div>;
}
function JobsPage({jobs,setPage,form,setForm,addJob}) {
  const set=(key,value)=>setForm(prev=>({...prev,[key]:value}));
  return <div className="page-enter"><PageHeading eyebrow="CORE OS / FR1" title="Jobs & Processes" subtitle="Create jobs, validate fields and inspect the process lifecycle data model."/><div className="stats-grid stats-grid--three"><StatCard icon={Layers3} label="Jobs in preview" value={jobs.length} foot="Stored in this browser session" tone="indigo"/><StatCard icon={Clock3} label="Total CPU burst" value={jobs.reduce((s,j)=>s+j.burst,0)} foot="Time units from entered jobs" tone="teal"/><StatCard icon={ListTodo} label="Lifecycle source" value="C engine" foot="Authoritative lifecycle uses FR1" tone="purple"/></div><div className="content-grid"><section className="panel"><PanelTitle eyebrow="JOB REGISTER" title="Job records" subtitle="Preview records are not persisted to the C engine."/><div className="table-scroll"><table><thead><tr><th>Job ID</th><th>Arrival</th><th>Burst</th><th>Priority</th><th>State</th></tr></thead><tbody>{jobs.map(j=><tr key={j.id}><td><span className="job-id">{j.id}</span></td><td>{j.arrival}</td><td>{j.burst}</td><td>{j.priority}</td><td><Badge tone="info">Preview</Badge></td></tr>)}</tbody></table></div></section><section className="panel"><PanelTitle eyebrow="NEW RECORD" title="Add a job" subtitle="Basic validation runs before the preview is updated."/><form className="form-stack" onSubmit={addJob}><Field label="Job ID"><input required maxLength="12" placeholder="e.g. P4" value={form.id} onChange={e=>set('id',e.target.value)}/></Field><div className="field-grid"><Field label="Arrival time"><input type="number" min="0" step="1" required value={form.arrival} onChange={e=>set('arrival',e.target.value)}/></Field><Field label="CPU burst"><input type="number" min="1" step="1" required value={form.burst} onChange={e=>set('burst',e.target.value)}/></Field></div><Field label="Priority (lower number = higher priority)"><input type="number" min="0" step="1" required value={form.priority} onChange={e=>set('priority',e.target.value)}/></Field><button className="button button-primary button-full" type="submit"><span>+</span> Add to preview</button></form><div className="info-note"><CircleHelp size={15}/> Use <code>make -C engine/src/job_manager clean test</code> to test the actual FR1 C module.</div></section></div><section className="panel lifecycle-panel"><PanelTitle eyebrow="PROCESS LIFECYCLE" title="Supported transition model"/><div className="lifecycle"><div className="life-state"><span>01</span><strong>CREATED</strong><small>Job validated</small></div><ArrowRight/><div className="life-state"><span>02</span><strong>READY</strong><small>Waiting for CPU</small></div><ArrowRight/><div className="life-state"><span>03</span><strong>RUNNING</strong><small>Being scheduled</small></div><ArrowRight/><div className="life-state life-state--done"><span>04</span><strong>COMPLETED</strong><small>Execution finished</small></div></div></section></div>;
}
function SchedulingPage({jobs,policy,setPolicy,quantum,setQuantum,schedule,runSchedule}) {
  return <div className="page-enter"><PageHeading eyebrow="CORE OS / FR2" title="CPU Scheduling" subtitle="Compare the three supported policies with the same job set."/><div className="stats-grid stats-grid--three"><StatCard icon={Cpu} label="Available policies" value="3" foot="FCFS · Round Robin · Priority" tone="indigo"/><StatCard icon={Layers3} label="Input jobs" value={jobs.length} foot="Browser preview job set" tone="teal"/><StatCard icon={Clock3} label="Quantum" value={policy==='Round Robin'?quantum:'—'} foot="Used only for Round Robin" tone="purple"/></div><div className="content-grid"><section className="panel"><PanelTitle eyebrow="RUN CONFIGURATION" title="Scheduler controls" subtitle="Calculation runs in this UI preview, not the C engine."/><div className="form-stack"><Field label="Scheduling policy"><select value={policy} onChange={e=>setPolicy(e.target.value)}><option>FCFS</option><option>Round Robin</option><option>Priority</option></select></Field>{policy==='Round Robin'&&<Field label="Time quantum"><input type="number" min="1" step="1" value={quantum} onChange={e=>setQuantum(e.target.value)}/></Field>}<div className="policy-help"><Badge tone="info">{policy}</Badge><p>{policy==='FCFS'?'Jobs execute in arrival order.':policy==='Round Robin'?'Ready jobs receive a fixed time slice in queue order.':'The lowest numeric priority is selected first, without preemption.'}</p></div><button className="button button-primary button-full" onClick={runSchedule}><Play size={16}/> Calculate preview</button></div></section><section className="panel"><PanelTitle eyebrow="INPUT JOB SET" title="Current jobs" subtitle="Edit these on Jobs & Processes."/><div className="table-scroll"><table><thead><tr><th>Job</th><th>Arrival</th><th>Burst</th><th>Priority</th></tr></thead><tbody>{jobs.map(j=><tr key={j.id}><td><span className="job-id">{j.id}</span></td><td>{j.arrival}</td><td>{j.burst}</td><td>{j.priority}</td></tr>)}</tbody></table></div><button className="button button-secondary button-full margin-top" onClick={()=>window.dispatchEvent(new CustomEvent('navigate-jobs'))}>Job set is editable on the Jobs screen <ArrowRight size={15}/></button></section></div>{schedule?<section className="panel result-panel"><PanelTitle eyebrow="PREVIEW RESULT" title={schedule.policy+" result"} subtitle="Calculated from the current browser-side job set." action={<Badge tone="warning">Preview only</Badge>}/><div className="mini-stats"><div><span>Average waiting</span><strong>{schedule.avgWaiting.toFixed(2)}</strong></div><div><span>Average turnaround</span><strong>{schedule.avgTurnaround.toFixed(2)}</strong></div><div><span>Makespan</span><strong>{Math.max(...schedule.rows.map(r=>r.completion))}</strong></div></div><div className="gantt"><div className="gantt-label">Execution timeline</div><div className="gantt-track">{schedule.timeline.map((t,i)=><div key={i} className={`gantt-piece gantt-piece--${i%5}`} style={{flexGrow:t.end-t.start,flexBasis:0}} title={`${t.id}: ${t.start}–${t.end}`}><strong>{t.id}</strong><small>{t.start}–{t.end}</small></div>)}</div></div><div className="table-scroll"><table><thead><tr><th>Job</th><th>Completion</th><th>Turnaround</th><th>Waiting</th></tr></thead><tbody>{schedule.rows.map(j=><tr key={j.id}><td><span className="job-id">{j.id}</span></td><td>{j.completion}</td><td>{j.turnaround}</td><td>{j.waiting}</td></tr>)}</tbody></table></div><p className="footnote">For verified results and assessment evidence, run <code>make -C engine/src/scheduler clean test</code> then <code>./engine/src/scheduler/cpu_scheduler</code> in Linux/Ubuntu.</p></section>:<section className="panel"><EmptyState icon={BarChart3} title="No scheduling run yet" text="Choose a policy and calculate a preview to see execution intervals and timing metrics."/></section>}</div>;
}
function BankerPage({available,setAvailable,alloc,setAlloc,max,setMax,result,run}) {
  return <div className="page-enter"><PageHeading eyebrow="RESOURCE MANAGEMENT / FR5" title="Banker's Algorithm" subtitle="Enter resource matrices and inspect Need, safety status and a candidate safe sequence."/><div className="status-banner status-banner--teal"><div className="status-banner-icon"><ShieldCheck size={19}/></div><div className="status-banner-copy"><strong>FR5 C module is implemented</strong><span>The screen includes a browser-side calculation for exploration. Validate the output with the compiled C module before using it as evidence.</span></div><Badge tone="success">C module ready</Badge></div><section className="panel banker-form"><PanelTitle eyebrow="RESOURCE STATE" title="Allocation inputs" subtitle="Use spaces between values and semicolons between process rows."/><form onSubmit={run}><Field label="Available vector"><input value={available} onChange={e=>setAvailable(e.target.value)} aria-label="Available vector" required/></Field><div className="field-grid"><Field label="Allocation matrix"><textarea rows="6" value={alloc} onChange={e=>setAlloc(e.target.value)} required/></Field><Field label="Maximum matrix"><textarea rows="6" value={max} onChange={e=>setMax(e.target.value)} required/></Field></div><div className="form-actions"><button type="button" className="button button-secondary" onClick={()=>{setAvailable('3 3 2');setAlloc('0 1 0; 2 0 0; 3 0 2; 2 1 1; 0 0 2');setMax('7 5 3; 3 2 2; 9 0 2; 2 2 2; 4 3 3');}}>Restore sample</button><button className="button button-primary" type="submit"><ShieldCheck size={16}/> Check safety preview</button></div></form></section>{result?<section className="panel result-panel"><PanelTitle eyebrow="SAFETY CHECK" title={result.safe?'System is in a SAFE state':'System is in an UNSAFE state'} action={<Badge tone={result.safe?'success':'danger'}>{result.safe?'Safe':'Unsafe'}</Badge>}/>{result.safe?<div className="safe-sequence">{result.seq.map((p,i)=><span key={p} className="sequence-step"><span>P{p}</span>{i<result.seq.length-1&&<ArrowRight size={15}/>}</span>)}</div>:<p className="result-explanation">No complete safe sequence was found for this resource state.</p>}<h3 className="subheading">Need = Maximum − Allocation</h3><div className="table-scroll"><table><thead><tr><th>Process</th>{result.need[0].map((_,j)=><th key={j}>R{j}</th>)}</tr></thead><tbody>{result.need.map((row,i)=><tr key={i}><td>P{i}</td>{row.map((v,j)=><td key={j}>{v}</td>)}</tr>)}</tbody></table></div><p className="footnote">Preview calculation only. Validate with <code>make -C engine/src/banker clean test</code> and <code>./engine/src/banker/banker</code>.</p></section>:<section className="panel"><EmptyState icon={ShieldCheck} title="No safety check yet" text="Submit the matrices to calculate Need and determine whether a safe sequence exists."/></section>}</div>;
}
function ProducerPage({count,setCount,notify}) {
  return <div className="page-enter"><PageHeading eyebrow="CONCURRENCY / FR3" title="Producer–Consumer" subtitle="Inspect the bounded queue logger and run it locally in Linux/Ubuntu."/><div className="status-banner status-banner--teal"><div className="status-banner-icon"><Activity size={19}/></div><div className="status-banner-copy"><strong>POSIX producer–consumer module is implemented</strong><span>The C module uses a bounded queue, mutex, condition variables, orderly shutdown and a TSV event log.</span></div><Badge tone="success">C module ready</Badge></div><div className="content-grid"><section className="panel"><PanelTitle eyebrow="LOCAL RUN CONFIGURATION" title="Event count" subtitle="This dashboard cannot start local processes while the API is disconnected."/><div className="form-stack"><Field label="Number of events (1–100000)"><input type="number" min="1" max="100000" step="1" value={count} onChange={e=>setCount(e.target.value)}/></Field><button className="button button-primary button-full" onClick={()=>{if(!Number.isInteger(Number(count))||Number(count)<1||Number(count)>100000) notify('Event count must be an integer from 1 through 100000.');else notify(`Local run not started. Execute: ./engine/src/producer_consumer/producer_consumer ${count} logs/activity.log`);}}><Play size={16}/> Show local run command</button></div><div className="info-note"><CircleHelp size={15}/> Create the log directory first: <code>mkdir -p logs</code>.</div></section><section className="panel"><PanelTitle eyebrow="OBSERVABILITY" title="Runtime log" subtitle="No live log stream is connected."/><EmptyState icon={FileText} title="No runtime events loaded" text="Run the C program locally, then inspect the generated tab-separated activity log."/></section></div><section className="panel"><PanelTitle eyebrow="BUILD & TEST" title="Run the real module"/><CodeLine>make -C engine/src/producer_consumer clean test</CodeLine><CodeLine>mkdir -p logs</CodeLine><CodeLine>./engine/src/producer_consumer/producer_consumer {count} logs/activity.log</CodeLine></section></div>;
}
function TerminalPage({notify}) {
  const commands=['make -C engine/src/job_manager clean test','make -C engine/src/scheduler clean test','make -C engine/src/producer_consumer clean test','make -C engine/src/banker clean test'];
  return <div className="page-enter"><PageHeading eyebrow="WORKSPACE / CLI" title="Terminal guide" subtitle="Use your Ubuntu/WSL terminal to compile and run the actual C programs."/><div className="status-banner"><div className="status-banner-icon"><Terminal size={19}/></div><div className="status-banner-copy"><strong>Browser terminal execution is disabled</strong><span>This guide copies commands; it does not run shell commands inside the browser or server.</span></div><Badge tone="warning">Local terminal</Badge></div><section className="panel"><PanelTitle eyebrow="REPOSITORY ROOT" title="Build and test modules" subtitle="Run these from the CampusCompute repository directory."/><div className="command-list">{commands.map(cmd=><CodeLine key={cmd} copy={() => {navigator.clipboard?.writeText(cmd);notify('Command copied when clipboard permission is available.');}}>{cmd}</CodeLine>)}</div><div className="info-note"><AlertTriangle size={15}/> FR4, FR6, FR7 and the integrated FR8 CLI are not yet implemented. Their commands are intentionally not listed.</div></section><section className="panel"><PanelTitle eyebrow="NEXT STEPS" title="Use a real Linux terminal"/><ol className="terminal-steps"><li><span>1</span><div><strong>Open Ubuntu / WSL</strong><p>Navigate to the cloned repository directory.</p></div></li><li><span>2</span><div><strong>Pull the latest main</strong><p>Fetch the reviewed modules from GitHub before building.</p><CodeLine>git pull origin main</CodeLine></div></li><li><span>3</span><div><strong>Capture genuine evidence</strong><p>Use actual compiler and program output for screenshots; do not use the dashboard previews as runtime proof.</p></div></li></ol></section></div>;
}
function UnavailablePage({item}) {
  const Icon=item?.icon||LockKeyhole;
  return <div className="page-enter"><PageHeading eyebrow={item?.group?.toUpperCase()||'MODULE'} title={item?.label||'Module'} subtitle={item?.description||'Module details are not available.'}/><section className="panel unavailable-panel"><div className="unavailable-illustration"><div className="unavailable-orbit unavailable-orbit--one"/><div className="unavailable-orbit unavailable-orbit--two"/><div className="unavailable-center"><Icon size={30}/></div></div><Badge tone="neutral">Not implemented</Badge><h2>This module is not available yet</h2><p>The interface is reserved for this OS requirement. No runtime result, status or completion percentage is fabricated.</p><div className="unavailable-checklist"><div><Check size={15}/> Navigation is available</div><div><Check size={15}/> Requirement status is explicit</div><div><Clock3 size={15}/> Engine work pending</div></div></section></div>;
}
function PageHeading({eyebrow,title,subtitle}) { return <div className="page-heading"><div className="eyebrow">{eyebrow}</div><h1>{title}<span className="title-period">.</span></h1><p className="page-lead">{subtitle}</p></div>; }
function Field({label,children}) { return <label className="field"><span>{label}</span>{children}</label>; }
function CodeLine({children,copy}) { return <div className="code-line"><code>{children}</code><button aria-label="Copy command" title="Copy command" onClick={()=>{if(copy)copy();else{navigator.clipboard?.writeText(String(children));}}}><FileText size={15}/></button></div>; }

window.addEventListener('navigate-jobs',()=>window.dispatchEvent(new CustomEvent('campuscompute-navigate',{detail:'jobs'})));
