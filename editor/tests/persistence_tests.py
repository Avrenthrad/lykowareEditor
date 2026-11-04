"""Exercise real filesystem publication and transactional scene loading."""
import json
import pathlib
import subprocess
import sys
import tempfile

exe = sys.argv[1]
def req(command, **args):
    return {'instance': 'test', 'command': command, 'arguments': args}
def run(commands):
    p = subprocess.run([exe, '--instance', 'test'], input='\n'.join(map(json.dumps, commands)) + '\n', text=True, capture_output=True)
    r = [json.loads(line) for line in p.stdout.splitlines()]
    if len(r) != len(commands):
        raise AssertionError(p)
    return r

def check(value, message):
    if not value:
        raise AssertionError(message)

with tempfile.TemporaryDirectory() as temp:
    path = pathlib.Path(temp) / 'scene.json'
    r = run([req('entity.create', name='Player'), req('play.start'),
             req('scene.save', path=str(path))])
    check(all(x['ok'] for x in r), r)
    original = path.read_bytes()
    scene = json.loads(original)
    check(set(scene) == {'schema_version', 'entities', 'next_id'}, scene)
    check(b'\n    {' in original, 'one entity per line for diffs')
    r = run([req('scene.load', path=str(path)), req('scene.query'),
             req('history.undo'), req('scene.query'), req('history.redo'), req('scene.query'),
             req('scene.save', path=str(path))])
    check(r[1]['data']['entities'] == [{'id': 1, 'name': 'Player'}], r)
    check(not r[1]['data']['runtime']['playing'], 'runtime not persisted')
    check(r[3]['data']['entities'] == [], 'load undo')
    check(r[5]['data'] == r[1]['data'], 'load redo')
    check(not r[6]['ok'] and path.read_bytes() == original, 'existing file preserved')
    invalid = [
        {**scene, 'schema_version': 2}, {**scene, 'next_id': 1},
        {**scene, 'entities': scene['entities'] * 2}, {**scene, 'unexpected': True},
        {**scene, 'entities': [{'id': 1, 'name': ''}]},
    ]
    for bad in invalid:
        broken = pathlib.Path(temp) / 'bad.json'
        broken.write_text(json.dumps(bad))
        r = run([req('entity.create', name='Existing'), req('scene.load', path=str(broken)), req('scene.query')])
        check(not r[1]['ok'] and r[2]['revision'] == 1, r)
        check(r[2]['data']['entities'] == [{'id': 1, 'name': 'Existing'}], r)
    check(not list(pathlib.Path(temp).glob('.lyko-save-*')), 'staging cleaned up')
print('Passed authoring round trip, undo/redo, no-overwrite publication and invalid-load rollback')
