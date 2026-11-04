"""Exercise the real headless frontend, including observable state after rejection."""
import subprocess
import sys

exe = sys.argv[1]

def run(text):
    return subprocess.run([exe, "--instance", "cli-test"], input=text,
                          text=True, capture_output=True, check=False)

success = run('entity.create "Player"\nplay.start\nplay.input jump\nscene.query\nhistory.undo\nscene.query\n')
if success.returncode != 0 or '1 "Player"' not in success.stdout:
    raise AssertionError(success)
if 'inputs=1' not in success.stdout or 'inputs=0' not in success.stdout:
    raise AssertionError(success)
for text in ['entity.create "unterminated\n', 'entity.rename -1 A\n',
             'play.start extra\n', 'play.input jump\n', 'unknown\n']:
    result = run(text + 'scene.query\n')
    if result.returncode != 1 or 'error ' not in result.stdout:
        raise AssertionError(result)
    if 'ok revision=0' not in result.stdout or 'ok revision=1' in result.stdout:
        raise AssertionError(result)
print('Passed CLI feedback loop and five rejection/rollback cases')
