"""Shrinks rendered icons in place: 256-colour palette with dithering (~180 KB -> ~70 KB each).

    python optimize.py <png>...
"""
import sys
from PIL import Image

for path in sys.argv[1:]:
    im = Image.open(path).convert('RGB')
    im.quantize(256, method=Image.Quantize.MEDIANCUT, dither=Image.Dither.FLOYDSTEINBERG).save(path, optimize=True)
