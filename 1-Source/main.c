#include <hidef.h> /* for EnableInterrupts macro */
#include "derivative.h" /* include peripheral declarations */

#define PushButton 	PTCD_PTCD3
#define AnInput		PTCD_PTCD2
#define LedOut		PTAD_PTAD1

#define _PushButton	PTCDD_PTCDD3
#define _AnInput	PTCDD_PTCDD2
#define _LedOut		PTADD_PTADD1

#define INPUT 0
#define OUTPUT 1

#define ON 1
#define OFF 0

#define PUSH 1
#define RELEASE 0

#define ALTO 1
#define BAJO 0

#define SUELTO 0
#define DETECCION 1

#define REBOTE 10

#define NO_LIGTH 0
#define LIGTH	 1


#define MODULO      1000
#define PRESCALER   0x03    // Prescaler => /8 
#define DUTY_MIN    100
#define DUTY_MAX    900
#define DUTY_START    50

void Init(void);
void CLCK_init(void);
void GPIO_Init(void);
void TPM1_init(void);
void TPM1_End(void);
void PWM_init(void);
void PWM_End(void);
void ADC_init(void);
void ADC_End(void);

unsigned char Debounce(void);

unsigned int conv_adc;

void main(void) {
	unsigned char state = NO_LIGTH;
	unsigned char button = RELEASE;
	unsigned int duty = DUTY_START;
	Init();
	EnableInterrupts
	;

	for (;;) {
		__RESET_WATCHDOG(); /* feeds the dog */

		//button = Debounce();

		switch (state) {
		case NO_LIGTH:
			if ((button = Debounce()) == 1) {
				state = LIGTH;
				TPM1_init();
				PWM_init();
				ADC_init();
			}
			break;
		case LIGTH:
			if (conv_adc > duty + 15 || conv_adc < duty - 15) {
				duty = conv_adc;
				TPM2C0V = duty;
			}
			if ((button = Debounce()) == 1) {
				state = NO_LIGTH;
				TPM1_End();
				PWM_End();
				ADC_End();
			}
			break;
		}

	}

}

void Init() {
	CLCK_init();
	GPIO_Init();

	PTBDD = 0x01;

}

void CLCK_init(void) {
	asm {
		LDA $FFAF;-- inicializa osc. interno
		STA ICSTRM
	}
	while (!ICSSC_IREFST)
		;
}

void GPIO_Init() {
	PTADD &= 0x00;
	PTBDD &= 0x00;
	PTCDD &= 0x00;

	PTAD &= 0x00;
	PTBD &= 0x00;
	PTCD &= 0x00;

	_PushButton = INPUT;
	_AnInput = INPUT;
	_LedOut = OUTPUT;
}

/* ---------------------------------------------- configura timer 1000ms --- */
void TPM1_init() {
	//TPM1MOD = 15625;
	TPM1MOD = 62499;
	TPM1SC = 0b01001111; // TOIE=1, TCKSRC = BUS DIV=128

	//TPM1SC_TOIE = 0;  // Deshabilita interrupciones por overflow
}

void TPM1_End() {
	TPM1SC = 0;
}

void PWM_init() {
	TPM2SC = 0;
	TPM2MOD = (MODULO - 1); // módulo: => período del PWM 
	TPM2SC_PS = PRESCALER;

	TPM2C0SC = 0b00101000;    // PWM => MSBA = 10 ELBA = 10
	TPM2C0V = DUTY_START;

	TPM2SC_CLKSA = 1;   // start TPM  

	//TPM2SC_TOIE = 1;  // habilita interrupciones por overflow
}

void PWM_End() {
	TPM2SC = 0;
	TPM2MOD = 0; // módulo: => período del PWM 
	TPM2SC_PS = 0;

	TPM2C0SC = 0x00;    // PWM => MSBA = 10 ELBA = 10
	TPM2C0V = 0;

	TPM2SC_CLKSA = 0;   // start TPM  

	//TPM2SC_TOIE = 1;  // habilita interrupciones por overflow
}

void ADC_init() {
	ADCCFG = 0b01000000; /* fadc clock: fbus / 4 = 2MHz */
	ADCSC2 = 0;
	ADCSC1 = 0x4A; /* Entrada para ADC: AD10 (pata 10 del SH8) e INterrupcion*/
	APCTL2 = 0b00000100; /* Entrada para ADC: AD10 (pata 10 del SH8) */
}

void ADC_End() {
	ADCCFG = 0; /* fadc clock: fbus / 4 = 2MHz */
}

unsigned char Debounce() {
	static unsigned char codAct;
	static unsigned char codAnt = ALTO;
	static unsigned char s = SUELTO;
	static unsigned char cont = REBOTE;

	codAct = PushButton;   //Leo entrada

	switch (s) {
	case SUELTO:
		if (codAct == BAJO) {
			s = DETECCION;
			codAnt = codAct;
		}
		return (0);
		break;
		
	case DETECCION:
		if (cont == 0) {
			s = SUELTO;
			cont = REBOTE;
			codAnt = ALTO;
			if (codAct == ALTO) {
				return (1);
			}
		}
		if (codAct == codAnt) {
			cont--;
		}
		else
			codAnt = codAct;
		return (0);
		break;
	}
}

/* ------------------------------------------------------ TPM1 TOF INT --- */
__interrupt 11 void tim_ovf(void) {
	TPM1SC_TOF = 0;
	//PTBD_PTBD0 ^= 0x01;

}

__interrupt 23 void vadc(void) {
	if (ADCSC1_COCO) {
		conv_adc = 10L * (((99 * ADCRL) / 255) + 1);
		ADCSC1 = 0x4A; /* Entrada para ADC: AD10 (pata 10 del SH8) e INterrupcion*/
	}

}
