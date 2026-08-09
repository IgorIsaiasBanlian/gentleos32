#!/usr/bin/env python3
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: mkspk.py - Convert between MID and SPK formats
#

# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "mido>=1.3.2,<2",
# ]
# ///

import argparse

import mido
from mkinitrd import SPK_NOTE_NAMES, read_spk, split_ext

DEBUG = 0

def die(msg):
    raise SystemExit(msg)


def ensure(cond, msg):
    if not cond:
        die(msg)


def note_name(note_idx):
    return f"{SPK_NOTE_NAMES[note_idx % 12]}{note_idx // 12}"


def merge_rests(notes):
    ret = []

    for idx, ms in notes:
        if idx is None:
            if not ret:
                continue
            if ret[-1][0] is None:
                ret[-1] = (None, ret[-1][1] + ms)
                continue
        ret.append((idx, ms))

    return ret


def close_micro_rests(notes):
    ret = []
    count = 0

    for (note_idx, ms) in notes:
        if note_idx is None and ms < 50 and ret:
            ret[-1] = (ret[-1][0], ret[-1][1] + ms)
            count += 1
        else:
            ret.append((note_idx, ms))

    if count:
        print(f"Closed {count} micro rests")

    return ret


def process_mid(mid, track):
    notes = []

    cur_tempo = 500000
    cur_us = 0.0
    cur_ms = 0
    prev_ms = 0
    prev_idx = None

    for msg in track:
        cur_us += msg.time * cur_tempo / mid.ticks_per_beat
        cur_ms = round(cur_us / 1000.0)
        cur_idx = 0
        cur_type = msg.type

        if DEBUG:
            print(f"{cur_ms:8d}  {msg}")

        if cur_type in ("note_on", "note_off"):
            cur_idx = msg.note - 12

        if cur_type == "note_on" and msg.velocity == 0:
            cur_type = "note_off"

        if cur_type == "set_tempo":
            cur_tempo = msg.tempo

        elif cur_type == "note_on" and msg.velocity > 0:
            ensure(cur_idx >= 0, f"Error: MIDI note {msg.note} is below C0")

            if cur_ms > prev_ms:
                notes.append((prev_idx, cur_ms - prev_ms))

            prev_idx = cur_idx
            prev_ms = cur_ms

        elif cur_type == "note_off" and cur_idx == prev_idx:
            if cur_ms > prev_ms:
                notes.append((prev_idx, cur_ms - prev_ms))

            prev_idx = None
            prev_ms = cur_ms

    notes = merge_rests(notes)
    notes = close_micro_rests(notes)
    notes = [(idx, min(ms, 0xffff)) for (idx, ms) in notes]

    return notes


def read_mid(path):
    mid = mido.MidiFile(path)

    ensure(mid.type in (0, 1), f"Error: {path}: MIDI format {mid.type} is not supported")

    if mid.type == 0:
        track = mid.tracks[0]
    else:
        track = mid.merged_track
        print(f"Merged {len(mid.tracks)} tracks")

    return process_mid(mid, track)


def write_spk(path, title, notes):
    with open(path, "w") as f:
        f.write(f"title: {title}\n")
        for note_idx, ms in notes:
            if note_idx is None:
                f.write(f"P, {ms}\n")
            else:
                f.write(f"{note_name(note_idx)}, {ms}\n")


def write_mid(path, title, notes):
    tempo = 500000
    ticks_per_beat = 480

    mid = mido.MidiFile(type=0, ticks_per_beat=ticks_per_beat)
    track = mido.MidiTrack()
    mid.tracks.append(track)

    track.append(mido.MetaMessage("track_name", name=title, time=0))
    track.append(mido.MetaMessage("set_tempo", tempo=tempo, time=0))

    delay = 0
    for note_idx, ms in notes:
        ticks = round(ms * 1000.0 * ticks_per_beat / tempo)
        if note_idx is None:
            delay += ticks
        else:
            track.append(mido.Message("note_on", note=note_idx + 12, velocity=64, time=delay))
            track.append(mido.Message("note_off", note=note_idx + 12, velocity=0, time=ticks))
            delay = 0

    mid.save(path)


def main():
    parser = argparse.ArgumentParser(description="Convert between MID and SPK formats")
    parser.add_argument("-i", "--input", metavar="PATH", required=True, help="MID or SPK file to read")
    parser.add_argument("-o", "--output", metavar="PATH", required=True, help="SPK or MID file to write")
    parser.add_argument("-t", "--title", metavar="TITLE", help="Song title")
    parser.add_argument("-d", "--debug", action="store_true", help="Dump MIDI messages")
    args = parser.parse_args()

    global DEBUG
    DEBUG = args.debug

    (basename, input_ext) = split_ext(args.input)
    (_, output_ext) = split_ext(args.output)

    if input_ext == "spk":
        (title, notes) = read_spk(args.input)
    elif input_ext in ("mid", "midi"):
        (title, notes) = basename, read_mid(args.input)
    else:
        die(f"Error: {args.input}: input file must be .spk or .mid")

    title = args.title or title

    ensure(notes, f"Error: no playable notes in {args.input}")

    if output_ext == "spk":
        write_spk(args.output, title, notes)
    elif output_ext in ("mid", "midi"):
        write_mid(args.output, title, notes)
    else:
        die(f"Error: {args.output}: output file must be .spk or .mid")

    total_ms = sum(ms for (_, ms) in notes)

    print(f"Saved {args.output}: \"{title}\" ({len(notes)} notes, {total_ms} ms)")

if __name__ == "__main__":
    main()
