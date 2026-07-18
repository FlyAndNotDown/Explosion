#!/usr/bin/env python3

"""Generate the C++ Tabler icon catalog from an official webfont release."""

from __future__ import annotations

import argparse
import re
import struct
import sys
from dataclasses import dataclass
from pathlib import Path


REPOSITORY_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_FONT_PATH = REPOSITORY_ROOT / "Editor" / "Resource" / "Icons" / "Tabler" / "tabler-icons.ttf"
DEFAULT_OUTPUT_PATH = REPOSITORY_ROOT / "Editor" / "Include" / "Editor" / "Widget" / "TablerIcons.h"

ICON_PATTERN = re.compile(r'\.ti-([a-z0-9-]+):before\s*\{\s*content:\s*"\\([0-9a-fA-F]+)";\s*\}')
VERSION_PATTERN = re.compile(r"Tabler Icons\s+([0-9]+\.[0-9]+\.[0-9]+)")
CPP_KEYWORDS = frozenset(
    {
        "alignas",
        "alignof",
        "and",
        "and_eq",
        "asm",
        "atomic_cancel",
        "atomic_commit",
        "atomic_noexcept",
        "auto",
        "bitand",
        "bitor",
        "bool",
        "break",
        "case",
        "catch",
        "char",
        "char8_t",
        "char16_t",
        "char32_t",
        "class",
        "compl",
        "concept",
        "const",
        "const_cast",
        "consteval",
        "constexpr",
        "constinit",
        "continue",
        "co_await",
        "co_return",
        "co_yield",
        "decltype",
        "default",
        "delete",
        "do",
        "double",
        "dynamic_cast",
        "else",
        "enum",
        "explicit",
        "export",
        "extern",
        "false",
        "float",
        "for",
        "friend",
        "goto",
        "if",
        "inline",
        "int",
        "long",
        "mutable",
        "namespace",
        "new",
        "noexcept",
        "not",
        "not_eq",
        "nullptr",
        "operator",
        "or",
        "or_eq",
        "private",
        "protected",
        "public",
        "reflexpr",
        "register",
        "reinterpret_cast",
        "requires",
        "return",
        "short",
        "signed",
        "sizeof",
        "static",
        "static_assert",
        "static_cast",
        "struct",
        "switch",
        "synchronized",
        "template",
        "this",
        "thread_local",
        "throw",
        "true",
        "try",
        "typedef",
        "typeid",
        "typename",
        "union",
        "unsigned",
        "using",
        "virtual",
        "void",
        "volatile",
        "wchar_t",
        "while",
        "xor",
        "xor_eq",
    }
)


class GenerationError(RuntimeError):
    pass


@dataclass(frozen=True)
class Icon:
    css_name: str
    identifier: str
    codepoint: int


def read_u16(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise GenerationError("Unexpected end of TTF data while reading a 16-bit value")
    return struct.unpack_from(">H", data, offset)[0]


def read_i16(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 2 > len(data):
        raise GenerationError("Unexpected end of TTF data while reading a signed 16-bit value")
    return struct.unpack_from(">h", data, offset)[0]


def read_u32(data: bytes, offset: int) -> int:
    if offset < 0 or offset + 4 > len(data):
        raise GenerationError("Unexpected end of TTF data while reading a 32-bit value")
    return struct.unpack_from(">I", data, offset)[0]


def ttf_table(font_data: bytes, tag: bytes) -> bytes:
    if len(font_data) < 12:
        raise GenerationError("TTF offset table is truncated")
    table_count = read_u16(font_data, 4)
    records_end = 12 + table_count * 16
    if records_end > len(font_data):
        raise GenerationError("TTF table directory is truncated")
    for index in range(table_count):
        record_offset = 12 + index * 16
        if font_data[record_offset : record_offset + 4] != tag:
            continue
        table_offset = read_u32(font_data, record_offset + 8)
        table_length = read_u32(font_data, record_offset + 12)
        if table_offset + table_length > len(font_data):
            raise GenerationError(f"TTF {tag.decode('ascii')} table is truncated")
        return font_data[table_offset : table_offset + table_length]
    raise GenerationError(f"TTF does not contain a {tag.decode('ascii')} table")


def parse_cmap_format_4(cmap: bytes, subtable_offset: int) -> set[int]:
    length = read_u16(cmap, subtable_offset + 2)
    subtable_end = subtable_offset + length
    if subtable_end > len(cmap):
        raise GenerationError("TTF cmap format 4 subtable is truncated")

    segment_count = read_u16(cmap, subtable_offset + 6) // 2
    end_codes_offset = subtable_offset + 14
    start_codes_offset = end_codes_offset + segment_count * 2 + 2
    deltas_offset = start_codes_offset + segment_count * 2
    range_offsets_offset = deltas_offset + segment_count * 2
    if range_offsets_offset + segment_count * 2 > subtable_end:
        raise GenerationError("TTF cmap format 4 segment arrays are truncated")

    codepoints: set[int] = set()
    for index in range(segment_count):
        end_code = read_u16(cmap, end_codes_offset + index * 2)
        start_code = read_u16(cmap, start_codes_offset + index * 2)
        delta = read_i16(cmap, deltas_offset + index * 2)
        range_offset_position = range_offsets_offset + index * 2
        range_offset = read_u16(cmap, range_offset_position)
        if start_code > end_code:
            raise GenerationError("TTF cmap format 4 contains an invalid segment")

        for codepoint in range(start_code, end_code + 1):
            if codepoint == 0xFFFF:
                continue
            if range_offset == 0:
                glyph_index = (codepoint + delta) & 0xFFFF
            else:
                glyph_position = range_offset_position + range_offset + (codepoint - start_code) * 2
                if glyph_position + 2 > subtable_end:
                    raise GenerationError("TTF cmap format 4 glyph array is truncated")
                glyph_index = read_u16(cmap, glyph_position)
                if glyph_index != 0:
                    glyph_index = (glyph_index + delta) & 0xFFFF
            if glyph_index != 0:
                codepoints.add(codepoint)
    return codepoints


def parse_cmap_format_12(cmap: bytes, subtable_offset: int) -> set[int]:
    length = read_u32(cmap, subtable_offset + 4)
    subtable_end = subtable_offset + length
    if subtable_end > len(cmap):
        raise GenerationError("TTF cmap format 12 subtable is truncated")

    group_count = read_u32(cmap, subtable_offset + 12)
    groups_offset = subtable_offset + 16
    if groups_offset + group_count * 12 > subtable_end:
        raise GenerationError("TTF cmap format 12 groups are truncated")

    codepoints: set[int] = set()
    for index in range(group_count):
        group_offset = groups_offset + index * 12
        start_codepoint = read_u32(cmap, group_offset)
        end_codepoint = read_u32(cmap, group_offset + 4)
        start_glyph = read_u32(cmap, group_offset + 8)
        if start_codepoint > end_codepoint or end_codepoint > 0x10FFFF:
            raise GenerationError("TTF cmap format 12 contains an invalid group")
        for codepoint in range(start_codepoint, end_codepoint + 1):
            if start_glyph + codepoint - start_codepoint != 0:
                codepoints.add(codepoint)
    return codepoints


def parse_ttf_codepoints(font_path: Path) -> set[int]:
    try:
        font_data = font_path.read_bytes()
    except OSError as error:
        raise GenerationError(f"Could not read TTF '{font_path}': {error}") from error
    cmap = ttf_table(font_data, b"cmap")
    if len(cmap) < 4:
        raise GenerationError("TTF cmap table is truncated")

    subtable_count = read_u16(cmap, 2)
    if 4 + subtable_count * 8 > len(cmap):
        raise GenerationError("TTF cmap encoding records are truncated")

    codepoints: set[int] = set()
    parsed_offsets: set[int] = set()
    for index in range(subtable_count):
        record_offset = 4 + index * 8
        platform_id = read_u16(cmap, record_offset)
        encoding_id = read_u16(cmap, record_offset + 2)
        if platform_id != 0 and not (platform_id == 3 and encoding_id in {1, 10}):
            continue
        subtable_offset = read_u32(cmap, record_offset + 4)
        if subtable_offset in parsed_offsets:
            continue
        parsed_offsets.add(subtable_offset)
        cmap_format = read_u16(cmap, subtable_offset)
        if cmap_format == 4:
            codepoints.update(parse_cmap_format_4(cmap, subtable_offset))
        elif cmap_format == 12:
            codepoints.update(parse_cmap_format_12(cmap, subtable_offset))

    if not codepoints:
        raise GenerationError("TTF cmap does not contain a supported Unicode format 4 or 12 subtable")
    return codepoints


def cpp_identifier(css_name: str) -> str:
    parts = css_name.split("-")
    identifier = parts[0] + "".join(part[:1].upper() + part[1:] for part in parts[1:])
    if identifier[0].isdigit():
        identifier = "icon" + identifier[:1].upper() + identifier[1:]
    if identifier in CPP_KEYWORDS:
        identifier += "Icon"
    return identifier


def utf8_literal(codepoint: int) -> str:
    try:
        encoded = chr(codepoint).encode("utf-8")
    except (ValueError, UnicodeEncodeError) as error:
        raise GenerationError(f"Invalid Unicode codepoint U+{codepoint:04X}") from error
    return "".join(f"\\x{byte:02x}" for byte in encoded)


def parse_css(css_path: Path) -> tuple[str, list[Icon]]:
    try:
        css = css_path.read_text(encoding="utf-8")
    except OSError as error:
        raise GenerationError(f"Could not read CSS '{css_path}': {error}") from error

    version_match = VERSION_PATTERN.search(css)
    if version_match is None:
        raise GenerationError("Could not determine the Tabler Icons version from the CSS header")

    raw_icons = ICON_PATTERN.findall(css)
    if not raw_icons:
        raise GenerationError("No Tabler icon declarations were found in the CSS")

    icons = [Icon(css_name, cpp_identifier(css_name), int(codepoint, 16)) for css_name, codepoint in raw_icons]
    css_names = [icon.css_name for icon in icons]
    identifiers = [icon.identifier for icon in icons]
    if len(set(css_names)) != len(css_names):
        raise GenerationError("The CSS contains duplicate icon names")
    if len(set(identifiers)) != len(identifiers):
        collisions = sorted(identifier for identifier in set(identifiers) if identifiers.count(identifier) > 1)
        raise GenerationError(f"Generated C++ identifier collisions: {', '.join(collisions)}")
    return version_match.group(1), icons


def validate_font(icons: list[Icon], font_path: Path) -> set[int]:
    css_codepoints = {icon.codepoint for icon in icons}
    font_codepoints = parse_ttf_codepoints(font_path)
    if css_codepoints != font_codepoints:
        missing_from_font = sorted(css_codepoints - font_codepoints)
        missing_from_css = sorted(font_codepoints - css_codepoints)
        format_codepoints = lambda values: ", ".join(f"U+{value:04X}" for value in values[:8]) or "none"
        raise GenerationError(
            "CSS and TTF codepoints differ; "
            f"missing from TTF: {format_codepoints(missing_from_font)}; "
            f"missing from CSS: {format_codepoints(missing_from_css)}"
        )
    return css_codepoints


def render_header(version: str, icons: list[Icon], codepoints: set[int]) -> str:
    lines = [
        "#pragma once",
        "",
        f"// Generated from @tabler/icons-webfont v{version} by Tool/Scripts/IconFontGenerator/tabler.py.",
        "namespace Editor::Icons::Tabler {",
    ]
    for icon in icons:
        lines.append(f'    inline constexpr char {icon.identifier}[] = "{utf8_literal(icon.codepoint)}";')

    lines.extend(["", "    inline constexpr char allGlyphs[] ="])
    sorted_codepoints = sorted(codepoints)
    for offset in range(0, len(sorted_codepoints), 32):
        literal = "".join(utf8_literal(codepoint) for codepoint in sorted_codepoints[offset : offset + 32])
        suffix = ";" if offset + 32 >= len(sorted_codepoints) else ""
        lines.append(f'        "{literal}"{suffix}')
    lines.extend(["}", ""])
    return "\n".join(lines)


def create_argument_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("css", type=Path, help="Path to dist/tabler-icons.css from the official webfont package")
    parser.add_argument("--font", type=Path, default=DEFAULT_FONT_PATH, help=f"TTF to validate (default: {DEFAULT_FONT_PATH})")
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT_PATH, help=f"Generated header path (default: {DEFAULT_OUTPUT_PATH})")
    parser.add_argument("--check", action="store_true", help="Verify that the output is current without modifying it")
    return parser


def run(arguments: argparse.Namespace) -> int:
    version, icons = parse_css(arguments.css.resolve())
    codepoints = validate_font(icons, arguments.font.resolve())
    generated = render_header(version, icons, codepoints)
    output_path = arguments.output.resolve()

    if arguments.check:
        try:
            current = output_path.read_text(encoding="ascii")
        except OSError:
            current = None
        if current != generated:
            print(f"Outdated generated header: {output_path}", file=sys.stderr)
            return 1
        print(f"Validated {len(icons)} names and {len(codepoints)} glyphs in {output_path}")
        return 0

    output_path.parent.mkdir(parents=True, exist_ok=True)
    if output_path.exists() and output_path.read_text(encoding="ascii") == generated:
        action = "Unchanged"
    else:
        output_path.write_text(generated, encoding="ascii", newline="\n")
        action = "Generated"
    print(f"{action} {output_path}: Tabler Icons {version}, {len(icons)} names, {len(codepoints)} glyphs")
    return 0


def main() -> int:
    parser = create_argument_parser()
    arguments = parser.parse_args()
    try:
        return run(arguments)
    except GenerationError as error:
        parser.error(str(error))
    return 2


if __name__ == "__main__":
    raise SystemExit(main())
