#!/usr/bin/env python3
"""Opt-in local mock benchmark. Writes JSON to stdout; uploads nothing."""
import argparse
import importlib.util
import json
import math
import os
import pathlib
import platform
import queue
import subprocess
import sys
import tempfile
import threading
import time

sys.dont_write_bytecode = True

class Session:
    def __init__(self, executable):
        self.errors = tempfile.TemporaryFile(mode='w+t')
        self.process = subprocess.Popen([str(executable), '--instance', 'benchmark'],
            stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=self.errors,
            text=True, encoding='utf8')
        self.lines = queue.Queue()
        self.reader = threading.Thread(target=self.read, daemon=True)
        self.reader.start()

    def read(self):
        for line in self.process.stdout:
            self.lines.put(line)
        self.lines.put(None)

    def call(self, command, **arguments):
        started = time.perf_counter_ns()
        self.process.stdin.write(json.dumps(dict(instance='benchmark', command=command,
            arguments=arguments), ensure_ascii=False) + '\n')
        self.process.stdin.flush()
        line = self.lines.get(timeout=10)
        if line is None:
            raise RuntimeError('Editor exited before a benchmark response')
        response = json.loads(line)
        elapsed = (time.perf_counter_ns() - started) / 1_000_000
        if not response['ok']:
            raise RuntimeError(response)
        return elapsed

    def close(self):
        try:
            self.process.stdin.close()
            self.process.wait(timeout=5)
        except (BrokenPipeError, subprocess.TimeoutExpired):
            self.process.kill()
            self.process.wait(timeout=5)
        finally:
            self.reader.join(timeout=5)
            self.process.stdout.close()
            self.errors.close()
        if self.process.returncode:
            raise RuntimeError('Benchmark editor exited unsuccessfully')

def summary(values):
    ordered = sorted(values)
    return dict(samples=len(values), minimum_ms=ordered[0],
        median_ms=ordered[(len(values) - 1)//2],
        p95_ms=ordered[math.ceil(len(values)*.95)-1],
        p99_ms=ordered[math.ceil(len(values)*.99)-1], maximum_ms=ordered[-1])

def linux_memory(pid):
    try:
        fields = dict(line.split(':', 1) for line in pathlib.Path(f'/proc/{pid}/status').read_text().splitlines() if ':' in line)
        return {key + '_bytes': int(fields[key].split()[0])*1024 for key in ('VmRSS', 'VmHWM')}
    except (OSError, KeyError, ValueError):
        return None

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=pathlib.Path)
    parser.add_argument('--samples', type=int, default=100)
    parser.add_argument('--startup-samples', type=int, default=20)
    parser.add_argument('--build-label', default='unspecified')
    options = parser.parse_args()
    if not 5 <= options.samples <= 1000 or not 5 <= options.startup_samples <= 100:
        parser.error('Use 5–1000 operation samples and 5–100 startup samples')
    executable = options.executable.resolve(strict=True)
    fixture_generator = pathlib.Path(__file__).resolve().parents[1] / 'tests/fixtures/generate_large.py'
    spec = importlib.util.spec_from_file_location('large_fixtures', fixture_generator)
    fixtures = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(fixtures)
    startup = []
    for _ in range(options.startup_samples):
        started = time.perf_counter_ns()
        session = Session(executable)
        try:
            session.call('scene.query')
            startup.append((time.perf_counter_ns() - started)/1_000_000)
        finally:
            session.close()
    measurements = {name: [] for name in ('scene_tree_1000', 'asset_list_4096', 'rename_undo_1000_4096')}
    with tempfile.TemporaryDirectory(prefix='lyko-benchmark-') as directory:
        scene, catalogue = fixtures.generate(directory)
        session = Session(executable)
        try:
            session.call('scene.load', path=str(scene))
            session.call('asset.catalog.load', path=str(catalogue))
            for index in range(options.samples + 10):
                tree = session.call('scene.tree')
                assets = session.call('asset.list', query='')
                rename = session.call('entity.rename', id=1, name='Benchmark')
                undo = session.call('history.undo')
                if index >= 10:
                    measurements['scene_tree_1000'].append(tree)
                    measurements['asset_list_4096'].append(assets)
                    measurements['rename_undo_1000_4096'].append(rename + undo)
            memory = linux_memory(session.process.pid)
        finally:
            session.close()
    report = dict(schema_version=1, mock=True, target_hardware_verified=False,
        build_label=options.build_label, platform=platform.platform(),
        machine=platform.machine(), logical_cpus=os.cpu_count(),
        clock='perf_counter_ns; wall-clock client round trip including JSON decoding',
        startup_cache='fresh processes; filesystem cache is uncontrolled, not disk-cold',
        startup_first_response=summary(startup),
        operations={name: summary(values) for name, values in measurements.items()},
        linux_process_memory=memory)
    print(json.dumps(report, indent=2))

if __name__ == '__main__':
    main()
