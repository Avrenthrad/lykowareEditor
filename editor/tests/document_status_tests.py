"""Persisted document dirty baselines, independent history and project settings."""
import json,pathlib,subprocess,sys,tempfile,os
exe=sys.argv[1]
def req(command,**arguments):return {'instance':'test','command':command,'arguments':arguments}
def run(commands,mcp=False):
 messages=commands
 if mcp:
  messages=[{'jsonrpc':'2.0','id':0,'method':'initialize','params':{'protocolVersion':'2025-06-18','capabilities':{},'clientInfo':{'name':'project-test','version':'1'}}},{'jsonrpc':'2.0','method':'notifications/initialized'}]+[{'jsonrpc':'2.0','id':i,'method':'tools/call','params':{'name':c['command'],'arguments':{'instance':'test','arguments':c['arguments']}}} for i,c in enumerate(commands,1)]
 p=subprocess.run([exe,'--instance','test']+(['--mcp'] if mcp else []),input='\n'.join(map(json.dumps,messages))+'\n',text=True,capture_output=True,timeout=20)
 assert p.returncode in (0,1),p.stderr
 rows=[json.loads(line) for line in p.stdout.splitlines()]
 return [row['result']['structuredContent'] for row in rows[1:]] if mcp else rows
def docs(row):return row['data']['documents']
with tempfile.TemporaryDirectory() as temp:
 root=pathlib.Path(temp);scene=root/'scene.json'
 commands=[req('document.status'),req('entity.create',name='A'),req('document.status'),req('scene.save',path=str(scene)),req('document.status'),req('entity.rename',id=1,name='B'),req('document.status'),req('history.undo'),req('document.status'),req('history.undo'),req('document.status'),req('history.redo'),req('document.status')]
 r=run(commands);assert all(x['ok'] for x in r),r
 assert not docs(r[0])['scene']['dirty'] and docs(r[0])['scene']['unsaved']
 assert docs(r[2])['scene']['dirty'] and not docs(r[4])['scene']['dirty'] and not docs(r[4])['scene']['unsaved'] and docs(r[4])['scene']['path']==scene.resolve().as_posix()
 assert docs(r[6])['scene']['dirty'] and not docs(r[8])['scene']['dirty'] and docs(r[10])['scene']['dirty'] and not docs(r[12])['scene']['dirty']
 # Runtime and selection are not scene/workspace authoring changes.
 r=run([req('scene.load',path=str(scene)),req('selection.set',id=1),req('play.start'),req('play.input',value='jump'),req('document.status')]);assert not docs(r[-1])['scene']['dirty'] and not docs(r[-1])['workspace']['dirty']
 # Failed export cannot reset the last clean baseline.
 r=run([req('scene.load',path=str(scene)),req('entity.rename',id=1,name='Changed'),req('document.status'),req('scene.save',path=str(scene)),req('document.status')]);assert not r[-2]['ok'] and r[-3]==r[-1]
 # A load followed by edit in one successful transaction keeps the loaded baseline.
 batch=[dict(command='scene.load',arguments={'path':str(scene)}),dict(command='entity.rename',arguments={'id':1,'name':'After load'})]
 r=run([req('transaction.execute',commands=batch),req('document.status')]);assert r[0]['ok'] and docs(r[-1])['scene']['dirty']
 batch.append(dict(command='entity.delete',arguments={'id':999}))
 r=run([req('document.status'),req('transaction.execute',commands=batch),req('document.status')]);assert not r[1]['ok'] and r[0]==r[2]
 catalog=root/'catalog.json';workspace=root/'workspace.toml'
 r=run([req('asset.register',path='tree.json',kind='prefab'),req('document.status'),req('asset.catalog.save',path=str(catalog)),req('workspace.configure',scale_percent=150,theme='dark',reduced_motion=False),req('workspace.save',path=str(workspace)),req('document.status')]);assert docs(r[1])['catalogue']['dirty'] and not docs(r[-1])['catalogue']['dirty'] and not docs(r[-1])['workspace']['dirty']
 r=run([req('workspace.load',path=str(workspace)),req('workspace.configure',scale_percent=100,theme='dark',reduced_motion=False),req('document.status'),req('history.undo'),req('document.status')]);assert docs(r[2])['workspace']['dirty'] and not docs(r[4])['workspace']['dirty']
 project=root/'lykoware.toml';project.write_text('schema_version = 1\nproject_id = "0123456789abcdef0123456789abcdef"\nname = "Original"\n')
 base=[req('project.open',path=str(project))]
 commands=base+[req('document.status'),req('project.configure',name='Edited',directories={}),req('document.status'),req('project.close'),req('project.settings.revert'),req('document.status'),req('history.undo'),req('document.status')]
 r=run(commands);assert r==run(commands,True) and all(x['ok'] for i,x in enumerate(r) if i!=4),r
 assert not docs(r[1])['project']['dirty'] and docs(r[3])['project']['dirty'] and r[4]['error']['code']=='unsaved_project_settings' and not docs(r[6])['project']['dirty'] and docs(r[8])['project']['dirty']
 # Even a newly staged project/configure/close must preserve unsaved settings.
 batch=[dict(command='project.open',arguments={'path':str(project)}),dict(command='project.configure',arguments={'name':'Dirty','directories':{}}),dict(command='project.close',arguments={})]
 r=run([req('editor.snapshot'),req('transaction.execute',commands=batch),req('editor.snapshot')]);assert r[1]['error']['code']=='unsaved_project_settings' and r[0]==r[2]
print('Passed document baselines, successful/failed saves, undo/redo cleanliness, grouped loads and project-settings guards')
