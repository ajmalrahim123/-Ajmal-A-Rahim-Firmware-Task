#include <avr/io.h>				//Hardware used: Arduino UNO
#include <avr/interrupt.h>

#define F_CPU 16000000UL  // Define clock speed
#define BAUD_RATE 2400

unsigned long startTime = 0;
int charCount = 0;
const int targetCharCount = 1000;
unsigned long  speedOfdata;

/********************************************************UART driver functions********************************************************/

// Initialize UART
void uart_init(uint16_t baudrate) {
    uint16_t ubrr_value = (F_CPU / (16 * baudrate)) - 1;
    UBRR0H = (ubrr_value >> 8);
    UBRR0L = ubrr_value;
    UCSR0B = (1 << RXEN0) | (1 << TXEN0); // Enable receiver and transmitter
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00); // 8-bit data format
}

// Transmit a single character over UART
void uart_transmit(char data) {
    while (!(UCSR0A & (1 << UDRE0)));  // Wait for empty transmit buffer
    UDR0 = data;
}

// Receive a single character from UART
char uart_receive(void) {
    while (!(UCSR0A & (1 << RXC0)));  // Wait for data to be received
    return UDR0;
}

// Print a string over UART
void uart_print(const char* str) {
    while (*str) {
        uart_transmit(*str++);
    }
}

// Print a number with commas over UART
void uart_print_num_with_commas(unsigned long num) {
    char buffer[15]; // Buffer to store formatted number
    int len = 0;
    
    do {
        if (len > 0 && len % 3 == 0) {
            buffer[sizeof(buffer) - len++ - 1] = ','; // Insert comma every 3 digits
        }
        buffer[sizeof(buffer) - len++ - 1] = '0' + (num % 10);
        num /= 10;
    } while (num > 0);
    
    uart_print(&buffer[sizeof(buffer) - len]); // Print formatted number
}


/********************************************************EEPROM related functions********************************************************/

// Write a byte to EEPROM
void eeprom_write(unsigned int address, char data) {
    while (EECR & (1 << EEPE));  // Wait for completion of previous write
    EEAR = address;  // Set up address register
    EEDR = data;     // Set up data register
    EECR |= (1 << EEMPE);  // Enable master write
    EECR |= (1 << EEPE);   // Start EEPROM write
}

// Read a byte from EEPROM
char eeprom_read(unsigned int address) {
    while (EECR & (1 << EEPE));  // Wait for completion of previous write
    EEAR = address;  // Set up address register
    EECR |= (1 << EERE);  // Start EEPROM read
    return EEDR;  // Return data from data register
}

/******************************************************** TIMER related functions ********************************************************/

// Initialize Timer1 for millis calculation
void timer1_init() {
    TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10); // CTC mode, prescaler 1024
    OCR1A = 15624; // 1ms at 16MHz with 1024 prescaler
    TCNT1 = 0;
    TIMSK1 |= (1 << OCIE1A); // Enable compare interrupt
    sei(); // Enable global interrupts
}

volatile unsigned long timer1_millis = 0;

// ISR to increment millis counter
ISR(TIMER1_COMPA_vect) {
    timer1_millis++;
}

// Millis function for time tracking
unsigned long millis() {
    unsigned long millis_copy;
    cli();
    millis_copy = timer1_millis;
    sei();
    return millis_copy;
}

/******************************************************** Initialization ********************************************************/

void setup() {
    uart_init(BAUD_RATE);
    timer1_init();
    uart_print("Ready to receive 1000 characters...\n");
}

/******************************************************** Recursive function ********************************************************/

void loop() {
    // Receive characters and store them in EEPROM
    if (UCSR0A & (1 << RXC0)) {
        if (charCount == 0) {
            startTime = millis();
        }

        char receivedChar = uart_receive();
        eeprom_write(charCount, receivedChar); // Store character in EEPROM
        charCount++;

        if (charCount >= targetCharCount) {
            unsigned long elapsedTime = millis() - startTime;
         
            speedOfdata=elapsedTime;
            uart_print("Time to receive 1000 characters: ");
             uart_print_num_with_commas(speedOfdata);
            uart_print(" milliseconds\n");

            uart_print("Data speed: ");
            uart_print_num_with_commas(speedOfdata);
            

            uart_print("\nRetransmitting stored data:\n");
            for (int i = 0; i < targetCharCount; i++) {
                char storedChar = eeprom_read(i);
                uart_transmit(storedChar);
            }
            uart_print("\nRetransmission complete.\n");

            charCount = 0;
            uart_print("Ready to receive 1000 characters...\n");
        }
    }
}

/******************************************************** Main function ********************************************************/

int main(void) {
    setup();
    while (1) {
        loop();
    }
    return 0;
}