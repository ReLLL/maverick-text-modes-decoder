"""Exercise the real native IPC reader without SDR# or a GUI."""
import json
from pathlib import Path
import struct
import subprocess
import sys


def packet(header, payload=b''):
    body = json.dumps(header).encode('utf-8')
    return struct.pack('<I', len(body)) + body + payload


def main(binary):
    meta = dict(kind='metadata', center=14070000, frequency=14070750,
                sample_rate=192000, bandwidth=400, mode='CW', playing=True, epoch=1)
    iq = dict(kind='iq', samples=1, channel='cw', sample_rate=8000,
              center=14070750, frequency=14070750, filter_width=400, mode='CW', epoch=1)
    data = packet(meta) + packet(iq, bytes(8))
    for change in ({'frequency': 14070850}, {'mode': 'USB'}, {'filter_width': 2400}):
        data += packet(iq | change, bytes(8))
    # A tuned VFO change must not reset unrelated full-spectrum reception.
    data += packet(iq | {'channel': 'digital', 'frequency': 14071000}, bytes(8))
    data += packet(meta | {'playing': False, 'epoch': 2}) * 4
    data += packet(iq, bytes(8))
    data += packet(meta | {'epoch': 3})
    data += packet(iq, bytes(8))  # Old epoch must be ignored after restart.
    data += packet(iq | {'epoch': 3}, bytes(8))
    data += packet(iq | {'channel': 'digital', 'epoch': 3}, bytes(8))
    result = subprocess.run([str(Path(binary).resolve())], input=data,
                            capture_output=True, timeout=20)
    print(result.stderr.decode('utf-8'), end='')
    if result.returncode:
        raise RuntimeError('Native stop/retune/epoch isolation failed')


if __name__ == '__main__':
    main(sys.argv[1])
