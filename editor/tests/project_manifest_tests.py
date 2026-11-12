"""Project manifest v1 TOML interoperability, validation and file-effect semantics."""
import tomllib,json,pathlib,subprocess,sys,tempfile,os
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
identity='0123456789abcdef'*2
def write(path,**overrides):
 values=dict(path=str(path),project_id=identity,name='Lykoleon 🐺 # = "Studio" \\ test',directories={});values.update(overrides)
 return req('project.manifest.write',**values)
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp);path=root/'lykoware.toml'
 commands=[req('entity.create',name='Keep'),req('editor.snapshot'),write(path),req('project.manifest.validate',path=str(path)),req('editor.snapshot')]
 rows=run(commands);assert all(r['ok'] for r in rows),rows
 original=path.read_bytes();normalized=rows[-2]['data']
 assert tomllib.loads(original.decode())==normalized and normalized==rows[-3]['data']['manifest']
 assert rows[1]==rows[-1] and list(root.iterdir())==[path],'manifest changed editor state or created directories'
 path.unlink();assert rows==run(commands,True),'CLI/MCP write+validate parity'
 assert path.read_bytes()==original,'deterministic TOML output'
 r=run([write(path)]);assert not r[-1]['ok'] and r[-1]['error']['field']=='path' and path.read_bytes()==original,'overwrite protection'
 descriptors=run([req('commands.list')])[0]['data'];tools={d['name']:d for d in descriptors}
 assert tools['project.manifest.write']['effect']=='filesystem_export' and not tools['project.manifest.write']['undoable']
 assert tools['project.manifest.validate']['effect']=='query'
 for name in ('project.manifest.write','project.manifest.validate'):
  args=write(root/'never.toml')['arguments'] if name.endswith('write') else dict(path=str(path))
  r=run([req('transaction.execute',commands=[dict(command='entity.create',arguments=dict(name='Rollback')),dict(command=name,arguments=args)]),req('scene.query')]);assert not r[0]['ok'] and not r[1]['data']['entities'] and not (root/'never.toml').exists()
 minimal='schema_version = 1\nproject_id = "'+identity+'"\nname = "Studio"\n'
 cases=[minimal,minimal+'[directories]\nscenes = "assets/scenes"\n',minimal.replace('"Studio"',"'Studio # = name'").replace('\n','\r\n'),minimal.replace('"Studio"',r'"\U0001F43A # = \"studio\" \\ test"')]
 for index,text in enumerate(cases):
  source=root/f'input-{index}.toml';source.write_bytes(text.encode())
  r=run([req('project.manifest.validate',path=str(source))]);assert r[-1]['ok'],(text,r)
  parsed=tomllib.loads(text);expected={**normalized,**parsed,'directories':{**normalized['directories'],**parsed.get('directories',{})}}
  assert r[-1]['data']==expected,(r,expected)
  target=root/f'roundtrip-{index}.toml';r=run([write(target,project_id=expected['project_id'],name=expected['name'],directories=expected['directories'])]);assert r[-1]['ok'] and tomllib.loads(target.read_text())==expected
 for bad in ('',minimal+'#bad\x01comment',minimal+'#bad\x7fcomment',minimal.replace('\n','\r'),minimal.replace('\"Studio\"',r'\"\U00110000\"'),minimal.replace('schema_version = 1','schema_version = 2'),minimal+'name = "again"',minimal+'extra = "x"',minimal+'[unknown]\n',minimal+'[directories]\n[directories]\n',minimal+'[directories]\nassets = "a"\nassets = "b"',minimal.replace('name = "Studio"','name = ""'),minimal.replace('"Studio"',r'"\uD800"'),minimal.replace('"Studio"',r'"\uD83D\uDC3A"'),minimal.replace('"Studio"',r'"bad\/escape"'),minimal.replace('"Studio"','"""multiline"""'),minimal.replace('schema_version','"schema_version"'),minimal.replace('1\n','1.0\n'),minimal+'[directories]\nassets = "../outside"',minimal+'[directories]\ncache = "ASSETS/cache"',minimal+'[directories]\nassets = "Cache"\ncache = "cache/data"',minimal+'[directories]\nunknown = "x"'):
  source=root/'bad.toml';source.write_text(bad)
  r=run([req('editor.snapshot'),req('project.manifest.validate',path=str(source)),req('editor.snapshot')]);assert not r[-2]['ok'] and r[-2]['error']['code']=='invalid_project_manifest' and r[0]==r[2],(bad,r)
 source=root/'bytes.toml'
 for content in (b'\xff',minimal.encode()+b'#'+b' '*(1024*1024-len(minimal)-1),minimal.encode()+b'#'+b' '*(1024*1024-len(minimal))):
  source.write_bytes(content);r=run([req('project.manifest.validate',path=str(source))]);assert r[-1]['ok']==(len(content)==1024*1024),r
 for overrides in (dict(project_id='A'*32),dict(project_id='a'*31),dict(name=''),dict(name='x\n'),dict(name='é'*65),dict(directories={'cache':'assets'}),dict(directories={'assets':'CON/file'}),dict(directories={'scenes':'/absolute'}),dict(directories={'scripts':'bad\\path'}),dict(directories={'other':'x'}),dict(directories={'assets':2})):
  target=root/'rejected.toml';r=run([write(target,**overrides)]);assert not r[-1]['ok'] and not target.exists(),(overrides,r)
 assert run([write(root/'boundary.toml',name='é'*64)])[-1]['ok']
 for missing in (root/'missing.toml',root):
  r=run([req('project.manifest.validate',path=str(missing))]);assert r[-1]['error']['field']=='path'
 if os.name!='nt':
  os.mkfifo(root/'pipe.toml');assert not run([req('project.manifest.validate',path=str(root/'pipe.toml'))])[-1]['ok']
 for rejected in ({**write(root/'stale.toml'),'expected_revision':1},{**write(root/'wrong.toml'),'instance':'other'}):
  r=run([rejected]);assert not r[-1]['ok']
 assert not (root/'stale.toml').exists() and not (root/'wrong.toml').exists()
 assert not run([write(root/'absent'/'project.toml')])[-1]['ok'] and not (root/'absent').exists()
print('Passed project TOML interoperability, defaults, Unicode, paths, bounds, no-overwrite/state/history and CLI/MCP parity')
