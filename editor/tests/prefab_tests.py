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

with tempfile.TemporaryDirectory() as temp:
 path=pathlib.Path(temp)/'subtree.json'
 value={**identity,'px':3.5,'sy':-2}
 base=[req('entity.create',name='Outside'),req('entity.create',name='Root'),req('entity.create',name='Child'),req('entity.reparent',id=2,parent=1),req('entity.reparent',id=3,parent=2),req('transform.set',id=2,**value)]
 r=run(base+[req('editor.snapshot'),req('prefab.export',id=2,path=str(path)),req('editor.snapshot')])
 assert all(x['ok'] for x in r) and r[-3]==r[-1],r
 original=path.read_bytes();saved=json.loads(original)
 assert saved['type']=='lykoware.prefab' and saved['root']==1 and len(saved['scene']['entities'])==2
 assert saved['scene']['entities'][0]['parent']==0 and saved['scene']['entities'][1]['parent']==1
 assert saved['scene']['entities'][0]['transform']==value
 commands=base+[req('prefab.instantiate',path=str(path),parent=1),req('editor.snapshot'),req('transform.query',id=4),req('history.undo'),req('editor.snapshot'),req('history.redo'),req('scene.tree')]
 r=run(commands);assert r==run(commands,True),'MCP instantiation parity'
 assert all(x['ok'] for x in r),r
 assert r[7]['data']['selection']['entity']=={'id':4,'name':'Root'} and r[8]['data']==value
 assert len(r[10]['data']['scene']['entities'])==3
 assert [(x['id'],x['parent']) for x in r[12]['data']['nodes']]==[(1,0),(2,1),(3,2),(4,1),(5,4)]
 r=run(base+[req('prefab.export',id=2,path=str(path))]);assert not r[-1]['ok'] and path.read_bytes()==original,'no overwrite'
 for mutation in [req('prefab.instantiate',path=str(path),parent=999),req('transaction.execute',commands=[{'command':'prefab.instantiate','arguments':{'path':str(path),'parent':0}},{'command':'entity.delete','arguments':{'id':999}}])]:
  r=run(base+[req('editor.snapshot'),mutation,req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1],r
 malformed=pathlib.Path(temp)/'bad.json'
 bad_values=[{**saved,'schema_version':99},{**saved,'extra':True},{**saved,'root':99},{**saved,'scene':{'schema_version':1,'next_id':3,'entities':[{'id':1,'name':'A'},{'id':2,'name':'B'}]}}]
 for data in bad_values:
  malformed.write_text(json.dumps(data));r=run(base+[req('editor.snapshot'),req('prefab.instantiate',path=str(malformed),parent=0),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1]
 exhausted=pathlib.Path(temp)/'exhausted.json'
 exhausted.write_text(json.dumps({'schema_version':1,'next_id':18446744073709551615,'entities':[{'id':1,'name':'Last'}]}))
 r=run([req('scene.load',path=str(exhausted)),req('editor.snapshot'),req('prefab.instantiate',path=str(path),parent=0),req('editor.snapshot')]);assert r[-2]['error']['code']=='id_exhausted' and r[-3]==r[-1]
 deep=pathlib.Path(temp)/'deep.json';deep.write_text(json.dumps({'schema_version':2,'next_id':66,'entities':[{'id':i,'name':str(i),'parent':i-1} for i in range(1,66)]}))
 r=run([req('scene.load',path=str(deep)),req('editor.snapshot'),req('prefab.instantiate',path=str(path),parent=65),req('editor.snapshot')]);assert r[-2]['error']['code']=='invalid_hierarchy' and r[-3]==r[-1]
 assert not list(pathlib.Path(temp).glob('.lyko-save-*')), 'staging cleanup'
 commands=[req('transaction.execute',commands=[{'command':'entity.create','arguments':{'name':'Batch parent'}},{'command':'prefab.instantiate','arguments':{'path':str(path),'parent':1}}]),req('scene.tree'),req('history.undo'),req('scene.query')]
 r=run(commands);assert all(x['ok'] for x in r),r
 assert [(x['id'],x['parent']) for x in r[1]['data']['nodes']]==[(1,0),(2,1),(3,2)] and r[3]['data']['entities']==[], 'staged parent and atomic undo'
 p=subprocess.Popen([exe,'--instance','test'],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
 def live(command):
  p.stdin.write(json.dumps(command)+'\n');p.stdin.flush();return json.loads(p.stdout.readline())
 for command in base:assert live(command)['ok']
 assert live(req('prefab.instantiate',path=str(path),parent=0))['ok']
 assert live(req('history.undo'))['ok']
 path.unlink()
 assert live(req('history.redo'))['ok'], 'redo uses captured values, not the file'
 assert len(live(req('scene.query'))['data']['entities'])==5
 p.stdin.close();assert p.wait(timeout=10)==0
print('Passed prefab export, remapping, local transforms, selection/history, import validation, rollback and MCP parity')
