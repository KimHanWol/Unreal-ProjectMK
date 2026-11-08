"""Save the Unreal application's current Slate screenshot and widget snapshot."""
import base64
import re
from datetime import datetime
from pathlib import Path
import unreal

tool = unreal.get_default_object(unreal.SlateInspectorToolset)
tool.call_method('Observe', args=('', 80))
snapshot = tool.call_method('Snapshot', args=('', 80, False))
folder = Path(unreal.Paths.project_saved_dir()) / 'AgentLogs'
folder.mkdir(parents=True, exist_ok=True)
stem = 'ui-' + datetime.now().strftime('%Y%m%d-%H%M%S')
main_window = re.search(r'window "ProjectMK - Unreal Editor".*?\[ref=([^\]]+)\]', snapshot)
image = tool.call_method('Screenshot', args=(main_window.group(1) if main_window else '',))
(folder / (stem + '.png')).write_bytes(base64.b64decode(image.get_editor_property('Data')))
(folder / (stem + '.txt')).write_text(snapshot, encoding='utf-8')
print(str(folder / (stem + '.png')))
