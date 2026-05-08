// tflm_runner.cpp — Run the quantized sine model using TFLM
#include <cstdio>
#include <cstdint>

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "TFLM_model/sine_model_quantized.h"
#include "UART_Debug/uart_debug.h"
#include "sys_time/sys_time.h"

constexpr int kTensorArenaSize = 8 * 1024;
static uint8_t tensor_arena[kTensorArenaSize];

using OpResolver = tflite::MicroMutableOpResolver<5>;

static uint8_t interpreter_buf[sizeof(tflite::MicroInterpreter)];
static OpResolver op_resolver;

static tflite::MicroInterpreter *gp_interpreter = nullptr;
static TfLiteTensor *gp_input = nullptr;
static TfLiteTensor *gp_output = nullptr;

static void print_uint(uint32_t v) {
    char buf[12];
    int i = 0;
    if (v == 0) { putchar('0'); return; }
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) putchar(buf[i]);
}

static void print_status(uint32_t infer_us, uint32_t count) {
    uint32_t arena_used = gp_interpreter->arena_used_bytes();

    printf("[#");
    print_uint(count);
    printf("] ");
    print_uint(infer_us);
    printf("us | arena ");
    print_uint(arena_used);
    printf("/");
    print_uint(kTensorArenaSize);
    printf("B (");
    print_uint(arena_used * 100 / kTensorArenaSize);
    printf("%) | model ");
    print_uint(g_model_len);
    printf("B\r\n");
}

static void tflm_init(void) {
    const tflite::Model *model = tflite::GetModel(g_model);

    op_resolver.AddQuantize();
    op_resolver.AddFullyConnected();
    op_resolver.AddRelu();
    op_resolver.AddAdd();
    op_resolver.AddDequantize();

    gp_interpreter = new (interpreter_buf) tflite::MicroInterpreter(
        model, op_resolver, tensor_arena, kTensorArenaSize);
    gp_interpreter->AllocateTensors();
    gp_input = gp_interpreter->input(0);
    gp_output = gp_interpreter->output(0);
}

static float tflm_infer(float x) {
    if (gp_input->type == kTfLiteInt8)
        gp_input->data.int8[0] = (int8_t)(x / gp_input->params.scale + gp_input->params.zero_point);
    else
        gp_input->data.f[0] = x;

    gp_interpreter->Invoke();

    if (gp_output->type == kTfLiteInt8)
        return (gp_output->data.int8[0] - gp_output->params.zero_point) * gp_output->params.scale;
    else
        return gp_output->data.f[0];
}

void tflm_run_sine(void) {
    tflm_init();

    float x = 0.0f;
    uint32_t count = 0;

    for (;;) {
        uint32_t t0 = SysTime_Get_us();
        float y = tflm_infer(x);
        uint32_t elapsed = SysTime_Elapsed_us(t0, SysTime_Get_us());

        print_status(elapsed, count);

        x += 0.01f;
        if (x > 6.283f) x = 0.0f;
        count++;

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
    }
}
