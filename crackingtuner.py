#!/usr/bin/env python
#import adddeps  # fix sys.path

import argparse
import logging

import opentuner
from opentuner.measurement import MeasurementInterface
from opentuner.search.manipulator import ConfigurationManipulator
from opentuner.search.manipulator import FloatParameter

parser = argparse.ArgumentParser(parents=opentuner.argparsers())

class Cracking(MeasurementInterface):
    def run(self, desired_result, input, limit):
        cfg = desired_result.configuration.data
        val = (cfg["x"]**2 + 10)
        return opentuner.resultsdb.models.Result(time=val)
        
    def manipulator(self):
        manipulator = ConfigurationManipulator()
        manipulator.add_parameter(FloatParameter("x", -10.0, 10.0))
        return manipulator

    def program_name(self):
        return "cracking"

    def save_final_config(self, configuration):
        print "result: ", configuration.data

if __name__ == '__main__':
    args = parser.parse_args()
    Cracking.main(args)
