"""Generate reusable large fixture data in a caller-owned directory, offline."""
import argparse
import json
import pathlib

def generate(destination):
    destination = pathlib.Path(destination)
    destination.mkdir(parents=True, exist_ok=True)
    scene = destination / 'scene-1000.json'
    catalogue = destination / 'catalogue-4096.json'
    # Refuse to overwrite user data; callers should pass a fresh temporary folder.
    with scene.open('x', encoding='utf8') as output:
        json.dump({'schema_version': 2, 'next_id': 1001, 'entities': [
            {'id': i, 'name': f'Entity {i}', 'parent': 0 if i == 1 else 1}
            for i in range(1, 1001)]}, output, separators=(',', ':'))
        output.write('\n')
    with catalogue.open('x', encoding='utf8') as output:
        json.dump({'schema_version': 1, 'next_id': 4097, 'assets': [
            {'id': i, 'path': f'meshes/mesh-{i}.gltf', 'kind': 'mesh'}
            for i in range(1, 4097)]}, output, separators=(',', ':'))
        output.write('\n')
    return scene, catalogue

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=pathlib.Path)
    generate(parser.parse_args().destination)
