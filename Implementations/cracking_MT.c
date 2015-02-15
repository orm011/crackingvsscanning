#include "../Framework/cracking_MT.h"


/* revised single-threaded crack code (used in multicore alternatives)*/
void
standard_cracking_revised (
	/* input */
	targetType* restrict const attribute,      /* attribute (array) */
	payloadType* payloadBuffer, 
	targetType pivot, /* pivot value for attribute*/
	size_t first,  /* first position of to-be-cracked piece */
	size_t last,    /* last position of to-be-cracked piece */
	size_t ml,
	size_t mr,
	size_t *pos, /*return position*/
	const targetType pivot_P
) {
	size_t pos_r;

	//assert(attribute);
	//assert(pos);
	//assert((ml == 0 && mr ==0) || first + ml - 1 + 1 == last + 1 - mr);

	standard_cracking_revised_x ( attribute, payloadBuffer, pivot, first, last, ml, mr, &pos_r, pivot_P);
	*pos = (size_t) (pos_r == 0 ? 0 : pos_r - 1);
}
void
standard_cracking_revised_x (
	/* input */
	targetType* restrict const attribute,      /* attribute (array) */
	payloadType* payloadBuffer, 
	const targetType pivot, /* pivot value for attribute*/
	const size_t first,  /* first position of to-be-cracked piece */
	const size_t last,    /* last position of to-be-cracked piece */
	const size_t ml, 
	const size_t mr, 
	size_t *pos_r, /*return position*/
	const targetType pivot_P
) {
	size_t p = first, q = last, pp = p + ml - 1, qq = q + 1 - mr;

	//assert(attribute);
	//assert(pos_r);

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

	/* return pivot position, i.e., first(!) pos in right(!) piece */
	//assert(p >= first && p <= last);
	if (ml && mr && pp + 1 < qq) {
		if (p <= pp + 1) {
			//assert(p == first || attribute[p-1] < pivot);
			while (p <= pp && attribute[p] < pivot)
				p++;
			if (p == pp + 1)
				p = qq;
		}
		if (p >= qq) {
			//assert((p == qq && attribute[pp] < pivot) || (p > qq && attribute[p-1] < pivot));
			while (p <= last && attribute[p] < pivot)
				p++;
		}
	} else {
		//assert(p == first || attribute[p-1] < pivot);
		while (p <= last && attribute[p] < pivot)
			p++;
	}

	//assert(p >= first && p <= last + 1);
	//assert(p == first || (ml && mr && pp + 1 < qq && p == qq && attribute[pp] < pivot) || (!(ml && mr && pp + 1 < qq && p == qq) && attribute[p-1] < pivot));
	//assert(p == last + 1 || attribute[p] >= pivot);
	*pos_r = p;
}
/* crackThread for multi-threaded crack code */
void
cracking_MT_crackThread ( c_Thread_t *arg )
{
        size_t pos_r;
        targetType pivot = * (targetType*) arg->pivot;
	targetType pivot_P = PIVOT;

        /*call revised cracking routine for this slice */
        standard_cracking_revised_x (arg->b, arg->payload, pivot, arg->first, arg->last, arg->ml, arg->mr, &pos_r, pivot_P);

        arg->pos_r = pos_r;
}
/* new multi-threaded crack code; Alternative 1+2 */
void
cracking_MT (size_t first, size_t last, targetType *b, payloadType* payloadBuffer, targetType pivot, size_t *pos, int nthreads, int alt, const targetType pivot_P)
{
        size_t n = last - first + 1; /* total # tuples / values */
        size_t ml;                   /* # tuples / values in left slice */
        size_t mr;                   /* # tuples / values in right slice */
        size_t mm;                   /* # tuples / values in tmp arrays */
        size_t f, l;                 /* first / last size_t per slice */
        c_Thread_t *c_Thread_arg; /* thread arguments array */
        int i, j;
	targetType *temp;

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
        mm = ml + mr;

#ifdef DO_PAYLOAD_SHUFFLE
	payloadType *tempPayload;
	tempPayload = malloc(mm * sizeof(payloadType));
	if (!tempPayload) {
                if (tempPayload)
       	                free(tempPayload);
		fprintf (stderr, "cracking_MT(): malloc() failed.\n");
		exit(EXIT_FAILURE);
	}
#endif
	temp = malloc(mm * sizeof(targetType));
        c_Thread_arg  = malloc(alt * nthreads * sizeof(c_Thread_t));
        if (!temp || !c_Thread_arg) {
                if (temp)
                        free(temp);
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
                        c_Thread_arg[i].pivot   = &pivot;
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
                        c_Thread_arg[i].pivot   = &pivot;
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

	MRschedule(nthreads, c_Thread_arg, cracking_MT_crackThread);

        /* "meta-crack": move "wrong" parts of slices to correct final location */
        if (alt == 2) {
                /* Alt. 2: treat each half-slice as individual slice */
                for (i = 0, j = 2 * nthreads - 2; i < nthreads - 1; i++, j--) {
			//assert(i < j - 1);
			//assert(c_Thread_arg[i].ml > 0 && c_Thread_arg[i].mr > 0);
                        c_Thread_arg[j].ml    = c_Thread_arg[i].mr;
                        c_Thread_arg[j].last  = c_Thread_arg[i].last;
                        c_Thread_arg[j].first = c_Thread_arg[i].last  - c_Thread_arg[i].mr + 1;
                        c_Thread_arg[i].last  = c_Thread_arg[i].first + c_Thread_arg[i].ml - 1;
			//assert(first <= c_Thread_arg[i].first);
			//assert(c_Thread_arg[i].first < c_Thread_arg[i].last);
			//assert(c_Thread_arg[i].last /*+ 1*/ < c_Thread_arg[j].first);
			//assert(c_Thread_arg[j].first < c_Thread_arg[j].last);
			//assert(c_Thread_arg[j].last <= last);
			if (c_Thread_arg[i].pos_r < c_Thread_arg[j].first) {
				c_Thread_arg[j].pos_r = c_Thread_arg[j].first;
			} else {
				c_Thread_arg[j].pos_r = c_Thread_arg[i].pos_r;
				c_Thread_arg[i].pos_r = c_Thread_arg[i].last + 1;
			}
                        c_Thread_arg[j].mr = c_Thread_arg[i].mr = 0;
                }
		//assert(i == j);       
	}
        i = 0;
        j = alt * nthreads - alt;
        while (i < j) {
		/* skip over entirely smaller slices from beginning */
		while (i < j && c_Thread_arg[i].pos_r > c_Thread_arg[i].last)
                        i++;
		{
			/* skip over entirely larger slices from end */
			while (i < j && (c_Thread_arg[j].pos_r <=  c_Thread_arg[j].first))
				j--;
			if (i < j) {
				/* size of "wrong" part of left slice with larger values */
				const size_t si = c_Thread_arg[i].last + 1 - c_Thread_arg[i].pos_r;
				/* size of "wrong" part of right slice with smaller values */
				const size_t sj = c_Thread_arg[j].pos_r - c_Thread_arg[j].first;
				const size_t sk = si > sj ? sj : si;
				const size_t st = sk * sizeof(targetType);
				const size_t pi = c_Thread_arg[i].pos_r;
				const size_t pj = c_Thread_arg[j].pos_r - sk;

				//assert(si <= mm);
				//assert(sj <= mm);
                                memcpy(temp, &b[pi], st);
                                memcpy(&b[pi], &b[pj], st);
                                memcpy(&b[pj], temp, st);
			#ifdef DO_PAYLOAD_SHUFFLE
					const size_t stp = sk * sizeof(payloadType);
                                	memcpy(tempPayload, &payloadBuffer[pi], stp);
                                	memcpy(&payloadBuffer[pi], &payloadBuffer[pj], stp);
                                	memcpy(&payloadBuffer[pj], tempPayload, stp);
			#endif
				c_Thread_arg[i].pos_r += sk;
				c_Thread_arg[j].pos_r -= sk;

				//assert((si <= sj) == (c_Thread_arg[i].pos_r == c_Thread_arg[i].last + 1));
				//assert((sj <= si) == (c_Thread_arg[j].pos_r == c_Thread_arg[j].first));

				i += (si <= sj);
				j -= (sj <= si);
                        }
                }
        }

        /* determine pivot (This is useful only for result validation)*/
        f = BUN_NONE;
        for (i = 0, j = alt * nthreads - alt; i <= j; j--) {
                l = c_Thread_arg[j].pos_r;
		if (l == last + 1 || (l <= last && b[l] >= pivot && (l == first || b[l - 1] < pivot)))
		{
                        if (f == BUN_NONE) {
                                f = l;
                        } 
			//else {
                                //assert(f == l);
                        //}
                }
        }
        //assert(f != BUN_NONE);
	//assert((f == last + 1 && b[f-1] < pivot) || b[f]   >= pivot);
	//assert((f == first    && b[f]   >= pivot) || b[f-1] < pivot);
        *pos = (size_t) (f == 0 ? 0 : f - 1);

        free(temp);
#ifdef DO_PAYLOAD_SHUFFLE
	free(tempPayload);
#endif
        free(c_Thread_arg);
}
