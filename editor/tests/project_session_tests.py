"""Undoable project context, conservative authoring guards and frozen history."""
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
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp);paths=[]
 for i in range(18):
  directory=root/f'project-{i}';directory.mkdir();path=directory/'lykoware.toml'
  path.write_text(f'schema_version = 1\nproject_id = "{i:032x}"\nname = "Studio {i} 🐺"\n',encoding='utf8');paths.append(path)
 opened=req('project.open',path=str(paths[0]));other=req('project.open',path=str(paths[1]))
 commands=[req('project.query'),opened,req('project.query'),req('editor.snapshot'),opened,req('editor.snapshot'),other,req('project.query'),req('history.undo'),req('project.query'),req('history.redo'),req('project.close'),req('project.query'),req('project.recent.clear'),req('project.query'),req('history.undo'),req('project.query')]
 r=run(commands);assert r==run(commands,True) and all(x['ok'] for x in r),r
 assert r[0]['data']==dict(active=None,recent=[]) and r[3]==r[5],'initial/no-op context'
 state=r[2]['data'];assert state['active']['path']==paths[0].as_posix() and state['active']['root']==paths[0].parent.as_posix() and state['active']['manifest']['name']=='Studio 0 🐺'
 assert r[3]['data']['project']==state
 assert [x['path'] for x in r[7]['data']['recent']]==[paths[1].as_posix(),paths[0].as_posix()]
 assert r[9]['data']==state and r[12]['data']['active'] is None and len(r[12]['data']['recent'])==2
 assert r[14]['data']==dict(active=None,recent=[]) and r[16]['data']==r[12]['data']
 # Authoring content is never discarded or reassigned, even after entity deletion.
 content=[req('entity.create',name='Keep'),req('asset.register',path='x.json',kind='scene'),req('play.start'),req('transaction.execute',commands=[{'command':'entity.create','arguments':{'name':'Deleted'}},{'command':'entity.delete','arguments':{'id':1}}])]
 for edit in content:
  for context in (req('project.close'),other):
   r=run([opened,edit,req('editor.snapshot'),context,req('editor.snapshot')]);assert r[-2]['error']['code']=='project_has_content' and r[-3]==r[-1],r
  # Reopening exactly the current captured context is a no-op with content present.
  r=run([opened,edit,req('editor.snapshot'),opened,req('editor.snapshot')]);assert r[-2]['ok'] and r[-3]==r[-1]
 # Open + staged creation is atomic; reversing those rows fails without partial opening.
 batch=[{'command':'project.open','arguments':{'path':str(paths[0])}},{'command':'entity.create','arguments':{'name':'Staged'}}]
 r=run([req('transaction.execute',commands=batch),req('project.query'),req('history.undo'),req('editor.snapshot')]);assert all(x['ok'] for x in r) and r[1]['data']['active'] and r[-1]['data']['project']==dict(active=None,recent=[]) and not r[-1]['data']['scene']['entities']
 r=run([req('editor.snapshot'),req('transaction.execute',commands=list(reversed(batch))),req('editor.snapshot')]);assert not r[1]['ok'] and r[0]==r[-1]
 bad=root/'invalid.toml';bad.write_text('schema_version = 999')
 for invalid in (bad,root,root/'missing.toml'):
  r=run([opened,req('editor.snapshot'),req('project.open',path=str(invalid)),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1]
 # Preconditions stop before reads, and canonical paths deduplicate normal aliases.
 for failed in ({**opened,'instance':'other'},{**opened,'expected_revision':99}):assert not run([failed])[-1]['ok']
 alias=paths[0].parent/'..'/paths[0].parent.name/paths[0].name
 r=run([opened,req('editor.snapshot'),req('project.open',path=str(alias)),req('editor.snapshot')]);assert r[1]==r[-1]
 if os.name!='nt':
  link=root/'alias.toml';link.symlink_to(paths[0]);r=run([opened,req('editor.snapshot'),req('project.open',path=str(link)),req('editor.snapshot')]);assert r[1]==r[-1]
 r=run([req('project.open',path=str(p)) for p in paths]+[req('project.query')]);recent=r[-1]['data']['recent'];assert len(recent)==16 and recent[0]['path']==paths[-1].as_posix() and recent[-1]['path']==paths[2].as_posix()
 # Scene and workspace persistence must not leak machine-local context/recent paths.
 scene=root/'scene.json';workspace=root/'workspace.toml'
 r=run([opened,req('scene.save',path=str(scene)),req('workspace.save',path=str(workspace))]);assert all(x['ok'] for x in r) and str(root) not in scene.read_text() and str(root) not in workspace.read_text()
 # Redo reuses the captured manifest and paths after the source disappears.
 p=subprocess.Popen([exe,'--instance','test'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
 def live(request):
  p.stdin.write(json.dumps(request)+'\n');p.stdin.flush();return json.loads(p.stdout.readline())
 assert live(opened)['ok'];assert live(req('entity.create',name='Preserve'))['ok']
 paths[0].write_text(paths[0].read_text().replace('Studio 0','Changed Studio'),encoding='utf8')
 assert live(opened)['error']['code']=='project_has_content' and live(req('project.query'))['data']==state
 assert live(req('history.undo'))['ok'];assert live(req('history.undo'))['ok'];paths[0].unlink();assert live(req('history.redo'))['ok'];assert live(req('project.query'))['data']==state
 p.stdin.close();assert p.wait(timeout=10)==1  # The intentional content-guard error sets CLI failure status.
print('Passed project context/history, content guards, canonical MRU, captured redo, rollback and CLI/MCP parity')
