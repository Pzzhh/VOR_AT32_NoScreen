const $ = (id) => document.getElementById(id);
let snapshot = null;
let logs = [];
let socket = null;

async function api(path, options = {}) {
  const response = await fetch(path, {headers:{"Content-Type":"application/json"}, ...options});
  const body = await response.json();
  if (!response.ok || !body.ok) throw new Error(body.message || body.code || `HTTP ${response.status}`);
  return body.data;
}
function toast(message) { const el=$("toast"); el.textContent=message; el.classList.add("show"); setTimeout(()=>el.classList.remove("show"),2500); }
async function action(path, body) { try { await api(path,{method:"POST",body:body===undefined?undefined:JSON.stringify(body)}); } catch(e) { toast(e.message); } }

function render(data) {
  snapshot = data;
  const serial=data.serial, run=data.run, connected=serial.connected;
  $("connectionDot").classList.toggle("online",connected);
  $("connectionText").textContent=connected?"USB已连接":"USB未连接";
  $("portText").textContent=serial.port || "未配置端口";
  $("connectButton").textContent=connected?"断开":"连接";
  $("taskCount").textContent=`${run.task_count} / 20`;
  $("runState").textContent=run.state;
  $("runMessage").textContent=run.message;
  const task=run.current_task;
  $("currentName").textContent=task?task.name:"等待任务";
  $("currentParams").textContent=task?`C=${task.count} · V=${task.velocity} · F=${task.frequency} · E=${task.ex_mode}`:"添加VOR任务后即可运行";
  const progress=task?task.progress:0;
  $("progressBar").style.width=`${progress}%`; $("progressText").textContent=`${progress}%`; $("remainingText").textContent=`剩余 ${task?task.remaining:0}`;
  $("encoderAngle").textContent=Number.isFinite(run.encoder_angle)?run.encoder_angle.toFixed(3):"--";
  renderTasks(run);
  const state=run.state;
  $("startButton").disabled=!connected || !run.task_count || !["IDLE","COMPLETED"].includes(state);
  $("pauseButton").disabled=state!=="RUNNING";
  $("resumeButton").disabled=state!=="PAUSED";
  $("stopButton").disabled=["IDLE","COMPLETED"].includes(state);
  $("retryButton").disabled=state!=="FAULT"; $("skipButton").disabled=state!=="FAULT";
  $("newTask").disabled=!["IDLE","COMPLETED"].includes(state) || run.task_count>=20;
}
function renderTasks(run) {
  const root=$("taskList");
  if(!run.tasks.length){root.innerHTML='<div class="empty">尚未添加任务</div>';return;}
  root.innerHTML=run.tasks.map((t,i)=>`<div class="task-card ${run.current_index===i?'active':''}">
    <div class="task-index">${String(i+1).padStart(2,'0')}</div><div><div class="task-name">${escapeHtml(t.name)}</div><div class="task-params">C ${t.count} · V ${t.velocity} · F ${t.frequency} · E ${t.ex_mode}</div><div class="task-state">${t.state}${t.error?` · ${escapeHtml(t.error)}`:''}</div></div>
    <div class="task-actions"><button onclick="moveTask(${i},-1)" ${i===0?'disabled':''}>↑</button><button onclick="moveTask(${i},1)" ${i===run.tasks.length-1?'disabled':''}>↓</button><button onclick="editTask(${t.id})">编辑</button><button onclick="deleteTask(${t.id})">删</button></div></div>`).join('');
}
function escapeHtml(value){const div=document.createElement('div');div.textContent=value;return div.innerHTML;}
function appendLogs(items){if(!items.length)return;logs.push(...items);if(logs.length>500)logs=logs.slice(-500);const root=$("logList");root.innerHTML=logs.map(x=>`<div class="log-row ${x.kind}"><span class="log-time">${x.timestamp.slice(11)}</span><span class="log-kind">${x.kind}</span><span>${escapeHtml(x.raw)}</span></div>`).join('');root.scrollTop=root.scrollHeight;}
function connectWs(){socket=new WebSocket(`${location.protocol==='https:'?'wss':'ws'}://${location.host}/ws/events`);socket.onmessage=(e)=>{const msg=JSON.parse(e.data);render(msg.data);appendLogs(msg.logs||[])};socket.onclose=()=>setTimeout(connectWs,1000);}
function openDialog(task){$("taskForm").reset();$("taskId").value=task?task.id:"";$("dialogTitle").textContent=task?"编辑 VOR 任务":"添加 VOR 任务";if(task){$("name").value=task.name;$("count").value=task.count;$("velocity").value=task.velocity;$("frequency").value=task.frequency;$("exMode").value=task.ex_mode;}$("taskDialog").showModal();}
window.editTask=(id)=>openDialog(snapshot.run.tasks.find(t=>t.id===id));
window.deleteTask=async(id)=>{try{await api(`/api/tasks/${id}`,{method:"DELETE"});}catch(e){toast(e.message)}};
window.moveTask=async(index,delta)=>{const ids=snapshot.run.tasks.map(t=>t.id),j=index+delta;[ids[index],ids[j]]=[ids[j],ids[index]];await action('/api/tasks/reorder',{ids});};
$("newTask").onclick=()=>openDialog(null);$("closeDialog").onclick=$("cancelDialog").onclick=()=>$("taskDialog").close();
$("taskForm").onsubmit=async(e)=>{e.preventDefault();const id=$("taskId").value,payload={name:$("name").value,count:Number($("count").value),velocity:Number($("velocity").value),frequency:Number($("frequency").value),ex_mode:Number($("exMode").value)};try{await api(id?`/api/tasks/${id}`:'/api/tasks',{method:id?'PUT':'POST',body:JSON.stringify(payload)});$("taskDialog").close();}catch(err){toast(err.message)}};
$("connectButton").onclick=()=>action(snapshot&&snapshot.serial.connected?'/api/serial/disconnect':'/api/serial/connect');
$("startButton").onclick=()=>action('/api/run/start',{start_index:0});$("pauseButton").onclick=()=>action('/api/run/pause');$("resumeButton").onclick=()=>action('/api/run/resume');$("stopButton").onclick=()=>action('/api/run/stop');$("retryButton").onclick=()=>action('/api/run/retry');$("skipButton").onclick=()=>action('/api/run/skip');$("clearLogs").onclick=()=>{logs=[];$("logList").innerHTML='';};
api('/api/status').then(render).catch(e=>toast(e.message));api('/api/logs?limit=200').then(data=>appendLogs(data.items)).catch(()=>{});connectWs();
