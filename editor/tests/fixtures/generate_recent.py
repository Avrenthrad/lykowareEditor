"""Generate a machine-local recent-history fixture without committing local paths."""
import argparse
import json
import pathlib
import tomllib

def generate(destination, project_root):
    project_root = pathlib.Path(project_root).resolve()
    metadata = tomllib.loads((pathlib.Path(__file__).parent / 'valid/project/lykoware.toml').read_text(encoding='utf8'))
    entry = dict(path=(project_root / 'lykoware.toml').as_posix(),
                 root=project_root.as_posix(), manifest=metadata)
    # Stored project_root may intentionally not exist: history is not activation.
    with pathlib.Path(destination).open('x', encoding='utf8') as output:
        json.dump(dict(schema_version=1, recent=[entry]), output, ensure_ascii=False, indent=2)
        output.write('\n')
    return entry

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('destination', type=pathlib.Path)
    parser.add_argument('project_root', type=pathlib.Path)
    options = parser.parse_args()
    generate(options.destination, options.project_root)
