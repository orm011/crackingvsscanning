#include "../Framework/cracking_MT.h"
#include <stdlib.h>


#define int64_t targetType // sorry, but easier to make it work this way.

// Helper routine.  See the comment in ps_hoare_p() for the meaning
// of filter_indices[].
static inline size_t less_suffix(size_t i, const size_t filter_indices[], size_t n)
{
  return (n - i) - (filter_indices[n-1] - filter_indices[i]);
}

// This routine performs partition swaps of elements in array[ll:lh] that are
// larger than pivot and elements in array[hl:hh] that are smaller than pivot.
// These swaps are performed using parallel divide-and-conquer.
static void ps_hoare_p_dac(int64_t array[], const size_t filter_indices[],
                           int64_t pivot, size_t n,
                           size_t ll, size_t lh, size_t hl, size_t hh)
{

  if (lh - ll < COARSENING && hh - hl < COARSENING) {
    // Perform a standard serial swap in the base case
    size_t left = ll;
    size_t right = hh-1;
    while (left < lh && right > hl) {
      while (left < lh &&
             (array[left] < pivot ||
              filter_indices[left] < less_suffix(right, filter_indices, n)) ) {
        ++left;
      }
      while (right > hl &&
             (array[right] >= pivot ||
              less_suffix(right, filter_indices, n) < filter_indices[left]) ) {
             --right;
      }
      if (left < lh && array[left] >= pivot && array[right] < pivot) {
        int64_t tmp = array[left];
        array[left++] = array[right];
        array[right] = tmp;
      }
    }
    return;
  }

  if (lh - ll > hh - hl) {
    // Split left block
    // For runs, keep the leftmost block
    size_t lm = (lh + ll) / 2;

    size_t greater_l = filter_indices[ll];
    size_t greater_m = filter_indices[lm];
    size_t greater_h = filter_indices[lh-1];
    size_t less_l = less_suffix(hl, filter_indices, n);
    size_t less_h = less_suffix(hh-1, filter_indices, n);

    if (less_h <= greater_m) {
      if (less_l >= greater_l) {
        _Cilk_spawn ps_hoare_p_dac(array, filter_indices, pivot, n, ll, lm, hl, hh);
        if (less_l >= greater_m && filter_indices[lm-1] < greater_h) {
          // 2nd condition verifies that the next block has something to swap.
          ps_hoare_p_dac(array, filter_indices, pivot, n, lm, lh, hl, hh);
        }
      }
    } else if (less_h <= greater_h && filter_indices[lm-1] < greater_h) {
      // 2nd condition verifies that the next block has something to swap.
      ps_hoare_p_dac(array, filter_indices, pivot, n, lm, lh, hl, hh);
    }

  } else {
    // Split right block
    // For runs, keep the leftmost block
    size_t hm = (hh + hl) / 2;

    size_t greater_h = filter_indices[lh-1];
    size_t greater_l = filter_indices[ll];
    size_t less_l = less_suffix(hl, filter_indices, n);
    size_t less_m = less_suffix(hm, filter_indices, n);
    size_t less_h = less_suffix(hh-1, filter_indices, n);
    if (greater_h >= less_m) {
      if (greater_l <= less_l) {
        _Cilk_spawn ps_hoare_p_dac(array, filter_indices, pivot, n, ll, lh, hl, hm);
        if (greater_l <= less_m && less_suffix(hm-1, filter_indices, n) > less_h) {
          // 2nd condition verifies that the next block has something to swap.
          ps_hoare_p_dac(array, filter_indices, pivot, n, ll, lh, hm, hh);
        }
      }
    } else if (greater_h >= less_h && less_suffix(hm-1, filter_indices, n) > less_h) {
      // 2nd condition verifies that the next block has something to swap.
      ps_hoare_p_dac(array, filter_indices, pivot, n, ll, lh, hm, hh);
    }
  }
}



// This routine effectively "searches" for where pivot should lie in array[]
// if array[] were sorted.
// As it performs this search, this routine spawns off calls to perform
// partition swaps on sections of array[] that are guaranteed to not
// contain pivot.
static size_t ps_hoare_p_search(int64_t array[], const size_t filter_indices[],
                                int64_t pivot, size_t n,
                                size_t l, size_t h)
{

  if (h - l < COARSENING) {
    // In the base case, simply execute a serial partition on array[].
    while (l < h) {
      while (l < h && array[l] < pivot) ++l;
      while (h > l && array[h] >= pivot) --h;
      if (l < h) {
        int64_t tmp = array[l];
        array[l++] = array[h];
        array[h] = tmp;
      }
    }
    return l;
  }

  size_t mid = (h + l) / 2;
  _Cilk_spawn ps_hoare_p_dac(array, filter_indices, pivot, n, l, mid, mid, h);
  if (filter_indices[mid] < less_suffix(mid, filter_indices, n)) {
    return ps_hoare_p_search(array, filter_indices, pivot, n, mid, h);
  } else {
    return ps_hoare_p_search(array, filter_indices, pivot, n, l, mid);
  }
}

static size_t prefix(
		const targetType* buffer,
		const size_t size,
		const targetType pivot,
		size_t *filter_indices)
{
	if (size < 4096) {
		size_t runningCount = 0;
		for (size_t i = 0; i < size; ++i) {
			runningCount += buffer[i] > pivot;
			filter_indices[i] = runningCount;
		}
	} else {
		const size_t half = size/2;
		_Cilk_spawn prefix(buffer, half, pivot, filter_indices);
		prefix(buffer + half, half, pivot, filter_indices + half);

		_Cilk_sync;

		const size_t acc = buffer[half - 1];

		_Cilk_for (int i = 0; i < half; ++i) {
			filter_indices[half + i] += acc;
		}
	}
}

// This is the top-level function that partitions array[] about pivot.
static size_t ps_hoare_p(int64_t array[], size_t n, int64_t pivot)
{
  size_t *filter_indices = new size_t[n];
  // filter_indices[i] equals the number of elements greater than the
  // pivot in array[0:i], and thus i - filter_indices[i] equals the
  // number of elements less than or equal to pivot in array[0:i].  We
  // want to find mid such that filter_indices[mid] = (n - mid) -
  // (filter_indices[n] - filter_indices[mid]).

  // ... compute filter_indices using a parallel prefix-sum computation ...
  prefix(array, n, pivot, filter_indices);

  size_t l = ps_hoare_p_search(array, filter_indices, pivot, n, 0, n);

  delete[] filter_indices;
  return l;
}



targetType* performCrack(targetType* buffer, payloadType* payloadBuffer, size_t bufferSize, targetType pivot, const targetType pivot_P) {
	ps_hoare_p(buffer, bufferSize, pivot);
	return NULL;
}

