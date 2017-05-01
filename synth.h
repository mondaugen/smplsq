#ifndef SYNTH_H
#define SYNTH_H 

#include "err.h"
#include "types.h"
#include "defs.h" 

typedef struct synth_vc_t {
    int playing;
    f64_t freq;
    struct {
        f64_t a;
        f64_t d;
        f64_t s;
        f64_t r;
        f64_t max_amp; /* maximum amplitude */
        f64_t sus_amp; /* sustain amplitude */
    } env;
    f64_t _tot_tm; /* total time (sum of a,d,s,r) */
    f64_t _phs;    /* current phase [0-1] */
    f64_t _tm;     /* current time */
} synth_vc_t;

typedef struct synth_vc_proc_t {
    f64_t sr; /* sample rate */
    f64_t *wt; /* wavetable */
    size_t len; /* length in samples */
} synth_vc_proc_t;

typedef struct synth_vc_init_t {
    f64_t freq;
    f64_t a;
    f64_t d;
    f64_t s;
    f64_t r;
    f64_t max_amp;
    f64_t sus_amp;
} synth_vc_init_t;

#define SYNTH_VC_INIT_DEFAULT (synth_vc_init_t) { \
    .freq = 440, \
    .a = 0.01, \
    .d = 0.01, \
    .s = 0.5, \
    .r = 0.5, \
    .max_amp = 1., \
    .sus_amp = 0.5, \
}

err_t synth_vc_init_from_str(synth_vc_init_t *svi, char *str);
err_t synth_vc_init(synth_vc_t *s,
                    synth_vc_init_t *spi);
err_t synth_vc_proc(synth_vc_t *s, synth_vc_proc_t *sp, f64_t *out, size_t nsamps);

#endif /* SYNTH_H */
