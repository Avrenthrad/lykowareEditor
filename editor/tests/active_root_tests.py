"""Active-project source roots, explicit overrides and transaction context guards."""
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
import hashlib
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp);projects=[]
 for i in range(2):
  folder=root/f'project-{i}';folder.mkdir();manifest=folder/'lykoware.toml';manifest.write_text(f'schema_version = 1\nproject_id = "{i:032x}"\nname = "Studio {i}"\n')
  prefab=folder/'tree.json';assert run([req('entity.create',name=f'Tree {i}'),req('prefab.export',id=1,path=str(prefab))])[-1]['ok'];projects.append((folder,manifest,prefab))
 a,manifest_a,prefab_a=projects[0];b,manifest_b,prefab_b=projects[1];digest=hashlib.sha256(prefab_a.read_bytes()).hexdigest()
 registered=[req('asset.register',path='tree.json',kind='prefab')]
 base=[req('project.open',path=str(manifest_a))]+registered
 commands=base+[req('editor.snapshot'),req('asset.validate',id=1),req('asset.check',id=1,sha256=digest),req('asset.audit'),req('editor.snapshot'),req('asset.instantiate_checked',id=1,parent=0,sha256=digest),req('scene.query'),req('history.undo'),req('asset.instantiate',id=1,parent=0,root=''),req('scene.query')]
 r=run(commands);assert r==run(commands,True) and all(x['ok'] for x in r),r
 assert r[2]==r[6] and r[3]['data']['root']==a.resolve().as_posix() and r[4]['data']['matches'] and r[5]['data']['root']==a.resolve().as_posix()
 assert r[8]['data']['entities'][0]['name']=='Tree 0' and r[-1]['data']['entities'][0]['name']=='Tree 0'
 # Explicit nonempty roots override active state; invalid overrides never fall back.
 r=run(base+[req('asset.validate',id=1,root=str(b)),req('asset.instantiate',id=1,parent=0,root=str(b)),req('scene.query')]);assert all(x['ok'] for x in r) and r[2]['data']['root']==b.resolve().as_posix() and r[-1]['data']['entities'][0]['name']=='Tree 1'
 assert run(base+[req('asset.validate',id=1,root='relative')])[-1]['error']['field']=='root'
 # Missing/blank root without an active project is a structured no-state-change error.
 requests=[req('asset.validate',id=1),req('asset.check',id=1,sha256=digest),req('asset.audit'),req('asset.instantiate',id=1,parent=0),req('asset.instantiate_checked',id=1,parent=0,sha256=digest)]
 for request in requests:
  for arguments in (request['arguments'],{**request['arguments'],'root':''}):
   rows=registered+[req('editor.snapshot'),{**request,'arguments':arguments},req('editor.snapshot')]
   result=run(rows);assert result==run(rows,True) and result[-2]['error']['code']=='no_active_project' and result[-2]['error']['field']=='root' and result[-3]==result[-1]
 assert run(registered+[req('asset.validate',id=1,root=str(a))])[-1]['ok']
 assert not run(base+[req('asset.validate',id=1,root=None)])[-1]['ok']
 # A catalogue can be recreated identically in another staged context: compare project too.
 empty=root/'empty-catalogue.json';empty.write_text('{"schema_version":1,"next_id":1,"assets":[]}')
 staged=[dict(command='asset.catalog.load',arguments={'path':str(empty)}),dict(command='project.close',arguments={}),dict(command='project.open',arguments={'path':str(manifest_b)}),dict(command='asset.register',arguments=registered[0]['arguments'])]
 for command in ('asset.instantiate','asset.instantiate_checked'):
  args={'id':1,'parent':0}
  if command.endswith('checked'):args['sha256']=digest
  rows=base+[req('editor.snapshot'),req('transaction.execute',commands=staged+[dict(command=command,arguments=args)]),req('editor.snapshot')]
  result=run(rows);assert result==run(rows,True) and result[-2]['error']['code']=='project_context_changed' and result[-3]==result[-1],result
 # Existing explicit-root behavior remains available when a project is staged.
 batch=[staged[0],dict(command='project.open',arguments={'path':str(manifest_a)}),staged[-1],dict(command='asset.instantiate',arguments={'id':1,'parent':0,'root':str(a)})]
 r=run(registered+[req('transaction.execute',commands=batch),req('scene.query')]);assert all(x['ok'] for x in r) and r[-1]['data']['entities'][0]['name']=='Tree 0'
 # An implicit root never resolves a newly staged open before that context commits.
 batch[-1]['arguments'].pop('root')
 r=run(registered+[req('editor.snapshot'),req('transaction.execute',commands=batch),req('editor.snapshot')]);assert r[-2]['error']['code']=='no_active_project' and r[-3]==r[-1]
 # Catalogue guards and revision preconditions are retained alongside the new context guard.
 r=run(base+[req('editor.snapshot'),req('transaction.execute',commands=[dict(command='asset.relocate',arguments={'id':1,'path':'other.json'}),dict(command='asset.instantiate',arguments={'id':1,'parent':0})]),req('editor.snapshot')]);assert r[-2]['error']['code']=='asset_reference_changed' and r[-3]==r[-1]
 r=run(base+[{**req('asset.audit'),'expected_revision':0}]);assert r[-1]['error']['code']=='stale_revision'
 tools={x['name']:x for x in run([req('commands.list')])[0]['data']}
 for name in ('asset.validate','asset.check','asset.audit','asset.instantiate','asset.instantiate_checked'):
  schema=tools[name]['inputSchema'];assert 'root' not in schema['required'] and schema['properties']['root']['default']=='' and schema['properties']['root']['minLength']==0
 # A root capability does not bypass current filesystem validation.
 prefab_a.unlink();r=run(base+[req('asset.validate',id=1)]);assert r[-1]['error']['code']=='invalid_asset_source'
print('Passed implicit/override roots, concrete observations, no-active errors, staged context/catalogue guards and CLI/MCP parity')
