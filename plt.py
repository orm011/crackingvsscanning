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

results =  sorted(results, key=lambda r : (r.experiment, r.sizemb, r.pivot))

byalgo = itertools.groupby(results, lambda r : r.experiment)
for (algo,gralgo) in byalgo:
    bysize = itertools.groupby(gralgo, lambda r : r.sizemb)
    for (sz, grsz) in bysize:
        (xax, yax) = zip(*map((lambda r: (r.pivot, r.wallclockmilli)), grsz)) 
        y10millis = [ y / 10 for y in yax ]
        ax = pl.plot(xax, y10millis, '.-', label='%s %s MB' %(algo, sz))

lgd = pl.legend(loc='center left', bbox_to_anchor=(1, 0.5))        
pl.xlabel('pivot val within range (rel)')
pl.ylabel('time (10 millis)')
pl.savefig(name + '.png', bbox_extra_artists=(lgd,), bbox_inches='tight')
