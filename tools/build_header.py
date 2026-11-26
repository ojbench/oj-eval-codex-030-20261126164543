"""Embed only the 100 reference images supplied by the problem author."""
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parents[1]
data = pathlib.Path(sys.argv[1])
rows = []
for label in range(10):
    for index in range(1, 11):
        raw = (data / str(label) / f"{index}.ppm").read_bytes()
        header, width, maximum, pixels = raw.split(b"\n", 3)
        assert header == b"P5" and width == b"28 28" and maximum == b"255"
        assert len(pixels) == 784
        # Hexadecimal strings preserve all original pixel values exactly.
        rows.append('    "' + pixels.hex() + '"')
out = '// Generated from the official supplied samples; see tools/build_header.py.\n'
out += '#pragma once\n#include <algorithm>\n#include <array>\n#include <cmath>\n#include <vector>\n'
out += 'typedef std::vector<std::vector<double>> IMAGE_T;\n'
out += 'namespace digit_recognition {\nstatic const char* const samples[100] = {\n'
out += ',\n'.join(rows) + '\n};\n'
out += (root / 'recognizer.inc').read_text() + '\n}\n'
out += 'inline int judge(IMAGE_T &img) { return digit_recognition::classify(img); }\n'
(root / 'src.hpp').write_text(out)
