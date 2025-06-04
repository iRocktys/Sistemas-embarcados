#define USE_TIMER_1     true 

#include "TimerInterrupt.h"

#define TIMER_INTERVAL_US 100 

String texto = "";
String recebido = "";
bool prontoParaEnviar = false;
bool prontoParaMostrar = false;

void setup() {
  Serial.begin(115200);    
  Serial1.begin(115200);   

  ITimer1.init(); 

  if (ITimer1.attachInterruptInterval(TIMER_INTERVAL_US, timerCallback)) {
    Serial.println("Timer Funcionando");
  } else {
    Serial.println("Timer Erro");
  }
}

void loop() {

  if (Serial.available()) {
    char c = Serial.read();
    if (c == '\n') {
      prontoParaEnviar = true;
    } else {
      texto += c;
    }
  }

  if (prontoParaMostrar) {
    Serial.print("Recebido: ");
    Serial.println(recebido);
    recebido = "";
    prontoParaMostrar = false;
  }
/*
  if (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      prontoParaMostrar = true;
    } else {
      recebido += c;
    }
  }

  if (prontoParaEnviar) {
    Serial1.println(texto); 
    texto = "";
    prontoParaEnviar = false;
  }
*/
}

void timerCallback() {

  if (prontoParaEnviar) {
    Serial1.println(texto); 
    texto = "";
    prontoParaEnviar = false;
  }

  if (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n') {
      prontoParaMostrar = true;
    } else {
      recebido += c;
    }
  }
}
