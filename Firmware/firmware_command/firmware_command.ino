#include <Arduino.h>


#define FADER1 A0

#define SR_DATA A5  //19
#define SR_CLOCK 5
#define SR_LATCH A4  //18
#define MTX_I 2      //18

bool last_button_state[80];


/* 
  UART-Init: 
Berechnung des Wertes für das Baudratenregister 
aus Taktrate und gewünschter Baudrate
*/

#ifndef F_CPU
#define F_CPU 16000000UL  
#endif

/// CONFIG
#define BAUD 19200UL  // Baudrate
#define CHANNEL 4     // midi channel
#define OFFSET 0      // first midi note

// Berechnungen
#define UBRR_VAL ((F_CPU + BAUD * 8) / (BAUD * 16) - 1)  // clever runden
#define BAUD_REAL (F_CPU / (16 * (UBRR_VAL + 1)))        // Reale Baudrate
#define BAUD_ERROR ((BAUD_REAL * 1000) / BAUD)           // Fehler in Promille, 1000 = kein Fehler.

#if ((BAUD_ERROR < 990) || (BAUD_ERROR > 1010))
#error Systematischer Fehler der Baudrate grösser 1% und damit zu hoch!
#endif

void setup() {
  // prints title with ending line break
  for (int i = 0; i < 80; i++) {
    last_button_state[i] = 0;
  }

  //DDRB |= 0b00111111;  // PORT_A contains matrix rows 1-2
  //DDRD |= 0b11000000;  // PORT_B contains matrix rows 3-8

  pinMode(SR_DATA, OUTPUT);
  pinMode(SR_CLOCK, OUTPUT);
  pinMode(SR_LATCH, OUTPUT);
  pinMode(MTX_I, OUTPUT);

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
  UCSR0B |= (1 << TXEN0);
  UCSR0B |= (1 << RXEN0);
  // Frame Format: Asynchron 8N1
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
  UCSR0B |= (1 << RXCIE0);
  sei();
}


int uart_putc(unsigned char c) {
  while (!(UCSR0A & (1 << UDRE0))) /* warten bis Senden moeglich */
  {
  }

  UDR0 = c; /* sende Zeichen */
  return 0;
}

// first visible ASCIIcharacter '!' is number 33:
int thisByte = 33;
// you can also write ASCII characters in single quotes.
// for example, '!' is the same as 33, so you could also use this:
// int thisByte = '!';

uint8_t noteON = B10010000;
uint8_t noteOFF = B10000000;

int last_fader[1] = { 0 };
int pins_fader[1] = { FADER1 };

char buttons[3] = { 'A', 'B', 'C' };
char in_msg[63];  //ringbuffer for incomming midi

uint16_t in_index_last = 0;
uint16_t in_index_top = 0;

long int lastin = 0;
int in_last = 0;

void loop() {
  processAnalog();
  //processQueue();
  processMatrix();
  //processQueue();
}

uint8_t start_ch = OFFSET;

void midi_put_buffer(char data) {
  //clear buffer if last tx is 1 sec ago
  long int now = millis();
  if (now - lastin > 1000) {
    in_index_top = 0;
    in_index_last = 0;
  }
  lastin = now;

  in_msg[in_index_top] = data;
  in_index_top++;
  if (in_index_top >= 63) {
    in_index_top = 0;
  }
}


void midi_send_buffer_single() {
  uint16_t diff = 0;
  if (in_index_top < in_index_last) {
    diff = (63 + in_index_top) - in_index_last;
  } else {
    diff = in_index_top - in_index_last;
  }
  if (diff > 2) {
    uart_putc(in_msg[in_index_last + 0]);
    uart_putc(in_msg[in_index_last + 1]);
    uart_putc(in_msg[in_index_last + 2]);
    in_index_last += 3;
    if (in_index_last >= 63) {
      in_index_last -= 63;
    }
  }
}


ISR(USART_RX_vect) {
  unsigned char b;
  b = UDR0;
  midi_put_buffer(b);
}

void processQueue() {
  midi_send_buffer_single();
}

void processAnalog() {
  for (int i = 0; i < 1; i++) {
    int val = analogRead(pins_fader[i]);
    if (val - last_fader[i] > 2 || val - last_fader[i] < -2) {
      sendMessageNote(CHANNEL, 1 + i, map(val, 1, 1022, 127, 0));
      last_fader[i] = val;
    } else if ((val < 2 && last_fader[i] > 2) || (val > 1022 && last_fader[i] < 1022)) {
      sendMessageNote(CHANNEL, 1 + i, map(val, 1, 1022, 127, 0));
      last_fader[i] = val;
    }
  }
}

void srClock(uint8_t bit) {
  digitalWrite(SR_DATA, bit);
  digitalWrite(SR_CLOCK, 1);
  digitalWrite(SR_CLOCK, 0);
  digitalWrite(SR_LATCH, 1);
  digitalWrite(SR_LATCH, 0);
}

void processMatrix() {
  srClock(1);  //shift 1st byte
  for (int i = 0; i < 9; i++) {
    if (i == 8) {
      digitalWrite(MTX_I, HIGH);
    } else {
      digitalWrite(MTX_I, LOW);
    }

    int baseId = i * 8;
    uint8_t data = getMatrix();

    for (int b = 0; b < 8; b++) {
      bool state = ((data & _BV(b)) > 0);
      if (state != last_button_state[baseId + b]) {
        last_button_state[baseId + b] = state;
        sendMessageNote(CHANNEL, 2 + baseId + b, state ? 127 : 0);
      }
    }

    //shift 0 byte
    srClock(0);
  }
}

uint8_t getMatrix() {
  uint8_t val1 = PINB & 0b00111111;
  uint8_t val2 = PIND & 0b11000000;
  return (val1 | val2);
}

void sendMessageNote(uint8_t channel, uint8_t note, int velocity) {
  uint8_t statusByte = (velocity > 0 ? noteON : noteOFF) & 0xF0;
  statusByte |= channel & 0x0F;

  uart_putc(statusByte);
  uart_putc(start_ch + note);
  uart_putc(velocity);
}
