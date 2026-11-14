"""Measure PIE request-to-ready and request-to-editor latency without changing saves.

Optional Saved/AgentLogs/pie-benchmark-config.json: {"tag":"before", "runs":3, "trace":true}.
Run through RunEditorPython.py; results are written asynchronously after the editor ticks.
"""
import json
import time
from pathlib import Path
import unreal


class PIEBenchmark:
    def __init__(self):
        self.folder = Path(unreal.Paths.project_saved_dir()) / 'AgentLogs'
        config_path = self.folder / 'pie-benchmark-config.json'
        self.config = json.loads(config_path.read_text(encoding='utf-8-sig')) if config_path.exists() else {}
        self.tag = self.config.get('tag', 'before')
        self.results = []
        self.editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
        self.worlds = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
        if self.worlds.get_game_world() is not None:
            raise RuntimeError('End the current PIE session before measuring.')
        self.settings = unreal.get_default_object(unreal.load_class(None, '/Script/UnrealEd.EditorPerformanceSettings'))
        self.old_throttle = self.settings.get_editor_property('bThrottleCPUWhenNotForeground')
        self.settings.set_editor_property('bThrottleCPUWhenNotForeground', False)
        if self.config.get('trace', True):
            trace_path = (self.folder / ('pie-' + self.tag + '.utrace')).as_posix()
            unreal.SystemLibrary.execute_console_command(None, 'Trace.File "' + trace_path + '" cpu,frame,bookmark,loadtime')
        self.phase = 'idle'
        self.changed = time.perf_counter()
        self.handle = unreal.register_slate_post_tick_callback(self.on_tick)

    def on_tick(self, delta):
        try:
            now = time.perf_counter()
            world = self.worlds.get_game_world()
            if self.phase == 'idle' and now - self.changed > 0.5:
                self.changed = time.perf_counter()
                self.phase = 'starting'
                self.editor.editor_request_begin_play()
            elif self.phase == 'starting' and world is not None:
                player = unreal.GameplayStatics.get_player_character(world, 0)
                panels = unreal.WidgetLibrary.get_all_widgets_of_class(world, unreal.PlaytestWidget, False)
                if player is None or not panels:
                    return
                elapsed = time.perf_counter() - self.changed
                panels[0].call_method('OnRestoreClicked')
                self.results.append({'start_seconds': elapsed,
                                     'blocks': len(unreal.GameplayStatics.get_all_actors_of_class(world, unreal.BlockBase))})
                unreal.log('[PIEBench] ' + self.tag + ' ready ' + str(self.results[-1]))
                self.changed = time.perf_counter()
                self.phase = 'running'
            elif self.phase == 'running' and now - self.changed > 1.0:
                self.phase = 'stopping'
                self.changed = time.perf_counter()
                self.editor.editor_request_end_play()
            elif self.phase == 'stopping' and world is None:
                self.results[-1]['stop_seconds'] = time.perf_counter() - self.changed
                unreal.log('[PIEBench] ' + self.tag + ' complete ' + str(self.results[-1]))
                self.changed = time.perf_counter()
                self.phase = 'idle'
                if len(self.results) >= self.config.get('runs', 3):
                    self.finish()
        except Exception as error:
            unreal.log_error('[PIEBench] ' + str(error))
            self.finish()

    def finish(self):
        unreal.unregister_slate_post_tick_callback(self.handle)
        if self.config.get('trace', True):
            unreal.SystemLibrary.execute_console_command(None, 'Trace.Stop')
        self.settings.set_editor_property('bThrottleCPUWhenNotForeground', self.old_throttle)
        (self.folder / ('pie-' + self.tag + '.json')).write_text(json.dumps(self.results, indent=2), encoding='utf-8')
        unreal.log('[PIEBench] Finished ' + self.tag)


MK_PIE_BENCHMARK = PIEBenchmark()
