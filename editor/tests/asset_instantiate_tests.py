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
 root=pathlib.Path(temp)/'project';root.mkdir();path=root/'tree.json'
 r=run([req('entity.create',name='Tree'),req('entity.create',name='Leaf'),req('entity.reparent',id=2,parent=1),req('prefab.export',id=1,path=str(path))]);assert all(x['ok'] for x in r),r
 base=[req('asset.register',path='tree.json',kind='prefab'),req('entity.create',name='Parent')]
 commands=base+[req('asset.instantiate',id=1,root=str(root),parent=1),req('editor.snapshot'),req('history.undo'),req('scene.query'),req('history.redo'),req('scene.tree')]
 r=run(commands);assert r==run(commands,True),'MCP parity';assert all(x['ok'] for x in r),r
 assert r[3]['data']['selection']['entity']=={'id':2,'name':'Tree'} and len(r[5]['data']['entities'])==1
 assert [(x['id'],x['parent']) for x in r[7]['data']['nodes']]==[(1,0),(2,1),(3,2)]
 for bad in (req('asset.instantiate',id=99,root=str(root),parent=0),req('asset.instantiate',id=1,root='relative',parent=0),req('asset.instantiate',id=1,root=str(root),parent=99)):
  r=run(base+[req('editor.snapshot'),bad,req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1],r
 for staged in ({'command':'asset.relocate','arguments':{'id':1,'path':'changed.json'}},{'command':'asset.forget','arguments':{'id':1}}):
  r=run(base+[req('editor.snapshot'),req('transaction.execute',commands=[staged,{'command':'asset.instantiate','arguments':{'id':1,'root':str(root),'parent':0}}]),req('editor.snapshot')]);assert r[-2]['error']['code']=='asset_reference_changed' and r[-3]==r[-1],r
 r=run([req('asset.register',path='tree.json',kind='scene'),req('asset.instantiate',id=1,root=str(root),parent=0)]);assert r[-1]['error']['code']=='unsupported_asset_kind'
 r=run(base+[req('transaction.execute',commands=[{'command':'entity.create','arguments':{'name':'Staged parent'}},{'command':'asset.instantiate','arguments':{'id':1,'root':str(root),'parent':2}}]),req('scene.tree')]);assert all(x['ok'] for x in r),r
 assert [(x['id'],x['parent']) for x in r[-1]['data']['nodes']]==[(1,0),(2,0),(3,2),(4,3)]
 r=run(base+[req('editor.snapshot'),req('transaction.execute',commands=[{'command':'asset.instantiate','arguments':{'id':1,'root':str(root),'parent':0}},{'command':'entity.delete','arguments':{'id':999}}]),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1]
 # Redo uses captured authoring state after the source disappears.
 p=subprocess.Popen([exe,'--instance','test'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
 def live(command):
  p.stdin.write(json.dumps(command)+'\n');p.stdin.flush();return json.loads(p.stdout.readline())
 for command in base:assert live(command)['ok']
 assert live(req('asset.instantiate',id=1,root=str(root),parent=0))['ok']
 assert live(req('history.undo'))['ok'];path.unlink();assert live(req('history.redo'))['ok']
 assert len(live(req('scene.query'))['data']['entities'])==3
 p.stdin.close();assert p.wait(timeout=10)==0
 path.write_text('invalid')
 r=run(base+[req('editor.snapshot'),req('asset.instantiate',id=1,root=str(root),parent=0),req('editor.snapshot')]);assert r[-2]['error']['code']=='invalid_asset_source' and r[-3]==r[-1]
 outside=pathlib.Path(temp)/'outside';outside.mkdir();(outside/'tree.json').write_text('{}')
 try:(root/'escape').symlink_to(outside,target_is_directory=True)
 except (OSError,NotImplementedError):assert os.name=='nt'
 else:
  r=run([req('asset.register',path='escape/tree.json',kind='prefab'),req('asset.instantiate',id=1,root=str(root),parent=0)]);assert r[-1]['error']['code']=='invalid_asset_source' and 'outside' in r[-1]['error']['message']
 stale=req('asset.instantiate',id=1,root=str(root),parent=0);stale['expected_revision']=0
 r=run(base+[req('editor.snapshot'),stale,req('editor.snapshot')]);assert r[-2]['error']['code']=='stale_revision' and r[-3]==r[-1]
print('Passed catalogue ID instantiation, frozen redo, metadata guards, staged parents, rollback, source containment and MCP parity')
