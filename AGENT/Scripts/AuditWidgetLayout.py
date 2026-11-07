"""Audit reusable widget assets for typography and fixed-size layout risks."""
import json
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir())
TEXT_WIDGETS = {
    "/Game/UI/BP/Slot/W_ItemSlot": ["Text_Count"],
    "/Game/UI/BP/Tooltip/W_Tooltip": ["Title", "Desc"],
    "/Game/UI/BP/W_Currency": ["Text_Coin"],
}

SIZE_BOXES = {
    "/Game/UI/BP/Tooltip/W_Tooltip": [],
    "/Game/UI/BP/Slot/W_ItemSlot": [],
}


report = {"blueprints": [], "fontSizes": [], "fixedSizes": [], "missing": []}
for blueprint_path in sorted(set(TEXT_WIDGETS) | set(SIZE_BOXES)):
    blueprint = unreal.load_asset(blueprint_path)
    if not blueprint:
        raise RuntimeError("Missing widget blueprint: " + blueprint_path)

    entry = {"asset": blueprint_path, "text": [], "sizeBoxes": []}
    asset_name = blueprint_path.rsplit("/", 1)[-1]
    widget_prefix = blueprint_path + "." + asset_name + ":WidgetTree."
    for widget_name in TEXT_WIDGETS.get(blueprint_path, []):
        obj = unreal.load_object(None, widget_prefix + widget_name)
        if not obj:
            report["missing"].append(widget_prefix + widget_name)
            continue
        font = obj.get_editor_property("font")
        text_entry = {
            "name": obj.get_name(),
            "fontSize": font.size,
            "justification": str(obj.get_editor_property("justification")),
            "autoWrap": obj.get_editor_property("auto_wrap_text"),
        }
        entry["text"].append(text_entry)
        report["fontSizes"].append(font.size)
    for widget_name in SIZE_BOXES.get(blueprint_path, []):
        obj = unreal.load_object(None, widget_prefix + widget_name)
        if not obj:
            report["missing"].append(widget_prefix + widget_name)
            continue
        size_entry = {
            "name": obj.get_name(),
            "widthOverride": obj.get_editor_property("width_override"),
            "heightOverride": obj.get_editor_property("height_override"),
            "minWidth": obj.get_editor_property("min_desired_width"),
            "minHeight": obj.get_editor_property("min_desired_height"),
        }
        entry["sizeBoxes"].append(size_entry)
        if size_entry["widthOverride"] > 0 or size_entry["heightOverride"] > 0:
            report["fixedSizes"].append({"asset": blueprint_path, **size_entry})
    entry["text"].sort(key=lambda item: item["name"])
    entry["sizeBoxes"].sort(key=lambda item: item["name"])
    report["blueprints"].append(entry)

report["fontSizes"] = sorted(set(report["fontSizes"]))
output = ROOT / "output/widget-art/layout-audit.json"
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding="utf-8")
print(json.dumps({
    "blueprints": len(report["blueprints"]),
    "fontSizes": report["fontSizes"],
    "fixedSizeCount": len(report["fixedSizes"]),
    "missing": report["missing"],
}, ensure_ascii=False))
