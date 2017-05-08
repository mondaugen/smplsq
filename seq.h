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
    int played;
} seq_event_t;

#define SEQ_EVENT_INIT_DEFAULT (seq_event_t) { \
    .freq = 440, \
    .env.a = 0.01, \
    .env.d = 0.01, \
    .env.s = 0.5, \
    .env.r = 0.5, \
    .env.max_amp = 1., \
    .env.sus_amp = 0.5, \
    .played = 0 \
}

typedef struct seq_t {
    seq_event_t **events;
    f64_t tick_len; /* in samples */
    size_t _seq_len;
    size_t _n_events_per_tick;
} seq_t;

#define seq_event_free(s) _F(s) 

err_t seq_init(seq_t *s, size_t seq_len, size_t n_events_per_tick, f64_t tick_len);
void seq_destroy(seq_t *s);
err_t seq_add_event(seq_t *s, seq_event_t *e, size_t tick);
seq_event_t *seq_remove_event(seq_t *, size_t tick, int (*cmp)(seq_event_t *, void*), void *data);
void seq_remove_all_events(seq_t *s);
int seq_event_chk_freq(seq_event_t *s, f64_t freq);
void seq_events_set_unplayed(seq_t *s);
seq_event_t **seq_get_events_at_tick(seq_t *s, size_t tick);
err_t seq_event_init_from_str(seq_event_t *se,
                              size_t *time_sec,
                              char *str);

#endif /* SEQ_H */
