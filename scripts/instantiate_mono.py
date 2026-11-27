"""Build a verified static JetBrains Mono regular font for the log viewer."""
import sys
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

source, output = map(Path, sys.argv[1:])
with TTFont(source) as font:
    if "fvar" not in font:
        raise ValueError("Expected variable JetBrains Mono source")
    axes = {a.axisTag: a.defaultValue for a in font["fvar"].axes}
    axes["wght"] = 400
    instantiateVariableFont(font, axes, inplace=True)
    font.save(output)
with TTFont(output) as font:
    if "fvar" in font or font["OS/2"].usWeightClass != 400:
        raise ValueError("Expected static regular JetBrains Mono")
if output.stat().st_size < 10000:
    raise ValueError("Invalid font output")
