"""Build static readable Karla weights from the pinned OFL variable source.

Runs in packaging only. Verifies the actual font weight, not just filenames.
M1 Arc's DX12/ImGui font atlas must never load an uninstanced variable font.
"""
from __future__ import annotations

import sys
from pathlib import Path

from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: instantiate_karla.py source-variable.ttf output-directory")
        return 2

    source = Path(sys.argv[1])
    destination = Path(sys.argv[2])
    destination.mkdir(parents=True, exist_ok=True)

    for name, weight in (("Medium", 500), ("SemiBold", 600), ("Bold", 700)):
        font = TTFont(str(source))
        if "fvar" not in font:
            raise ValueError("Karla source is not a variable font")
        axes = {axis.axisTag for axis in font["fvar"].axes}
        if "wght" not in axes:
            raise ValueError("Karla source does not contain wght axis")
        instantiateVariableFont(font, {"wght": float(weight)}, inplace=True)
        output = destination / f"Karla-{name}.ttf"
        font.save(output)
        font.close()

        verify = TTFont(str(output))
        actual = verify["OS/2"].usWeightClass
        if actual != weight or "fvar" in verify:
            raise ValueError(f"Incorrect static Karla {name}: actual weight={actual}")
        verify.close()
        if output.stat().st_size < 10_000:
            raise ValueError(f"Empty or invalid Karla font: {name}")
        print(f"Verified Karla {name} (wght={weight}): {output}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
