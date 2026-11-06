"""Reusable valid, large, malformed and migration project fixtures."""
import json,pathlib,subprocess,sys,tempfile,os
exe=sys.argv[1]
def req(command,**arguments):return {'instance':'test','command':command,'arguments':arguments}
def run(commands,mcp=False):
 messages=commands
 if mcp:
  messages=[{'jsonrpc':'2.0','id':0,'method':'initialize','params':{'protocolVersion':'2025-06-18','capabilities':{},'clientInfo':{'name':'source-test','version':'1'}}},{'jsonrpc':'2.0','method':'notifications/initialized'}]+[{'jsonrpc':'2.0','id':i,'method':'tools/call','params':{'name':c['command'],'arguments':{'instance':'test','arguments':c['arguments']}}} for i,c in enumerate(commands,1)]
 p=subprocess.run([exe,'--instance','test']+(['--mcp'] if mcp else []),input='\n'.join(map(json.dumps,messages))+'\n',text=True,capture_output=True,timeout=10)
 assert p.returncode in (0,1),p.stderr
 rows=[json.loads(line) for line in p.stdout.splitlines()]
 return [row['result']['structuredContent'] for row in rows[1:]] if mcp else rows
import importlib.util
sys.dont_write_bytecode=True
fixtures=pathlib.Path(__file__).resolve().parent/'fixtures'
spec=importlib.util.spec_from_file_location('large_fixtures',fixtures/'generate_large.py');large=importlib.util.module_from_spec(spec);spec.loader.exec_module(large)
project=fixtures/'valid/project'
original={p:p.read_bytes() for p in fixtures.rglob('*') if p.is_file() and '__pycache__' not in str(p)}
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp)
 commands=[req('project.manifest.validate',path=str(project/'lykoware.toml')),req('scene.load',path=str(project/'scenes/main.json')),req('asset.catalog.load',path=str(project/'assets.json')),req('asset.audit',root=str(project)),req('prefab.instantiate',path=str(project/'prefabs/tree.json'),parent=1),req('scene.tree'),req('history.undo'),req('scene.tree')]
 r=run(commands);assert r==run(commands,True) and all(row['ok'] for row in r),r
 assert r[3]['data']['valid']==2 and r[3]['data']['skipped']==1
 assert len(r[5]['data']['nodes'])==4 and len(r[7]['data']['nodes'])==2
 identity=dict(zip(('px','py','pz','rx','ry','rz','sx','sy','sz'),(0,0,0,0,0,0,1,1,1)))
 for version in (1,2):
  output=root/f'promoted-v{version}.json'
  commands=[req('scene.load',path=str(fixtures/f'migration/scene-v{version}.json')),req('scene.tree'),req('transform.set',id=1,**{**identity,'px':3.25}),req('scene.save',path=str(output)),req('scene.load',path=str(output)),req('scene.tree')]
  r=run(commands);assert all(row['ok'] for row in r),r
  data=json.loads(output.read_text());assert data['schema_version']==3 and data['next_id']==3 and [e['name'] for e in data['entities']]==['Root','Child']
  assert data['entities'][0]['transform']['px']==3.25
  assert [(e['id'],e['parent']) for e in r[1]['data']['nodes']]==[(e['id'],e['parent']) for e in r[-1]['data']['nodes']]
 migrated=root/'workspace-v2.toml'
 r=run([req('workspace.load',path=str(fixtures/'migration/workspace-v1.toml')),req('workspace.query'),req('workspace.save',path=str(migrated)),req('workspace.load',path=str(migrated)),req('workspace.query')]);assert all(row['ok'] for row in r) and r[1]['data']==r[-1]['data'],r
 import tomllib
 assert tomllib.loads(migrated.read_text())['schema_version']==2
 assert run([req('workspace.load',path=str(fixtures/'valid/workspace-v2.toml'))])[-1]['ok']
 invalid=[('scene-cycle.json','scene.load',{}),('scene-zero-scale.json','scene.load',{}),('catalogue-collision.json','asset.catalog.load',{}),('prefab-disconnected.json','prefab.instantiate',{'parent':0}),('project-future.toml','project.manifest.validate',{}),('workspace-duplicate.toml','workspace.load',{}),('truncated.json','scene.load',{})]
 for name,command,args in invalid:
  commands=[req('entity.create',name='Preserve'),req('editor.snapshot'),req(command,path=str(fixtures/'malformed'/name),**args),req('editor.snapshot')]
  r=run(commands);assert r==run(commands,True) and not r[-2]['ok'] and r[-3]==r[-1],(name,r)
 scene,catalogue=large.generate(root/'large')
 r=run([req('scene.load',path=str(scene)),req('scene.tree'),req('asset.catalog.load',path=str(catalogue)),req('asset.audit',root=str(root/'large')),req('history.undo'),req('asset.list',query='')]);assert all(row['ok'] for row in r),r
 assert len(r[1]['data']['nodes'])==1000 and r[3]['data']['skipped']==4096 and r[3]['data']['invalid']==0 and not r[-1]['data']['assets']
 other_scene,other_catalogue=large.generate(root/'repeat')
 assert scene.read_bytes()==other_scene.read_bytes() and catalogue.read_bytes()==other_catalogue.read_bytes()
 try:large.generate(root/'large')
 except FileExistsError:pass
 else:raise AssertionError('large generator overwrote existing fixture')
 spec=importlib.util.spec_from_file_location('recent_fixture',fixtures/'generate_recent.py');recent_fixture=importlib.util.module_from_spec(spec);spec.loader.exec_module(recent_fixture)
 recent_store=root/'recent-history.json';entry=recent_fixture.generate(recent_store,root/'missing-project')
 r=run([req('project.recent.load',path=str(recent_store)),req('project.query')]);assert all(row['ok'] for row in r) and r[-1]['data']==dict(active=None,recent=[entry])
assert all(p.read_bytes()==content for p,content in original.items()),'fixture mutation'
print('Passed reusable small/large/malformed fixtures, scene/workspace migrations, identity/reference preservation and CLI/MCP parity')
