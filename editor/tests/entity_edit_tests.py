"""Authoring command parity, atomic history, selection and stable IDs."""
import json
import pathlib
import subprocess
import sys
import tempfile

exe = sys.argv[1]
def req(command, **arguments):
    return {'instance': 'test', 'command': command, 'arguments': arguments}

def run(commands, mcp=False):
    messages = commands
    if mcp:
        messages = [
            {'jsonrpc': '2.0', 'id': 0, 'method': 'initialize', 'params': {
                'protocolVersion': '2025-06-18', 'capabilities': {},
                'clientInfo': {'name': 'entity-test', 'version': '1'}}},
            {'jsonrpc': '2.0', 'method': 'notifications/initialized'}]
        for i, command in enumerate(commands, 1):
            arguments = {key: value for key, value in command.items() if key != 'command'}
            messages.append({'jsonrpc': '2.0', 'id': i, 'method': 'tools/call',
                             'params': {'name': command['command'], 'arguments': arguments}})
    p = subprocess.run([exe, '--instance', 'test'] + (['--mcp'] if mcp else []),
                       input='\n'.join(map(json.dumps, messages))+'\n', text=True, capture_output=True)
    assert p.returncode in (0, 1), p.stderr
    rows = [json.loads(line) for line in p.stdout.splitlines()]
    return [row['result']['structuredContent'] for row in rows[1:]] if mcp else rows

commands = [req('entity.create', name='Source'), req('selection.set', id=1),
            req('entity.duplicate', id=1, name='Copy'), req('editor.snapshot'),
            req('history.undo'), req('editor.snapshot'), req('history.redo'),
            req('entity.delete', id=2), req('editor.snapshot'), req('history.undo'),
            req('editor.snapshot'), req('history.redo'), req('entity.create', name='Next'),
            req('scene.query')]
r = run(commands)
assert r == run(commands, True), 'CLI/MCP parity'
assert all(row['ok'] for row in r), r
assert r[3]['data']['selection']['entity'] == {'id': 2, 'name': 'Copy'}, r[3]
assert r[5]['data']['selection']['entity'] == {'id': 1, 'name': 'Source'}, r[5]
assert len(r[5]['data']['scene']['entities']) == 1
assert r[8]['data']['selection']['entity'] is None
assert r[10]['data']['selection']['entity'] == {'id': 2, 'name': 'Copy'}
assert r[13]['data']['entities'] == [{'id': 1, 'name': 'Source'}, {'id': 3, 'name': 'Next'}], 'deleted IDs are not reused'

for invalid in [req('entity.delete', id=0), req('entity.delete', id=999),
                req('entity.duplicate', id=999, name='Missing'),
                req('entity.duplicate', id=1, name=''), req('entity.duplicate', id=1, name='x'*129),
                req('entity.duplicate', id=-1, name='Bad')]:
    rows = run([req('entity.create', name='Source'), req('editor.snapshot'), invalid, req('editor.snapshot')])
    assert not rows[2]['ok'] and rows[1] == rows[3], rows

rows = run([req('entity.create', name='Source'), req('selection.set', id=1),
            req('editor.snapshot'), req('transaction.execute', commands=[
                {'command': 'entity.delete', 'arguments': {'id': 1}},
                {'command': 'entity.duplicate', 'arguments': {'id': 1, 'name': 'Gone'}}]),
            req('editor.snapshot')])
assert not rows[3]['ok'] and rows[2] == rows[4], 'failed batch restores selection, IDs and history'
stale = req('entity.delete', id=1)
stale['expected_revision'] = 0
rows = run([req('entity.create', name='Source'), req('editor.snapshot'), stale, req('editor.snapshot')])
assert rows[2]['error']['code'] == 'stale_revision' and rows[1] == rows[3]
with tempfile.TemporaryDirectory() as temp:
    path = str(pathlib.Path(temp)/'scene.json')
    rows = run([req('entity.create', name='Source'), req('entity.duplicate', id=1, name='Copy'),
                req('entity.delete', id=1), req('scene.save', path=path), req('scene.load', path=path),
                req('scene.query')])
    assert all(row['ok'] for row in rows), rows
    saved = json.loads(pathlib.Path(path).read_text())
    assert saved == {'schema_version': 1, 'next_id': 3, 'entities': [{'id': 2, 'name': 'Copy'}]}
    exhausted = pathlib.Path(temp)/'exhausted.json'
    exhausted.write_text(json.dumps({'schema_version': 1, 'next_id': 18446744073709551615,
                                    'entities': [{'id': 1, 'name': 'Source'}]}))
    rows = run([req('scene.load', path=str(exhausted)), req('editor.snapshot'),
                req('entity.duplicate', id=1, name='No ID'), req('editor.snapshot')])
    assert rows[2]['error']['code'] == 'id_exhausted' and rows[1] == rows[3]
rows = run([req('entity.create', name='Selected'), req('entity.create', name='Other'),
            req('selection.set', id=1), req('entity.delete', id=2), req('selection.query')])
assert rows[-1]['data']['entity'] == {'id': 1, 'name': 'Selected'}, 'unrelated deletion preserves selection'
print('Passed duplicate/delete selection, atomic history, stable IDs, rollback, CLI/MCP parity and persistence')
