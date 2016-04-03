import sys
import os
import re
import configparser

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.font_manager import FontProperties

BENCH_DIR = os.path.dirname(os.path.realpath(__file__))

def get_metadata_from_filename(name):
  ret = {}
  basename = os.path.basename(name).split('-')
  ret['compilation_mode'] = basename.pop(0)
  ret['running_mode'] = basename.pop(0)
  ret['workers'] = basename.pop(0)
  assert basename.pop(0) == 'workers'
  ret['password_count'] = basename.pop(0)
  assert basename.pop(0) == 'passwords'
  assert basename.pop(0) == 'job'
  assert basename.pop(0) == 'size'
  ret['job_size'] = basename.pop(0)

  return ret

def unquote(string):
  if string[0] == "\"":
    string = string[1:]

  if string[-1] == "\"":
    string = string[:-1]

  return string


def humanize(snake_cased):
  s = re.sub('_', ' ', snake_cased)
  if s:
    return s[:1].upper() + s[1:]
  return s


def gen_graph_for_file(f, destination, metadata):
  header = None

  fig = plt.figure()
  description_meta = get_metadata_from_filename(destination)
  description_meta['processor_name'] = metadata.get('processor', 'name');
  description_meta['processor_threads'] = metadata.get('processor', 'threads');
  description_meta['processor_frequency'] = metadata.get('processor', 'frequency');

  plt.gca().set_position((.1, .4, .8, .5)) # to make a bit of room for extra text

  description = ["{}: {}".format(humanize(key), value) for key, value in description_meta.items()]
  plt.figtext(.9, .05, "\n".join(description), ha='right',
      bbox=dict(facecolor='none', edgecolor='black'), fontsize='small')

  plt.xlabel('Worker')
  plt.ylabel('Iterations')
  workers = 0

  for line in f:
    if not header:
      header = list(map(str.strip, line.split(',')))
      workers = len(header) - 3
      plt.barh(0, workers, align='center', alpha=0.4)
      # First one is zero
      plt.xlim(xmax=workers - 1)
      continue

    data = list(map(str.strip, line.split(',')))
    label = "{} ({}): {}s".format(unquote(data.pop(0)),
                                  unquote(data.pop(0)),
                                  unquote(data.pop(0)))
    # Plot the lines and the points
    plt.plot(range(0, workers), list(map(int, data)), marker='o', label=label)

  font_props = FontProperties()
  font_props.set_size('small')
  plt.gca().legend(loc='lower left', bbox_to_anchor=(-0.05, -0.75),
                    ncol=1, fancybox=True, prop=font_props)
  plt.ylim(ymin=0)
  plt.savefig(destination)

def gen_graphs_for_dir(directory):
  for subdir, dirs, files in os.walk(directory):
    for file in files:
      if not file.endswith('.csv'):
        continue

      metadata = configparser.ConfigParser()
      metadata.read(os.path.join(subdir, 'metadata.ini'))

      path = os.path.join(subdir, file)
      with open(path) as f:
        gen_graph_for_file(f, path + '.svg', metadata)

      print(path)

def main():
  gen_graphs_for_dir(BENCH_DIR)

if __name__ == '__main__':
  main()
