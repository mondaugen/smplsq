/* A simple synthesizer */

typedef struct synth_vc_t {
    int playing;
    f64_t freq;
    struct {
        f64_t a;
        f64_t d;
        f64_t s;
        f64_t r;
        f64_t max_amp; /* maximum amplitude */
        f64_t sus_samp /* sustain amplitude */
    } env;
    f64_t _tot_tm; /* total time (sum of a,d,sr) */
    f64_t _phs;    /* current phase [0-1] */
    f64_t _tm;     /* current time */
} synth_vc_t;

typedef struct synth_vc_proc_t {
    f64_t sr; /* sample rate */
    f64_t *wt; /* wavetable */
    size_t len; /* length in samples */
} synth_vc_proc_t;

err_t synth_vc_init(synth_vc_t *s,
                    f64_t freq,
                    f64_t a,
                    f64_t d,
                    f64_t s,
                    f64_t r,
                    f64_t max_amp,
                    f64_t sus_samp
                    )
{
    if ((a < 0) || (d < 0) || (s < 0) || (r < 0)) {
        return err_EINVAL;
    }
    _MZ(s,synth_vc_t,1);
    s->freq = freq;
    s->env.a    = a;
    s->env.d    = d;
    s->env.s    = s;
    s->env.r    = r;
    s->env.max_amp    = max_amp;
    s->env.sus_amp    = sus_amp;
    return err_NONE;
}

static inline f64_t adsr_amp(f64_t cur_tm, f64_t a, f64_t d, f64_t s, f64_t r, f64_t max_amp, f64_t sus_amp)
{
    if (cur_tm < 0) {
        return 0;
    }
    if (cur_tm < a) {
        return max_amp * (cur_tm/a);
    }
    if (cur_tm < a+d) {
        return sus_amp + (max_amp - sus_amp) * (1. - (cur_tm - a)/d);
    }
    if (cur_tm < a+d+s) {
        return sus_amp;
    }
    if (cur_tm < a+d+s+r) {
        return sus_amp * (1. - (cur_tm - a - d - s)/r);
    }
    return 0;
}

err_t synth_vc_proc(synth_vc_t *s, synth_vc_proc_t *sp, f64_t *out, size_t nsamps)
{
    /* assumes s set to "playing" */
    size_t n;
    f64_t smp_inc = s->freq/(sp->sr/sp->len),
          smp_cur = s->_phs*sp->len,
          tm_cur = s->_tm;
    for (n = 0; n < nsamps; n++) {
        if (tm_cur > s->_tot_tm) {
            s->playing = 0;
            break;
        }
        f64_t diff = smp_cur - (size_t)smp_cur;
        size_t nxt_smp = (size_t)smp_cur + 1;
        *out += (sp->wt[(size_t)smp_cur] 
                + sp->wt[nxt_smp >= sp->len ? 0 : nxt_smp]*diff)
            * adsr_amp(tm_cur,sp->env.a,sp->env.d,sp->env.s,sp->env.r,sp->env.max_amp,sp->env.sus_amp);
        out++;
        smp_cur += smp_inc;
        while (smp_cur >= sp->len) {
            smp_cur -= sp->len;
        }
        while (smp_cur < 0) {
            smp_cur += sp->len;
        }
    }
    s->_phs = smp_cur / sp->len;
    return err_NONE;
}
