#!/usr/bin/env python3

import markdown
import re
import sys
import xml.etree.ElementTree as ET

[changelog_file, xml_template_file, xml_output_file] = sys.argv[1:]

with open(changelog_file) as fp:
    md_text = fp.read()

html_content = markdown.markdown(md_text)
html_root = ET.fromstring(f"<root>{html_content}</root>")
release_re = re.compile(r"(?P<version>[\d.]+) (?P<type>[a-z]+) \((?P<date>\d{4}-\d{2}-\d{2})\)")

tree = ET.parse(xml_template_file)
releases = tree.getroot().find("releases")
release_elem = None

for elem in html_root:
    if elem.tag == "h2":
        if elem.text[0].isdecimal():
            match = release_re.match(elem.text)
            release = match.groupdict()
            if release["type"] == "beta":
                release["type"] = "development"
            release_elem = ET.SubElement(releases, "release", release)
    elif elem.tag == "ul" and release_elem is not None:
        for child_elem in elem.iter():
            # https://www.freedesktop.org/software/appstream/docs/chap-Metadata.html#tag-description
            if child_elem.tag == "strong":
                child_elem.tag = "em"
        desc_elem = ET.SubElement(release_elem, "description")
        desc_elem.append(elem)

ET.indent(tree)
tree.write(xml_output_file, encoding="utf-8", xml_declaration=True)
