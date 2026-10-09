#include <ESP32Servo.h>

const int LED_PIN = 2; //LED
const int SERVO_PIN = 18;

Servo myServo;

//Task 
TaskHandle_t LedTaskHandle;
TaskHandle_t ServoTaskHandle;
TaskHandle_t MlTaskHandle;

//1.High Priority: Heartbeat LED (Strict 1Hz)
void vTaskBlinkLED(void *pvParameters) {
    pinMode(LED_PIN, OUTPUT);
    for (;;) {
        digitalWrite(LED_PIN, !digitalRead(LED_PIN));
        vTaskDelay(pdMS_TO_TICKS(500)); //Block  500ms
    }
}

//2.Medium Priority: Smooth Servo Sweep
void vTaskServoControl(void *pvParameters) {
    myServo.attach(SERVO_PIN);
    int angle = 0;
    int step = 1;

    for (;;) {
        myServo.write(angle);
        angle += step;
        if (angle >= 180 || angle <= 0) {
            step = -step; //Reverse direction
        }
        vTaskDelay(pdMS_TO_TICKS(15)); //Yield 15ms per degree
    }
}

//3.Low Priority: I2S Audio & TinyML Inference (Heavy CPU Load)
void vTaskAudioML(void *pvParameters) {
    for (;;) {
        //use a busy while-loop here instead of vTaskDelay.
        //intentionally burns CPU cycles to simulate the blocking nature of extracting audio MFCCs and running a neural network inference.
        unsigned long startCompute = millis();
        while (millis() - startCompute < 200) {
            //imulating 200ms of intensive blocking math
        }

        Serial.println("Keyword spotting inference complete.");

        //yields tiny bit so the FreeRTOS idle task can run and reset the watchdog timer
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void setup() {
    Serial.begin(115200);

    //Create tasks and pin them exclusively to Core 1.
    //By forcing them to share a single core, they must compete for CPU time.

    xTaskCreatePinnedToCore(
        vTaskBlinkLED,     //Task function
        "LED_Task",        //Name
        1024,              //Stack size
        NULL,              //Parameters
        3,                 //priority (3 = Highest)
        &LedTaskHandle,    //Handle
        1                  //Core ID
    );

    xTaskCreatePinnedToCore(
        vTaskServoControl,
        "Servo_Task",
        2048,
        NULL,
        2,                 //Priority (2 = Medium)
        &ServoTaskHandle,
        1
    );

    xTaskCreatePinnedToCore(
        vTaskAudioML,
        "ML_Audio_Task",
        4096,              //Larger stack needed for ML math
        NULL,
        1,                 //Priority (1 = Lowest)
        &MlTaskHandle,
        1
    );
}

void loop() {
    //Empty. The FreeRTOS scheduler handles execution entirely in the background.
    vTaskDelete(NULL);
}