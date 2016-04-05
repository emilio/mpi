#!/usr/bin/env python

"""
Pandoc filter to remove Horizontal rules and remove the
lasts slides
"""

import sys
from pandocfilters import toJSONFilter, Emph, Para, Str

found_ending = False

def behead(key, value, format, meta):
  global found_ending

  # Remove horizontal rules
  if key == 'HorizontalRule':
    return []

  # Remove everything from the 'preguntas' section
  if key == 'Header' and value[0] == 1 and value[1][0] == 'preguntas':
    found_ending = True
    return []

  # Don't return anything after finding the end
  if found_ending:
    return []

if __name__ == "__main__":
  toJSONFilter(behead)
  # sys.exit(1)
