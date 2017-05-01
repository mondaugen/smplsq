/* A simple synthesizer */
#include "synth.h" 
#include <stdio.h> 

err_t synth_vc_init_from_str(synth_vc_init_t *svi, char *str)
{
    *svi = SYNTH_VC_INIT_DEFAULT;
    sscanf(str,"%f %f %f %f %f %f %f",
        &svi->freq,
        &svi->a,
        &svi->d,
        &svi->s,
        &svi->r,
        &svi->max_amp,
        &svi->sus_samp);
    return err_NONE;
}

err_t synth_vc_init(synth_vc_t *s,
                    synth_vc_init_t *spi)
{
    if ((spi->a < 0) || (spi->d < 0) || (spi->s < 0) || (spi->r < 0)) {
        return err_EINVAL;
    }
    _MZ(s,synth_vc_t,1);
    s->freq = spi->freq;
    s->env.a = spi->a;
    s->env.d = spi->d;
    s->env.s = spi->s;
    s->env.r = spi->r;
    s->env.max_amp = spi->max_amp;
    s->env.sus_amp = spi->sus_amp;
    s->_tot_tm = spi->a + spi->d + spi->s + spi->r;
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
    f64_t t_s = sp->sr; /* sample period */
    for (n = 0; n < nsamps; n++) {
        if (tm_cur > s->_tot_tm) {
            s->playing = 0;
            break;
        }
        /* linear interpolation */
        f64_t diff = smp_cur - (size_t)smp_cur;
        size_t nxt_smp = (size_t)smp_cur + 1;
        *out += (sp->wt[(size_t)smp_cur] 
                + sp->wt[nxt_smp >= sp->len ? 0 : nxt_smp]*diff)
            * adsr_amp(tm_cur,sp->env.a,sp->env.d,sp->env.s,sp->env.r,sp->env.max_amp,sp->env.sus_amp);
        tm_cur += t_s;
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
