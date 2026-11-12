"""Agents can capture and compare simulated visual feedback without a window."""
import json
import subprocess
import sys

def req(command, **args):
    return {'instance': 'test', 'command': command, 'arguments': args}
commands = [req('entity.create', name='Player'), req('preview.capture'), req('play.start'),
            req('play.input', value='right'), req('preview.capture'), req('logs.query'),
            req('history.undo'), req('preview.capture'), req('logs.query'),
            req('history.redo'), req('preview.capture')]
p = subprocess.run([sys.argv[1], '--instance', 'test'], input='\n'.join(map(json.dumps, commands)) + '\n', text=True, capture_output=True)
r = [json.loads(x) for x in p.stdout.splitlines()]
if p.returncode != 0 or len(r) != len(commands) or not all(x['ok'] for x in r):
    raise AssertionError(p)
initial, moved, undone, redone = [r[i]['data'] for i in (1, 4, 7, 10)]
if initial == moved or moved == undone or moved != redone:
    raise AssertionError('Preview responds to input and follows undo/redo')
ppm = moved['ppm'].splitlines()
values = ' '.join(line for line in ppm if not line.startswith('#')).split()
if values[:4] != ['P3', '64', '36', '255'] or len(values[4:]) != 64 * 36 * 3:
    raise AssertionError('Invalid PPM image')
if not all(0 <= int(x) <= 255 for x in values[4:]):
    raise AssertionError('Invalid pixel range')
if r[5]['data']['messages'] != ['mock: play started', 'mock: input right'] or r[8]['data']['messages'] != ['mock: play started']:
    raise AssertionError('Logs follow transaction history')
print('Passed mock play/input/logs/software capture loop and visual undo/redo')
