import sys
import os
import re
import configparser
import collections

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
  ret['password_hash'] = basename.pop(0)

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


# Generate a plot for a file, and returns each iteration information
# in a dictionary
def gen_graph_for_file(f, destination, description_meta):
  header = None

  fig = plt.figure()
  plt.gca().set_position((.1, .4, .8, .5)) # to make a bit of room for extra text

  description = ["{}: {}".format(humanize(key), value) for key, value in description_meta.items()]
  plt.figtext(.9, .05, "\n".join(description), ha='right',
      bbox=dict(facecolor='none', edgecolor='black'), fontsize='small')

  plt.xlabel('Worker')
  plt.ylabel('Iterations')
  workers = 0

  rounds_info = list()

  for line in f:
    if not header:
      header = list(map(str.strip, line.split(',')))
      workers = len(header) - 3
      plt.barh(0, workers, align='center', alpha=0.4)
      # First one is zero
      plt.xlim(xmax=workers - 1)
      continue

    data = list(map(str.strip, line.split(',')))
    password = unquote(data.pop(0))
    decrypted = unquote(data.pop(0))
    time = unquote(data.pop(0))

    label = "{} ({}): {}s".format(password,
                                  decrypted,
                                  time)

    rounds_info.append({
      'password': password,
      'decrypted': decrypted,
      'time': float(time)
    })

    # Plot the lines and the points
    plt.plot(range(0, workers), list(map(int, data)), marker='o', label=label)

  font_props = FontProperties()
  font_props.set_size('small')
  plt.gca().legend(loc='lower left', bbox_to_anchor=(-0.05, -0.75),
                    ncol=1, fancybox=True, prop=font_props)
  plt.ylim(ymin=0)
  plt.savefig(destination)
  plt.close()

  return rounds_info

def gen_graphs_for_dir(directory):
  cases_by_passwords = collections.defaultdict(
                         lambda: collections.defaultdict(
                           lambda: collections.defaultdict(
                             lambda: collections.defaultdict(dict))))

  for subdir, dirs, files in os.walk(directory):
    for file in files:
      if not file.endswith('.csv'):
        continue

      config = configparser.ConfigParser()
      config.read(os.path.join(subdir, 'metadata.ini'))
      metadata = get_metadata_from_filename(file)
      metadata['processor_name'] = config.get('processor', 'name')
      metadata['processor_threads'] = config.get('processor', 'threads')
      metadata['processor_frequency'] = config.get('processor', 'frequency')

      path = os.path.join(subdir, file)
      rounds_info = None
      with open(path) as f:
        rounds_info = gen_graph_for_file(f, path + '.svg', metadata)

      assert rounds_info is not None
      metadata['rounds'] = rounds_info
      passwords = ''.join(map(lambda r: r['password'], rounds_info)).replace('/', '_')
      cases_by_passwords[passwords][metadata['compilation_mode']][metadata['running_mode']][metadata['job_size']][metadata['workers']] = metadata

      print(file)

  gen_graphs_by_password(directory, cases_by_passwords)

def gen_graphs_by_password(directory, cases_by_password):
  out_dir = os.path.join(directory, 'by-password')

  if not os.path.isdir(out_dir):
    os.makedirs(out_dir)

  for password_hash, case in cases_by_password.items():
    for compilation_mode, case in case.items():
      for running_mode, case in case.items():
        for job_size, case in case.items():
          out_file = os.path.join(out_dir, "{}-{}-job-size-{}-{}.svg".format(compilation_mode,
                                                                             running_mode,
                                                                             job_size,
                                                                             password_hash))
          gen_graph_for_case(out_file, case)

def gen_graph_for_case(out_file, case):
  fig = plt.figure()
  lines = []
  passwords = []

  cases = 0
  for workers, metadata in case.items():
    cases += 1
    total_time = 0
    assert workers == metadata['workers']
    i = 0
    for round_info in metadata['rounds']:
      if len(lines) == i:
        lines.append([])
        passwords.append(round_info['password'])

      lines[i].append((int(workers), round_info['time']))
      total_time += round_info['time']
      i += 1

    if len(lines) > 1:
      if len(lines) == i:
        lines.append([])
        passwords.append('Total')

      lines[i].append((int(workers), total_time))

  for i, points in enumerate(lines):
    # Sort by x
    points.sort(key=lambda p: p[0])
    x = list(map(lambda p: p[0], points))
    y = list(map(lambda p: p[1], points))
    plt.plot(x, y, marker='o', label=passwords[i])

  plt.xlabel('Number of workers')
  plt.ylabel('Computation time')
  plt.ylim(ymin=0)
  plt.legend()
  plt.savefig(out_file)

  plt.close()

def main():
  gen_graphs_for_dir(BENCH_DIR)

if __name__ == '__main__':
  main()
