import subprocess
import multiprocessing
import json
import sys

def get_time_milli(result):
    return json.loads(result)['wallclockmilli']

def bench(cmd, env = None, n=5):
    print cmd, env
    results = [get_time_milli(subprocess.check_output(cmd, env=env)) for i in range(n)]
    return sorted(results)

def median(arr):
    return sorted(arr)[len(arr)/2]

def sort_by_median(dict):
    return sorted([ (median(res), k, res) for (k,res) in dict.items()])

def affinity_exp(sizemb, pivot, reps):
    assert (pivot > 0 and pivot < 100 )
    threadn = multiprocessing.cpu_count()

    cmd = ['./bin/scanning', 
           '--sizemb',
           str(sizemb), 
           '--pivot',
           str(pivot)]

    env ={'OMP_NUM_THREADS':str(threadn)}
    res1 = bench(cmd, env)
    
    env['GOMP_CPU_AFFINITY'] = "0-{}".format(threadn-1)
    res2 = bench(cmd, env)

    return sort_by_median({'no_affinity':res1, 'affinity':res2})

def print_by_line(lst):
    for l in lst:
        print l

def memcpy_exp(sizemb):
    cmd_base  = ['./bin/memcpybench', '-s', str(sizemb), '-a']
    vars = ['memcpy', 'naive', 'nt', 'builtin']
    return sort_by_median(dict([ (v, bench(cmd_base + [v])) for v in vars ]))

if sys.argv[1] == 'affinity':
    print_by_line(affinity_exp(int(sys.argv[2]), 1, 5))
elif sys.argv[1] == 'memcpy':
    print_by_line(memcpy_exp(int(sys.argv[2])))
else:
    assert false

