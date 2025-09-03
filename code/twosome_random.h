#ifndef TWOSOME_RANDOM_H
#define TWOSOME_RANDOM_H

//
// NOTE: Algorithm for PCG from: http://www.pcg-random.org/download.html
//

struct pcg_state_setseq_64 {    // Internals are *Private*.
    u64 state;             // RNG state.  All values are possible.
    u64 inc;               // Controls which RNG sequence (stream) is
                                // selected. Must *always* be odd.
};
typedef struct pcg_state_setseq_64 pcg32_random_t;

typedef pcg32_random_t RNG;


internal u32 random(RNG *rng)
{
    u64 oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    u32 xorshifted = u32(((oldstate >> 18u) ^ oldstate) >> 27u);
    u32 rot = u32(oldstate >> 59u);
    
    u32 result = (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
    return result;
}

// NOTE: Range is inclusive
internal s32 random(RNG *rng, s32 min, s32 max)
{
    s32 result = min;
    
    if(min < max)
    {
        // NOTE: So we can get have a range between -2 billion and 2 billion we're going to use 64 bit integers. Is this a stupid way to do it?
        s64 max_64 = (s64)max;
        s64 min_64 = (s64)min;
    
        s64 rand_val = (s64)random(rng);
        rand_val = (rand_val % (max_64 - min_64 + 1));

        result = (s32)(rand_val + min_64);   
    }

    return result;
}

internal s32 random_exclusive(RNG *rng, s32 min, s32 max)
{
    s32 result = random(rng, min, (max - 1));
    return result;
}

internal b32 random_boolean(RNG *rng)
{
    b32 result = random(rng, 0, 1);
    return result;
}

internal void seed_rng(RNG *rng, u64 initstate, u64 initseq)
{
    rng->state = 0U;
    rng->inc = (initseq << 1u) | 1u;
    random(rng);
    rng->state += initstate;
    random(rng);
}

#endif
