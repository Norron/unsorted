#include <avr/io.h>
#include <avr/interrupt.h>
#define F_CPU		14745600UL
#include <util/delay.h>
#include <stdint.h>

static uint8_t bitcnt = 0;
static uint8_t byte;

static const uint16_t etu = 766UL;
static const uint16_t etu15 = 1149UL;

static volatile uint8_t rbuf[256];
static volatile uint8_t rb_fill = 0;
static uint8_t rb_put = 0;

// TODOs: F_CPU fast enough? ringbuffer large enough? testing!

ISR(TIMER1_COMPA_vect)
{
	//TCNT1 = 0;
	byte >>= 1;
	byte |= 0x80;
	if (bitcnt == 8) {
		TIMSK &= ~(1 << OCIE1A);
		//UDR = byte;
		rbuf[rb_put++] = byte;
		rb_fill++;
		bitcnt = 0;
		return;
	}
	OCR1A += etu;
	bitcnt++;
}

ISR(TIMER1_CAPT_vect)
{
	uint16_t val;

	//TCNT1 = 0;
	val = ICR1;

	TIMSK &= ~(1 << OCIE1A);

#if 1
		byte >>= 1;
		if (bitcnt == 8) {
			//UDR = byte;
			rbuf[rb_put++] = byte;
			rb_fill++;
			bitcnt = 0;
			return;
		}
		bitcnt++;
		val += etu15; // + etu/2;
		OCR1A = val; //etu + etu/2;
		TIFR = (1 << OCF1A);
		TIMSK |= (1 << OCIE1A);
#else

	if (TCCR1B & (1 << ICES1)) {
		// rising edge - add a zero
		byte >>= 1;
		if (bitcnt == 8) {
			//UDR = byte;
			rbuf[rb_put++] = byte;
			rb_fill++;
			bitcnt = 0;
			return;
		}
		bitcnt++;
	} else {
		// falling edge
		// setup COMPA - val is 3/16 of a bit
		//etu = (val * 16) / 3;
		val += etu + etu/2;
		OCR1A = val; //etu + etu/2;
		TIFR = (1 << OCF1A);
		TIMSK |= (1 << OCIE1A);
	}
	TCCR1B ^= (1 << ICES1);
#endif
}

// 16/3 = 5.3
// 1: 92.9us hi, total: 491.7us (5.29)

// bursts: 9.664 us hi, total: 52us (5.38)

// 14.7456 MHz eq 67.8ns -> max. time: 4.444444 ms


int main(void)
{
	uint8_t rb_get = 0;

	UBRRH = 0;
	UBRRL = 7;		// 115.2 kBaud

	UCSRB = (1 << TXEN);
	UCSRC = (1 << URSEL) | (3 << UCSZ0);	// 8 bit, 1 stopbit

	UDR = '#'; // test

	TCCR1A = 0;
	TCCR1B = (1 << ICNC1) | (1 << ICES1) | (1 << CS10);
	TIMSK = (1 << TICIE1);

	sei();

	while (1) {
		
		loop_until_bit_is_set(UCSRA, UDRE);
		cli();
		if (rb_fill) {
			rb_fill--;
			UDR = rbuf[rb_get++];
		}
		sei();
		
	}

	return 0;
}
