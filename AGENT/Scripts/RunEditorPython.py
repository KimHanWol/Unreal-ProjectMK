"""Run a project script in the already-open local Unreal Editor."""
import json
import pathlib
import sys
import time

sys.path.insert(0, 'C:/Program Files/Epic Games/UE_5.8/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python')
import remote_execution

remote = remote_execution.RemoteExecution()
remote.start()
try:
    for attempt in range(20):
        nodes = [node for node in remote.remote_nodes if pathlib.Path(node['project_root']).resolve() == pathlib.Path(__file__).resolve().parents[2]]
        if nodes:
            break
        time.sleep(0.25)
    if not nodes:
        raise RuntimeError('ProjectMK editor Python connection is unavailable.')
    remote.open_command_connection(nodes[0]['node_id'])
    if sys.argv[1] == '--command':
        command = sys.argv[2]
    else:
        command = pathlib.Path(sys.argv[1]).resolve().as_posix()
    result = remote.run_command(command, raise_on_failure=False)
    print(json.dumps(result, ensure_ascii=False))
    if not result['success']:
        sys.exit(1)
finally:
    remote.stop()
