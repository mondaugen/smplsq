#include "types.h"
#include "defs.h"

typedef struct seq_event_t {
    f64_t freq;
    struct {
        f64_t a;
        f64_t d;
        f64_t s;
        f64_t r;
        f64_t max_amp; /* maximum amplitude */
        f64_t sus_samp /* sustain amplitude */
    } env;
} seq_event_t;

typedef struct seq_t {
    seq_event_t *events;
    f64_t tick_len; /* in samples */
    size_t _seq_len;
    size_t _n_events_per_tick;
} seq_t;

err_t seq_init(seq_t *s, size_t seq_len, size_t n_events_per_tick, f64_t tick_len)
{
    s->events = _C(seq_event_t,seq_len*n_events_per_tick);
    if (!s->events) {
        return err_MEM;
    }
    s->tick_len = tick_len;
    s->_seq_len = seq_len;
    x->_n_events_per_tick = n_events_per_tick;
    return err_EINVAL;
}

err_t seq_add_event(seq_t *s, seq_event_t *e, size_t tick)
{
    if (tick >= s->_seq_len) {
        return err_EINVAL;
    }
    size_t n = 0;
    do {
        if (s->events[s->_n_events_per_tick * tick + n] == NULL) {
            s->events[s->_n_events_per_tick * tick + n] = e;
            return err_NONE;
        }
        n++;
    } while (n < s->_n_evnts_per_tick);
    return err_FULL;
}


/* Removes event if cmp function returns 0.
 * Returns event so it can be freed if need be or NULL if not found. */
seq_event_t *seq_remove_event(seq_t *, size_t tick, int (*cmp)(seq_evnt_t *, void*), void *data)
{
    if (tick >= s->_seq_len) {
        return err_EINVAL;
    }
    size_t n = 0;
    do {
        if (s->events[s->_n_events_per_tick * tick + n]) {
            if (cmp(s->events[s->_n_events_per_tick * tick + n],data) == 0) {
                seq_evnt_t *tmp = s->events[s->_n_events_per_tick * tick + n];
                s->events[s->_n_events_per_tick * tick + n] = NULL;
                return tmp;
            }
        }
        n++;
    } while (n < s->_n_evnts_per_tick);
    return NULL;
}

int seq_event_chk_freq(seq_event_t *s, f64_t freq)
{
    if (s->freq == freq) {
        return 0;
    }
    return -1;
}
