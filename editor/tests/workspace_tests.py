"""Test shell behavior through public commands without a GUI dependency."""
import json
import pathlib
import subprocess
import sys
import tempfile

exe = sys.argv[1]
def req(command_name, **arguments):
    return {'instance': 'test', 'command': command_name, 'arguments': arguments}
def run(commands):
    p = subprocess.run([exe, '--instance', 'test'], input='\n'.join(map(json.dumps, commands)) + '\n', text=True, capture_output=True)
    replies = [json.loads(line) for line in p.stdout.splitlines()]
    if len(replies) != len(commands):
        raise AssertionError(p)
    return replies

def check(condition, message):
    if not condition:
        raise AssertionError(message)

def contrast(a, b):
    def luminance(color):
        rgb = [int(color[i:i+2], 16) / 255 for i in (1, 3, 5)]
        rgb = [x / 12.92 if x <= 0.04045 else ((x + .055) / 1.055) ** 2.4 for x in rgb]
        return sum(x * weight for x, weight in zip(rgb, (.2126, .7152, .0722)))
    values = sorted([luminance(a), luminance(b)])
    return (values[1] + .05) / (values[0] + .05)

r = run([req('workspace.query'), req('design.tokens'), req('commands.list'), req('palette.search', query=''),
         req('workspace.configure', scale_percent=200, theme='high_contrast', reduced_motion=True),
         req('design.tokens'), req('history.undo'), req('workspace.query'), req('history.redo'), req('workspace.query')])
check(all(x['ok'] for x in r), r)
check(r[0]['data']['panels']['viewport'] == 'center' and r[0]['revision'] == 0, r[0])
check({x['name'] for x in r[2]['data']} == {x['name'] for x in r[3]['data']}, 'all commands discoverable')
check(next(x for x in r[3]['data'] if x['name'] == 'palette.search')['shortcut'] == 'Ctrl+K', 'default palette shortcut')
check(r[5]['data']['typography']['body_px'] == 30 and r[5]['data']['motion']['transition_ms'] == 0, 'scaled and reduced motion tokens')
check(r[7]['data'] == r[0]['data'] and r[9]['data']['scale_percent'] == 200, 'settings undo/redo')
for tokens, target in [(r[1]['data'], 4.5), (r[5]['data'], 7)]:
    colors = tokens['colors']
    for text in ('text', 'muted_text'):
        for background in ('background', 'surface', 'surface_hover', 'surface_active'):
            check(contrast(colors[text], colors[background]) >= target, ('contrast', text, background))

r = run([req('entity.create', name='Player'), req('selection.set', id=1), req('selection.query'),
         req('transaction.execute', commands=[
             {'command': 'panel.dock', 'arguments': {'panel': 'inspector', 'dock': 'bottom'}},
             {'command': 'layout.resize', 'arguments': {'dock': 'left', 'percent': 30}},
             {'command': 'keymap.bind', 'arguments': {'command': 'history.undo', 'shortcut': 'Ctrl+U'}}]),
         req('workspace.query'), req('palette.search', query='UNDO'), req('history.undo'), req('workspace.query'),
         req('history.redo'), req('workspace.query')])
check(all(x['ok'] for x in r), r)
check(r[2]['data']['entity'] == {'id': 1, 'name': 'Player'}, 'inspector selection')
check(r[4]['data']['panels']['inspector'] == 'bottom' and r[4]['data']['splits']['left'] == 30, 'panel layout transaction')
check(r[5]['data'][0]['name'] == 'history.undo' and r[5]['data'][0]['shortcut'] == 'Ctrl+U', 'palette uses live rebindings')
check(r[7]['data']['keymap']['history.undo'] == 'Ctrl+Z' and r[9]['data'] == r[4]['data'], 'workspace batch history')

invalid = [req('workspace.configure', scale_percent=74, theme='dark', reduced_motion=False),
           req('workspace.configure', scale_percent=100, theme='unknown', reduced_motion=False),
           req('workspace.configure', scale_percent=100, theme='dark', reduced_motion='yes'),
           req('panel.dock', panel='unknown', dock='center'), req('panel.dock', panel='scene', dock='floating'),
           req('layout.resize', dock='center', percent=20), req('layout.resize', dock='left', percent=51),
           req('layout.resize', dock='left', percent=50),
           req('keymap.bind', command='history.undo', shortcut='Ctrl+K'),
           req('keymap.bind', command='unknown', shortcut='Ctrl+U'),
           req('keymap.bind', command='history.undo', shortcut='Ctrl+Ctrl+U'), req('selection.set', id=99),
           req('palette.search', query='x' * 129)]
for item in invalid:
    r = run([req('workspace.query'), item, req('workspace.query')])
    check(not r[1]['ok'] and r[2] == r[0], (item, r))

with tempfile.TemporaryDirectory() as temp:
    path = pathlib.Path(temp) / 'scene.json'
    path.write_text(json.dumps({'schema_version': 1, 'next_id': 8, 'entities': [{'id': 7, 'name': 'Loaded'}]}))
    r = run([req('workspace.configure', scale_percent=150, theme='high_contrast', reduced_motion=True),
             req('entity.create', name='Player'), req('selection.set', id=1), req('workspace.query'),
             req('scene.load', path=str(path)), req('workspace.query'), req('selection.query'),
             req('history.undo'), req('workspace.query'), req('selection.query')])
    check(all(x['ok'] for x in r), r)
    check(r[5]['data'] == {**r[3]['data'], 'selection': None}, 'load preserves workspace and clears selection')
    check(r[6]['data']['entity'] is None and r[8]['data'] == r[3]['data'], 'undo restores selection and workspace')
print('Passed workspace transactions, inspector selection, palette coverage/rebinding, theme contrast and scene-load isolation')

# Validate emitted configuration with Python's independent TOML implementation.
import tomllib
with tempfile.TemporaryDirectory() as temp:
    path = pathlib.Path(temp) / 'workspace-界面.toml'
    r = run([req('workspace.configure', scale_percent=150, theme='high_contrast', reduced_motion=True),
             req('panel.dock', panel='inspector', dock='bottom'), req('layout.resize', dock='left', percent=30),
             req('entity.create', name='Player'), req('selection.set', id=1),
             req('workspace.query'), req('workspace.save', path=str(path))])
    check(all(x['ok'] for x in r), r)
    expected = r[5]['data']
    check(r[6]['revision'] == r[5]['revision'], 'save is an export, not a document mutation')
    original = path.read_bytes()
    saved = tomllib.loads(original.decode())
    check(saved['scale_percent'] == 150 and saved['panels']['inspector'] == 'bottom', saved)
    check('selection' not in saved and 'entities' not in saved, 'config contains no scene data')
    r = run([req('entity.create', name='Existing'), req('selection.set', id=1), req('workspace.query'),
             req('workspace.load', path=str(path)), req('workspace.query'), req('selection.query'),
             req('history.undo'), req('workspace.query'), req('history.redo'), req('workspace.query'),
             req('workspace.save', path=str(path))])
    check(r[4]['data'] == expected and r[5]['data']['entity']['name'] == 'Existing', 'config load preserves scene and selection')
    check(r[7]['data'] == r[2]['data'] and r[9]['data'] == expected, 'config undo/redo')
    check(not r[10]['ok'] and path.read_bytes() == original, 'config save preserves existing file')
    bad_configs = [
        original.decode().replace('schema_version = 2', 'schema_version = 999'),
        original.decode() + '\n[panels]\n',
        original.decode().replace('scale_percent = 150', 'scale_percent = 999'),
        original.decode().replace('"play.start" = "F5"', '"play.start" = "Ctrl+K"'),
        original.decode().replace('\n[layout]\n', '\n"unknown.command" = "Ctrl+U"\n\n[layout]\n'),
        original.decode().replace('theme = "high_contrast"', 'theme = "high_contrast"\ntheme = "dark"'),
    ]
    for text in bad_configs:
        broken = pathlib.Path(temp) / 'bad.toml'
        broken.write_text(text)
        r = run([req('workspace.query'), req('workspace.load', path=str(broken)), req('workspace.query')])
        check(not r[1]['ok'] and r[2] == r[0], (text, r))
    r = run([req('workspace.load', path=str(path) + '\x00suffix'), req('workspace.save', path=str(path) + '\x00suffix')])
    check(not r[0]['ok'] and not r[1]['ok'] and path.read_bytes() == original, 'embedded-null paths rejected')
    check(not list(pathlib.Path(temp).glob('.lyko-save-*')), 'config staging cleaned')
print('Passed independent TOML parsing, layout/keymap round trip, undo/redo, scene isolation and invalid config rollback')

# Shell commands have identical semantics over MCP and the CLI.
messages = [
    {"jsonrpc": "2.0", "id": 1, "method": "initialize", "params": {"protocolVersion": "2025-06-18", "capabilities": {}, "clientInfo": {"name": "workspace-test", "version": "1"}}},
    {"jsonrpc": "2.0", "method": "notifications/initialized"},
    {"jsonrpc": "2.0", "id": 2, "method": "tools/call", "params": {"name": "workspace.configure", "arguments": {"instance": "test", "arguments": {"scale_percent": 150, "theme": "high_contrast", "reduced_motion": True}}}},
    {"jsonrpc": "2.0", "id": 3, "method": "tools/call", "params": {"name": "design.tokens", "arguments": {"instance": "test"}}},
]
p = subprocess.run([exe, '--instance', 'test', '--mcp'], input='\n'.join(map(json.dumps, messages)) + '\n', text=True, capture_output=True)
replies = [json.loads(line) for line in p.stdout.splitlines()]
cli = run([req('workspace.configure', scale_percent=150, theme='high_contrast', reduced_motion=True), req('design.tokens')])
check(p.returncode == 0 and len(replies) == 3 and replies[-1]['result']['structuredContent'] == cli[-1], 'MCP/CLI shell parity')
print('Passed MCP/CLI workspace and design-token parity')
