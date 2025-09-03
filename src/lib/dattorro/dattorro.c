#include "dattorro.h"
#include <math.h>
#include <string.h> // For memset

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

// Helper to get the next power of two for a number
static int next_power_of_two(int n) {
    if (n == 0) return 1;
    int power = 1;
    while (power < n) {
        power <<= 1;
    }
    return power;
}

// Helper function to initialize a single delay line
static void make_delay(DelayLine* delay_line, float* buffer, float length_in_seconds, float sample_rate) {
    int len = (int)roundf(length_in_seconds * sample_rate);
    int pow2_size = next_power_of_two(len);

//     delay_line->buffer = buffer;
    delay_line->size = len - 1;
    delay_line->write_pos = 0;
    delay_line->mask = pow2_size - 1;

    // Clear the buffer
    memset(buffer, 0, pow2_size * sizeof(float));
}

// Cubic interpolation function
static float read_delay_cat(const DelayLine* delay, float i) {
    int int_i = (int)floorf(i);
    float frac = i - int_i;
    // The read position is relative to the write position
    int read_pos_base = delay->write_pos - delay->size + int_i - 1;

    float x0 = delay->buffer[(read_pos_base++) & delay->mask];
    float x1 = delay->buffer[(read_pos_base++) & delay->mask];
    float x2 = delay->buffer[(read_pos_base++) & delay->mask];
    float x3 = delay->buffer[read_pos_base & delay->mask];

    float a = (3.0f * (x1 - x2) - x0 + x3) * 0.5f;
    float b = 2.0f * x2 + x0 - (5.0f * x1 + x3) * 0.5f;
    float c = (x2 - x0) * 0.5f;

    return (((a * frac) + b) * frac + c) * frac + x1;
}

bool dattorro_reverb_init(DattorroReverb* reverb, float sample_rate) {
    if (!reverb || sample_rate > DATTORRO_MAX_SR) {
        return false; // Invalid arguments
    }

    // Clear the entire struct to zero out all buffers and state
    memset(reverb, 0, sizeof(DattorroReverb));

    reverb->sample_rate = sample_rate;

    // Default parameter values from the JS implementation
    dattorro_reverb_set_parameter(reverb, PRE_DELAY, 0.0f);
    dattorro_reverb_set_parameter(reverb, BANDWIDTH, 0.9999f);
    dattorro_reverb_set_parameter(reverb, INPUT_DIFFUSION_1, 0.75f);
    dattorro_reverb_set_parameter(reverb, INPUT_DIFFUSION_2, 0.625f);
    dattorro_reverb_set_parameter(reverb, DECAY, 0.5f);
    dattorro_reverb_set_parameter(reverb, DECAY_DIFFUSION_1, 0.7f);
    dattorro_reverb_set_parameter(reverb, DECAY_DIFFUSION_2, 0.5f);
    dattorro_reverb_set_parameter(reverb, DAMPING, 0.005f);
    dattorro_reverb_set_parameter(reverb, EXCURSION_RATE, 0.5f);
    dattorro_reverb_set_parameter(reverb, EXCURSION_DEPTH, 0.7f);
    dattorro_reverb_set_parameter(reverb, WET, 0.3f);
    dattorro_reverb_set_parameter(reverb, DRY, 0.6f);

    reverb->pre_delay_length = (int)sample_rate; // 1 second pre-delay buffer

    // Delay lengths in seconds
    const float delay_lengths[] = {
        0.004771345f, 0.003595309f, 0.012734787f, 0.009307483f,
        0.022579886f, 0.149625349f, 0.060481839f, 0.1249958f,
        0.030509727f, 0.141695508f, 0.089244313f, 0.106280031f
    };

    // Point the delay lines to their static buffers
    float* buffers[] = {
        reverb->delay_buf_0, reverb->delay_buf_1, reverb->delay_buf_2, reverb->delay_buf_3,
        reverb->delay_buf_4, reverb->delay_buf_5, reverb->delay_buf_6, reverb->delay_buf_7,
        reverb->delay_buf_8, reverb->delay_buf_9, reverb->delay_buf_10, reverb->delay_buf_11
    };

    for (int i = 0; i < 12; ++i) {
        make_delay(&reverb->delays[i], buffers[i], delay_lengths[i], sample_rate);
    }

    // Tap lengths in seconds
    const float tap_lengths[] = {
        0.008937872f, 0.099929438f, 0.064278754f, 0.067067639f, 0.066866033f, 0.006283391f, 0.035818689f,
        0.011861161f, 0.121870905f, 0.041262054f, 0.08981553f, 0.070931756f, 0.011256342f, 0.004065724f
    };

    for (int i = 0; i < 14; ++i) {
        reverb->taps[i] = (int16_t)roundf(tap_lengths[i] * sample_rate);
    }

    return true;
}


void dattorro_reverb_process(DattorroReverb* reverb, float* buffer, int num_frames) {
    // Get current parameter values
    const int pd = (int)reverb->params[PRE_DELAY];
    const float bw = reverb->params[BANDWIDTH];
    const float fi = reverb->params[INPUT_DIFFUSION_1];
    const float si = reverb->params[INPUT_DIFFUSION_2];
    const float dc = reverb->params[DECAY];
    const float ft = reverb->params[DECAY_DIFFUSION_1];
    const float st = reverb->params[DECAY_DIFFUSION_2];
    const float dp = 1.0f - reverb->params[DAMPING];
    const float ex = reverb->params[EXCURSION_RATE] / reverb->sample_rate;
    const float ed = reverb->params[EXCURSION_DEPTH] * reverb->sample_rate / 1000.0f;
    const float we = reverb->params[WET] * 0.6f;
    const float dr = reverb->params[DRY];

    // Process each stereo frame
    for (int i = 0; i < num_frames; ++i) {
        // Store the original dry signal from the input buffer
        const float dry_left = buffer[i * 2];
        const float dry_right = buffer[i * 2 + 1];

        // Create mono input for the reverb algorithm
        float mono_input = (dry_left + dry_right) * 0.5f;

        // Write to pre-delay line
        reverb->pre_delay_buffer[reverb->pre_delay_write_pos] = mono_input;

        // Read from pre-delay line
        int read_pos = (reverb->pre_delay_write_pos - pd + reverb->pre_delay_length) % reverb->pre_delay_length;
        float pre_delay_out = reverb->pre_delay_buffer[read_pos];

        // Low-pass filter on the pre-delay output
        reverb->lp1 += bw * (pre_delay_out - reverb->lp1);

        // --- Pre-tank Diffusion ---
        float d0_out = reverb->delays[0].buffer[(reverb->delays[0].write_pos - reverb->delays[0].size) & reverb->delays[0].mask];
        float d1_out = reverb->delays[1].buffer[(reverb->delays[1].write_pos - reverb->delays[1].size) & reverb->delays[1].mask];
        float d2_out = reverb->delays[2].buffer[(reverb->delays[2].write_pos - reverb->delays[2].size) & reverb->delays[2].mask];
        float d3_out = reverb->delays[3].buffer[(reverb->delays[3].write_pos - reverb->delays[3].size) & reverb->delays[3].mask];

        float pre = reverb->lp1 - fi * d0_out;
        reverb->delays[0].buffer[reverb->delays[0].write_pos] = pre;

        pre = fi * (pre - d1_out) + d0_out;
        reverb->delays[1].buffer[reverb->delays[1].write_pos] = pre;

        pre = fi * pre + d1_out - si * d2_out;
        reverb->delays[2].buffer[reverb->delays[2].write_pos] = pre;

        pre = si * (pre - d3_out) + d2_out;
        reverb->delays[3].buffer[reverb->delays[3].write_pos] = pre;

        float split = si * pre + d3_out;

        // --- Excursions (Modulation) ---
        float exc = ed * (1.0f + cosf(reverb->exc_phase * 2.0f * M_PI));
        float exc2 = ed * (1.0f + sinf(reverb->exc_phase * 2.0f * M_PI + M_PI / 2.0f));

        // --- Left Loop ---
        float d11_out = reverb->delays[11].buffer[(reverb->delays[11].write_pos - reverb->delays[11].size) & reverb->delays[11].mask];
        float temp = split + dc * d11_out + ft * read_delay_cat(&reverb->delays[4], exc);
        reverb->delays[4].buffer[reverb->delays[4].write_pos] = temp;

        float d4_out_mod = read_delay_cat(&reverb->delays[4], exc);
        reverb->delays[5].buffer[reverb->delays[5].write_pos] = d4_out_mod - ft * temp;

        float d5_out = reverb->delays[5].buffer[(reverb->delays[5].write_pos - reverb->delays[5].size) & reverb->delays[5].mask];
        reverb->lp2 += dp * (d5_out - reverb->lp2);

        float d6_out = reverb->delays[6].buffer[(reverb->delays[6].write_pos - reverb->delays[6].size) & reverb->delays[6].mask];
        temp = dc * reverb->lp2 - st * d6_out;
        reverb->delays[6].buffer[reverb->delays[6].write_pos] = temp;
        reverb->delays[7].buffer[reverb->delays[7].write_pos] = d6_out + st * temp;

        // --- Right Loop ---
        float d7_out = reverb->delays[7].buffer[(reverb->delays[7].write_pos - reverb->delays[7].size) & reverb->delays[7].mask];
        temp = split + dc * d7_out + ft * read_delay_cat(&reverb->delays[8], exc2);
        reverb->delays[8].buffer[reverb->delays[8].write_pos] = temp;

        float d8_out_mod = read_delay_cat(&reverb->delays[8], exc2);
        reverb->delays[9].buffer[reverb->delays[9].write_pos] = d8_out_mod - ft * temp;

        float d9_out = reverb->delays[9].buffer[(reverb->delays[9].write_pos - reverb->delays[9].size) & reverb->delays[9].mask];
        reverb->lp3 += dp * (d9_out - reverb->lp3);

        float d10_out = reverb->delays[10].buffer[(reverb->delays[10].write_pos - reverb->delays[10].size) & reverb->delays[10].mask];
        temp = dc * reverb->lp3 - st * d10_out;
        reverb->delays[10].buffer[reverb->delays[10].write_pos] = temp;
        reverb->delays[11].buffer[reverb->delays[11].write_pos] = d10_out + st * temp;

        // --- Taps to Output ---
        float lo = 0.0f; // Wet left
        float ro = 0.0f; // Wet right

        lo += reverb->delays[9].buffer[(reverb->delays[9].write_pos - reverb->taps[0]) & reverb->delays[9].mask];
        lo += reverb->delays[9].buffer[(reverb->delays[9].write_pos - reverb->taps[1]) & reverb->delays[9].mask];
        lo -= reverb->delays[10].buffer[(reverb->delays[10].write_pos - reverb->taps[2]) & reverb->delays[10].mask];
        lo += reverb->delays[11].buffer[(reverb->delays[11].write_pos - reverb->taps[3]) & reverb->delays[11].mask];
        lo -= reverb->delays[5].buffer[(reverb->delays[5].write_pos - reverb->taps[4]) & reverb->delays[5].mask];
        lo -= reverb->delays[6].buffer[(reverb->delays[6].write_pos - reverb->taps[5]) & reverb->delays[6].mask];
        lo -= reverb->delays[7].buffer[(reverb->delays[7].write_pos - reverb->taps[6]) & reverb->delays[7].mask];

        ro += reverb->delays[5].buffer[(reverb->delays[5].write_pos - reverb->taps[7]) & reverb->delays[5].mask];
        ro += reverb->delays[5].buffer[(reverb->delays[5].write_pos - reverb->taps[8]) & reverb->delays[5].mask];
        ro -= reverb->delays[6].buffer[(reverb->delays[6].write_pos - reverb->taps[9]) & reverb->delays[6].mask];
        ro += reverb->delays[7].buffer[(reverb->delays[7].write_pos - reverb->taps[10]) & reverb->delays[7].mask];
        ro -= reverb->delays[9].buffer[(reverb->delays[9].write_pos - reverb->taps[11]) & reverb->delays[9].mask];
        ro -= reverb->delays[10].buffer[(reverb->delays[10].write_pos - reverb->taps[12]) & reverb->delays[10].mask];
        ro -= reverb->delays[11].buffer[(reverb->delays[11].write_pos - reverb->taps[13]) & reverb->delays[11].mask];

        // --- Final Mix and In-Place Write ---
        // Mix the original dry signal with the new wet signal and overwrite the buffer
        buffer[i * 2]     = dry_left  * dr + lo * we;
        buffer[i * 2 + 1] = dry_right * dr + ro * we;

        // --- Update State for next sample ---
        reverb->exc_phase += ex;
        if (reverb->exc_phase >= 1.0f) {
            reverb->exc_phase -= 1.0f;
        }

        reverb->pre_delay_write_pos = (reverb->pre_delay_write_pos + 1) % reverb->pre_delay_length;

        for (int j = 0; j < 12; ++j) {
            reverb->delays[j].write_pos = (reverb->delays[j].write_pos + 1) & reverb->delays[j].mask;
        }
    }
}


void dattorro_reverb_set_parameter(DattorroReverb* reverb, DattorroReverbParameter param, float value) {
    if (reverb && param >= 0 && param < NUM_DATTORRO_PARAMS) {
        reverb->params[param] = value;
    }
}

float dattorro_reverb_get_parameter(DattorroReverb* reverb, DattorroReverbParameter param) {
    if (reverb && param >= 0 && param < NUM_DATTORRO_PARAMS) {
        return reverb->params[param];
    }
    return 0.0f;
}
