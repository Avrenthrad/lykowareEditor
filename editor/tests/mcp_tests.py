"""Black-box stdio MCP lifecycle, discovery, targeting and CLI parity."""
import json
import subprocess
import sys

exe = sys.argv[1]
def rpc(identifier, method, params=None):
    return {'jsonrpc': '2.0', 'id': identifier, 'method': method, 'params': params or {}}
def tool(identifier, name, args=None, instance='test', **extra):
    return rpc(identifier, 'tools/call', {'name': name, 'arguments': {
        'instance': instance, 'arguments': args or {}, **extra}})
def check(condition, message):
    if not condition:
        raise AssertionError(message)

messages = [rpc(1, 'tools/list'), rpc('init', 'initialize', {
    'protocolVersion': '2025-06-18', 'capabilities': {}, 'clientInfo': {'name': 'tests', 'version': '1'}}),
    {'jsonrpc': '2.0', 'method': 'notifications/initialized'}, rpc(-1, 'tools/list'),
    tool(3, 'entity.create', {'name': 'Player'}, instance='other'),
    tool(4, 'entity.create', {'name': 'Player'}, expected_revision=0),
    tool(5, 'play.start', expected_revision=0),
    tool(6, 'transaction.execute', {'commands': [{'command': 'play.start'}, {'command': 'play.input', 'arguments': {'value': 'jump'}}]}),
    tool(7, 'scene.query'),
    {'jsonrpc': '2.0', 'method': 'tools/call', 'params': {'name': 'play.stop', 'arguments': {'instance': 'test'}}},
    tool(8, 'scene.query'), rpc(9, 'does/not/exist'), {}, '{', rpc(10, 'ping')]
p = subprocess.run([exe, '--instance', 'test', '--mcp'], input='\n'.join(
    x if isinstance(x, str) else json.dumps(x) for x in messages) + '\n', text=True, capture_output=True)
check(p.returncode == 0, p)
r = [json.loads(x) for x in p.stdout.splitlines()]
check(len(r) == len(messages) - 2, r)
by_id = {x['id']: x for x in r if x['id'] is not None}
check(by_id[1]['error']['code'] == -32600, by_id[1])
check(by_id['init']['result']['protocolVersion'] == '2025-06-18', by_id['init'])
tools = by_id[-1]['result']['tools']
check('instance' in tools[0]['inputSchema']['required'], tools)
check(by_id[3]['result']['isError'] and by_id[5]['result']['isError'], 'target and revision rejection')
check(not by_id[4]['result']['isError'] and not by_id[6]['result']['isError'], r)
state = by_id[7]['result']['structuredContent']
check(state['data']['runtime'] == {'playing': True, 'inputs': ['jump']}, state)
check(by_id[8]['result']['structuredContent'] == state, 'notification did not mutate')
check(by_id[9]['error']['code'] == -32601 and by_id[10]['result'] == {}, r)
check(any(x.get('error', {}).get('code') == -32700 for x in r), 'parse errors')
check(any(x.get('error', {}).get('code') == -32600 and x['id'] is None for x in r), 'invalid envelopes')
cli_requests = [
    {'instance': 'test', 'command': 'entity.create', 'arguments': {'name': 'Player'}},
    {'instance': 'test', 'command': 'transaction.execute', 'arguments': {'commands': [
        {'command': 'play.start'}, {'command': 'play.input', 'arguments': {'value': 'jump'}}]}},
    {'instance': 'test', 'command': 'scene.query'}]
cli = subprocess.run([exe, '--instance', 'test'], input='\n'.join(map(json.dumps, cli_requests)) + '\n', text=True, capture_output=True)
check(cli.returncode == 0 and json.loads(cli.stdout.splitlines()[-1]) == state, 'CLI and MCP parity')
print('Passed MCP lifecycle, schemas, signed IDs, isolation, revision checks, notification safety and CLI parity')
