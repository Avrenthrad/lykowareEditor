"""Persistent user-local recent-project metadata without implicit activation."""
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
import copy
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp);paths=[]
 for i in range(2):
  folder=root/f'project-{i}';folder.mkdir();path=folder/'lykoware.toml';path.write_text(f'schema_version = 1\nproject_id = "{i:032x}"\nname = "Studio {i} 🐺"\n',encoding='utf8');paths.append(path)
 store=root/'recent-projects.json'
 base=[req('project.open',path=str(p)) for p in paths]
 commands=base+[req('editor.snapshot'),req('project.recent.save',path=str(store)),req('editor.snapshot'),req('project.recent.clear'),req('project.recent.load',path=str(store)),req('project.query'),req('history.undo'),req('project.query')]
 r=run(commands);assert all(x['ok'] for x in r) and r[2]==r[4],r
 saved=json.loads(store.read_text());assert saved['schema_version']==1 and saved['recent']==r[7]['data']['recent'] and r[9]['data']['recent']==[]
 original=store.read_bytes();store.unlink();assert r==run(commands,True) and store.read_bytes()==original,'CLI/MCP persistence parity'
 assert not run([req('project.recent.save',path=str(store))])[-1]['ok'] and store.read_bytes()==original,'existing store preservation'
 # Read in a separate process, retaining untitled authored work and active state.
 commands=[req('entity.create',name='Untitled'),req('project.recent.load',path=str(store)),req('editor.snapshot')]
 r=run(commands);assert all(x['ok'] for x in r) and r[-1]['data']['project']['active'] is None and r[-1]['data']['scene']['entities']==[{'id':1,'name':'Untitled'}]
 assert r[-1]['data']['project']['recent']==saved['recent']
 # Stale recent metadata loads without opening; actual Open checks the file again.
 paths[0].unlink()
 r=run([req('project.recent.load',path=str(store)),req('editor.snapshot'),req('project.open',path=str(paths[0])),req('editor.snapshot')]);assert r[0]['ok'] and not r[2]['ok'] and r[1]==r[3]
 invalid=[]
 def changed(edit):
  value=copy.deepcopy(saved);edit(value);invalid.append(value)
 changed(lambda x:x.update(schema_version=99));changed(lambda x:x.update(extra=True))
 changed(lambda x:x['recent'].append(x['recent'][0]));changed(lambda x:x.update(recent=x['recent']*9))
 changed(lambda x:x['recent'][0].update(path='relative.toml'))
 changed(lambda x:x['recent'][0].update(root='/wrong-root'))
 changed(lambda x:x['recent'][0]['manifest'].update(name=''))
 changed(lambda x:x['recent'][0]['manifest'].update(schema_version=2))
 changed(lambda x:x['recent'][0]['manifest']['directories'].update(cache='assets'))
 changed(lambda x:x['recent'][0]['manifest']['directories'].pop('graphs'))
 for value in invalid:
  bad=root/'bad.json';bad.write_text(json.dumps(value))
  r=run([req('project.open',path=str(paths[1])),req('editor.snapshot'),req('project.recent.load',path=str(bad)),req('editor.snapshot')]);assert r[-2]['error']['code']=='invalid_project_recents' and r[-3]==r[-1],r
 for content in (b'not json',b'\xff',b'x'*(1024*1024+1)):
  bad=root/'bad.json';bad.write_bytes(content);assert not run([req('project.recent.load',path=str(bad))])[-1]['ok']
 for path in (root,root/'missing.json'):
  assert run([req('project.recent.load',path=str(path))])[-1]['error']['field']=='path'
 if os.name!='nt':
  os.mkfifo(root/'pipe');assert not run([req('project.recent.load',path=str(root/'pipe'))])[-1]['ok']
 # Import is one atomic undoable mutation, and does not open or consult its stored roots.
 batch=[{'command':'entity.create','arguments':{'name':'Atomic'}},{'command':'project.recent.load','arguments':{'path':str(store)}}]
 r=run([req('transaction.execute',commands=batch),req('editor.snapshot'),req('history.undo'),req('editor.snapshot')]);assert all(x['ok'] for x in r) and r[1]['data']['project']['recent']==saved['recent'] and not r[-1]['data']['project']['recent'] and not r[-1]['data']['scene']['entities']
 batch[-1]['arguments']['path']=str(root/'bad.json')
 r=run([req('editor.snapshot'),req('transaction.execute',commands=batch),req('editor.snapshot')]);assert not r[1]['ok'] and r[0]==r[2]
 empty=root/'empty.json';r=run([req('project.recent.save',path=str(empty))]);assert r[-1]['ok'] and json.loads(empty.read_text())==dict(schema_version=1,recent=[])
 for name in ('project.recent.load','project.recent.save'):
  bad={**req(name,path=str(root/'never.json')),'expected_revision':99};assert not run([bad])[-1]['ok']
 assert not (root/'never.json').exists()
 # Redo freezes imported metadata, independent of its store file.
 p=subprocess.Popen([exe,'--instance','test'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
 def live(request):
  p.stdin.write(json.dumps(request)+'\n');p.stdin.flush();return json.loads(p.stdout.readline())
 assert live(req('project.recent.load',path=str(store)))['ok'];assert live(req('history.undo'))['ok'];store.unlink();assert live(req('history.redo'))['ok'];assert live(req('project.query'))['data']['recent']==saved['recent']
 p.stdin.close();assert p.wait(timeout=10)==0
print('Passed persistent recent metadata, separate-session import, no activation, stale paths, validation, atomic history and CLI/MCP parity')
