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

import hashlib
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp);path=root/'scene.json'
 empty=json.dumps({'schema_version':1,'next_id':1,'entities':[]},separators=(',',':')).encode()
 base=[req('asset.register',path='scene.json',kind='scene')]
 for size in (55,56,63,64,65,127,128,129,1024*1024):
  content=empty+b' '*(size-len(empty));path.write_bytes(content)
  r=run(base+[req('editor.snapshot'),req('asset.validate',id=1,root=str(root)),req('editor.snapshot')]);assert r[-2]['ok'] and r[-2]['data']['sha256']==hashlib.sha256(content).hexdigest() and r[-3]==r[-1],size
 path.write_bytes(empty);digest=hashlib.sha256(empty).hexdigest()
 commands=base+[req('asset.check',id=1,root=str(root),sha256=digest)]
 r=run(commands);assert r==run(commands,True) and r[-1]['data']['matches']
 path.write_bytes(empty+b' ')
 r=run(base+[req('editor.snapshot'),req('asset.check',id=1,root=str(root),sha256=digest),req('editor.snapshot')]);assert not r[-2]['data']['matches'] and r[-2]['data']['sha256']==hashlib.sha256(empty+b' ').hexdigest() and r[-3]==r[-1]
 for invalid in ('', 'x'*64, digest.upper(), digest+'0'):
  r=run(base+[req('asset.check',id=1,root=str(root),sha256=invalid)]);assert r[-1]['error']['code']=='invalid_fingerprint'
 prefab=root/'tree.json';assert run([req('entity.create',name='Tree'),req('prefab.export',id=1,path=str(prefab))])[-1]['ok']
 expected=hashlib.sha256(prefab.read_bytes()).hexdigest()
 registered=[req('asset.register',path='tree.json',kind='prefab')]
 commands=registered+[req('asset.instantiate_checked',id=1,root=str(root),parent=0,sha256=expected),req('editor.snapshot'),req('history.undo'),req('scene.query')]
 r=run(commands);assert r==run(commands,True) and all(x['ok'] for x in r) and r[2]['data']['selection']['entity']['name']=='Tree' and r[-1]['data']['entities']==[]
 prefab.write_bytes(prefab.read_bytes()+b' ')
 r=run(registered+[req('editor.snapshot'),req('asset.instantiate_checked',id=1,root=str(root),parent=0,sha256=expected),req('editor.snapshot')]);assert r[-2]['error']['code']=='asset_source_changed' and r[-3]==r[-1]
 r=run(registered+[req('editor.snapshot'),req('transaction.execute',commands=[{'command':'entity.create','arguments':{'name':'Rollback'}},{'command':'asset.instantiate_checked','arguments':{'id':1,'root':str(root),'parent':0,'sha256':expected}}]),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-3]==r[-1]
print('Passed exact-byte hashes, SHA padding boundaries, source comparison, checked mutation, rollback and MCP parity')
