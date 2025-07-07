# Sistemas Embarcados

Este repositório contém as práticas e os códigos desenvolvidos na disciplina de Sistemas Embarcados, ministrada pelo Prof. Dr. Marcelo de Oliveira. Todos os códigos foram desenvolvidos e testados utilizando a plataforma Arduino.

## Laboratórios

- **Laboratório-01**: implementação de um sistema de comunicação serial bidirecional entre duas placas Arduino Uno, utilizando interrupções de hardware acionadas por timers de alta precisão por meio da biblioteca TimerInterrupt.h. O firmware configura duas interfaces UART a 115200 bps, captura automaticamente strings de entrada via USB e gerencia o envio e a recepção de mensagens através de rotinas de serviço de interrupção. O uso de callbacks garante latência determinística e separação clara entre aquisição de dados e processamento, validados pelo monitor serial. .
- **Laboratório-02**: implementação de um sistema de alarme baseado em Arduino utilizando máquina de estados. O sistema permite ajuste do horário atual e do horário do alarme via joystick, exibindo-os em um display LCD. Um buzzer é acionado quando o horário atual coincide com o alarme. Máquinas de estado garantem modularidade e robustez no projeto, especialmente no tratamento das entradas e do controle lógico..
- **Laboratório-03**: implementação de um sistema de alarme baseado em Arduino utilizando máquina de estados. O sistema permite ajuste do horário atual e do alarme via joystick, exibindo as informações em um display LCD e acionando um buzzer quando os horários coincidem. A lógica é organizada em múltiplas tarefas executadas pelo FreeRTOS, com uso de filas e semáforos para comunicação e sincronização. O alarme alterna entre dois modos sonoros, garantindo um funcionamento eficiente, modular e adequado a aplicações embarcadas.

### Autores

- Leandro M. Tosta
- [Eiti P. Adama](https://github.com/eitip)
