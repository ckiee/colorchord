//Copyright 2015-2022 <>< Charles Lohr, ckie under the ColorChord License.

#include "outdrivers.h"
#include "notefinder.h"
#include <stdio.h>
#include "parameters.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "color.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include "CNFG.h"

// note_amplitudes2[note] for how many lights to use.
// note_amplitudes_out[note] for how bright it should be.

struct DPODriver
{
	int serial_port;
};

typedef struct DPONote {
	bool valid;
	float freq;
	float amp;
	float mult;
	/* bool in_key; */
} DPONote;


static void DPOUpdate(void * id, struct NoteFinder*nf)
{
	int x,y;
	struct DPODriver * d = (struct DPODriver*)id;
	static DPONote notes[64];
	static size_t note_count;

	float notes_x = 0, notes_y = 0;
	for (int i = 0; i < nf->freqbins; i++) {
		if (nf->note_amplitudes2[i]<0.01) continue;
		DPONote null = {false};
		DPONote *note = &null; // Don't ask. C is weird.
		// find a matching note, if any
		for (int j = 0; j < note_count; j++) {
			/* printf("fdifff(%f,%f) : %f\n",nf->frequencies[i],notes[j].freq, fabsf(nf->frequencies[i] - notes[j].freq)); */
			if (fabsf(nf->frequencies[i] - notes[j].freq) < 1.0) {
				/* printf("%x(note) = &notes[%d]\n", note, j); */
				note = &notes[j];
				note->valid = true;
				break;
			}
		}
		if (!note->valid) {
			// A new note
			note = &notes[++note_count];
			note->mult = 1.0;
			note_count%=64;
		}
		note->freq = nf->frequencies[i];
		float smoothing = 0.1;
		note->amp = nf->note_amplitudes2[i] * (1-smoothing) + note->amp * smoothing;
		note->mult = 1.0;
		note->valid = true;
		printf("Identify %d#(Freq %f : %f, Amplit %f)\n", i, nf->frequencies[i], note->freq, note->amp);
	}
	// Group notes into two bins. Kinda music theoryish, but idk music theory.
	for (int i = 0; i < note_count; i++) {
		DPONote *note = &notes[i];
		if (!note || !note->valid) continue;

		char *names[12] = {"F\0\0","F#\0","G\0\0","G#\0","A\0\0","A#\0","B\0\0","C\0\0","C#\0","D\0\0","D#\0","E\0\0"};
		float pabs_note = (12.0*(log2f(note->freq/440.0)+49.0))/12-2;
		int pnote = abs((int)(12.0*(log2f(note->freq/440.0)+49.0))%12);
		printf("Render %d#(%s %f, Amplit %f, Mult %f)\n", i, names[pnote], note->freq, note->amp, note->mult);
		/* notes[i].multiplier -= 0.7 / nf->sps_rec; */
		/* printf("CALCfmr\t\t\t= %d (%f)\n",); */
		// lol.. loll.. aaaaa ow
		int key[12] = {1,1,0,1,0,1,0,1,1,0,1,0};
		float mult_slope = 1.2;
		float final = note->amp*100*(mult_slope/note->mult);
		note->mult -= 10.0/63.0;
		if (note->mult<0.01)note->valid=false;
		if (key[pnote]) notes_x+=final;
		else notes_y+=final;
	}

	uint16_t l_ww = 500 + notes_x;
	uint16_t l_cw = 500 + notes_y;
	uint16_t serial_cmd_buf[3] = {/*P*/(l_ww>l_cw?l_ww:l_cw),/*WW*/l_ww,/*CW*/l_cw};
	printf("%6d\t%6d\t%6d\n", serial_cmd_buf[0], serial_cmd_buf[1], serial_cmd_buf[2]);
	write(d->serial_port, serial_cmd_buf, 3*2);

}

	/* static const size_t ringbuf_size = 512; */
	/* static uint16_t intensity_ringbuf[512]; // prev frames for smoothing */
	/* static int ringbuf_pos = 0; */
	/* uint16_t avg_prev_intensity; */
	/* for (int i = ringbuf_pos; i >= 0; i--) { */
	/* 	avg_prev_intensity += intensity_ringbuf[i]; */
	/* } */
	/* avg_prev_intensity /= ringbuf_pos + 1; */
	/* // Quantize for less eye pain */
	/* avg_prev_intensity = avg_prev_intensity / 1000 * 1000; */
	/* if (ringbuf_pos>=ringbuf_size) { */
	/* 	ringbuf_pos =0; */
	/* 	printf("Wrap!\n"); */
	/* } */
	/* intensity_ringbuf[++ringbuf_pos] = smoothed; */
static void DPOParams(void * id )
{
	struct DPODriver * d = (struct DPODriver*)id;

	/* d->xn = 16;		RegisterValue(  "lightx", PAINT, &d->xn, sizeof( d->xn ) ); */
	/* d->yn = 9;		RegisterValue(  "lighty", PAINT, &d->yn, sizeof( d->yn ) ); */
	/* d->zigzag = 0;	RegisterValue(  "zigzag", PAINT, &d->zigzag, sizeof( d->zigzag ) ); */
	/* d->rot90 = 0;	RegisterValue(  "rot90", PAINT, &d->rot90, sizeof( d->rot90 ) ); */
	if (system("picocom -qb 115200 -X /dev/ttyUSB1")) perror("set the serial port up");
	d->serial_port = open("/dev/ttyUSB1", O_RDWR|O_NOCTTY|O_SYNC);
	if (!d->serial_port) perror("===== serial port open error");
}

static struct DriverInstances * DisplaySerial(const char * parameters)
{
	struct DriverInstances * ret = malloc( sizeof( struct DriverInstances ) );
	struct DPODriver * d = ret->id = malloc( sizeof( struct DPODriver ) );
	memset( d, 0, sizeof( struct DPODriver ) );
	ret->Func = DPOUpdate;
	ret->Params = DPOParams;
	DPOParams( d );
	return ret;
}

REGISTER_OUT_DRIVER(DisplaySerial);
