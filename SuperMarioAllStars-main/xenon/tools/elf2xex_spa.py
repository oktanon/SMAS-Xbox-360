"""Packs the port's ELF into a XEX with xsdk's tools/elf2xex.py, unchanged, but with a title database shaped like a
game's SPA (tarea 0022): Aurora showed the file name, not the title name, with xsdk's database, which holds only the
default language (XSTC) and the title string (XSTR, string 0x8000). A game's SPA also has its title header (XTHD:
title ID, title type, version) and its achievements (XACH), each a metadata entry named after its magic
(free60.org/System-Software/Formats/SPA/, XDBF/). This one adds those two, an empty XACH, and keeps the rest as xsdk
writes it, in the room xsdk gives the database beside the PE headers (elf2xex.py: TITLE_NAME_END).

    python3 elf2xex_spa.py <xsdk>/tools/elf2xex.py <elf> <xex> --title-id 534D4153 --title-name "..." --version 0.4.0.0

(No change to xsdk: its module is loaded and its title_database replaced; everything else is its own.)
"""
import importlib.util
import struct
import sys


def load(path):
    spec = importlib.util.spec_from_file_location("elf2xex", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def arg(argv, name, default=None):
    if name in argv:
        return argv[argv.index(name) + 1]
    return default


def main():
    tool, rest = sys.argv[1], sys.argv[2:]
    xsdk = load(tool)
    title_id = int(arg(rest, "--title-id", "5853444B"), 16)
    parts = [int(p) for p in arg(rest, "--version", "0.0.0.0").split(".")] + [0, 0, 0, 0]
    major, minor, build, revision = parts[:4]

    def title_database(name):
        text = name.encode()
        # XACH: no achievements yet (AGENTS.md §7: they come at the end, with their own XACH)
        xach = struct.pack(">4sIIH", b"XACH", 1, 2, 0)
        # XTHD: the title header; title type 1 = full game; the version's four parts; 16 bytes unused
        xthd = struct.pack(">4sIIIIHHHH16x", b"XTHD", 1, 32, title_id, 1, major, minor, build, revision)
        # XSTC and XSTR, as xsdk writes them: English, string 0x8000 = the title's name
        xstc = struct.pack(">4sIII", b"XSTC", 1, 4, xsdk.LANGUAGE_ENGLISH)
        strings = struct.pack(">HH", xsdk.XDBF_ID_TITLE, len(text)) + text
        xstr = struct.pack(">4sIIH", b"XSTR", 1, 2 + len(strings), 1) + strings
        records = [
            (xsdk.XDBF_SECTION_METADATA, 0x58414348, xach),  # 'XACH'
            (xsdk.XDBF_SECTION_METADATA, 0x58544844, xthd),  # 'XTHD'
            (xsdk.XDBF_SECTION_METADATA, xsdk.XDBF_ID_XSTC, xstc),
            (xsdk.XDBF_SECTION_STRING_TABLE, xsdk.LANGUAGE_ENGLISH, xstr),
        ]
        entries = b""
        data = b""
        for section, ident, blob in records:
            entries += struct.pack(">HQII", section, ident, len(data), len(blob))
            data += blob
        free = struct.pack(">II", len(data), 0xFFFFFFFF - len(data))
        count = len(records)
        return struct.pack(">4sIIIII", b"XDBF", xsdk.XDBF_VERSION, count, count, 1, 1) + entries + free + data

    xsdk.title_database = title_database
    sys.argv = [tool] + rest
    xsdk.main()


if __name__ == "__main__":
    main()
