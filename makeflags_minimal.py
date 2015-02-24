#!/usr/bin/env python
#
# Autotune flags to g++ to optimize the performance of cracking
# Adapted from the example from open-tuner
#
#import adddeps  # fix sys.path

import opentuner
from opentuner import ConfigurationManipulator
from opentuner import EnumParameter
from opentuner import IntegerParameter
from opentuner import MeasurementInterface
from opentuner import Result

import json

GCC_FLAGS = [
  'align-loops','tree-vectorize',
  'unroll-loops', 'sched-spec-load','align-loops',
  #'aggressive-loop-optimizations',
  'align-functions', 'align-jumps', 'align-labels',
  'asynchronous-unwind-tables',
  'branch-count-reg', 'branch-probabilities',
]

# (name, min, max)
GCC_PARAMS = [
#  ('early-inlining-insns', 0, 1000),
#  ('gcse-cost-distance-ratio', 0, 100),
#  ('iv-max-considered-uses', 0, 1000),
  # ... (145 total)
]


class GccFlagsTuner(MeasurementInterface):

  def manipulator(self):
    """
    Define the search space by creating a
    ConfigurationManipulator
    """
    manipulator = ConfigurationManipulator()
    manipulator.add_parameter(IntegerParameter('opt_level', 2, 3))
    manipulator.add_parameter(opentuner.search.manipulator.PowerOfTwoParameter('vectorsize', 32, 2048))
    manipulator.add_parameter(EnumParameter('debugsym', ['on', 'off'])) 

    pr = self.call_program('cat /proc/cpuinfo | grep processor | wc -l')
    assert pr['returncode'] == 0
    numprocs = int(pr['stdout'])
    manipulator.add_parameter(IntegerParameter('threads', numprocs/4-1, numprocs + 1))

    for flag in GCC_FLAGS:
      manipulator.add_parameter(
        EnumParameter(flag,
                      ['on', 'off', 'default']))

    for param, min, max in GCC_PARAMS:
      manipulator.add_parameter(
        IntegerParameter(param, min, max))
    return manipulator

  def run(self, desired_result, input, limit):
    """
    Compile and run a given configuration then
    return performance
    """
    cfg = desired_result.configuration.data
    outflags = ' -O{0}'.format(cfg['opt_level'])


    if cfg['debugsym'] == 'on':
      outflags += ' -g'

    for flag in GCC_FLAGS:
      if cfg[flag] == 'on':
        outflags += ' -f{0}'.format(flag)
      elif cfg[flag] == 'off':
        outflags += ' -fno-{0}'.format(flag)
    for param, min, max in GCC_PARAMS:
      outflags += ' --param {0}={1}'.format(
        param, cfg[param])

    gcc_cmd = 'make cracking_mt_alt_2_vectorized THREADS="%d" VECTORSIZE="%d" OUTFLAGS="%s"' % (cfg['threads'], cfg['vectorsize'], outflags)
    compile_result = self.call_program(gcc_cmd)
    assert compile_result['returncode'] == 0

    run_result = self.call_program('./bin/cracking_mt_alt_2_vectorized --sizemb 2048 --pivot 50')
    assert run_result['returncode'] == 0

    lat = int(json.loads(run_result['stdout'])['wallclockmilli'])
    return Result(time=lat)

  def save_final_config(self, configuration):
    """called at the end of tuning"""
    print configuration.data
    print "Best flags also  written to gccflags_final_config.json"
    self.manipulator().save_to_file(configuration.data,
                                    'gccflags_final_config.json')


if __name__ == '__main__':
  argparser = opentuner.default_argparser()
  GccFlagsTuner.main(argparser.parse_args())
