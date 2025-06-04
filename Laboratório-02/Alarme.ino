#include <LiquidCrystal.h>

enum EstadoSistema { MOSTRAR_HORA, AJUSTAR_HORA, AJUSTAR_ALARME, TOCANDO_ALARME };
EstadoSistema estadoAtual = MOSTRAR_HORA;

class Joystick {
  int pinX, pinY, pinBotao;
  int limiar = 300;
  unsigned long ultimaLeitura = 0;
  bool botaoPressionado = false;
  bool botaoAnterior = false;

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
  int minutos = 0, segundos = 0;
  int alarmeMinuto = 0, alarmeSegundo = 0;
  unsigned long ultimoSegundo = 0;

public:
  void update() {
    if (millis() - ultimoSegundo >= 1000) {
      ultimoSegundo = millis();
      segundos++;
      if (segundos >= 60) {
        segundos = 0;
        minutos = (minutos + 1) % 60;
      }
    }
  }

  void ajustarHora(String dir) {
    if (dir == "CIMA") minutos = (minutos + 1) % 60;
    if (dir == "BAIXO") minutos = (minutos + 59) % 60;
    if (dir == "DIREITA") segundos = (segundos + 1) % 60;
    if (dir == "ESQUERDA") segundos = (segundos + 59) % 60;
    delay(200);
  }

  void ajustarAlarme(String dir) {
    if (dir == "CIMA") alarmeMinuto = (alarmeMinuto + 1) % 60;
    if (dir == "BAIXO") alarmeMinuto = (alarmeMinuto + 59) % 60;
    if (dir == "DIREITA") alarmeSegundo = (alarmeSegundo + 1) % 60;
    if (dir == "ESQUERDA") alarmeSegundo = (alarmeSegundo + 59) % 60;
    delay(200);
  }

  bool horaIgualAlarme() {
    return (minutos == alarmeMinuto && segundos == alarmeSegundo);
  }

  String getHoraAtual() {
    return (minutos < 10 ? "0" : "") + String(minutos) + ":" + (segundos < 10 ? "0" : "") + String(segundos);
  }

  String getHoraAlarme() {
    return (alarmeMinuto < 10 ? "0" : "") + String(alarmeMinuto) + ":" + (alarmeSegundo < 10 ? "0" : "") + String(alarmeSegundo);
  }
};
/*
class Buzzer {
  int pino;
  bool ativo = false;
  unsigned long ultimaTroca = 0;
  bool estadoSom = false;

public:
  Buzzer(int p) : pino(p) {
    pinMode(pino, OUTPUT);
  }

  void tocar() {
    if (millis() - ultimaTroca > 500) {  // alterna a cada 0.5s
      ultimaTroca = millis();
      estadoSom = !estadoSom;
      if (estadoSom) {
        tone(pino, 1000); // frequência de 1000 Hz
      } else {
        noTone(pino);
      }
    }
    ativo = true;
  }

  void parar() {
    noTone(pino);
    ativo = false;
  }

  bool estaTocando() {
    return ativo;
  }
};
*/

#define NOTE_C6 1047
#define NOTE_E6 1319
#define NOTE_G6 1568
#define NOTE_A6 1760
#define NOTE_AS6 1865
#define NOTE_B6 1976
#define NOTE_C7 2093
#define NOTE_D7 2349
#define NOTE_E7 2637
#define NOTE_F7 2794
#define NOTE_G7 3136
#define NOTE_A7 3520

int melody[] = {
  NOTE_E7, NOTE_E7, 0, NOTE_E7,
  0, NOTE_C7, NOTE_E7, 0,
  NOTE_G7, 0, 0,  0,
  NOTE_G6, 0, 0, 0,

  NOTE_C7, 0, 0, NOTE_G6,
  0, 0, NOTE_E6, 0,
  0, NOTE_A6, 0, NOTE_B6,
  0, NOTE_AS6, NOTE_A6, 0,

  NOTE_G6, NOTE_E7, NOTE_G7,
  NOTE_A7, 0, NOTE_F7, NOTE_G7,
  0, NOTE_E7, 0, NOTE_C7,
  NOTE_D7, NOTE_B6, 0, 0
};

int noteDurations[] = {
  12, 12, 12, 12,
  12, 12, 12, 12,
  9, 12, 12, 12,
  12, 12, 12, 12,

  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12,

  9, 9, 9,
  12, 12, 12, 12,
  12, 12, 12, 12,
  12, 12, 12, 12
};

class Buzzer {
  int pino;
  bool ativo = false;
  int notaAtual = 0;
  unsigned long ultimaNota = 0;
  bool tocandoNota = false;

public:
  Buzzer(int p) : pino(p) {
    pinMode(pino, OUTPUT);
  }

  void tocar() {
    ativo = true;
    if (notaAtual >= sizeof(melody) / sizeof(int)) {
      notaAtual = 0;
      return;
    }

    unsigned long tempoAtual = millis();
    int duracaoNota = 1000 / noteDurations[notaAtual];

    if (!tocandoNota) {
      int nota = melody[notaAtual];
      if (nota != 0) tone(pino, nota, duracaoNota);
      ultimaNota = tempoAtual;
      tocandoNota = true;
    } else {
      if (tempoAtual - ultimaNota >= duracaoNota * 1.3) {
        notaAtual++;
        tocandoNota = false;
      }
    }
  }

  void parar() {
    noTone(pino);
    notaAtual = 0;
    tocandoNota = false;
    ativo = false;
  }

  bool estaTocando() {
    return ativo;
  }
};

LiquidCrystal lcd(7, 8, 9, 10, 11, 12);
Joystick joystick(A0, A1, 2);
Clock relogio;
Buzzer buzzer(3);

void limparLinha(int linha) {
  lcd.setCursor(0, linha);
  lcd.print("                "); 
}

void setup() {
  lcd.begin(16, 2);
  lcd.clear();
  delay(1000);
  lcd.clear();
}

void loop() {
  joystick.update();
  relogio.update();

  switch (estadoAtual) {
    case MOSTRAR_HORA:
      limparLinha(0);
      lcd.setCursor(0, 0);
      lcd.print("Hora: " + relogio.getHoraAtual());

      limparLinha(1);
      lcd.setCursor(0, 1);
      lcd.print("Alarme: " + relogio.getHoraAlarme());

      if (joystick.pressionado()) estadoAtual = AJUSTAR_HORA;
      if (relogio.horaIgualAlarme()) estadoAtual = TOCANDO_ALARME;
      break;

    case AJUSTAR_HORA:
      relogio.ajustarHora(joystick.getDirecao());

      limparLinha(0);
      lcd.setCursor(0, 0);
      lcd.print("Ajuste " + relogio.getHoraAtual());

      limparLinha(1);
      lcd.setCursor(0, 1);
      lcd.print("-> Alarme");

      if (joystick.pressionado()) estadoAtual = AJUSTAR_ALARME;
      break;

    case AJUSTAR_ALARME:
      relogio.ajustarAlarme(joystick.getDirecao());

      limparLinha(0);
      lcd.setCursor(0, 0);
      lcd.print("Alarme: " + relogio.getHoraAlarme());

      limparLinha(1);
      lcd.setCursor(0, 1);
      lcd.print("-> Voltar");

      if (joystick.pressionado()) estadoAtual = MOSTRAR_HORA;
      break;

    case TOCANDO_ALARME:
      buzzer.tocar();

      limparLinha(0);
      lcd.setCursor(0, 0);
      lcd.print("** TOCANDO **");

      limparLinha(1);
      lcd.setCursor(0, 1);
      lcd.print("Botao -> Parar");

      if (joystick.pressionado()) {
        buzzer.parar();
        estadoAtual = MOSTRAR_HORA;
      }
      break;
  }
}
