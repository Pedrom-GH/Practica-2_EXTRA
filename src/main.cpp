#include <Arduino.h>

// --- Configuración de Pines ---
#define LED_PIN 2        // LED integrado en la mayoría de ESP32
#define BTN_UP 12       // Pulsador para subir frecuencia
#define BTN_DOWN 14     // Pulsador para bajar frecuencia

// --- Variables de Control (Volatile porque se usan en la ISR) ---
volatile int blinkIntervalMs = 500; // Intervalo inicial (500ms = 1Hz parpadeo completo)
volatile int timerCounter = 0;
volatile bool ledState = false;

// Variables para Debouncing (filtrado)
const int DEBOUNCE_TIME = 50; // 50ms de estabilidad requerida
volatile int countUp = 0;
volatile int countDown = 0;
volatile bool lastBtnUp = HIGH;
volatile bool lastBtnDown = HIGH;

// Puntero para el timer
hw_timer_t * timer = NULL;

// --- Rutina de Interrupción del Timer (ISR) ---
// Se ejecuta cada 1ms
void IRAM_ATTR onTimer() {
    // 1. Lógica del LED
    timerCounter++;
    if (timerCounter >= blinkIntervalMs) {
        timerCounter = 0;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState);
    }

    // 2. Filtrado (Debounce) del Pulsador UP
    bool currentUp = digitalRead(BTN_UP);
    if (currentUp == LOW && lastBtnUp == HIGH) { // Detecta flanco de bajada (presionado)
        countUp++;
        if (countUp > DEBOUNCE_TIME) {
            blinkIntervalMs = max(50, blinkIntervalMs - 50); // Sube frecuencia (baja el periodo)
            countUp = 0; 
            lastBtnUp = LOW; 
        }
    } else if (currentUp == HIGH) {
        countUp = 0;
        lastBtnUp = HIGH;
    }

    // 3. Filtrado (Debounce) del Pulsador DOWN
    bool currentDown = digitalRead(BTN_DOWN);
    if (currentDown == LOW && lastBtnDown == HIGH) {
        countDown++;
        if (countDown > DEBOUNCE_TIME) {
            blinkIntervalMs = min(2000, blinkIntervalMs + 50); // Baja frecuencia (sube el periodo)
            countDown = 0;
            lastBtnDown = LOW;
        }
    } else if (currentDown == HIGH) {
        countDown = 0;
        lastBtnDown = HIGH;
    }
}

void setup() {
    Serial.begin(115200);
    
    pinMode(LED_PIN, OUTPUT);
    pinMode(BTN_UP, INPUT_PULLUP);
    pinMode(BTN_DOWN, INPUT_PULLUP);

    // Configuración del Timer:
    // - Frecuencia base del ESP32: 80MHz. 
    // - Prescaler de 80 -> 1 tick = 1 microsegundo.
    timer = timerBegin(0, 80, true); 
    timerAttachInterrupt(timer, &onTimer, true);
    
    // Alarma cada 1000 ticks = 1ms. Autoreload = true.
    timerAlarmWrite(timer, 1000, true); 
    timerAlarmEnable(timer);
    
    Serial.println("Sistema iniciado. Usa los botones para cambiar la frecuencia.");
}

void loop() {
    // El loop queda libre para otras tareas o debug.
    static int lastInterval = 0;
    if (blinkIntervalMs != lastInterval) {
        Serial.printf("Nuevo intervalo: %d ms\n", blinkIntervalMs);
        lastInterval = blinkIntervalMs;
    }
    delay(100); 
}