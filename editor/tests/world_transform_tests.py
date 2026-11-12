"""World matrix contract with analytic geometry checks, not renderer output."""
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
base=[req('entity.create',name='Parent'),req('entity.create',name='Child'),req('entity.reparent',id=2,parent=1)]
def set_local(i,**values):return req('transform.set',id=i,**{**identity,**values})
def near(actual,expected):assert all(math.isclose(a,b,rel_tol=1e-12,abs_tol=1e-12) for a,b in zip(actual,expected)),(actual,expected)
commands=base+[set_local(1,px=10,rz=90,sx=2,sy=2,sz=2),set_local(2,px=3),req('editor.snapshot'),req('transform.world',id=2),req('editor.snapshot')]
r=run(commands);assert r==run(commands,True),'CLI/MCP parity'
assert r[-3]==r[-1], 'query leaves history and revision unchanged'
data=r[-2]['data'];m=data['matrix']
near([m[3],m[7],m[11]],[10,6,0]);near([m[0],m[4],m[8]],[0,2,0])
assert data['storage']=='row_major' and data['vectors']=='column' and data['local_order']=='T*Rx*Ry*Rz*S'
r=run(base+[set_local(1,rx=90,ry=90),req('transform.world',id=1)])
m=r[-1]['data']['matrix'];near([m[2],m[6],m[10]],[1,0,0])
r=run(base+[set_local(1,sx=2),set_local(2,rz=45),req('transform.world',id=2)])
m=r[-1]['data']['matrix'];near([m[0],m[4],m[1],m[5]],[math.sqrt(2),math.sqrt(.5),-math.sqrt(2),math.sqrt(.5)])
assert abs(m[0]*m[1]+m[4]*m[5])>1, 'shear retained'
r=run(base+[set_local(1,px=10),set_local(2,px=3),req('entity.reparent',id=2,parent=0),req('transform.world',id=2),req('history.undo'),req('transform.world',id=2)])
near([r[6]['data']['matrix'][3],r[8]['data']['matrix'][3]],[3,13])
r=run(base+[set_local(1,sx=-2,rz=1e308),req('transform.world',id=1)])
assert r[-1]['ok'] and all(math.isfinite(x) for x in r[-1]['data']['matrix']), 'large angles reduced safely'
r=run(base+[set_local(1,sx=1e308),set_local(2,sx=1e308),req('editor.snapshot'),req('transform.world',id=2),req('editor.snapshot')])
assert r[-2]['error']['code']=='transform_range' and r[-3]==r[-1], 'overflow is a read-only structured failure'
assert run([req('transform.world',id=99)])[0]['error']['code']=='not_found'
print('Passed analytic composition, Euler order, shear, hierarchy updates, overflow, read-only history and MCP parity')
