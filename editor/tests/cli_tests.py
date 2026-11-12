"""Run the real JSON frontend and verify observable rollback and history."""
import json
import subprocess
import sys

exe = sys.argv[1]
def request(command, arguments=None, **extra):
    return {"instance": "test", "command": command, "arguments": arguments or {}, **extra}
def run(lines):
    text = '\n'.join(x if isinstance(x, str) else json.dumps(x) for x in lines) + '\n'
    process = subprocess.run([exe, '--instance', 'test'], input=text, text=True, capture_output=True)
    responses = [json.loads(line) for line in process.stdout.splitlines()]
    if len(responses) != len(lines):
        raise AssertionError(process)
    return process.returncode, responses

def check(condition, message):
    if not condition:
        raise AssertionError(message)

code, r = run([
    request('commands.list'),
    request('transaction.execute', {'commands': [
        {'command': 'entity.create', 'arguments': {'name': 'Player 😀'}},
        {'command': 'play.start'}, {'command': 'play.input', 'arguments': {'value': 'jump'}}]}),
    request('scene.query'), request('history.undo'), request('scene.query'),
    request('history.redo'), request('scene.query')])
check(code == 0, r)
check(len(r[0]['data']) == len({d['name'] for d in r[0]['data']}), 'unique registry')
check(r[2]['revision'] == 1 and r[2]['data']['runtime']['inputs'] == ['jump'], r)
check(r[4]['data']['entities'] == [] and r[4]['data']['runtime']['inputs'] == [], r)
check(r[6]['data'] == r[2]['data'], 'redo restores exact state')

invalid = [
    '{', '{"instance":"test","instance":"test"}',
    request('play.input', {'value': 'jump'}), request('entity.create', {'name': ''}),
    request('entity.rename', {'id': -1, 'name': 'A'}),
    request('play.start', {'unexpected': True}),
    request('unknown'), request('play.start', expected_revision=12),
    {**request('play.start'), 'instance': 'other'},
    request('transaction.execute', {'commands': [{'command': 'entity.create', 'arguments': {'name': 'A'}},
                                              {'command': 'entity.rename', 'arguments': {'id': 99, 'name': 'B'}}]}),
    request('transaction.execute', {'commands': [{'command': 'scene.query'}]}),
    '{"instance":"test","command":"entity.create","arguments":{"name":"\\ud800"}}',
    '[' * 66 + ']' * 66,
    request('entity.create', {'name': 'A', 'typo': 1}),
    request('entity.create', {'name': 18446744073709551616}),
]
for item in invalid:
    code, r = run([item, request('scene.query')])
    check(code == 1 and not r[0]['ok'], (item, r))
    check(r[1]['revision'] == 0 and r[1]['data']['entities'] == [], (item, r))
print('Passed JSON CLI discovery, Unicode, atomic feedback/undo/redo and 15 rejection cases')
