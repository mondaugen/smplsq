#include "seq.h"

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

void seq_destroy(seq_t *s)
{
    _F(s->events);
    _MZ(s,seq_t,1);
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
 * Returns event so it can be freed if need be or NULL if not found. 
 * If cmp NULL then any event at the tick is removed. */
seq_event_t *seq_remove_event(seq_t *, size_t tick, int (*cmp)(seq_evnt_t *, void*), void *data)
{
    if (tick >= s->_seq_len) {
        return err_EINVAL;
    }
    size_t n = 0;
    do {
        if (s->events[s->_n_events_per_tick * tick + n]) {
            int dorm = 0;
            if (cmp) {
                dorm = cmp(s->events[s->_n_events_per_tick * tick + n],data);
            }
            if (dorm == 0) {
                seq_evnt_t *tmp = s->events[s->_n_events_per_tick * tick + n];
                s->events[s->_n_events_per_tick * tick + n] = NULL;
                return tmp;
            }
        }
        n++;
    } while (n < s->_n_evnts_per_tick);
    return NULL;
}

void seq_remove_all_events(seq_t *s)
{
    for (size_t n; n < s->_seq_len; n++) {
        seq_event_t *se;
        while ((se = seq_remove_event(s,n,NULL,NULL))) {
            seq_event_free(se);
        }
    }
}

int seq_event_chk_freq(seq_event_t *s, f64_t freq)
{
    if (s->freq == freq) {
        return 0;
    }
    return -1;
}
