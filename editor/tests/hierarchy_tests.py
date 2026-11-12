"""Scene hierarchy invariants, transaction history and text migration."""
import json
import pathlib
import subprocess
import sys
import tempfile
exe = sys.argv[1]
def req(command, **arguments):
    return {'instance':'test','command':command,'arguments':arguments}
def run(commands):
    p = subprocess.run([exe,'--instance','test'],input='\n'.join(map(json.dumps,commands))+'\n',text=True,capture_output=True)
    assert p.returncode in (0,1), p.stderr
    result = [json.loads(line) for line in p.stdout.splitlines()]
    assert len(result)==len(commands)
    return result
base=[req('entity.create',name='Root'),req('entity.create',name='Child'),req('entity.create',name='Leaf'),req('entity.reparent',id=2,parent=1),req('entity.reparent',id=3,parent=2)]
r=run(base+[req('scene.tree'),req('entity.duplicate',id=2,name='Sibling'),req('scene.tree'),req('entity.delete',id=1),req('scene.tree'),req('history.undo'),req('scene.tree')])
assert all(x['ok'] for x in r),r
assert [(n['id'],n['parent'],n['depth']) for n in r[5]['data']['nodes']]==[(1,0,0),(2,1,1),(3,2,2)]
assert [(n['id'],n['parent']) for n in r[7]['data']['nodes']]==[(1,0),(2,1),(3,2),(4,1)]
assert [(n['id'],n['parent']) for n in r[9]['data']['nodes']]==[(2,0),(3,2),(4,0)]
assert r[11]['data']==r[7]['data'], 'undo restores promoted children'
for bad in [req('entity.reparent',id=1,parent=3),req('entity.reparent',id=2,parent=2),req('entity.reparent',id=1,parent=999),req('entity.reparent',id=999,parent=0)]:
    r=run(base+[req('editor.snapshot'),bad,req('editor.snapshot')])
    assert not r[-2]['ok'] and r[-3]==r[-1],r
r=run(base+[req('editor.snapshot'),req('transaction.execute',commands=[{'command':'entity.reparent','arguments':{'id':3,'parent':0}},{'command':'entity.reparent','arguments':{'id':1,'parent':2}}]),req('editor.snapshot')])
assert not r[-2]['ok'] and r[-3]==r[-1], 'failed batch preserves edges/history'
with tempfile.TemporaryDirectory() as temp:
    scene=pathlib.Path(temp)/'tree.json'
    r=run(base+[req('scene.save',path=str(scene)),req('scene.load',path=str(scene)),req('scene.tree')])
    assert all(x['ok'] for x in r),r
    data=json.loads(scene.read_text())
    assert data['schema_version']==2 and data['entities'][1]['parent']==1
    assert r[-1]['data']['nodes'][2]['depth']==2
    for entities in [ [{'id':1,'name':'Cycle','parent':1}], [{'id':1,'name':'Missing','parent':99}]]:
        scene.write_text(json.dumps({'schema_version':2,'next_id':4,'entities':entities}))
        r=run(base+[req('editor.snapshot'),req('scene.load',path=str(scene)),req('editor.snapshot')])
        assert not r[-2]['ok'] and r[-3]==r[-1]
    scene.write_text(json.dumps({'schema_version':1,'next_id':2,'entities':[{'id':1,'name':'Old'}]}))
    r=run([req('scene.load',path=str(scene)),req('scene.tree')])
    assert r[-1]['data']['nodes']==[{'id':1,'name':'Old','parent':0,'depth':0}]
    scene.write_text(json.dumps({'schema_version':2,'next_id':67,'entities':[{'id':i,'name':str(i),'parent':i-1} for i in range(1,67)]}))
    assert not run([req('scene.load',path=str(scene))])[0]['ok'], 'depth bound'
commands=base+[req('scene.tree')]
messages=[{'jsonrpc':'2.0','id':0,'method':'initialize','params':{'protocolVersion':'2025-06-18','capabilities':{},'clientInfo':{'name':'tree-test','version':'1'}}},{'jsonrpc':'2.0','method':'notifications/initialized'}]
for i, command in enumerate(commands,1):
    messages.append({'jsonrpc':'2.0','id':i,'method':'tools/call','params':{'name':command['command'],'arguments':{'instance':'test','arguments':command['arguments']}}})
p=subprocess.run([exe,'--instance','test','--mcp'],input='\n'.join(map(json.dumps,messages))+'\n',text=True,capture_output=True)
assert p.returncode==0,p.stderr
assert [json.loads(line)['result']['structuredContent'] for line in p.stdout.splitlines()[1:]]==run(commands), 'MCP hierarchy parity'
r=run(base+[req('entity.reparent',id=2,parent=0),req('scene.tree'),req('history.undo'),req('scene.tree'),req('history.redo'),req('scene.tree')])
assert r[6]['data']==r[10]['data'] and r[8]['data']['nodes'][1]['parent']==1, 'reparent undo/redo'
print('Passed hierarchy cycles, deterministic tree, duplicate/delete edges, rollback, history and v1/v2 persistence')
