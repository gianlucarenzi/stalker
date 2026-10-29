#!/usr/bin/env python3
"""
Creates the grouped BOM CSV for JLCPCB from the XML netlist that eeschema
writes with "Generate Bill of Materials".

It replaces the old bom2grouped_csv_jlcpcb.xsl + xsltproc step and gives
the same output: components grouped by Value, Footprint and LCSC field,
designators sorted by number, groups in the same order as before.

Only the standard library is used, so it runs with any Python 3 (Debian 11
system Python, the KiCad 5 AppImage Python 3.8, Debian 13 ...).

Usage: generate_bom.py <project.xml> <output.csv>
"""
import re
import sys
import xml.etree.ElementTree as ET

HEADER = "Comment,Designator,Footprint,LCSC"
LCSC_FIELD = "LCSC"


def get_field(comp, name):
    fields = comp.find("fields")
    if fields is None:
        return ""
    for field in fields.findall("field"):
        if field.get("name") == name:
            return field.text or ""
    return ""


def ref_number(ref):
    m = re.search(r"\d+$", ref)
    return int(m.group()) if m else 0


def quote(text):
    return '"' + text.replace('"', '""') + '"'


def generate_bom(xml_file, csv_file):
    root = ET.parse(xml_file).getroot()
    groups = {}
    for comp in root.find("components").findall("comp"):
        key = (comp.findtext("value") or "",
               comp.findtext("footprint") or "",
               get_field(comp, LCSC_FIELD))
        groups.setdefault(key, []).append(comp.get("ref"))

    # As the old XSL: a group is ordered by the designator of its first
    # component in the XML, compared as a string (so C13 comes before C1)
    rows = []
    for (value, footprint, lcsc), refs in groups.items():
        first = refs[0]
        refs = sorted(refs, key=ref_number)
        rows.append((first, [value, ",".join(refs), footprint, lcsc]))
    rows.sort(key=lambda row: row[0])

    lines = [HEADER] + [",".join(quote(col) for col in row) for _, row in rows]
    with open(csv_file, "w", encoding="utf-8", newline="") as out:
        out.write("\n".join(lines))
    print("BOM: {} groups written to '{}'.".format(len(rows), csv_file))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: {} <project.xml> <output.csv>".format(sys.argv[0]))
        sys.exit(1)
    generate_bom(sys.argv[1], sys.argv[2])
