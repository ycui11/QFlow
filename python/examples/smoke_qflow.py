#!/usr/bin/env python3
"""Smoke test for the qflow Python extension (L10)."""

import sys

try:
    import qflow
except ImportError as exc:
    print("FAIL: could not import qflow:", exc, file=sys.stderr)
    sys.exit(1)

assert qflow.version()
sma = qflow.Sma(3)
sma.update(1.0)
sma.update(2.0)
sma.update(3.0)
assert sma.ready()
assert abs(sma.value() - 2.0) < 1e-9
ticks = qflow.to_ticks(190.25, 0.01)
assert ticks == 19025
print("All python qflow checks passed")
