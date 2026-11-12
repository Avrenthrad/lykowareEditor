"""World-preserving reparent: subtree equivalence, transaction history and rejection."""
import json, math, subprocess, sys
exe=sys.argv[1]
identity=dict(zip(('px','py','pz','rx','ry','rz','sx','sy','sz'),(0,0,0,0,0,0,1,1,1)))
def req(command,**arguments):return {'instance':'test','command':command,'arguments':arguments}
def run(commands,mcp=False):
 messages=commands
 if mcp:
  messages=[{'jsonrpc':'2.0','id':0,'method':'initialize','params':{'protocolVersion':'2025-06-18','capabilities':{},'clientInfo':{'name':'world-test','version':'1'}}},{'jsonrpc':'2.0','method':'notifications/initialized'}]+[{'jsonrpc':'2.0','id':i,'method':'tools/call','params':{'name':c['command'],'arguments':{'instance':'test','arguments':c['arguments']}}} for i,c in enumerate(commands,1)]
 p=subprocess.run([exe,'--instance','test']+(['--mcp'] if mcp else []),input='\n'.join(map(json.dumps,messages))+'\n',text=True,capture_output=True)
 assert p.returncode in (0,1),p.stderr
 rows=[json.loads(line) for line in p.stdout.splitlines()]
 return [row['result']['structuredContent'] for row in rows[1:]] if mcp else rows

identity=dict(zip(('px','py','pz','rx','ry','rz','sx','sy','sz'),(0,0,0,0,0,0,1,1,1)))
def local(i,**values):return req('transform.set',id=i,**{**identity,**values})
def near(a,b):assert len(a)==len(b) and all(math.isclose(x,y,rel_tol=1e-9,abs_tol=1e-9) for x,y in zip(a,b)),(a,b)
base=[req('entity.create',name='Old'),req('entity.create',name='New'),req('entity.create',name='Child'),req('entity.create',name='Leaf'),req('entity.reparent',id=3,parent=1),req('entity.reparent',id=4,parent=3)]
for rotation in (-90,0,37,90):
 commands=base+[local(1,px=10,rz=35,sx=2,sy=2,sz=2),local(2,px=-5,ry=20),local(3,rx=15,ry=rotation,rz=25,sx=-1),local(4,px=2),req('transform.world',id=3),req('transform.world',id=4),req('entity.reparent_world',id=3,parent=2),req('transform.world',id=3),req('transform.world',id=4),req('history.undo'),req('transform.world',id=3),req('history.redo'),req('transform.world',id=3)]
 r=run(commands);assert all(x['ok'] for x in r),r
 near(r[10]['data']['matrix'],r[13]['data']['matrix']);near(r[11]['data']['matrix'],r[14]['data']['matrix']);near(r[10]['data']['matrix'],r[16]['data']['matrix']);near(r[10]['data']['matrix'],r[18]['data']['matrix'])
 assert r==run(commands,True),'MCP parity'
# Detaching a sheared world matrix would require a non-TRS local representation.
r=run(base+[local(1,sx=2),local(3,rz=45),req('editor.snapshot'),req('entity.reparent_world',id=3,parent=0),req('editor.snapshot')])
assert r[-2]['error']['code']=='cannot_preserve_world' and r[-3]==r[-1]
# A nearly singular target is conservatively rejected.
r=run(base+[local(2,sx=1e-20),req('editor.snapshot'),req('entity.reparent_world',id=3,parent=2),req('editor.snapshot')])
assert r[-2]['error']['code']=='cannot_preserve_world' and r[-3]==r[-1]
for parent in (3,4,999):
 r=run(base+[req('editor.snapshot'),req('entity.reparent_world',id=3,parent=parent),req('editor.snapshot')])
 assert not r[-2]['ok'] and r[-3]==r[-1]
r=run(base+[local(1,px=10),local(3,px=3),req('transform.world',id=3),req('entity.reparent_world',id=3,parent=0),req('transform.world',id=3)])
near(r[-3]['data']['matrix'],r[-1]['data']['matrix'])
r=run(base+[req('editor.snapshot'),req('transaction.execute',commands=[{'command':'entity.reparent_world','arguments':{'id':3,'parent':2}},{'command':'entity.delete','arguments':{'id':999}}]),req('editor.snapshot')])
assert not r[-2]['ok'] and r[-3]==r[-1]
for angle in (-90,90):
 r=run(base+[local(3,rx=20,ry=angle,rz=35),req('transform.world',id=3),req('entity.reparent_world',id=3,parent=0),req('transform.world',id=3)])
 assert all(x['ok'] for x in r),r
 near(r[-3]['data']['matrix'],r[-1]['data']['matrix'])
r=run(base+[req('editor.snapshot'),req('entity.reparent_world',id=3,parent=1),req('editor.snapshot')])
assert r[-3]==r[-1], 'same-parent no-op creates no history'
for axis in ('sx','sy','sz'):
 r=run(base+[local(3,rx=30,ry=20,rz=60,**{axis:-2}),req('transform.world',id=3),req('entity.reparent_world',id=3,parent=2),req('transform.world',id=3)])
 assert all(x['ok'] for x in r),r
 near(r[-3]['data']['matrix'],r[-1]['data']['matrix'])
r=run(base+[req('transaction.execute',commands=[{'command':'transform.set','arguments':local(1,px=10)['arguments']},{'command':'entity.reparent_world','arguments':{'id':3,'parent':2}}]),req('transform.world',id=3),req('history.undo'),req('transform.world',id=3)])
assert all(x['ok'] for x in r),r
assert math.isclose(r[-3]['data']['matrix'][3],10) and math.isclose(r[-1]['data']['matrix'][3],0), 'batch uses staged world state and undoes atomically'
print('Passed world-preserving reparent, reflection/Euler cases, subtree stability, history, rejection rollback and MCP parity')
