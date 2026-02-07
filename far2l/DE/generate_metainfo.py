#!/usr/bin/env python3

import re
import sys
import xml.etree.ElementTree as ET

import markdown
from markdown.inlinepatterns import LINK_RE, LinkInlineProcessor

[changelog_file, xml_template_file, xml_output_file] = sys.argv[1:]


class StripLinksInlineProcessor(LinkInlineProcessor):
    def handleMatch(self, m, data):
        node, start, end = super().handleMatch(m, data)
        if node is not None:
            node = node.text
        return node, start, end


with open(changelog_file) as fp:
    md_text = fp.read()

md = markdown.Markdown()
md.inlinePatterns.register(
    StripLinksInlineProcessor(LINK_RE, md),
    name="link",
    priority=160,
)

html_content = md.convert(md_text)
html_root = ET.fromstring(f"<root>{html_content}</root>")
release_re = re.compile(r"(?P<version>[\d.]+) (?P<type>[a-z]+) \((?P<date>\d{4}-\d{2}-\d{2})\)")

tree = ET.parse(xml_template_file)
releases = tree.getroot().find("releases")
release_elem = None

for elem in html_root:
    if elem.tag == "h2":
        if match := release_re.match(elem.text):
            release = match.groupdict()
            if release["type"] == "beta":
                release["type"] = "development"
            release_elem = ET.SubElement(releases, "release", release)
        elif elem.text != "Master (current development)":
            print(
                f"Warning: cannot parse header {elem.text!r} in {changelog_file}",
                file=sys.stderr,
            )
    elif elem.tag == "ul" and release_elem is not None:
        for child_elem in elem.iter():
            # https://www.freedesktop.org/software/appstream/docs/chap-Metadata.html#tag-description
            if child_elem.tag == "strong":
                child_elem.tag = "em"
        desc_elem = ET.SubElement(release_elem, "description")
        desc_elem.append(elem)

ET.indent(tree)
tree.write(xml_output_file, encoding="utf-8", xml_declaration=True)
