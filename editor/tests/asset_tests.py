"""Local authoring transforms: validation, history, copying and persistence."""
import json, pathlib, subprocess, sys, tempfile
exe=sys.argv[1]
identity=dict(zip(('px','py','pz','rx','ry','rz','sx','sy','sz'),(0,0,0,0,0,0,1,1,1)))
def req(command,**arguments):return {'instance':'test','command':command,'arguments':arguments}
def run(commands,mcp=False):
 messages=commands
 if mcp:
  messages=[{'jsonrpc':'2.0','id':0,'method':'initialize','params':{'protocolVersion':'2025-06-18','capabilities':{},'clientInfo':{'name':'transform-test','version':'1'}}},{'jsonrpc':'2.0','method':'notifications/initialized'}]
  messages += [{'jsonrpc':'2.0','id':i,'method':'tools/call','params':{'name':c['command'],'arguments':{'instance':'test','arguments':c['arguments']}}} for i,c in enumerate(commands,1)]
 p=subprocess.run([exe,'--instance','test']+(['--mcp'] if mcp else []),input='\n'.join(map(json.dumps,messages))+'\n',text=True,capture_output=True)
 assert p.returncode in (0,1),p.stderr
 rows=[json.loads(line) for line in p.stdout.splitlines()]
 return [r['result']['structuredContent'] for r in rows[1:]] if mcp else rows

commands=[req('asset.register',path='prefabs/Tree.json',kind='prefab'),req('asset.query',id=1),req('asset.relocate',id=1,path='prefabs/Oak.json'),req('asset.list',query='OAK'),req('asset.forget',id=1),req('history.undo'),req('asset.query',id=1),req('history.redo'),req('asset.register',path='scripts/Main.cpp',kind='script'),req('asset.query',id=2)]
r=run(commands);assert r==run(commands,True),'MCP parity'
assert all(x['ok'] for x in r),r
assert r[1]['data']['id']==1 and r[3]['data']['assets'][0]['path']=='prefabs/Oak.json' and r[6]['data']['path']=='prefabs/Oak.json' and r[9]['data']['id']==2
base=[req('entity.create',name='Scene untouched'),req('asset.register',path='a.json',kind='scene')]
for path in ('/absolute','../outside','a/../b','a//b','a/','C:/file','a\\file','a/CON.txt','a/file.','a/file ','a?b','', 'x'*513):
 r=run(base+[req('editor.snapshot'),req('asset.register',path=path,kind='scene'),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1],path
for bad in (req('asset.register',path='A.JSON',kind='prefab'),req('asset.register',path='other',kind='unknown'),req('asset.relocate',id=999,path='ok'),req('asset.forget',id=999)):
 r=run(base+[req('editor.snapshot'),bad,req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1],r
r=run(base+[req('editor.snapshot'),req('transaction.execute',commands=[{'command':'asset.relocate','arguments':{'id':1,'path':'new.json'}},{'command':'asset.forget','arguments':{'id':99}}]),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1]
with tempfile.TemporaryDirectory() as temp:
 path=pathlib.Path(temp)/'catalog.json';scene=pathlib.Path(temp)/'scene.json';source=pathlib.Path(temp)/'source.txt';source.write_text('keep')
 r=run(base+[req('asset.catalog.save',path=str(path)),req('asset.forget',id=1),req('asset.catalog.load',path=str(path)),req('asset.query',id=1),req('history.undo'),req('asset.list',query='')]);assert all(x['ok'] for x in r) and r[-1]['data']['assets']==[]
 original=path.read_bytes();assert json.loads(original)=={'schema_version':1,'next_id':2,'assets':[{'id':1,'kind':'scene','path':'a.json'}]}
 assert not run(base+[req('asset.catalog.save',path=str(path))])[-1]['ok'] and path.read_bytes()==original
 r=run(base+[req('scene.save',path=str(scene)),req('scene.load',path=str(scene)),req('asset.query',id=1)]);assert r[-1]['data']['path']=='a.json' and 'assets' not in json.loads(scene.read_text())
 for data in ({'schema_version':99,'next_id':2,'assets':[]},{'schema_version':1,'next_id':2,'assets':[{'id':1,'path':'../x','kind':'scene'}]},{'schema_version':1,'next_id':3,'assets':[{'id':1,'path':'a','kind':'scene'},{'id':2,'path':'A','kind':'prefab'}]}):
  path.write_text(json.dumps(data));r=run(base+[req('editor.snapshot'),req('asset.catalog.load',path=str(path)),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1]
 assert source.read_text()=='keep' and not list(pathlib.Path(temp).glob('.lyko-save-*'))
 path.write_text(json.dumps({'schema_version':1,'next_id':18446744073709551615,'assets':[]}))
 r=run([req('asset.catalog.load',path=str(path)),req('editor.snapshot'),req('asset.register',path='next.json',kind='scene'),req('editor.snapshot')]);assert r[-2]['error']['code']=='id_exhausted' and r[-3]==r[-1]
 path.write_text(json.dumps({'schema_version':1,'next_id':1,'assets':[{}]*4097}))
 r=run(base+[req('editor.snapshot'),req('asset.catalog.load',path=str(path)),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1]
r=run(base+[req('asset.register',path='b.json',kind='scene'),req('editor.snapshot'),req('asset.relocate',id=2,path='A.JSON'),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1], 'relocation cannot alias an existing path'
print('Passed catalog IDs, relocation/removal, path validation, uniqueness, atomic history, persistence isolation and MCP parity')
