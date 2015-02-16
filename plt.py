import json
import sys
import string
from collections import namedtuple
import matplotlib.pyplot as pl
import itertools

if len(sys.argv) < 1:
    print "usage: %s %s" % (sys.argv[0], "filename")

def objhook(d): return namedtuple('TestResult', 
                                  map(lambda x: string.lower(x), d.keys()))(*d.values())

name = sys.argv[1]
results = []
with open(name, 'r') as r:
    for l in r:
        results.append(json.loads(l, object_hook=objhook))

results =  sorted(results, key=lambda r : (r.experiment, r.fieldsize, r.pivot))

byalgo = itertools.groupby(results, lambda r : r.experiment)
for (algo,gralgo) in byalgo:
    bysize = itertools.groupby(gralgo, lambda r : r.fieldsize)
    for (sz, grsz) in bysize:
        (xax, yax) = zip(*map((lambda r: (r.pivot, r.wallclock)), grsz)) 
        ax = pl.plot(xax,yax, 'o-', label='%s %s MB' %(algo, sz))

lgd = pl.legend(loc='center left', bbox_to_anchor=(1, 0.5))        
pl.savefig(name + '.png', bbox_extra_artists=(lgd,), bbox_inches='tight')

