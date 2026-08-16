#!/usr/bin/env python3
#
# Copyright (c) 2026 luke8086
# Distributed under the terms of GPL-2 License.
#
# File: mkspk.py - Convert between MID, MusicXML and SPK formats
#

# /// script
# requires-python = ">=3.10"
# dependencies = [
#     "mido>=1.3.2,<2",
# ]
# ///

import argparse
import xml.etree.ElementTree as ET

import mido
from mkinitrd import SPK_NOTE_NAMES, read_spk, split_ext

DEBUG = 0
MIN_REST_MS = 1

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


def split_repeated_notes(notes):
    ret = []
    count = 0

    for (cur_idx, cur_ms) in notes:
        (prev_idx, prev_ms) = ret[-1] if ret else (None, 0)

        if cur_idx is not None and prev_idx == cur_idx:
            ret[-1] = (prev_idx, max(1, prev_ms - MIN_REST_MS))
            ret.append((None, MIN_REST_MS))
            count += 1

        ret.append((cur_idx, cur_ms))

    if count:
        print(f"Split {count} repeated notes")

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

    notes.append((None, 2000))
    notes = merge_rests(notes)
    notes = close_micro_rests(notes)
    notes = split_repeated_notes(notes)
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


def mxml_text(elem, path):
    child = elem.find(path)

    if child is None or child.text is None:
        return None

    return child.text.strip() or None


def mxml_note_idx(elem, where):
    steps = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}

    pitch = elem.find("pitch")
    ensure(pitch is not None, f"Error: {where}: note has no pitch")

    step = mxml_text(pitch, "step")
    ensure(step in steps, f"Error: {where}: invalid pitch step {step!r}")

    octave = mxml_text(pitch, "octave")
    ensure(octave is not None, f"Error: {where}: note has no octave")

    alter = mxml_text(pitch, "alter") or "0"
    note_idx = int(octave) * 12 + steps[step] + round(float(alter))

    ensure(note_idx >= 0, f"Error: {where}: note is below C0")
    ensure(note_idx <= 119, f"Error: {where}: note is above B9")

    return note_idx


def mxml_append_note(notes, held):
    (note_idx, ms, staccato) = held

    if staccato and note_idx is not None and ms >= 2:
        notes.append((note_idx, ms // 2))
        notes.append((None, ms - ms // 2))
    else:
        notes.append((note_idx, ms))


def process_musicxml(root):
    notes = []
    divisions = None
    tempo = 120.0
    held = None

    parts = root.findall("part")
    ensure(parts, "Error: score contains no parts")
    ensure(len(parts) == 1, "Error: multiple parts are not supported")

    for measure in parts[0].findall("measure"):
        number = measure.get("number") or "?"
        loc = f"measure {number}"

        for elem in measure:
            if elem.tag == "attributes":
                if text := mxml_text(elem, "divisions"):
                    divisions = int(text)

                if text := mxml_text(elem, "staves"):
                    ensure(int(text) == 1, f"Error: {loc}: multiple staves are not supported")

            elif elem.tag in ("direction", "sound"):
                sound = elem if elem.tag == "sound" else elem.find("sound")

                if sound is not None and (text := sound.get("tempo")):
                    tempo = float(text)

            elif elem.tag in ("backup", "forward"):
                die(f"Error: {loc}: <{elem.tag}> is not supported")

            elif elem.tag == "note":
                if elem.find("grace") is not None:
                    continue

                ensure(elem.find("chord") is None, f"Error: {loc}: chords are not supported")

                duration = mxml_text(elem, "duration")
                ensure(duration is not None, f"Error: {loc}: note has no duration")
                ensure(divisions, f"Error: {loc}: note before <divisions> was declared")

                note_idx = None if elem.find("rest") is not None else mxml_note_idx(elem, loc)
                ms = max(1, round(int(duration) / divisions * 60000.0 / tempo))

                ties = {tie.get("type") for tie in elem.findall("tie")}
                staccato = elem.find("notations/articulations/staccato") is not None

                if DEBUG:
                    name = "P" if note_idx is None else note_name(note_idx)
                    print(f"{loc:10s}  {name:4s}  {ms}")

                if held is not None and "stop" in ties and held[0] == note_idx:
                    held = (note_idx, held[1] + ms, held[2] or staccato)
                else:
                    if held is not None:
                        mxml_append_note(notes, held)
                    held = (note_idx, ms, staccato)

                if "start" not in ties:
                    mxml_append_note(notes, held)
                    held = None

    if held is not None:
        mxml_append_note(notes, held)

    notes.append((None, 2000))
    notes = merge_rests(notes)
    notes = split_repeated_notes(notes)
    notes = [(idx, min(ms, 0xffff)) for (idx, ms) in notes]

    return notes


def read_musicxml(path):
    try:
        tree = ET.parse(path)
    except ET.ParseError as e:
        die(f"Error: {path}: {e}")

    root = tree.getroot()

    for elem in root.iter():
        if isinstance(elem.tag, str) and "}" in elem.tag:
            elem.tag = elem.tag.split("}")[-1]

    ensure(root.tag == "score-partwise", f"Error: {path}: only score-partwise is supported")

    title = mxml_text(root, "work/work-title") or "Unnamed"
    notes = process_musicxml(root)

    return title, notes


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
    parser = argparse.ArgumentParser(description="Convert between MID, MusicXML and SPK formats")
    parser.add_argument("-i", "--input", metavar="PATH", required=True, help="MID, MusicXML or SPK file to read")
    parser.add_argument("-o", "--output", metavar="PATH", required=True, help="SPK or MID file to write")
    parser.add_argument("-t", "--title", metavar="TITLE", help="Song title")
    parser.add_argument("-d", "--debug", action="store_true", help="Dump input events")
    args = parser.parse_args()

    global DEBUG
    DEBUG = args.debug

    (basename, input_ext) = split_ext(args.input)
    (_, output_ext) = split_ext(args.output)

    if input_ext == "spk":
        (title, notes) = read_spk(args.input)
    elif input_ext in ("mid", "midi"):
        (title, notes) = basename, read_mid(args.input)
    elif input_ext in ("musicxml", "mxml", "xml"):
        (title, notes) = read_musicxml(args.input)
    else:
        die(f"Error: {args.input}: input file must be .spk, .mid or .musicxml")

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
