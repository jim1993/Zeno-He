#include "zhe-config-deriv.h"

#if MAX_PEERS > 0

#include "zhe-assert.h"
#include "zhe-binheap.h"

/* for zhe_seq_lt() */
#include "zhe-int.h"

static void minseqheap_heapify(peeridx_t j, peeridx_t n, peeridx_t * restrict p, minseqheap_idx_t * restrict q, const seq_t * restrict v)
{
    peeridx_t k;
    /* j < n/2 term to protect against overflow of k:
       - for n even: j < n/2 => k < 2*(n/2)+1 = n+1
       - for n odd:  j < n/2 = (n-1)/2 => k < 2*((n-1)/2)+1 = n 
       n+1 potentially overflows, but j < n/2 takes care of that
       --
       in the loop body k can only be incremented if k+1 < n, 
       but (1) k < n and (2) n is in range, so k+1 is
       at most n and therefore also in range */
    for (k = 2*j+1; j < n/2 && k < n; j = k, k += k + 1) {
        if (k+1 < n && zhe_seq_lt(v[p[k+1]], v[p[k]])) {
            k++;
        }
        if (zhe_seq_lt(v[p[k]], v[p[j]])) {
            peeridx_t t;
            t = p[j]; p[j] = p[k]; p[k] = t;
            q[p[j]].i = j; q[p[k]].i = k;
        }
    }
}

#ifndef NDEBUG
static void check_heap(struct minseqheap * const h)
{
    peeridx_t cnt = 0;
    zhe_assert(h->n <= MAX_PEERS_1);
    for (peeridx_t j = 0; j < MAX_PEERS_1; j++) {
        zhe_assert(h->ix[j].i == PEERIDX_INVALID || (h->ix[j].i < h->n && h->hx[h->ix[j].i] == j));
        cnt += (h->ix[j].i != PEERIDX_INVALID);
    }
    zhe_assert(cnt == h->n);
    for (peeridx_t j = 0; j < h->n/2; j++) {
        peeridx_t k = 2*j+1;
        zhe_assert (k >= h->n || zhe_seq_le(h->vs[h->hx[j]], h->vs[h->hx[k]]));
        zhe_assert (k+1 >= h->n || zhe_seq_le(h->vs[h->hx[j]], h->vs[h->hx[k+1]]));
    }
}
#endif

void zhe_minseqheap_insert(peeridx_t peeridx, seq_t seqbase, struct minseqheap * const h)
{
    peeridx_t i;
#ifndef NDEBUG
    check_heap(h);
    zhe_assert(h->ix[peeridx].i == PEERIDX_INVALID);
#endif
    h->vs[peeridx] = seqbase;
    i = h->n++;
    while (i > 0 && zhe_seq_lt(h->vs[peeridx], h->vs[h->hx[(i-1)/2]])) {
        h->hx[i] = h->hx[(i-1)/2];
        h->ix[h->hx[i]].i = i;
        i = (i-1)/2;
    }
    h->hx[i] = peeridx;
    h->ix[h->hx[i]].i = i;
#ifndef NDEBUG
    check_heap(h);
#endif
}

seq_t zhe_minseqheap_get_min(struct minseqheap const * const h)
{
    zhe_assert (h->n > 0);
    return h->vs[h->hx[0]];
}

seq_t zhe_minseqheap_update_seq(peeridx_t peeridx, seq_t seqbase, seq_t seqbase_if_discarded, struct minseqheap * const h)
{
    /* peeridx must be contained in heap and new seqbase must be >= h->vs[peeridx] */
#ifndef NDEBUG
    check_heap(h);
#endif
    if (h->ix[peeridx].i == PEERIDX_INVALID || zhe_seq_le(seqbase, h->vs[peeridx])) {
        return seqbase_if_discarded;
    } else {
        zhe_assert(h->hx[h->ix[peeridx].i] == peeridx);
        h->vs[peeridx] = seqbase;
        minseqheap_heapify(h->ix[peeridx].i, h->n, h->hx, h->ix, h->vs);
#ifndef NDEBUG
        check_heap(h);
#endif
        return h->vs[h->hx[0]];
    }
}

int zhe_minseqheap_delete(peeridx_t peeridx, struct minseqheap * const h)
{
    /* returns 0 if peeridx not contained in heap; 1 if it is contained */
    const peeridx_t i = h->ix[peeridx].i;
#ifndef NDEBUG
    check_heap(h);
#endif
    if (i == PEERIDX_INVALID) {
        return 0;
    } else {
        zhe_assert(h->hx[i] == peeridx);
        h->ix[peeridx].i = PEERIDX_INVALID;
        if (h->n == 1) {
            h->n = 0;
        } else {
            h->n--;
            if (i < h->n) {
                h->hx[i] = h->hx[h->n];
                h->ix[h->hx[i]].i = i;
                minseqheap_heapify(i, h->n, h->hx, h->ix, h->vs);
            }
#ifndef NDEBUG
            check_heap(h);
#endif
        }
        return 1;
    }
}

int zhe_minseqheap_isempty(struct minseqheap const * const h)
{
    return h->n == 0;
}

#endif
