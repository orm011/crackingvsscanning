import subprocess
import multiprocessing
import json

def get_time_millis(result):
    return json.loads(result)['wallclockmilli']

def affinity_exp(sizemb, pivot, reps):
    assert (pivot > 0 and pivot < 100 )
    threadn = multiprocessing.cpu_count()

    envAffinity={'OMP_NUM_THREADS':str(threadn),
         'GOMP_CPU_AFFINITY':"0-{}".format(threadn-1) }

    cmd = ['./bin/scanning', 
           '--sizemb',
           str(sizemb), 
           '--pivot',
           str(pivot)]

    times_affinity = []
    for i in range(reps):
        result =  subprocess.check_output(cmd, env=envAffinity)
        times_affinity.append(get_time_millis(result))
        
    envNoAffinity={'OMP_NUM_THREADS':str(threadn)}
    
    times_naffinity = []
    for i in range(reps):
        result =  subprocess.check_output(cmd, env=envNoAffinity)
        times_naffinity.append(get_time_millis(result))
        
    return { 'affinity':sorted(times_affinity), 
             'no affinity':sorted(times_naffinity)}
