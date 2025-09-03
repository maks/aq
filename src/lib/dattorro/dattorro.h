#ifndef DATTORRO_REVERB_H
#define DATTORRO_REVERB_H

#include <stdint.h>
#include <stdbool.h>

// --- CONFIGURATION ---
// Define the maximum sample rate your application will use.
// All buffers will be sized at compile time to handle this rate.
// Common values: 44100, 48000, 96000
#define DATTORRO_MAX_SR 44100

// --- PUBLIC API ---

// Opaque struct for the reverb context, now with all buffers inside.
// Its size is determined by DATTORRO_MAX_SR.
typedef struct DattorroReverb DattorroReverb;

// Enum for the reverb parameters
typedef enum {
    PRE_DELAY,
    BANDWIDTH,
    INPUT_DIFFUSION_1,
    INPUT_DIFFUSION_2,
    DECAY,
    DECAY_DIFFUSION_1,
    DECAY_DIFFUSION_2,
    DAMPING,
    EXCURSION_RATE,
    EXCURSION_DEPTH,
    WET,
    DRY,
    NUM_DATTORRO_PARAMS
} DattorroReverbParameter;


/**
 * @brief Initializes a DattorroReverb instance.
 *
 * @param reverb A pointer to the DattorroReverb struct to initialize.
 * @param sample_rate The sample rate of the audio to be processed.
 * @return True on success, false if sample_rate exceeds DATTORRO_MAX_SR.
 */
bool dattorro_reverb_init(DattorroReverb* reverb, float sample_rate);

/**
 * @brief Processes a buffer of audio samples in-place.
 *
 * @param reverb A pointer to the DattorroReverb instance.
 * @param buffer A pointer to the buffer of audio samples to be processed (interleaved stereo).
 * The contents of this buffer will be overwritten with the processed audio.
 * @param num_frames The number of stereo frames in the buffer.
 */
void dattorro_reverb_process(DattorroReverb* reverb, float* buffer, int num_frames);

/**
 * @brief Sets the value of a reverb parameter.
 *
 * @param reverb A pointer to the DattorroReverb instance.
 * @param param The parameter to set.
 * @param value The new value for the parameter.
 */
void dattorro_reverb_set_parameter(DattorroReverb* reverb, DattorroReverbParameter param, float value);

/**
 * @brief Gets the current value of a reverb parameter.
 *
 * @param reverb A pointer to the DattorroReverb instance.
 * @param param The parameter to get.
 * @return The current value of the parameter.
 */
float dattorro_reverb_get_parameter(DattorroReverb* reverb, DattorroReverbParameter param);


// --- INTERNAL STRUCTURES ---
// These are defined in the header so the compiler knows the size of DattorroReverb.

// Helper function to calculate the next power of two at compile time
#define NEXT_POW2(n) (1 << (32 - __builtin_clz(n - 1)))

// Calculate buffer sizes based on max sample rate and delay times from the JS code
#define DELAY_BUF_SIZE_0  NEXT_POW2((int)(0.004771345f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_1  NEXT_POW2((int)(0.003595309f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_2  NEXT_POW2((int)(0.012734787f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_3  NEXT_POW2((int)(0.009307483f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_4  NEXT_POW2((int)(0.022579886f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_5  NEXT_POW2((int)(0.149625349f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_6  NEXT_POW2((int)(0.060481839f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_7  NEXT_POW2((int)(0.1249958f   * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_8  NEXT_POW2((int)(0.030509727f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_9  NEXT_POW2((int)(0.141695508f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_10 NEXT_POW2((int)(0.089244313f * DATTORRO_MAX_SR))
#define DELAY_BUF_SIZE_11 NEXT_POW2((int)(0.106280031f * DATTORRO_MAX_SR))

// Internal delay line structure with fixed-size buffer
typedef struct {
    float buffer[DELAY_BUF_SIZE_5]; // Sized to the largest delay
    int size;          // Actual size based on current sample rate
    int write_pos;
    int mask;
} DelayLine;

// Main reverb context structure with static buffers
struct DattorroReverb {
    float sample_rate;
    float params[NUM_DATTORRO_PARAMS];

    // Statically allocated delay lines
    float delay_buf_0[DELAY_BUF_SIZE_0];
    float delay_buf_1[DELAY_BUF_SIZE_1];
    float delay_buf_2[DELAY_BUF_SIZE_2];
    float delay_buf_3[DELAY_BUF_SIZE_3];
    float delay_buf_4[DELAY_BUF_SIZE_4];
    float delay_buf_5[DELAY_BUF_SIZE_5];
    float delay_buf_6[DELAY_BUF_SIZE_6];
    float delay_buf_7[DELAY_BUF_SIZE_7];
    float delay_buf_8[DELAY_BUF_SIZE_8];
    float delay_buf_9[DELAY_BUF_SIZE_9];
    float delay_buf_10[DELAY_BUF_SIZE_10];
    float delay_buf_11[DELAY_BUF_SIZE_11];

    DelayLine delays[12];
    int16_t taps[14];

    float pre_delay_buffer[DATTORRO_MAX_SR];
    int pre_delay_length;
    int pre_delay_write_pos;

    float lp1, lp2, lp3;
    float exc_phase;
};


#endif // DATTORRO_REVERB_H
