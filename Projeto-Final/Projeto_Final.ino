#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <LiquidCrystal.h>

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

//Frequências das 7 notas: C, D, E, F, G, A, B
int notas[] = {262, 294, 330, 349, 392, 440, 494};
const char* nomesNotas[] = {"C", "D", "E", "F", "G", "A", "B"};

SemaphoreHandle_t lcdMutex;
QueueHandle_t filaSom;
QueueHandle_t filaGravacao;

enum Estado {NORMAL, GRAVANDO, REPRODUZINDO};
Estado estadoAtual = NORMAL;

class Joystick {
  int pinX, pinY, pinBotao;
  unsigned long ultimaLeituraBotao = 0;
  bool botaoAnterior = false;
  bool botaoPressionado = false;

  unsigned long lastClickTime = 0;
  int clickCount = 0;

public:
  Joystick(int x, int y, int botao): pinX(x), pinY(y), pinBotao(botao) {
    pinMode(pinBotao, INPUT_PULLUP);
  }

  void update() {
    bool leituraAtual = !digitalRead(pinBotao);
    if (leituraAtual && !botaoAnterior && millis() - ultimaLeituraBotao > 50) {
      // Botao foi pressionado
      unsigned long now = millis();
      if (now - lastClickTime < 400) {
        clickCount++;
      } else {
        clickCount = 1;
      }
      lastClickTime = now;
      botaoPressionado = true;
      ultimaLeituraBotao = now;
    } else {
      botaoPressionado = false;
    }
    botaoAnterior = leituraAtual;
  }

  bool pressionado() {
    return botaoPressionado;
  }

  int getClickCount() {
    unsigned long now = millis();
    if (clickCount > 0 && (now - lastClickTime > 400)) {
      int ret = clickCount;
      clickCount = 0;
      return ret;
    }
    return 0;
  }

  String getDirecao() {
    int x = analogRead(pinX);
    int y = analogRead(pinY);

    bool left = x < 300;
    bool right = x > 700;

    bool up = y > 700;
    bool down = y < 300;


    if (up && left) return "CIMA_ESQUERDA";      // G
    if (up && right) return "CIMA_DIREITA";      // A
    if (down && left) return "BAIXO_ESQUERDA";   // B

    if (up) return "CIMA";                        // C
    if (down) return "BAIXO";                     // D
    if (left) return "ESQUERDA";                   // E
    if (right) return "DIREITA";                   // F

    return "CENTRO"; // 
  }
};

class Buzzer {
  int pino;
public:
  Buzzer(int p): pino(p) {
    pinMode(pino, OUTPUT);
  }

  void tocar(int freq) {
    tone(pino, freq);
  }

  void parar() {
    noTone(pino);
  }
};

Joystick joystick(A0, A1, 2);
Buzzer buzzer(3);

int notaAtualIndex = 0;
String direcaoAnterior = "CENTRO";

void taskInterface(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(lcdMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      lcd.setCursor(0, 0);
      lcd.print("Nota: ");
      lcd.print(nomesNotas[notaAtualIndex]);
      lcd.print("   ");
      lcd.setCursor(9, 0);
      lcd.print(notas[notaAtualIndex]);
      lcd.print("Hz  ");

      lcd.setCursor(0, 1);
      switch (estadoAtual) {
        case NORMAL:    lcd.print("Modo: NORMAL    "); break;
        case GRAVANDO:  lcd.print("Modo: GRAVANDO  "); break;
        case REPRODUZINDO: lcd.print("Modo: PLAY      "); break;
      }

      xSemaphoreGive(lcdMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(250));
  }
}

void taskJoystick(void *pvParameters) {
  while (1) {
    joystick.update();

    int clicks = joystick.getClickCount();
    if (clicks == 1) {

      if (estadoAtual == GRAVANDO) {
        estadoAtual = NORMAL;
      } else {
        estadoAtual = GRAVANDO;
      }
    } else if (clicks == 2) {

      if (estadoAtual != REPRODUZINDO) {
        estadoAtual = REPRODUZINDO;
      }
    }

    String dir = joystick.getDirecao();
    if (dir != "CENTRO" && dir != direcaoAnterior) {

      if (dir == "CIMA") notaAtualIndex = 0;              // C
      else if (dir == "CIMA_DIREITA") notaAtualIndex = 1; // D
      else if (dir == "DIREITA") notaAtualIndex = 2;      // E
      else if (dir == "BAIXO") notaAtualIndex = 3;        // F
      else if (dir == "BAIXO_ESQUERDA") notaAtualIndex = 4; // G
      else if (dir == "ESQUERDA") notaAtualIndex = 5;     // A
      else if (dir == "CIMA_ESQUERDA") notaAtualIndex = 6; // B

      else notaAtualIndex = 0; 

      int freq = notas[notaAtualIndex];
      xQueueSend(filaSom, &freq, 0);

      if (estadoAtual == GRAVANDO) {
        xQueueSend(filaGravacao, &notaAtualIndex, 0);
      }
    }
    direcaoAnterior = dir;

    vTaskDelay(pdMS_TO_TICKS(150));
  }
}

void taskSom(void *pvParameters) {
  int freq;
  while (1) {
    if (xQueueReceive(filaSom, &freq, portMAX_DELAY)) {
      buzzer.tocar(freq);
      vTaskDelay(pdMS_TO_TICKS(500));
      buzzer.parar();
    }
  }
}

void taskReproducao(void *pvParameters) {
  while (1) {
    if (estadoAtual == REPRODUZINDO) {
      int notaIndex;
      int count = uxQueueMessagesWaiting(filaGravacao);
      for (int i = 0; i < count; i++) {
        if (xQueueReceive(filaGravacao, &notaIndex, 0) == pdTRUE) {
          int freq = notas[notaIndex];
          xQueueSend(filaSom, &freq, 0);
          xQueueSend(filaGravacao, &notaIndex, 0); 
          vTaskDelay(pdMS_TO_TICKS(100));
        }
      }
      estadoAtual = NORMAL;
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void setup() {
  lcd.begin(16, 2);
  lcd.clear();

  lcdMutex = xSemaphoreCreateMutex();
  filaSom = xQueueCreate(10, sizeof(int));
  filaGravacao = xQueueCreate(50, sizeof(int));

  xTaskCreate(taskInterface, "Interface", 128, NULL, 1, NULL);
  xTaskCreate(taskJoystick, "Joystick", 128, NULL, 2, NULL);
  xTaskCreate(taskSom, "Som", 128, NULL, 2, NULL);
  xTaskCreate(taskReproducao, "Reproducao", 128, NULL, 1, NULL);
}

void loop() {}

