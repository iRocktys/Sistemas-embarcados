#include <Arduino_FreeRTOS.h>
#include <queue.h>
#include <semphr.h>
#include <LiquidCrystal.h>

enum EstadoSistema { MOSTRAR_HORA, AJUSTAR_HORA, AJUSTAR_ALARME, TOCANDO_ALARME };
EstadoSistema estadoAtual = MOSTRAR_HORA;

SemaphoreHandle_t lcdMutex;
QueueHandle_t filaEstado;

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

bool alarmeDisparado = false;  
int alarmeAtual = 0;       

class Joystick {
  int pinX, pinY, pinBotao;
  unsigned long ultimaLeitura = 0;
  bool botaoAnterior = false;
  bool botaoPressionado = false;

public:
  Joystick(int x, int y, int botao) : pinX(x), pinY(y), pinBotao(botao) {
    pinMode(pinBotao, INPUT_PULLUP);
  }

  void update() {
    bool leituraAtual = !digitalRead(pinBotao);
    if (leituraAtual && !botaoAnterior && millis() - ultimaLeitura > 300) {
      botaoPressionado = true;
      ultimaLeitura = millis();
    } else {
      botaoPressionado = false;
    }
    botaoAnterior = leituraAtual;
  }

  bool pressionado() {
    return botaoPressionado;
  }

  String getDirecao() {
    int x = analogRead(pinX);
    int y = analogRead(pinY);
    if (y > 800) return "CIMA";
    if (y < 200) return "BAIXO";
    if (x > 800) return "DIREITA";
    if (x < 200) return "ESQUERDA";
    return "CENTRO";
  }
};

class Clock {
public:
  int horas = 0, minutos = 0, segundos = 0;
  int alarmeHora = 0, alarmeMinuto = 0, alarmeSegundo = 0;

  void tick() {
    segundos++;
    if (segundos >= 60) {
      segundos = 0;
      minutos++;
      if (minutos >= 60) {
        minutos = 0;
        horas = (horas + 1) % 24;
      }
    }
  }

  void ajustarHora(String dir) {
    if (dir == "CIMA") horas = (horas + 1) % 24;
    if (dir == "BAIXO") horas = (horas + 23) % 24;
    if (dir == "DIREITA") minutos = (minutos + 1) % 60;
    if (dir == "ESQUERDA") minutos = (minutos + 59) % 60;
  }

  void ajustarAlarme(String dir) {
    if (dir == "CIMA") alarmeHora = (alarmeHora + 1) % 24;
    if (dir == "BAIXO") alarmeHora = (alarmeHora + 23) % 24;
    if (dir == "DIREITA") alarmeMinuto = (alarmeMinuto + 1) % 60;
    if (dir == "ESQUERDA") alarmeMinuto = (alarmeMinuto + 59) % 60;
  }

  bool horaIgualAlarme() {
    return (horas == alarmeHora && minutos == alarmeMinuto && segundos == alarmeSegundo);
  }

  String getHoraAtual() {
    return (horas < 10 ? "0" : "") + String(horas) + ":" +
           (minutos < 10 ? "0" : "") + String(minutos) + ":" +
           (segundos < 10 ? "0" : "") + String(segundos);
  }

  String getHoraAlarme() {
    return (alarmeHora < 10 ? "0" : "") + String(alarmeHora) + ":" +
           (alarmeMinuto < 10 ? "0" : "") + String(alarmeMinuto) + ":" +
           (alarmeSegundo < 10 ? "0" : "") + String(alarmeSegundo);
  }
};

class Buzzer {
  int pino;
  bool estado = false;
  int frequenciaAtual = 0;

public:
  Buzzer(int p) : pino(p) {
    pinMode(pino, OUTPUT);
  }

  void tocar(int freq) {
    if (!estado || frequenciaAtual != freq) {
      tone(pino, freq);
      frequenciaAtual = freq;
      estado = true;
    }
  }

  void parar() {
    noTone(pino);
    estado = false;
    frequenciaAtual = 0;
  }

  bool estaTocando() {
    return estado;
  }
};

Joystick joystick(A0, A1, 2);
Clock relogio;
Buzzer buzzer(3);

void limparLinha(int linha) {
  lcd.setCursor(0, linha);
  lcd.print("                ");
}

void taskRelogio(void *pvParameters) {
  while (1) {
    relogio.tick();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void taskJoystick(void *pvParameters) {
  while (1) {
    joystick.update();
    if (joystick.pressionado()) {
      xQueueSend(filaEstado, &estadoAtual, 0);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void taskInterface(void *pvParameters) {
  while (1) {
    if (xSemaphoreTake(lcdMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
      limparLinha(0);
      limparLinha(1);
      switch (estadoAtual) {
        case MOSTRAR_HORA:
          lcd.setCursor(0, 0);
          lcd.print("Hora:   " + relogio.getHoraAtual());
          lcd.setCursor(0, 1);
          lcd.print("Alarme: " + relogio.getHoraAlarme());
          break;
        case AJUSTAR_HORA:
          lcd.setCursor(0, 0);
          lcd.print("Ajuste: " + relogio.getHoraAtual());
          lcd.setCursor(0, 1);
          lcd.print("-> Alarme");
          break;
        case AJUSTAR_ALARME:
          lcd.setCursor(0, 0);
          lcd.print("Alarme: " + relogio.getHoraAlarme());
          lcd.setCursor(0, 1);
          lcd.print("-> Voltar");
          break;
        case TOCANDO_ALARME:
          lcd.setCursor(0, 0);
          lcd.print("** TOCANDO **");
          lcd.setCursor(0, 1);
          break;
      }
      xSemaphoreGive(lcdMutex);
    }
    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

void taskControle(void *pvParameters) {
  while (1) {
    if (!alarmeDisparado && relogio.horaIgualAlarme()) {
      estadoAtual = TOCANDO_ALARME;
      alarmeDisparado = true;
    }

    if (estadoAtual == AJUSTAR_HORA) {
      relogio.ajustarHora(joystick.getDirecao());
    } else if (estadoAtual == AJUSTAR_ALARME) {
      relogio.ajustarAlarme(joystick.getDirecao());
    }

    EstadoSistema recebido;
    if (xQueueReceive(filaEstado, &recebido, 0) == pdPASS) {
      switch (estadoAtual) {
        case MOSTRAR_HORA:
          estadoAtual = AJUSTAR_HORA;
          break;
        case AJUSTAR_HORA:
          estadoAtual = AJUSTAR_ALARME;
          break;
        case AJUSTAR_ALARME:
          estadoAtual = MOSTRAR_HORA;
          break;
        case TOCANDO_ALARME:
          buzzer.parar();
          alarmeDisparado = false;
          alarmeAtual = (alarmeAtual + 1) % 2; // Alterna entre alarme 0 e 1
          estadoAtual = MOSTRAR_HORA;
          break;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(150));
  }
}

void taskAlarme(void *pvParameters) {
  while (1) {
    if (estadoAtual == TOCANDO_ALARME) {
      if (alarmeAtual == 0) {
        buzzer.tocar(1000);  // Frequência do primeiro alarme
      } else {
        buzzer.tocar(1500);  // Frequência do segundo alarme
      }
    }
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void setup() {
  lcd.begin(16, 2);
  lcd.clear();

  lcdMutex = xSemaphoreCreateMutex();
  filaEstado = xQueueCreate(5, sizeof(EstadoSistema));

  xTaskCreate(taskRelogio, "Relogio", 128, NULL, 2, NULL);
  xTaskCreate(taskJoystick, "Joystick", 128, NULL, 2, NULL);
  xTaskCreate(taskInterface, "Interface", 256, NULL, 1, NULL);
  xTaskCreate(taskControle, "Controle", 256, NULL, 3, NULL);
  xTaskCreate(taskAlarme, "Alarme", 128, NULL, 1, NULL);
}

void loop() {}
