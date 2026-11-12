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
value={**identity,'px':-12.25,'ry':90,'sx':-2,'sy':.5}
base=[req('entity.create',name='Source')]
commands=base+[req('transform.query',id=1),req('transform.set',id=1,**value),req('transform.query',id=1),req('history.undo'),req('transform.query',id=1),req('history.redo'),req('entity.duplicate',id=1,name='Copy'),req('transform.query',id=2),req('entity.delete',id=1),req('history.undo'),req('transform.query',id=1)]
r=run(commands)
assert r==run(commands,True),'CLI/MCP parity'
assert all(row['ok'] for row in r),r
assert r[1]['data']==identity and r[3]['data']==value and r[5]['data']==identity
assert r[8]['data']==value and r[11]['data']==value
for args in [{**value,'sx':0},{**value,'px':True},{**value,'py':'bad'}]:
 r=run(base+[req('editor.snapshot'),req('transform.set',id=1,**args),req('editor.snapshot')])
 assert not r[-2]['ok'] and r[-3]==r[-1],r
r=run(base+[req('editor.snapshot'),req('transaction.execute',commands=[{'command':'transform.set','arguments':{'id':1,**value}},{'command':'transform.set','arguments':{'id':99,**value}}]),req('editor.snapshot')])
assert not r[-2]['ok'] and r[-3]==r[-1], 'failed batch rollback'
with tempfile.TemporaryDirectory() as temp:
 path=pathlib.Path(temp)/'scene.json'
 r=run(base+[req('transform.set',id=1,**value),req('scene.save',path=str(path)),req('scene.load',path=str(path)),req('transform.query',id=1)])
 assert all(row['ok'] for row in r) and r[-1]['data']==value,r
 saved=json.loads(path.read_text());assert saved['schema_version']==3 and saved['entities'][0]['transform']==value
 saved['entities'][0]['transform']['sz']=0;path.write_text(json.dumps(saved))
 r=run(base+[req('editor.snapshot'),req('scene.load',path=str(path)),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1]
 path.write_text(json.dumps({'schema_version':1,'next_id':2,'entities':[{'id':1,'name':'Old'}]}))
 assert run([req('scene.load',path=str(path)),req('transform.query',id=1)])[-1]['data']==identity
 canonical=pathlib.Path(temp)/'identity.json'
 r=run(base+[req('transform.set',id=1,**value),req('transform.set',id=1,**identity),req('scene.save',path=str(canonical))])
 assert all(row['ok'] for row in r) and json.loads(canonical.read_text())['schema_version']==1, 'identity export normalizes to v1'
r=run(base+[req('transform.set',id=1,**value),req('entity.delete',id=1),req('editor.snapshot')])
assert r[-1]['data']['transforms']=={}, 'deletion removes transform metadata'
print('Passed transform numbers, identity migration, history, copying/deletion, rollback, v3 persistence and MCP parity')
