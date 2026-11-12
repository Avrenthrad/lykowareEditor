"""Dock-tree validation, atomic history and v1/v2 workspace persistence."""
import copy
import json
import pathlib
import subprocess
import sys
import tempfile
import tomllib

exe=sys.argv[1]
def req(command_name, **args):
    return {'instance':'test','command':command_name,'arguments':args}
def run(commands):
    p=subprocess.run([exe,'--instance','test'],input='\n'.join(map(json.dumps,commands))+'\n',text=True,capture_output=True)
    r=[json.loads(line) for line in p.stdout.splitlines()]
    if len(r)!=len(commands): raise AssertionError(p)
    return r
def check(value,message):
    if not value: raise AssertionError(message)
def tabs(panels,active=None):
    return {'kind':'tabs','panels':panels,'active':active if active is not None else (panels[0] if panels else '')}
def split(axis,ratio,a,b):
    return {'kind':'split','axis':axis,'ratio':ratio,'first':a,'second':b}
custom={'root':'main','hidden':[],'nodes':{
    'main':split('vertical',6500,'tools','work'),
    'tools':split('horizontal',3000,'a','b'),
    'a':tabs(['scene','inspector'],'inspector'),
    'b':tabs(['history','console'],'console'),
    'work':tabs(['viewport'])}}
r=run([req('layout.query'),req('layout.replace',layout=custom),req('layout.query'),req('history.query'),
       req('history.undo'),req('layout.query'),req('history.redo'),req('layout.query')])
check(all(x['ok'] for x in r),r)
initial=r[0]['data']; canonical=r[2]['data']
check(canonical['root']=='root' and canonical['nodes']['root']['ratio']==6500,canonical)
check(canonical['nodes']['root-0-0']['panels']==['scene','inspector'],canonical)
check(r[3]['data']['entries'][0]['commands']==['layout.replace'],r)
check(r[5]['data']==initial and r[7]['data']==canonical,'layout undo/redo')
r=run([req('layout.replace',layout=canonical),req('layout.replace',layout=canonical),
       req('layout.query'),req('layout.reset'),req('layout.query'),req('history.undo'),req('layout.query')])
check(r[1]['revision']==r[0]['revision']==1,'identical replacements are no-ops')
check(r[4]['data']==initial and r[6]['data']==canonical,'reset undoable')
invalid=[]
for mutate in [
    lambda x:x['nodes']['main'].update(first='main'),
    lambda x:x['nodes']['main'].update(second='tools'),
    lambda x:x['nodes']['main'].update(axis='diagonal'),
    lambda x:x['nodes']['main'].update(ratio=0),
    lambda x:x['nodes']['main'].update(ratio=9501),
    lambda x:x['nodes'].update(orphan=tabs([])),
    lambda x:x['nodes']['a'].update(panels=['scene','scene']),
    lambda x:x['nodes']['a'].update(active='viewport'),
    lambda x:x.update(hidden=['scene']),
    lambda x:x['nodes']['work'].update(panels=[] ,active=''),
    lambda x:x['nodes']['a'].update(unexpected=True),
    lambda x:x.update(root='missing'),
]:
    item=copy.deepcopy(custom);mutate(item);invalid.append(item)
deep={'root':'n0','hidden':[],'nodes':{}}
for i in range(10):
    deep['nodes'][f'n{i}']=split('horizontal',5000,f'n{i+1}',f'e{i}')
    deep['nodes'][f'e{i}']=tabs([])
deep['nodes']['n10']=tabs(['scene','inspector','viewport','console','history'])
invalid.append(deep)
for bad in invalid:
    r=run([req('layout.query'),req('layout.replace',layout=bad),req('layout.query'),req('history.query')])
    check(not r[1]['ok'] and r[2]==r[0] and not r[3]['data']['entries'],r)
r=run([req('transaction.execute',commands=[{'command':'entity.create','arguments':{'name':'Rollback'}},
                                        {'command':'layout.replace','arguments':{'layout':invalid[0]}}]),req('scene.query')])
check(not r[0]['ok'] and r[1]['revision']==0 and r[1]['data']['entities']==[],'batch parse failure is atomic')
r=run([req('layout.replace',layout=custom),{**req('layout.replace',layout=initial),'expected_revision':0},req('layout.query')])
check(not r[1]['ok'] and r[1]['error']['code']=='stale_revision' and r[2]['data']==canonical,'stale layout edit rejected')
with tempfile.TemporaryDirectory() as temp:
    path=pathlib.Path(temp)/'workspace.toml'
    r=run([req('layout.replace',layout=custom),req('workspace.save',path=str(path))])
    check(all(x['ok'] for x in r),r)
    text=path.read_text(); parsed=tomllib.loads(text)
    check(parsed['schema_version']==2 and parsed['layout']['nodes']['root']['ratio']==6500,parsed)
    r=run([req('workspace.load',path=str(path)),req('layout.query')])
    check(r[1]['data']==canonical,r)
    # Genuine legacy v1 data uses the starter tables; no embedded custom graph.
    legacy=pathlib.Path(temp)/'legacy.toml'
    legacy.write_text(text.split('\n[layout]\n')[0].replace('schema_version = 2','schema_version = 1'))
    r=run([req('layout.replace',layout=custom),req('workspace.load',path=str(legacy)),req('layout.query')])
    check(r[1]['ok'] and r[2]['data']==initial,'v1 workspace migration')
    bad=pathlib.Path(temp)/'bad.toml';bad.write_text(text.replace('ratio = 6500','ratio = 0'))
    r=run([req('layout.replace',layout=custom),req('workspace.load',path=str(bad)),req('layout.query')])
    check(not r[1]['ok'] and r[2]['data']==canonical,'invalid persisted tree leaves layout unchanged')
print('Passed dock-tree validation/canonicalization, atomic history, stale edits and independent TOML v1/v2 round trips')

# Empty regions collapse, while hiding all panels remains a usable empty root.
known=['scene','inspector','viewport','console','history']
empty_region={'root':'top','hidden':[],'nodes':{'top':split('horizontal',5000,'blank','content'),'blank':tabs([]),'content':tabs(known,'viewport')}}
r=run([req('layout.replace',layout=empty_region),req('layout.query')])
check(r[1]['data']=={'root':'root','hidden':[],'nodes':{'root':tabs(known,'viewport')}},'empty split regions normalize without dropping panels')
all_hidden={'root':'root','hidden':known,'nodes':{'root':tabs([])}}
r=run([req('layout.replace',layout=all_hidden),req('workspace.query'),req('layout.reset'),req('layout.query'),req('history.undo'),req('layout.query')])
check(all(x['ok'] for x in r),r)
check(set(r[1]['data']['layout']['hidden'])==set(known) and r[3]['data']==initial and set(r[5]['data']['hidden'])==set(known),'empty layout reset/undo')
r=run([req('panel.dock',panel='scene',dock='hidden'),req('layout.query')])
check(r[1]['data']['hidden']==['scene'] and all(n['panels'] for n in r[1]['data']['nodes'].values() if n['kind']=='tabs'),'starter hidden panel leaves no dead split region')
print('Passed hidden-panel layouts, empty-region normalization and reset/undo')
