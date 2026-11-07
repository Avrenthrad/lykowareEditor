"""Checked existing-file saves, byte observations, backups and explicit conflicts."""
import json,pathlib,subprocess,sys,tempfile,os
exe=sys.argv[1]
def req(command,**arguments):return {'instance':'test','command':command,'arguments':arguments}
def run(commands,mcp=False):
 messages=commands
 if mcp:
  messages=[{'jsonrpc':'2.0','id':0,'method':'initialize','params':{'protocolVersion':'2025-06-18','capabilities':{},'clientInfo':{'name':'project-test','version':'1'}}},{'jsonrpc':'2.0','method':'notifications/initialized'}]+[{'jsonrpc':'2.0','id':i,'method':'tools/call','params':{'name':c['command'],'arguments':{'instance':'test','arguments':c['arguments']}}} for i,c in enumerate(commands,1)]
 p=subprocess.run([exe,'--instance','test']+(['--mcp'] if mcp else []),input='\n'.join(map(json.dumps,messages))+'\n',text=True,capture_output=True,timeout=20)
 assert p.returncode in (0,1),p.stderr
 rows=[json.loads(line) for line in p.stdout.splitlines()]
 return [row['result']['structuredContent'] for row in rows[1:]] if mcp else rows
import hashlib,stat,concurrent.futures
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp);path=root/'scene.json'
 original=b'{"schema_version":1,"next_id":2,"entities":[{"id":1,"name":"Original"}]}  \n'
 path.write_bytes(original)
 if os.name!='nt':path.chmod(0o600)
 token=hashlib.sha256(original).hexdigest()
 r=run([req('scene.load',path=str(path)),req('document.status'),req('entity.rename',id=1,name='Edited'),req('scene.save_checked',path=str(path)),req('document.status'),req('history.undo'),req('document.status')]);assert all(x['ok'] for x in r),r
 assert r[1]['data']['documents']['scene']['source_sha256']==token and pathlib.Path(r[3]['data']['backup']).read_bytes()==original
 assert r[3]['data']['sha256']==hashlib.sha256(path.read_bytes()).hexdigest() and not r[4]['data']['documents']['scene']['dirty'] and r[6]['data']['documents']['scene']['dirty']
 if os.name!='nt':assert stat.S_IMODE(path.stat().st_mode)==0o600 and stat.S_IMODE(pathlib.Path(r[3]['data']['backup']).stat().st_mode)==0o600
 # Competing clients never blindly replace an intervening generation.
 race=root/'race.json';race.write_bytes(original)
 def contender(index):return run([req('scene.load',path=str(race)),req('entity.rename',id=1,name=f'Winner {index}'),req('scene.save_checked',path=str(race),sha256=token)])[-1]
 with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:results=list(pool.map(contender,range(2)))
 assert sum(x['ok'] for x in results)==1 and next(x for x in results if not x['ok'])['error']['code'] in ('save_busy','external_change_conflict'),results
 # External modifications after load retain dirty state and disk contents on conflict.
 path.write_bytes(original)
 p=subprocess.Popen([exe,'--instance','test'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
 def live(request):
  p.stdin.write(json.dumps(request)+'\n');p.stdin.flush();return json.loads(p.stdout.readline())
 assert live(req('scene.load',path=str(path)))['ok'];assert live(req('entity.rename',id=1,name='Local'))['ok']
 changed=original+b' ';path.write_bytes(changed)
 before=live(req('editor.snapshot'));conflict=live(req('scene.save_checked',path=str(path)));after=live(req('editor.snapshot'))
 assert conflict['error']['code']=='external_change_conflict' and before==after and path.read_bytes()==changed
 inspected=live(req('file.inspect',path=str(path)));assert inspected['data']['sha256']==hashlib.sha256(changed).hexdigest()
 saved=live(req('scene.save_checked',path=str(path),sha256=inspected['data']['sha256']));assert saved['ok'] and pathlib.Path(saved['data']['backup']).read_bytes()==changed
 p.stdin.close();assert p.wait(timeout=10)==1
 # Missing baselines do not grant overwrite authority; explicit inspected tokens do.
 r=run([req('scene.save_checked',path=str(path))]);assert r[-1]['error']['code']=='no_save_baseline'
 before=path.read_bytes();r=run([req('scene.save_checked',path=str(path),sha256='bad')]);assert r[-1]['error']['code']=='invalid_fingerprint' and path.read_bytes()==before
 lock=root/'.lyko-write-lock';lock.mkdir();r=run([req('scene.save_checked',path=str(path),sha256=hashlib.sha256(before).hexdigest())]);assert r[-1]['error']['code']=='save_busy' and lock.is_dir() and path.read_bytes()==before;lock.rmdir()
 # Every supported document writer gets the same checked replacement behavior.
 for kind,save,load,checked,mutate in [('catalogue','asset.catalog.save','asset.catalog.load','asset.catalog.save_checked',req('asset.register',path='x.json',kind='scene')),('workspace','workspace.save','workspace.load','workspace.save_checked',req('workspace.configure',scale_percent=150,theme='dark',reduced_motion=False)),('recents','project.recent.save','project.recent.load','project.recent.save_checked',None)]:
  target=root/(kind+'.txt');assert run([req(save,path=str(target))])[-1]['ok'];before=target.read_bytes()
  requests=[req(load,path=str(target))]+([mutate] if mutate else [])+[req(checked,path=str(target)),req('document.status')]
  r=run(requests);assert all(x['ok'] for x in r),r
  assert pathlib.Path(r[-2]['data']['backup']).read_bytes()==before and not r[-1]['data']['documents'][kind]['dirty']
 project=root/'lykoware.toml';project.write_text('schema_version = 1\nproject_id = "0123456789abcdef0123456789abcdef"\nname = "Original"\n');before=project.read_bytes()
 r=run([req('project.open',path=str(project)),req('project.configure',name='Saved settings',directories={}),req('project.settings.save'),req('document.status'),req('project.close')]);assert all(x['ok'] for x in r),r
 assert pathlib.Path(r[2]['data']['backup']).read_bytes()==before and not r[3]['data']['documents']['project']['dirty']
 # A failed revision check cannot touch disk; ordinary save still preserves existing files.
 before=path.read_bytes();r=run([{**req('scene.save_checked',path=str(path),sha256=hashlib.sha256(before).hexdigest()),'expected_revision':999},req('scene.save',path=str(path))]);assert all(not x['ok'] for x in r) and path.read_bytes()==before
 assert not (root/'.lyko-write-lock').exists()
print('Passed checked saves, exact observations, backups/modes, cooperating races, external conflicts, explicit review and project settings')
