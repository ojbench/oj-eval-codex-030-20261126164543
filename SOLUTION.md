# Digit recognition

The classifier uses only the 100 reference images distributed in the problem's
linked repository, https://github.com/bytetriper/Number-Recognition (`data/`).
The advertised local data directory was empty. No external dataset, pretrained
model, neural network, or image-processing library is used.

Each image is normalized to a 24 by 24 grid. Bounding-box normalization handles
translation and size; a second representation removes the horizontal shear
estimated from image moments. Intensity and horizontal/vertical derivatives
describe the strokes. Symmetric nearest-template matching permits each local
feature to move by one pixel. The plain and deskewed distances are combined.
Block-normalized histograms of gradient directions supply a complementary
description of stroke orientation. Flood filling the background at three
intensity thresholds measures enclosed loops and their vertical positions.
This distinguishes otherwise similar open and closed strokes without relying
on one binary threshold. These distances are combined with the local
deformation distance. The nearest reference supplies the predicted label.

All reference pixels are embedded losslessly in `src.hpp`. The `judge` function
does no I/O. Initialization is thread-safe and performed once. Runtime is
O(R * S^2) per image, with R=100 references and S=24; the local search has nine
positions. Storage is O(R * S^2).

## Reproduction

`recognizer.inc` is the maintained algorithm source. To regenerate the single
submission header using the original provided data:

```
python3 tools/build_header.py /path/to/Number-Recognition/data
g++ -std=c++17 -O2 -Wall -Wextra -pedantic tests/validate.cpp -o /tmp/validate
/tmp/validate
```

The test holds out each reference in turn, preventing self-matches. The initial
solution correctly classified 87/100 held-out examples in approximately 0.27 s.
Adding gradient histograms increases this to 89/100; `tests/compare.cpp` compares
the topology descriptor at several relative weights using leave-one-out
validation. Adding the topology descriptor increases this to 94/100.
This is a small validation set, so judge accuracy may differ.

The final online evaluation scores 181/200 (90.5% accuracy) in 21.441 seconds,
using about 6 MB of memory. See `SUBMISSIONS.md` for the complete attempt log.
