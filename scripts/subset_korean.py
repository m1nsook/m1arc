"""Build an OFL-licensed, static Korean fallback for the M1 Arc ImGui atlas.

Input: pinned Google Fonts Noto Sans KR variable font.
Output: a smaller font containing Hangul syllables, Jamo and compatibility Jamo.
"""
import sys
from fontTools.subset import Options, Subsetter
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

if len(sys.argv) != 3:
    raise SystemExit("Usage: subset_korean.py input.ttf output.ttf")

font = TTFont(sys.argv[1])
if "fvar" in font:
    font = instantiateVariableFont(font, {"wght": 600}, inplace=True)

ranges = [(0x1100, 0x11FF), (0x3130, 0x318F), (0xA960, 0xA97F),
          (0xAC00, 0xD7A3), (0xD7B0, 0xD7FF)]
unicodes = set()
for first, last in ranges:
    unicodes.update(range(first, last + 1))

options = Options()
options.layout_features = ["*"]
subset = Subsetter(options=options)
subset.populate(unicodes=unicodes)
subset.subset(font)
font.save(sys.argv[2])
