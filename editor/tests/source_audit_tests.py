"""Catalogue-wide source reports preserve state and collect independent failures."""
import hashlib,json,pathlib,subprocess,sys,tempfile,os
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
 root=pathlib.Path(temp)/'project';root.mkdir()
 scene=root/'z-scène.json';scene.write_text(json.dumps({'schema_version':1,'next_id':1,'entities':[]}),encoding='utf8')
 prefab=root/'tree.json';assert run([req('entity.create',name='Tree'),req('prefab.export',id=1,path=str(prefab))])[-1]['ok']
 (root/'bad.json').write_text('not json')
 (root/'schema.json').write_text('{"schema_version":99}')
 (root/'large.json').write_bytes(b' '*(1024*1024+1))
 registrations=[('z-scène.json','scene'),('missing.json','prefab'),('unread.png','texture'),('tree.json','prefab'),('bad.json','scene'),('schema.json','scene'),('large.json','scene')]
 base=[req('asset.register',path=p,kind=k) for p,k in registrations]
 audit=req('asset.audit',root=str(root))
 commands=base+[req('editor.snapshot'),audit,audit,req('editor.snapshot')]
 rows=run(commands);assert rows==run(commands,True),'CLI/MCP parity'
 assert rows[-4]==rows[-1] and rows[-3]==rows[-2],'audit changed state or order'
 report=rows[-3]['data'];assert rows[-3]['ok']
 assert {k:report[k] for k in ('total','valid','invalid','skipped')}==dict(total=7,valid=2,invalid=4,skipped=1),report
 entries=report['entries'];assert [r['id'] for r in entries]==list(range(1,8))
 assert [r['status'] for r in entries]==['valid','invalid','skipped','valid','invalid','invalid','invalid']
 assert entries[0]['sha256']==hashlib.sha256(scene.read_bytes()).hexdigest() and entries[0]['entities']==0
 assert entries[3]['sha256']==hashlib.sha256(prefab.read_bytes()).hexdigest() and entries[3]['entities']==1
 assert entries[2]['reason']=='unsupported_asset_kind' and 'sha256' not in entries[2]
 for index in (1,4,5,6):
  assert entries[index]['error']['code']=='invalid_asset_source' and entries[index]['error']['field']=='path'
 # Root validation is unconditional, even with no supported entries.
 for before in ([],base,[req('asset.register',path='x.gltf',kind='mesh')]):
  for directory in ('relative',str(root/'absent'),str(scene)):
   r=run(before+[req('editor.snapshot'),req('asset.audit',root=directory),req('editor.snapshot')]);assert r[-2]['error']['field']=='root' and r[-3]==r[-1]
 empty=run([audit])[-1]['data'];assert empty==dict(total=0,valid=0,invalid=0,skipped=0,entries=[])
 # A deleted catalogue ID is omitted, and unsupported sources need not exist.
 r=run(base+[req('asset.forget',id=2),audit]);assert [x['id'] for x in r[-1]['data']['entries']]==[1,3,4,5,6,7]
 # Failed preconditions stop before filesystem reads; queries cannot be batched as mutations.
 stale={**audit,'expected_revision':0}
 r=run(base+[req('editor.snapshot'),stale,req('transaction.execute',commands=[{'command':'asset.audit','arguments':{'root':str(root)}}]),req('editor.snapshot')]);assert not r[-3]['ok'] and not r[-2]['ok'] and r[-4]==r[-1]
 if os.name!='nt':
  outside=pathlib.Path(temp)/'outside.json';outside.write_bytes(scene.read_bytes())
  (root/'link.json').symlink_to(outside)
  r=run([req('asset.register',path='link.json',kind='scene'),audit]);assert r[-1]['data']['entries'][0]['error']['field']=='path'
  os.mkfifo(root/'pipe.json')
  r=run([req('asset.register',path='pipe.json',kind='scene'),audit]);assert r[-1]['data']['invalid']==1
 # Audits observe new bytes on each invocation without a persistent baseline.
 scene.write_bytes(scene.read_bytes()+b' ')
 r=run(base+[audit]);assert r[-1]['data']['entries'][0]['sha256']!=entries[0]['sha256']
print('Passed mixed source audit, deterministic ordering, roots, containment, state/history and CLI/MCP parity')
