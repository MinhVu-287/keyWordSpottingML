/* Edge Impulse Arduino examples
 * Copyright (c) 2022 EdgeImpulse Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// These sketches are tested with 2.0.4 ESP32 Arduino Core
// https://github.com/espressif/arduino-esp32/releases/tag/2.0.4

// If your target is limited in memory remove this macro to save 10K RAM
#include "edge-impulse-sdk/classifier/ei_run_dsp.h"
#define EIDSP_QUANTIZE_FILTERBANK 0

/*
 ** NOTE: If you run into TFLite arena allocation issue.
 **
 ** This may be due to may dynamic memory fragmentation.
 ** Try defining "-DEI_CLASSIFIER_ALLOCATION_STATIC" in boards.local.txt (create
 ** if it doesn't exist) and copy this file to
 ** `<ARDUINO_CORE_INSTALL_PATH>/arduino/hardware/<mbed_core>/<core_version>/`.
 **
 ** See
 ** (https://support.arduino.cc/hc/en-us/articles/360012076960-Where-are-the-installed-cores-located-)
 ** to find where Arduino installs cores on your machine.
 **
 ** If the problem persists then there's not enough memory for this model and application.
 */

/* Includes ---------------------------------------------------------------- */
#include <keyword_spotting_2cnn_inferencing.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "HardwareSerial.h"
#include <I2S.h>
#define SAMPLE_RATE 16000U
#define SAMPLE_BITS 16
#define APP_LOG 1

static void audio_inference_callback(uint32_t n_bytes);
static void capture_samples(void *arg);
static bool microphone_inference_start(uint32_t n_samples);
static bool microphone_inference_record(void);
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr);
static void microphone_inference_end(void);
static void controll(void *arg);

#define led_wakeup LED_BUILTIN
#define led_on D1
// Define Event Group
EventGroupHandle_t ledEventGroup;
#define LED_ON_BIT (1 << 0)
#define LED_OFF_BIT (1 << 1)
#define LED_WAKEUP_BIT (1 << 2)
HardwareSerial uart1(1);
/** Audio buffers, pointers and selectors */
typedef struct
{
    signed short *buffers[2];
    unsigned char buf_select;
    unsigned char buf_ready;
    unsigned int buf_count;
    unsigned int n_samples;
} inference_t;

static inference_t inference;
static const uint32_t sample_buffer_size = 2048;
static signed short sampleBuffer[sample_buffer_size];
static bool debug_nn = false; // Set this to true to see e.g. features generated from the raw signal
static int print_results = -(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW);
static bool record_status = true;

/**
 * @brief      Arduino setup function
 */
void setup()
{
    // put your setup code here, to run once:
    Serial.begin(115200);
    // comment out the below line to cancel the wait for USB connection (needed for native USB)
    pinMode(led_wakeup, OUTPUT);
    pinMode(led_on, OUTPUT);
    while (!Serial)
        ;
    Serial.println("Edge Impulse Inferencing Demo");
    I2S.setAllPins(-1, 42, 41, -1, -1);
    if (!I2S.begin(PDM_MONO_MODE, SAMPLE_RATE, SAMPLE_BITS))
    {
        Serial.println("Failed to initialize I2S!");
        while (1)
            ;
    }
    // Initialize Event Group
    ledEventGroup = xEventGroupCreate();

    // summary of inferencing settings (from model_metadata.h)
    ei_printf("Inferencing settings:\n");
    ei_printf("\tInterval: ");
    ei_printf_float((float)EI_CLASSIFIER_INTERVAL_MS);
    ei_printf(" ms.\n");
    ei_printf("\tFrame size: %d\n", EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE);
    ei_printf("\tSample length: %d ms.\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT / 16);
    ei_printf("\tNo. of classes: %d\n", sizeof(ei_classifier_inferencing_categories) / sizeof(ei_classifier_inferencing_categories[0]));

    run_classifier_init();
    ei_printf("\nStarting continious inference in 2 seconds...\n");
    ei_sleep(2000);

    if (microphone_inference_start(EI_CLASSIFIER_RAW_SAMPLE_COUNT) == false)
    {
        ei_printf("ERR: Could not allocate audio buffer (size %d), this could be due to the window length of your model\r\n", EI_CLASSIFIER_RAW_SAMPLE_COUNT);
        return;
    }

    ei_printf("Recording...\n");
}

/**
 * @brief      Arduino main function. Runs the inferencing loop.
 */

void loop()
{
    bool m = microphone_inference_record();
    if (!m)
    {
        ei_printf("ERR: Failed to record audio...\n");
        return;
    }
    signal_t signal;
    signal.total_length = EI_CLASSIFIER_RAW_SAMPLE_COUNT;
    signal.get_data = &microphone_audio_signal_get_data;
    ei_impulse_result_t result = {0};

    EI_IMPULSE_ERROR r = run_classifier_continuous(&signal, &result, debug_nn);
    if (r != EI_IMPULSE_OK)
    {
        ei_printf("ERR: Failed to run classifier (%d)\n", r);
        return;
    }
    size_t pre_ix = 0;
    float pre_value = 0.0;
    if (++print_results >= (EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW))
    {
        // print the predictions
        for (size_t ix = 0; ix < EI_CLASSIFIER_LABEL_COUNT; ix++)
        {
            if (result.classification[ix].value > pre_value)
            {
                pre_ix = ix;
                pre_value = result.classification[ix].value;
            }
        }

        // Set Event Group bits based on classification
        if (result.classification[pre_ix].label == "bật đèn")
        {
            xEventGroupSetBits(ledEventGroup, LED_ON_BIT);
            Serial.println("Led is on");
            xEventGroupClearBits(ledEventGroup, LED_OFF_BIT);
        }
        else if (result.classification[pre_ix].label == "tắt đèn")
        {
            xEventGroupClearBits(ledEventGroup, LED_ON_BIT);
            Serial.println("Led is off");
            xEventGroupSetBits(ledEventGroup, LED_OFF_BIT);
        }
        else if (result.classification[pre_ix].label == "mở cửa")
        {
            xEventGroupSetBits(ledEventGroup, LED_ON_BIT); 
            Serial.println("door is open");
            xEventGroupClearBits(ledEventGroup, LED_OFF_BIT);
        }
        else if (result.classification[pre_ix].label == "đóng cửa")
        {
            xEventGroupClearBits(ledEventGroup, LED_ON_BIT); 
            Serial.println("door is close");
            xEventGroupSetBits(ledEventGroup, LED_OFF_BIT);
        }
        else
        {
            xEventGroupClearBits(ledEventGroup, LED_ON_BIT | LED_OFF_BIT);
            Serial.println("noise is detected");
        }
    }
}

static void audio_inference_callback(uint32_t n_bytes)
{
    for (int i = 0; i < n_bytes >> 1; i++)
    {
        inference.buffers[inference.buf_select][inference.buf_count++] = sampleBuffer[i];

        if (inference.buf_count >= inference.n_samples)
        {
            inference.buf_select ^= 1;
            inference.buf_count = 0;
            inference.buf_ready = 1;
        }
    }
}

static void capture_samples(void *arg)
{

    const int32_t i2s_bytes_to_read = (uint32_t)arg;
    size_t bytes_read = i2s_bytes_to_read;

    while (record_status)
    {

        /* read data at once from i2s */
        esp_i2s::i2s_read(esp_i2s::I2S_NUM_0, (void *)sampleBuffer, i2s_bytes_to_read, &bytes_read, 100);

        if (bytes_read <= 0)
        {
            ei_printf("Error in I2S read : %d", bytes_read);
        }
        else
        {
            if (bytes_read < i2s_bytes_to_read)
            {
                ei_printf("Partial I2S read");
            }

            // scale the data (otherwise the sound is too quiet)
            for (int x = 0; x < i2s_bytes_to_read / 2; x++)
            {
                sampleBuffer[x] = (int16_t)(sampleBuffer[x]) * 8;
            }

            if (record_status)
            {
                audio_inference_callback(i2s_bytes_to_read);
            }
            else
            {
                break;
            }
        }
    }
    vTaskDelete(NULL);
}

/**
 * @brief      Init inferencing struct and setup/start PDM
 *
 * @param[in]  n_samples  The n samples
 *
 * @return     { description_of_the_return_value }
 */
static bool microphone_inference_start(uint32_t n_samples)
{
    inference.buffers[0] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[0] == NULL)
    {
        return false;
    }

    inference.buffers[1] = (signed short *)malloc(n_samples * sizeof(signed short));

    if (inference.buffers[1] == NULL)
    {
        ei_free(inference.buffers[0]);
        return false;
    }

    inference.buf_select = 0;
    inference.buf_count = 0;
    inference.n_samples = n_samples;
    inference.buf_ready = 0;

    ei_sleep(100);

    record_status = true;
    xTaskCreate(controll, "Controll", 1024 * 20, NULL, 8, NULL);
    xTaskCreate(capture_samples, "CaptureSamples", 1024 * 32, (void *)sample_buffer_size, 10, NULL);

    return true;
}

/**
 * @brief      Wait on new data
 *
 * @return     True when finished
 */
static bool microphone_inference_record(void)
{
    bool ret = true;

    if (inference.buf_ready == 1)
    {
        ei_printf(
            "Error sample buffer overrun. Decrease the number of slices per model window "
            "(EI_CLASSIFIER_SLICES_PER_MODEL_WINDOW)\n");
        ret = false;
    }

    while (inference.buf_ready == 0)
    {
        delay(1);
    }

    inference.buf_ready = 0;
    return true;
}

/**
 * Get raw audio signal data
 */
static int microphone_audio_signal_get_data(size_t offset, size_t length, float *out_ptr)
{
    numpy::int16_to_float(&inference.buffers[inference.buf_select ^ 1][offset], out_ptr, length);

    return 0;
}

/**
 * @brief      Stop PDM and release buffers
 */
static void microphone_inference_end(void)
{
    free(sampleBuffer);
    ei_free(inference.buffers[0]);
    ei_free(inference.buffers[1]);
}

void controll(void *arg)
{
    while (1)
    {
        EventBits_t bits = xEventGroupWaitBits(ledEventGroup,
                                               LED_ON_BIT | LED_OFF_BIT | LED_WAKEUP_BIT,
                                               pdTRUE,
                                               pdFALSE,
                                               portMAX_DELAY);
        if (bits & LED_WAKEUP_BIT)
        {
#if APP_LOG
            ei_printf("led wakeup\n");
#endif
            digitalWrite(led_wakeup, LOW);      
            unsigned long startTime = millis(); 

            while (millis() - startTime < 5000)
            { 
                EventBits_t currentBits = xEventGroupGetBits(ledEventGroup);
                if (currentBits & LED_ON_BIT)
                {
                    digitalWrite(led_on, HIGH);
#if APP_LOG
                    ei_printf("Led2 is on\n");
                    uart1.println('1');
#endif
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    break;
                }
                else if (currentBits & LED_OFF_BIT)
                {
                    vTaskDelay(50 / portTICK_PERIOD_MS);
                    break;
                }

                vTaskDelay(20 / portTICK_PERIOD_MS); 
            }
#if APP_LOG
            ei_printf("led sleep\n");
#endif
            digitalWrite(led_wakeup, HIGH); 
        }
    }
}

#if !defined(EI_CLASSIFIER_SENSOR) || EI_CLASSIFIER_SENSOR != EI_CLASSIFIER_SENSOR_MICROPHONE
#error "Invalid model for current sensor."
#endif