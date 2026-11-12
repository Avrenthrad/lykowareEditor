"""Project directory reservation, readiness manifest and failure cleanup."""
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
import shutil,concurrent.futures,tomllib
identity='0123456789abcdef'*2
def create(path,**overrides):
 args=dict(path=str(path),project_id=identity,name='Studio 🐺',directories={});args.update(overrides)
 return req('project.create',**args)
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp);destination=root/'new-é'
 command=create(destination,directories={'scenes':'assets/scenes','cache':'.generated/cache'})
 commands=[req('entity.create',name='Preserve'),req('editor.snapshot'),command,req('editor.snapshot')]
 r=run(commands);assert all(x['ok'] for x in r) and r[1]==r[-1],r
 project=r[-2]['data']['project'];assert project['root']==destination.resolve().as_posix() and project['path']==(destination/'lykoware.toml').resolve().as_posix()
 assert tomllib.loads((destination/'lykoware.toml').read_text())==project['manifest']
 assert all((destination/path).is_dir() for path in project['manifest']['directories'].values())
 assert not list(destination.rglob('.lyko-save-*'))
 shutil.rmtree(destination);assert r==run(commands,True),'CLI/MCP creation parity'
 generated=[]
 for index in range(2):
  created=run([create(root/f'generated-{index}',project_id='')])[0];assert created['ok']
  token=created['data']['project']['manifest']['project_id'];assert len(token)==32 and all(c in '0123456789abcdef' for c in token)
  generated.append(token)
 assert generated[0]!=generated[1]
 # Existing destinations of every kind are preserved, including empty roots.
 before=(destination/'lykoware.toml').read_bytes();r=run([command]);assert not r[-1]['ok'] and (destination/'lykoware.toml').read_bytes()==before
 empty=root/'empty';empty.mkdir();regular=root/'file';regular.write_bytes(b'preserve')
 for path in (empty,regular,root,root/'.'):
  r=run([create(path)]);assert not r[-1]['ok'] and regular.read_bytes()==b'preserve'
 assert list(empty.iterdir())==[]
 if os.name!='nt':
  link=root/'linked';link.symlink_to(empty,target_is_directory=True);assert not run([create(link)])[-1]['ok'] and link.is_symlink() and not list(empty.iterdir())
 # Bad configuration and invalid namespace fail before reserving anything.
 for overrides in (dict(project_id='bad'),dict(name=''),dict(directories={'assets':'../outside'}),dict(directories={'cache':'assets/cache'}),dict(directories={'assets':'LyKoWaRe.ToMl/nested'}),dict(directories={'scenes':'lykoware.toml'})):
  target=root/'invalid';r=run([create(target,**overrides)]);assert not r[-1]['ok'] and not target.exists(),r
 # Real OS component-length failure occurs after root reservation; cleanup removes it.
 target=root/'cleanup';r=run([create(target,directories={'assets':'x'*300})]);assert not r[-1]['ok'] and r[-1]['error']['code']=='project_creation_failed' and not target.exists(),r
 target=root/'absent-parent'/'child';assert not run([create(target)])[-1]['ok'] and not target.parent.exists()
 target=regular/'child';assert not run([create(target)])[-1]['ok'] and regular.read_bytes()==b'preserve'
 # Creation is an export; activation is explicit and retains normal content guards.
 r=run([req('project.open',path=project['path']),req('entity.create',name='Keep'),create(root/'another'),req('project.query'),req('project.open',path=str(root/'another'/'lykoware.toml'))]);assert r[2]['ok'] and r[3]['data']['active']==project and r[4]['error']['code']=='project_has_content'
 r=run([req('project.open',path=project['path']),req('project.query'),req('history.undo'),req('project.query')]);assert all(x['ok'] for x in r) and r[-1]['data']['active'] is None and destination.exists()
 # One winner across separate processes. Loser must not clean up the winner's root.
 target=root/'race'
 with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
  outcomes=list(pool.map(lambda _:run([create(target)])[0],range(2)))
 assert sum(x['ok'] for x in outcomes)==1 and (target/'lykoware.toml').is_file(),outcomes
 assert all((target/folder).is_dir() for folder in ('assets','scenes','scripts','graphs','.lyko/cache'))
 for rejected in ({**create(root/'stale'),'expected_revision':1},{**create(root/'wrong'),'instance':'other'}):assert not run([rejected])[-1]['ok']
 assert not (root/'stale').exists() and not (root/'wrong').exists()
 target=root/'batch';r=run([req('transaction.execute',commands=[dict(command='entity.create',arguments={'name':'Rollback'}),dict(command='project.create',arguments=create(target)['arguments'])]),req('scene.query')]);assert not r[0]['ok'] and not r[-1]['data']['entities'] and not target.exists()
 descriptor=next(x for x in run([req('commands.list')])[0]['data'] if x['name']=='project.create');assert descriptor['effect']=='filesystem_export' and not descriptor['mutation'] and not descriptor['undoable']
print('Passed project directory creation, namespace checks, reservation races, cleanup, activation guards and CLI/MCP parity')
