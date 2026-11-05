"""Exercise MCP inside the actual rendered desktop process, not a second instance."""
import json
import subprocess
import sys
messages = [
    {'jsonrpc': '2.0', 'id': 1, 'method': 'initialize', 'params': {'protocolVersion': '2025-06-18', 'capabilities': {}, 'clientInfo': {'name': 'desktop-test', 'version': '1'}}},
    {'jsonrpc': '2.0', 'method': 'notifications/initialized'},
    {'jsonrpc': '2.0', 'id': 2, 'method': 'tools/call', 'params': {'name': 'workspace.configure', 'arguments': {'instance': 'desktop-test', 'arguments': {'scale_percent': 125, 'theme': 'high_contrast', 'reduced_motion': True}}}},
    {'jsonrpc': '2.0', 'id': 3, 'method': 'tools/call', 'params': {'name': 'design.tokens', 'arguments': {'instance': 'desktop-test'}}},
    {'jsonrpc': '2.0', 'id': 4, 'method': 'tools/call', 'params': {'name': 'scene.query', 'arguments': {'instance': 'other'}}},
]
p = subprocess.run([sys.argv[1], '--instance', 'desktop-test', '--smoke', '--mcp'], input='\n'.join(map(json.dumps, messages))+'\n', text=True, capture_output=True, timeout=25)
if p.returncode != 0:
    raise AssertionError(p)
r = [json.loads(line) for line in p.stdout.splitlines()]
if len(r) != 4 or r[1]['result']['isError'] or r[2]['result']['isError']:
    raise AssertionError(r)
tokens = r[2]['result']['structuredContent']['data']
if tokens['scale_percent'] != 125 or tokens['theme'] != 'high_contrast' or tokens['motion']['transition_ms'] != 0:
    raise AssertionError(tokens)
if not r[3]['result']['isError'] or 'Desktop smoke passed' not in p.stderr:
    raise AssertionError(p)
print('Passed shared-service desktop MCP mutation, token query, instance rejection and clean EOF shutdown')

# Losing the MCP client's stdout consumer must leave the desktop alive.
p = subprocess.Popen([sys.argv[1], '--smoke', '--mcp'], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
p.stdout.close()
p.stdin.write(json.dumps(messages[0]) + '\n')
p.stdin.close()
code = p.wait(timeout=25)
errors = p.stderr.read()
if code != 0 or 'Desktop smoke passed' not in errors:
    raise AssertionError((code, errors))
print('Passed desktop survival after MCP output disconnect')
