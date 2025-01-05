#include <Arduino.h>


#define FADER1 A0
#define FADER2 A1
#define FADER3 A2
#define FADER4 A3
#define FADER5 A4


bool last_button_state[30];
int last_button_time[30];

/* 
  UART-Init: 
Berechnung des Wertes für das Baudratenregister 
aus Taktrate und gewünschter Baudrate
*/

#ifndef F_CPU
#define F_CPU 16000000UL  // Systemtakt in Hz - Definition als unsigned long beachten 
                         // Ohne ergeben sich unten Fehler in der Berechnung
#endif

/// CONFIG
#define BAUD 19200UL      // Baudrate
#define CHANNEL 1      // midi channel
#define OFFSET 60      // first midi note 

// Berechnungen
#define UBRR_VAL ((F_CPU+BAUD*8)/(BAUD*16)-1)   // clever runden
#define BAUD_REAL (F_CPU/(16*(UBRR_VAL+1)))     // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL*1000)/BAUD) // Fehler in Promille, 1000 = kein Fehler.

#if ((BAUD_ERROR<990) || (BAUD_ERROR>1010))
  #error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch! 
#endif

void setup() {
  for (int i = 0; i < 30; i++) {
    last_button_state[i] = 0;
    last_button_time[i] = 0;
  }

  DDRB &= 0b11111000;  // PORT_A contains matrix col 1-3
  DDRB |= 0b00011000;  // PORT_A contains matrix rows 1-2
  DDRD |= 0b11111100;  // PORT_B contains matrix rows 3-8

  PORTB &= ~(1 << PB0);
  PORTB &= ~(1 << PB1);
  PORTB &= ~(1 << PB2);

  //pinMode(12, OUTPUT);  // 1
  //pinMode(11, OUTPUT);  // 2
  //pinMode(2, OUTPUT);   // 3
  //pinMode(3, OUTPUT);   // 4
  //pinMode(4, OUTPUT);   // 5
  //pinMode(5, OUTPUT);   // 6
  //pinMode(6, OUTPUT);   // 7
  //pinMode(7, OUTPUT);   // 8
  //pinMode(8, INPUT);    // A
  //pinMode(9, INPUT);    // B
  //pinMode(10, INPUT);   // V

  // init uart
  UBRR0 = UBRR_VAL;
  UCSR0B |= (1<<TXEN0);
  UCSR0B |= (1<<RXEN0);
  // Frame Format: Asynchron 8N1
  UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
  UCSR0B |= (1<<RXCIE0);
  sei();
}


int uart_putc(unsigned char c)
{
    while (!(UCSR0A & (1<<UDRE0)))  /* warten bis Senden moeglich */
    {
    }                             

    UDR0 = c;                      /* sende Zeichen */
    return 0;
}

// first visible ASCIIcharacter '!' is number 33:
int thisByte = 33;
// you can also write ASCII characters in single quotes.
// for example, '!' is the same as 33, so you could also use this:
// int thisByte = '!';

uint8_t noteON  = B10010000;
uint8_t noteOFF = B10000000;

int last_fader[5] = { 0, 0, 0, 0, 0 };
int pins_fader[5] = { FADER1, FADER2, FADER3, FADER4, FADER5 };

char buttons[3] = { 'A', 'B', 'C' };
char in_msg[63];  //ringbuffer for incomming midi

uint16_t in_index_last = 0;
uint16_t in_index_top = 0;

long int lastin = 0;

int in_last = 0;

void loop() {
  processAnalog();
  processQueue();
  processMatrix();
  processQueue();

}

uint8_t start_ch = OFFSET;

void midi_put_buffer(char data){
  //clear buffer if last tx is 1 sec ago
  long int now = millis();
  if (now - lastin > 1000){
    in_index_top = 0;
    in_index_last = 0;
  }
  lastin = now;

  in_msg[in_index_top] = data;
  in_index_top++;
  if ( in_index_top >= 63){
    in_index_top = 0;
  }
}


void midi_send_buffer_single(){
  uint16_t diff = 0;
  if (in_index_top < in_index_last){
    diff = (63 + in_index_top) - in_index_last;
  }else{
    diff = in_index_top - in_index_last;
  }
  if (diff > 2){
    uart_putc(in_msg[in_index_last + 0]);
    uart_putc(in_msg[in_index_last + 1]);
    uart_putc(in_msg[in_index_last + 2]);
    in_index_last += 3;
    if (in_index_last >= 63){
      in_index_last -= 63;
    }
  }
}


ISR(USART_RX_vect)
{
  unsigned char b;
  b=UDR0;
  midi_put_buffer(b);
}

void processQueue(){
  midi_send_buffer_single();
}

void processAnalog() {
  for (int i = 0; i < 5; i++) {
    int val = analogRead(pins_fader[i]);
    if (val - last_fader[i] > 2 || val - last_fader[i] < -2) {
      sendMessageNote(1, 1 + i, map(val, 1, 1022, 127, 0));
      last_fader[i] = val;
    }else if ((val < 2 && last_fader[i] >= 2) || (val > 1022 && last_fader[i] <= 1022)){
      sendMessageNote(1, 1 + i, map(val, 1, 1022, 127, 0));
      last_fader[i] = val;
    }
  }
}

void processMatrix() {
  for (int i = 0; i < 8; i++) {
    int baseId = i * 3;
    uint8_t data = putMatrixCol(i);
    bool state1 = ((data & 1) > 0);
    bool state2 = ((data & 2) > 0);
    bool state3 = ((data & 4) > 0);


    //Serial.println(baseId);
    if (state1 != last_button_state[baseId + 0]) {
      last_button_state[baseId + 0] = state1;
      sendMessageNote(1, 6 + baseId + 0, state1 ? 127:0);
    }

    if (state2 != last_button_state[baseId + 1]) {
      last_button_state[baseId + 1] = state2;
      sendMessageNote(1, 6 + baseId + 1,  state2 ? 127:0);
    }

    if (state3 != last_button_state[baseId + 2]) {
      last_button_state[baseId + 2] = state3;
      sendMessageNote(1, 6 + baseId + 2,  state3 ? 127:0);
    }
  }
}

uint8_t putMatrixCol(uint8_t col) {
  uint8_t fullByte = (1 << col);
  digitalWrite(12, (col == 0));
  digitalWrite(11, (col == 1));
  digitalWrite(2, (col == 2));
  digitalWrite(3, (col == 3));
  digitalWrite(4, (col == 4));
  digitalWrite(5, (col == 5));
  digitalWrite(6, (col == 6));
  digitalWrite(7, (col == 7));
  return (PINB & 0b00000111);
}

void sendMessageNote(uint8_t channel, uint8_t note, int velocity) {
  uint8_t statusByte = (velocity > 0 ? noteON : noteOFF) & 0xF0;
  statusByte |= channel & 0x0F;

  uart_putc(statusByte);
  uart_putc(start_ch + note);
  uart_putc(velocity);

}
