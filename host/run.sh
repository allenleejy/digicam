#!/bin/sh
# Start the digicam simulator. First run creates a Python environment with pyserial.
cd "$(dirname "$0")"
if [ ! -x .venv/bin/python ]; then
  python3 -m venv .venv && .venv/bin/pip install -q -r requirements.txt || exit 1
fi
exec .venv/bin/python server.py "$@"
