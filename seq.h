#ifndef SEQ_H
#define SEQ_H 

#include "err.h" 
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
        f64_t sus_amp; /* sustain amplitude */
    } env;
} seq_event_t;

typedef struct seq_t {
    seq_event_t *events;
    f64_t tick_len; /* in samples */
    size_t _seq_len;
    size_t _n_events_per_tick;
} seq_t;

#define seq_event_free(s) _F(s) 

err_t seq_init(seq_t *s, size_t seq_len, size_t n_events_per_tick, f64_t tick_len);
void seq_destroy(seq_t *s);
err_t seq_add_event(seq_t *s, seq_event_t *e, size_t tick);
seq_event_t *seq_remove_event(seq_t *, size_t tick, int (*cmp)(seq_evnt_t *, void*), void *data);
void seq_remove_all_events(seq_t *s);
int seq_event_chk_freq(seq_event_t *s, f64_t freq);

#endif /* SEQ_H */
