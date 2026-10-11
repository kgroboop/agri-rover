#include <Arduino.h>

// Built-in Espressif ESP-TFLite-Micro Headers (ESP32 Core 3.x)
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "model_data.h"

// 2 MB Tensor Arena in PSRAM
constexpr int kTensorArenaSize = 2048 * 1024; 
uint8_t* tensor_arena = nullptr;

// Global TFLite Pointers
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;

// Op Resolver with 40-op capacity
tflite::MicroMutableOpResolver<40> resolver;

void setupModel() {
    // 1. Allocate PSRAM with explicit 16-byte alignment
    if (psramFound()) {
        tensor_arena = (uint8_t*) heap_caps_aligned_alloc(16, kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        Serial.println("Allocated Tensor Arena in PSRAM (16-byte aligned)!");
    } else {
        tensor_arena = (uint8_t*) heap_caps_aligned_alloc(16, kTensorArenaSize, MALLOC_CAP_8BIT);
        Serial.println("Allocated Tensor Arena in Internal SRAM!");
    }

    if (tensor_arena == nullptr) {
        Serial.println("Error: Failed to allocate memory for Tensor Arena!");
        return;
    }

    // 2. Load FlatBuffer Model
    model = tflite::GetModel(g_model);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        Serial.printf("Model schema version %d not supported (expected %d).\n",
                      model->version(), TFLITE_SCHEMA_VERSION);
        return;
    }

    // 3. Register required operations
    resolver.AddConv2D();
    resolver.AddDepthwiseConv2D();
    resolver.AddFullyConnected();
    resolver.AddReshape();
    resolver.AddSoftmax();
    resolver.AddQuantize();
    resolver.AddDequantize();
    resolver.AddMaxPool2D();
    resolver.AddAveragePool2D();
    resolver.AddAdd();
    resolver.AddMul();
    resolver.AddSub();
    resolver.AddPad();
    resolver.AddPadV2();
    resolver.AddRelu();
    resolver.AddRelu6();
    resolver.AddLogistic();
    resolver.AddConcatenation();
    resolver.AddMean();
    resolver.AddSqueeze();
    resolver.AddResizeNearestNeighbor();
    resolver.AddResizeBilinear();
    resolver.AddArgMax();
    resolver.AddCast();
    resolver.AddExpandDims();
    resolver.AddStridedSlice();
    resolver.AddPack();
    resolver.AddUnpack();
    resolver.AddMaximum();
    resolver.AddMinimum();

    // 4. Instantiate Interpreter
    static tflite::MicroInterpreter static_interpreter(
        model, 
        resolver, 
        tensor_arena, 
        kTensorArenaSize
    );
    interpreter = &static_interpreter;

    // 5. Allocate Tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        Serial.println("Error: AllocateTensors() failed!");
        return;
    }

    // 6. Get Input / Output Pointers
    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TFLite Model setup complete!");
    Serial.printf("Input Tensor Bytes: %d\n", input->bytes);
    Serial.printf("Input Tensor Type: %d\n", input->type);
}

// Helper function to load frame buffer and run inference
void runInference(const uint8_t* image_data) {
    if (interpreter == nullptr || input == nullptr) {
        Serial.println("Model not properly setup!");
        return;
    }

    // 1. Copy raw RGB bytes into input tensor
    memcpy(input->data.uint8, image_data, input->bytes);

    // 2. Invoke model and measure execution time
    unsigned long start_time = millis();
    TfLiteStatus invoke_status = interpreter->Invoke();
    unsigned long duration = millis() - start_time;

    if (invoke_status != kTfLiteOk) {
        Serial.println("Inference execution failed!");
        return;
    }

    Serial.printf("Inference completed in %lu ms\n", duration);

    // 3. Read Output Predictions
    int output_elements = output->bytes;
    Serial.printf("Output tensor elements: %d\n", output_elements);

    // Find class with highest confidence score
    int best_class = 0;
    uint8_t highest_score = output->data.uint8[0];

    for (int i = 1; i < output_elements; i++) {
        if (output->data.uint8[i] > highest_score) {
            highest_score = output->data.uint8[i];
            best_class = i;
        }
    }

    Serial.printf("Detected Class: %d (Confidence: %d/255)\n", best_class, highest_score);
}

void setup() {
    Serial.begin(115200);
    while (!Serial && millis() < 3000);

    Serial.println("Initializing TFLite Model on ESP32-S3...");
    setupModel();
}

void loop() {
    static uint8_t* test_buffer = nullptr;
    static int pattern_id = 0;

    // Allocate buffer in PSRAM
    if (test_buffer == nullptr) {
        test_buffer = (uint8_t*) heap_caps_malloc(input->bytes, MALLOC_CAP_SPIRAM);
    }

    if (test_buffer != nullptr) {
        // Generate different test patterns each cycle
        switch (pattern_id % 4) {
            case 0:
                Serial.println("\n--- Test Pattern 1: Solid Black (0) ---");
                memset(test_buffer, 0, input->bytes);
                break;

            case 1:
                Serial.println("\n--- Test Pattern 2: Solid White (255) ---");
                memset(test_buffer, 255, input->bytes);
                break;

            case 2:
                Serial.println("\n--- Test Pattern 3: Diagonal Gradient ---");
                for (int i = 0; i < input->bytes; i++) {
                    test_buffer[i] = (uint8_t)(i % 256);
                }
                break;

            case 3:
                Serial.println("\n--- Test Pattern 4: Alternating Stripe Pattern ---");
                for (int i = 0; i < input->bytes; i++) {
                    test_buffer[i] = (i % 2 == 0) ? 200 : 50;
                }
                break;
        }

        pattern_id++;

        // Run inference
        runInference(test_buffer);
    } else {
        Serial.println("Failed to allocate test buffer!");
    }

    delay(3000);
}