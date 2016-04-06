#! /usr/bin/env python

"""
Pandoc filter to convert svg files to pdf as suggested at:
https://github.com/jgm/pandoc/issues/265#issuecomment-27317316
"""

__author__ = "Jerome Robert"

import mimetypes
import subprocess
import os
import sys
import base64
import binascii
from pandocfilters import toJSONFilter, Image

#TODO add emf export if fmt=="docx" ?
fmt_to_option = {
  "latex": ("--export-pdf","pdf"),
  "beamer": ("--export-pdf","pdf"),
}

def svg_to_any(key, value, fmt, meta):
  if key == 'Image':
    src = value[2][0]
    mimet,_ = mimetypes.guess_type(src)
    option = fmt_to_option.get(fmt)
    eps_name = None
    if mimet == 'image/svg+xml':
      if option:
        base_name,_ = os.path.splitext(src)
        eps_name = base_name + "." + option[1]
        try:
          mtime = os.path.getmtime(eps_name)
        except OSError:
          mtime = -1
        if mtime < os.path.getmtime(src):
          cmd_line = ['inkscape', option[0], eps_name, src]
          sys.stderr.write("Running %s\n" % " ".join(cmd_line))
          subprocess.check_call(cmd_line, stdout=sys.stderr.fileno())
      elif fmt == 'html' or fmt == 'revealjs':
        with open(src, "rb") as f:
          eps_name = 'data:image/svg+xml;base64,' + base64.b64encode(f.read()).decode("utf-8")

      if eps_name:
        value[2][0] = eps_name

      return Image(value[0], value[1], value[2])

if __name__ == "__main__":
  toJSONFilter(svg_to_any)
