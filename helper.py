import subprocess
import multiprocessing
import json
import sys
import ast

def get_time_milli(result):
    return json.loads(result)['wallclockmilli']

def get_sum(result):
    return json.loads(result)['sum']

def check_partition(result):
    assert(json.loads(result)['partitioned'] == 1)

def bench(cmd, env = None, n=5, check=False):
    print cmd, env
    outputs = [subprocess.check_output(cmd, env=env) for i in range(n)]
    results = [get_time_milli(o) for o in outputs]
    if check:
        [check_partition(o) for o in outputs] 
        sums = [ get_sum(o) for o in outputs]
        assert(len(set(sums)) == 1) #only one workload => only one sum
        
    return sorted(results)

def median(arr):
    return sorted(arr)[len(arr)/2]

def sort_by_median(dict):
    return sorted([ (median(res), k, res) for (k,res) in dict.items()])

def framework_command(program, sizemb, pivot):
    return  ['./bin/' + program, 
             '--sizemb',
             str(sizemb), 
             '--pivot',
             str(pivot)]

def cracking_exp(sizemb, reps=5, only=False, pivot=50, env={}):
    immutable = ['scanning', 'copying']
    in_place = ['cracking_mt_alt_2_vectorized']
    if not only:
        programs = immutable + in_place
    else:
        programs = in_place
    
    results = {}
    for prg in programs:
        cmd = framework_command(prg, sizemb, pivot)
        results[str(cmd)] = bench(cmd, env=env, 
                                  n=reps, check=(prg not in immutable))
    return sort_by_median(results)

def affinity_exp(sizemb, reps):
    assert (pivot > 0 and pivot < 100 )
    threadn = multiprocessing.cpu_count()

    cmd = ['./bin/scanning', 
           '--sizemb',
           str(sizemb), 
           '--pivot',
           str(50)]

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
elif sys.argv[1] == 'compare':
    print_by_line(cracking_exp(int(sys.argv[2])))
elif sys.argv[1] == 'cracking':
    print_by_line(cracking_exp(int(sys.argv[2]), reps=10, only=True, env=ast.literal_eval(sys.argv[3])))
else:
    assert false

