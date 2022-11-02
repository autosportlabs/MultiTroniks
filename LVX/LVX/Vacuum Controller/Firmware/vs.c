/*    Vaccuum Switch Controller    							*/
/*    Revision 1.0 5/5/98          							*/
/*    Revision 1.1 6/3/98       In function 'mode' the led
/*		will only light when it sees 6 complete transistions not 5  */
/*	  Revision 1.2 2/18/99		Made more sensitive for small parts	*/
 

#include <16C71.H>

			//application specific definitions
#define min_vac 20
#define vac_solonoid PIN_B0		//input pc calling for vacuum
#define vacuum_sense PIN_B1		//output component confirmation to pc
#define vacuum_sol_drive PIN_B2		//output to turn vac sol on
#define vac_generator PIN_B6		//output to turn vac gen on

			//global definitions
int baseline,time;



#fuses XT,NOPROTECT,NOWDT
#use DELAY(clock=4000000)
#use Standard_IO(A)
#use Standard_IO(B)


void INITIALIZE()			//startup
{
	//set_tris_a(0x0F);		//=b'1111'
	//set_tris_b(0xFF);		//=b'11111001'
	setup_port_a(RA0_RA1_ANALOG);	//set up a2d
	setup_adc(ADC_CLOCK_DIV_2);
	set_adc_channel(0);
	baseline = 100;
	time = 32;			//each 16 is one second delay
	output_high(vacuum_sense);
	delay_ms(400);
	output_low(vacuum_sense);
	delay_ms(400);
	output_high(vacuum_sense);
	delay_ms(400);
	output_low(vacuum_sense);
	delay_ms(400);
	output_high(vacuum_sense);
	delay_ms(400);
	output_low(vacuum_sense);
}


void SAMPLING()
{	int sample;

	output_high(vac_generator);
	do
	{	delay_us(400);				//400sets sample rate at 2500Hz
		sample = read_adc()/2;			//takes average of two samples
		delay_us(400);
		sample = sample + read_adc()/2;
		if (sample < baseline) output_low(vacuum_sense);
		else output_high(vacuum_sense);
	} while (!input(vac_solonoid));			//is solinoid on?
	if (sample > baseline) 
	{	
 		do
		{delay_us(400);				//sets sample rate at 2500Hz
		sample = read_adc()/2;			//takes average of two samples
		delay_us(400);
		sample = sample + read_adc()/2;
		} while (sample > min_vac);		//is vacuum low?
 		output_low(vacuum_sense);
	}
	setup_counters(rtcc_internal,rtcc_div_256);	//sets up 2 second delay for generator
	set_rtcc(1);
	time = 0;
	return;
}


void READ_BASELINE()
{	int x;
	long temp;

	output_high(vac_generator);
	output_high(vacuum_sol_drive);
	delay_ms(400);							//rev 1.2... was 300ms
	baseline = 0;
	temp = 0;
	for (x = 0;x != 8;x++){
		delay_ms(2);
		temp = temp + read_adc();
		}
	temp = temp / 8;				//take average of 8 samples
	baseline = temp + (211 - temp)/8;		//rev 1.2... baseline1 = baseline0 +.125*(maxvac-baseline0)
	output_low(vacuum_sense);				//was baseline1 = 4 + baseline0 +.125*(maxvac-baseline0)
	output_low(vac_generator);
	output_low(vacuum_sol_drive);
	return;
}


void MODE()						//setup baseline or normal sample mode?
{	int test,logic;

	logic = 1;
	test = 1;
	setup_counters(rtcc_internal,rtcc_div_8);
	set_rtcc(68);
	do						//test to see 3 changes in state
	{	if (input(vac_solonoid) == logic)
		{	test = test + 1;
			if (logic == 0) logic = 1;
			else logic = 0;
		}
	} while (get_rtcc() > 67);			//check mode for 1.5ms
	if (test == 6) output_high(vacuum_sense);
	if (test >= 4) READ_BASELINE();			//if one or more toggle then goto setup
	else SAMPLING();
	return;
}

	
TIMEOFF()						//check to see if 2 seconds has elapsed	
{	if (time == 32) return;				//before the vacuum gen will be shut off
	else
	{	time++;
		set_rtcc(1);
		if (time == 32) output_low(vac_generator);
		return;
	}
}

MAIN()
{	INITIALIZE();
	do
	{
		if (!input(vac_solonoid)) MODE();
		if (get_rtcc() == 0) TIMEOFF();
	} while(true);	
}
