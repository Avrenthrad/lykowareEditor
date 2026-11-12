"""Coherent panel snapshots and named, branch-aware transaction history."""
import json
import subprocess
import sys

exe = sys.argv[1]
def req(command_name, **arguments):
    return {'instance': 'test', 'command': command_name, 'arguments': arguments}
def check(condition, message):
    if not condition:
        raise AssertionError(message)

batch = [
    {'command': 'entity.create', 'arguments': {'name': 'Player'}},
    {'command': 'selection.set', 'arguments': {'id': 1}},
    {'command': 'workspace.configure', 'arguments': {'scale_percent': 150, 'theme': 'high_contrast', 'reduced_motion': True}},
    {'command': 'play.start'}, {'command': 'play.input', 'arguments': {'value': 'right'}}]
requests = [req('editor.snapshot'), req('transaction.execute', commands=batch), req('editor.snapshot'),
            req('history.query'), req('history.undo'), req('editor.snapshot'),
            req('entity.rename', id=99, name='Missing'), req('history.query'), req('history.redo'), req('editor.snapshot'),
            req('history.undo'), req('entity.create', name='Branch'), req('history.query'), req('editor.snapshot'),
            req('workspace.configure', scale_percent=100, theme='dark', reduced_motion=False), req('history.query'),
            {**req('editor.snapshot'), 'expected_revision': 0}]
p = subprocess.run([exe, '--instance', 'test'], input='\n'.join(map(json.dumps, requests)) + '\n', text=True, capture_output=True)
r = [json.loads(line) for line in p.stdout.splitlines()]
check(p.returncode == 1 and len(r) == len(requests), p)
check(all(x['ok'] for i, x in enumerate(r) if i not in (6, 16)), r)
check(not r[6]['ok'] and not r[16]['ok'] and r[16]['error']['code'] == 'stale_revision', r)
initial = r[0]['data']
check(initial['history'] == {'can_undo': False, 'can_redo': False, 'cursor': 0, 'entries': []}, initial)
frame = r[2]['data']
check(r[2]['revision'] == 1 and frame['scene']['entities'] == [{'id': 1, 'name': 'Player'}], frame)
check(frame['selection']['entity'] == {'id': 1, 'name': 'Player'} and frame['workspace']['selection'] == 1, frame)
check(frame['tokens']['scale_percent'] == frame['workspace']['scale_percent'] == 150, frame)
check(frame['tokens']['theme'] == frame['workspace']['theme'] == 'high_contrast', frame)
check(frame['logs']['messages'] == ['mock: play started', 'mock: input right'], frame)
check(frame['history'] == r[3]['data'] and frame['history']['cursor'] == 1, frame)
entry = frame['history']['entries'][0]
check(entry == {'revision': 1, 'commands': [x['command'] for x in batch], 'mutation_count': 5, 'applied': True}, entry)
undone = r[5]['data']
check(undone['scene'] == initial['scene'] and undone['workspace'] == initial['workspace'], undone)
check(undone['selection'] == initial['selection'] and undone['logs'] == initial['logs'], undone)
check(undone['history']['can_redo'] and not undone['history']['entries'][0]['applied'], undone)
check(r[7]['data'] == undone['history'] and r[9]['data'] == frame, 'failure preserves redo; redo restores full frame')
branch = r[12]['data']
check(branch['entries'] == [{'revision': 5, 'commands': ['entity.create'], 'mutation_count': 1, 'applied': True}], branch)
check(not branch['can_redo'] and r[13]['data']['scene']['entities'] == [{'id': 1, 'name': 'Branch'}], branch)
check(r[14]['revision'] == 5 and r[15]['data'] == branch, 'no-op settings do not add history')
print('Passed coherent panel snapshots, named batch history, undo/redo, failed-edit preservation and branch/no-op behavior')
