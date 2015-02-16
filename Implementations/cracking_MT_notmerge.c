#include "../Framework/cracking_MT_notmerge.h"


/* revised single-threaded crack code (used in multicore alternatives)*/
void
standard_cracking_revised_notmerge (
	/* input */
	targetType* restrict const attribute,      /* attribute (array) */
	payloadType* payloadBuffer,
	targetType pivot, /* pivot value for attribute*/
	size_t first,  /* first position of to-be-cracked piece */
	size_t last,    /* last position of to-be-cracked piece */
	size_t ml,
	size_t mr,
	const targetType pivot_P
) {
	size_t pos_r;

	//assert(attribute);
	//assert((ml == 0 && mr == 0) || first + ml - 1 + 1 == last + 1 - mr);

	standard_cracking_revised_notmerge_x ( attribute, payloadBuffer, pivot, first, last, ml, mr, pivot_P);
}
void
standard_cracking_revised_notmerge_x (
	/* input */
	targetType* restrict const attribute,      /* attribute (array) */
	payloadType* payloadBuffer,
	const targetType pivot, /* pivot value for attribute*/
	const size_t first,  /* first position of to-be-cracked piece */
	const size_t last,    /* last position of to-be-cracked piece */
	const size_t ml,
	const size_t mr,
	const targetType pivot_P
) {
	size_t p = first, q = last, pp = p + ml - 1, qq = q + 1 - mr;

	//assert(attribute);

	if (ml && mr && pp + 1 < qq) {
		/* crack disjoint left- & right-half of piece / slice */
		while (p <= pp && q >= qq) {
			/* skip over smaller values from beginning */
			while (p <= pp && attribute[p] < pivot)
				p++;
			if (p > pp) {
				/* exhausted left half, skip to right one */
				p = qq;
				break; /* not really required */
			}
			else {
				/* skip over larger values from end */
				while (q >= qq && attribute[q] >= pivot)
					q--;
				if (q < qq) {
					/* exhausted right half, skip to left one */
					q = pp;
					break; /* not really required */
				} else {
					/* swap values */
					const targetType temp=attribute[p];
					attribute[p]=attribute[q];
					attribute[q]=temp;
				#ifdef DO_PAYLOAD_SHUFFLE
					const payloadType tempPayload=payloadBuffer[p];
					payloadBuffer[p]=payloadBuffer[q];
					payloadBuffer[q]=tempPayload;
				#endif
					p++;
					q--;
					if (p > pp) {
						/* exhausted left half, skip to right one */
						p = qq;
					}
					if (q < qq) {
						/* exhausted left half, skip to right one */
						q = pp;
					}
					if (p > pp || q < qq) {
						break; /* not really required */
					}
				}
			}
		}
	}

	/* crack (remaining) consequtive piece / slice */
	if (!(ml && mr && pp + 1 < qq) || p >= qq || q <= pp) {
		while (p < q) {
			/* skip over smaller values from beginning */
			while (p < q && attribute[p] < pivot) {
				p++;
			}
			/* skip over larger values from end */
			while (p < q && attribute[q] >= pivot) {
				q--;
			}
			if (p < q)
			{
				/* swap values */
				const targetType temp=attribute[p];
				attribute[p]=attribute[q];
				attribute[q]=temp;
			#ifdef DO_PAYLOAD_SHUFFLE
				const payloadType tempPayload=payloadBuffer[p];
				payloadBuffer[p]=payloadBuffer[q];
				payloadBuffer[q]=tempPayload;
			#endif
				p++;
				q--;
			}
		}
	}

}
/* crackThread for multi-threaded crack code */
void
cracking_MT_crackThread_notmerge ( c_Thread_t *arg )
{
        targetType pivot = * (targetType*) arg->pivot;
        targetType pivot_P = arg->pivotR;

        /*call revised cracking routine for this slice */
        standard_cracking_revised_notmerge_x (arg->b, arg->payload, pivot, arg->first, arg->last, arg->ml, arg->mr, pivot_P);
}
/* new multi-threaded crack code; Alternative 1+2 */
void
cracking_MT_notmerge (size_t first, size_t last, targetType *b, payloadType* payloadBuffer, targetType pivot, int nthreads, int alt, const targetType pivot_P)
{
        size_t n = last - first + 1; /* total # tuples / values */
        size_t ml;                   /* # tuples / values in left slice */
        size_t mr;                   /* # tuples / values in right slice */
        size_t f, l;                 /* first / last size_t per slice */
        c_Thread_t *c_Thread_arg; /* thread arguments array */
        int i, j;

        /* adjust nthreads */
        if ((size_t) nthreads > n / 10) {
                /* more threads / smaller slices does not make sense */
                nthreads = (int) (n / 10) + 1;
        }
        if (alt == 1) {
		ml = (n / nthreads);
		mr = 0;
	} else /* alt == 2 */ {
		size_t m = (n / nthreads);
		ml = (size_t) round((double) (m * pivot_P) / 100.0);
		mr = m - ml;
	}

        c_Thread_arg  = malloc(alt * nthreads * sizeof(c_Thread_t));
        if (!c_Thread_arg) {
                if (c_Thread_arg)
                        free(c_Thread_arg);
                fprintf (stderr, "cracking_MT(): malloc() failed.\n");
		exit(EXIT_FAILURE);
        }

        /* initialize crackThread arguments */
        if (alt == 1) {
                /* Alternative 1: each thread cracks one consecutive slice */
                for (i = 0, f = first, l = f + ml - 1; i < nthreads; i++, f += ml, l += ml) {
                        c_Thread_arg[i].b       = b;
                        c_Thread_arg[i].payload = payloadBuffer;
                        c_Thread_arg[i].pivot    = &pivot;
                        c_Thread_arg[i].first   = f;
                        c_Thread_arg[i].last    = (i < nthreads - 1) ? l : last;
                        c_Thread_arg[i].ml      = 0;
                        c_Thread_arg[i].mr      = 0;
                }
        } else /* alt == 2 */ {
                /* Alternative 2: nthreads-1 threads crack two disjoint half-slices (from either end of the input piece), each - last thread cracks center consecutive slice */
                for (i = 0, f = first, l = last; i < nthreads; i++, f += ml, l -= mr) {
                        c_Thread_arg[i].b       = b;
                        c_Thread_arg[i].payload = payloadBuffer;
                        c_Thread_arg[i].pivot    = &pivot;
                        c_Thread_arg[i].first   = f;
                        c_Thread_arg[i].last    = l;
                        if (nthreads!=1) {
                                c_Thread_arg[i].ml = (i < nthreads - 1) ? ml : l - f + 1;
                                c_Thread_arg[i].mr = (i < nthreads - 1) ? mr : 0;
                        } else {
                                c_Thread_arg[i].ml = 0;
                                c_Thread_arg[i].mr = 0;
                        }
                }
        }

	MRschedule(nthreads, c_Thread_arg, cracking_MT_crackThread_notmerge);

        free(c_Thread_arg);
}
