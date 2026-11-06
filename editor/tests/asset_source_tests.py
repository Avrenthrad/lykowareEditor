"""Read-only registered scene/prefab validation under explicit project roots."""
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
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp)/'project';root.mkdir();(root/'scènes').mkdir()
 path=root/'scènes'/'é.json';scene={'schema_version':1,'next_id':2,'entities':[{'id':1,'name':'Player'}]};path.write_text(json.dumps(scene),encoding='utf8');original=path.read_bytes()
 base=[req('asset.register',path='scènes/é.json',kind='scene')]
 commands=base+[req('editor.snapshot'),req('asset.validate',id=1,root=str(root)),req('editor.snapshot')]
 r=run(commands);assert r==run(commands,True),'MCP validation parity'
 assert r[2]['ok'] and r[2]['data']['entities']==1 and r[2]['data']['bytes']==len(original) and r[1]==r[3] and path.read_bytes()==original,r
 prefab=root/'tree.json';r=run([req('entity.create',name='Tree'),req('prefab.export',id=1,path=str(prefab))]);assert all(x['ok'] for x in r)
 r=run([req('asset.register',path='tree.json',kind='prefab'),req('asset.validate',id=1,root=str(root))]);assert r[-1]['ok'] and r[-1]['data']['entities']==1
 for directory in ('relative',str(root/'missing'),str(path)):
  r=run(base+[req('editor.snapshot'),req('asset.validate',id=1,root=directory),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-2]['error']['field']=='root' and r[-3]==r[-1],r
 for content in ('not json',json.dumps({**scene,'schema_version':99}),json.dumps({**scene,'entities':[{'id':1,'name':''}]}),'x'*(1024*1024+1)):
  path.write_text(content);r=run(base+[req('editor.snapshot'),req('asset.validate',id=1,root=str(root)),req('editor.snapshot')]);assert r[-2]['error']['code']=='invalid_asset_source' and r[-3]==r[-1]
 r=run([req('asset.register',path='absent.png',kind='texture'),req('asset.validate',id=1,root=str(root))]);assert r[-1]['error']['code']=='unsupported_asset_kind'
 for relative in ('missing.json','scènes'):
  r=run([req('asset.register',path=relative,kind='scene'),req('asset.validate',id=1,root=str(root))]);assert r[-1]['error']['field']=='path'
 assert run([req('asset.validate',id=99,root=str(root))])[0]['error']['code']=='not_found'
 outside=pathlib.Path(temp)/'project-sibling';outside.mkdir();(outside/'outside.json').write_text(json.dumps(scene))
 try:
  (root/'linked').symlink_to(outside,target_is_directory=True)
 except (OSError,NotImplementedError):
  assert os.name=='nt','unexpected symlink test setup failure'
 else:
  r=run([req('asset.register',path='linked/outside.json',kind='scene'),req('asset.validate',id=1,root=str(root))]);assert not r[-1]['ok'] and 'outside' in r[-1]['error']['message']
 if hasattr(os,'mkfifo'):
  os.mkfifo(root/'pipe.json')
  r=run([req('asset.register',path='pipe.json',kind='scene'),req('asset.validate',id=1,root=str(root))]);assert r[-1]['error']['field']=='path', 'nonregular source is rejected without reading'
 assert not list(root.rglob('.lyko-save-*'))
print('Passed scene/prefab validation, read-only state, Unicode, roots, file bounds, observed symlink containment and MCP parity')
