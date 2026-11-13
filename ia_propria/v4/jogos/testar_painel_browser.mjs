// Optional integration check using an installed Chrome, no npm dependencies.
import {spawn} from 'node:child_process';
import {mkdtempSync,readFileSync,existsSync,rmSync,writeFileSync,mkdirSync} from 'node:fs';
import {tmpdir} from 'node:os';
import {join,dirname,resolve} from 'node:path';
import {fileURLToPath,pathToFileURL} from 'node:url';
import assert from 'node:assert/strict';
const here=dirname(fileURLToPath(import.meta.url));
const html=resolve(process.argv[2]||join(here,'painel_progresso.html'));
const profile=mkdtempSync(join(tmpdir(),'ghost-panel-browser-'));
const browser=spawn(process.env.GHOST_CHROME||'/usr/bin/google-chrome',[
 '--headless=new','--no-sandbox','--disable-gpu','--disable-background-networking',
 '--no-first-run','--no-default-browser-check','--remote-debugging-address=127.0.0.1','--remote-debugging-port=0',
 '--user-data-dir='+profile,'about:blank'],{stdio:'ignore'});
let socket;const pending=new Map();let sequence=0;const exceptions=[];
const sleep=ms=>new Promise(r=>setTimeout(r,ms));
function send(method,params={}){return new Promise((resolve,reject)=>{const id=++sequence;const timer=setTimeout(()=>{pending.delete(id);reject(new Error('CDP timeout: '+method));},10000);pending.set(id,{resolve,reject,timer});socket.send(JSON.stringify({id,method,params}));});}
async function evaluate(expression){const r=await send('Runtime.evaluate',{expression,returnByValue:true,awaitPromise:true});if(r.exceptionDetails)throw new Error(JSON.stringify(r.exceptionDetails));return r.result.value;}
async function select(id,value){await evaluate(`document.getElementById(${JSON.stringify(id)}).value=${JSON.stringify(value)};document.getElementById(${JSON.stringify(id)}).dispatchEvent(new Event('change'))`);}
try{
 for(let i=0;i<200&&!existsSync(join(profile,'DevToolsActivePort'));i++)await sleep(100);
 const port=readFileSync(join(profile,'DevToolsActivePort'),'utf8').split('\n')[0];
 const pages=await (await fetch(`http://127.0.0.1:${port}/json/list`)).json();
 socket=new WebSocket(pages.find(p=>p.type==='page').webSocketDebuggerUrl);
 await new Promise((res,rej)=>{socket.addEventListener('open',res,{once:true});socket.addEventListener('error',rej,{once:true});});
 socket.addEventListener('message',event=>{const r=JSON.parse(event.data);if(r.method==='Runtime.exceptionThrown')exceptions.push(r.params);const task=pending.get(r.id);if(task){clearTimeout(task.timer);pending.delete(r.id);r.error?task.reject(new Error(JSON.stringify(r.error))):task.resolve(r.result);}});
 await send('Runtime.enable');await send('Page.enable');
 await send('Emulation.setDeviceMetricsOverride',{width:1280,height:1000,deviceScaleFactor:1,mobile:false});
 await send('Page.navigate',{url:pathToFileURL(html).href});
 for(let i=0;i<100;i++){if(await evaluate("document.readyState==='complete' && !!document.getElementById('afterScore')?.textContent"))break;await sleep(100);}
 assert.match(await evaluate("document.getElementById('afterScore').textContent"),/^\d+\/\d+$/);
 for(const partition of ['teste','teste_formulacao']){
  await select('partition',partition);
  const scores=await evaluate(`({before:document.getElementById('beforeScore').textContent,after:document.getElementById('afterScore').textContent,
   sourceBefore:DATA.reports[Number(document.getElementById('before').value)].rows,sourceAfter:DATA.reports[Number(document.getElementById('after').value)].rows})`);
  assert.equal(scores.before,scores.sourceBefore.filter(r=>r.correct).length+'/'+scores.sourceBefore.length);
  assert.equal(scores.after,scores.sourceAfter.filter(r=>r.correct).length+'/'+scores.sourceAfter.length);
 }
 await select('level','1');await select('game','memoria');await select('status','wrong');
 const tags=await evaluate("[...document.querySelectorAll('.example .tag')].map(e=>e.textContent)");
 assert(tags.length>0);assert(tags.every(x=>x.includes('Memória')&&x.includes('nível 1')&&x.includes('Errou ambos')));
 await evaluate("document.getElementById('search').value='sem_resultado_837292';document.getElementById('search').dispatchEvent(new Event('input'))");
 assert.equal(await evaluate("document.querySelectorAll('.example').length"),0);
 await evaluate("document.getElementById('search').value='';document.getElementById('search').dispatchEvent(new Event('input'))");
 const same=await evaluate("document.getElementById('after').value");await select('before',same);
 assert.match(await evaluate("document.getElementById('verdictTitle').textContent"),/Mesmos pesos/);
 await select('partition','teste');await select('level','all');await select('game','all');await select('status','all');
 await evaluate("document.querySelector('#more').click()");
 assert.equal(await evaluate("document.querySelectorAll('.example').length"),24);
 const screenshots=join(here,'relatorios');mkdirSync(screenshots,{recursive:true});
 const desktop=await send('Page.captureScreenshot',{format:'png'});writeFileSync(join(screenshots,'painel_desktop.png'),Buffer.from(desktop.data,'base64'));
 await send('Emulation.setDeviceMetricsOverride',{width:390,height:844,deviceScaleFactor:1,mobile:true});
 assert(await evaluate('document.documentElement.scrollWidth<=390'));
 const mobile=await send('Page.captureScreenshot',{format:'png'});writeFileSync(join(screenshots,'painel_mobile.png'),Buffer.from(mobile.data,'base64'));
 assert.deepEqual(exceptions,[]);
 console.log('OK: contagens, partições, níveis, exemplos, busca, pesos iguais, paginação e layout móvel.');
}finally{
 if(socket)socket.close();browser.kill('SIGTERM');
 await Promise.race([new Promise(r=>browser.once('exit',r)),sleep(2000)]);
 rmSync(profile,{recursive:true,force:true,maxRetries:5,retryDelay:100});
}
