#!/usr/bin/env python3
"""Write a demo song as a save file: python3 tools/demo_song.py out.sav

Put the .sav next to the ROM with the same name (gbasequencer.sav) and the song loads at power-on.
The layout matches struct Song in src/song.h and the header in src/song.c.
"""
import struct
import sys

STEPS, PATTERNS, POSITIONS = 64, 16, 64
NO_PATTERN = 0xFF
CRASH, RIDE, OHAT, CHAT, HTOM, MTOM, LTOM, CLAP, RIM, SN2, SN1, BD2, BD1 = range(13)


def note(name):
    names = {'C': 0, 'C#': 1, 'D': 2, 'D#': 3, 'E': 4, 'F': 5, 'F#': 6, 'G': 7, 'G#': 8, 'A': 9, 'A#': 10, 'B': 11}
    return names[name[:-1]] + 12 * (int(name[-1]) + 1)


def synth(length, notes):
    """notes: {step: 'C3', ...}"""
    data = [0] * STEPS
    for step, n in notes.items():
        data[step] = note(n)
    return bytes([length] + data)


def drums(length, hits):
    """hits: {drum: [steps]}"""
    data = [0] * STEPS
    for d, steps in hits.items():
        for s in steps:
            data[s] |= 1 << d
    return struct.pack('<BB64H', length, 0, *data)


tempo = 118
synths = [[synth(16, {})] * PATTERNS for _ in range(3)]
drum_patterns = [drums(16, {})] * PATTERNS

# Bass, pattern A: a C minor riff; B: the same a fourth up
synths[0][0] = synth(16, {0: 'C3', 3: 'C3', 6: 'G2', 8: 'A#2', 10: 'C3', 12: 'D#3', 14: 'F3'})
synths[0][1] = synth(16, {0: 'F3', 3: 'F3', 6: 'C3', 8: 'D#3', 10: 'F3', 12: 'G#3', 14: 'A#3'})
# Syn 2: off-beat stabs
synths[1][0] = synth(16, {2: 'G4', 6: 'D#4', 10: 'G4', 14: 'A#4'})
synths[1][1] = synth(16, {2: 'C5', 6: 'G#4', 10: 'C5', 14: 'D#5'})
# Syn 3: a melody over 32 steps
synths[2][0] = synth(32, {0: 'C5', 1: 'C5', 2: 'C5', 4: 'D#5', 5: 'D#5', 6: 'G5', 7: 'G5', 8: 'G5', 10: 'F5', 12: 'D#5',
                          13: 'D#5', 14: 'D5', 16: 'C5', 17: 'C5', 18: 'C5', 19: 'C5', 22: 'A#4', 24: 'C5', 25: 'C5',
                          26: 'D5', 28: 'D#5', 29: 'D#5', 30: 'D#5'})
# Drums A: four on the floor; B: the same with a tom fill at the end
groove = {BD1: [0, 4, 8, 12], SN1: [4, 12], CHAT: [0, 2, 4, 6, 8, 10, 12], OHAT: [14]}
drum_patterns[0] = drums(16, groove)
drum_patterns[1] = drums(16, {BD1: [0, 4, 8], SN1: [4], CHAT: [0, 2, 4, 6], HTOM: [8, 9], MTOM: [10, 11],
                              LTOM: [12, 13], SN2: [14, 15]})
drum_patterns[2] = drums(16, {**groove, CRASH: [0]})

# Arrangement: (bass, syn2, syn3, drums) per position; None = silent
A, B, C = 0, 1, 2
arrangement = [
    (A, None, None, C), (A, None, None, A), (A, A, None, A), (A, A, None, B),
    (A, A, A, C), (A, A, None, A), (B, B, A, A), (B, B, None, B),
]
order = [[NO_PATTERN] * POSITIONS for _ in range(4)]
for pos, cols in enumerate(arrangement):
    for t, p in enumerate(cols):
        if p is not None:
            order[t][pos] = p

# Voices: volume, tone, attack, decay, release, vibrato, slide, sweep (see SynthVoice in src/song.h)
voices = [
    [15, 2, 0, 6, 3, 0, 0, 7],   # Bass: 50% pulse, a bit shorter
    [11, 0, 1, 7, 7, 2, 0, 7],   # Syn 2: thin 12.5% pulse, soft attack, a little vibrato
    [4, 1, 2, 0, 3, 3, 2, 7],    # Syn 3: triangle that swells in, vibrato, short glides, fades out
]

song = bytes([tempo]) + b''.join(bytes(row) for row in order)
song += b''.join(b''.join(patterns) for patterns in synths)
song += b'\0'  # padding: DrumPattern is 2-byte aligned
song += b''.join(drum_patterns)
song += b''.join(bytes(v) for v in voices)
assert len(song) == 5482, len(song)  # sizeof(Song)

total = 0x5A5A
for b in song:
    total = (((total << 1) | (total >> 15)) + b) & 0xFFFF
data = b'GBSQ' + bytes([2, 0]) + struct.pack('<H', len(song)) + song + struct.pack('<H', total)
data += b'\xFF' * (32768 - len(data))
open(sys.argv[1], 'wb').write(data)
print('wrote', sys.argv[1])
