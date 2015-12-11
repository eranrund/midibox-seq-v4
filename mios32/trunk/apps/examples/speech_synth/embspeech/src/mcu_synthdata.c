/***************************************************************************
 *   mcu_synthdata.cpp                                                     *
 *   Copyright (C) 2007 by Nic Birsan & Ionut Tarsa                        *
 *                                                                         *
 *   -------------------------------------------------------------------   *
 *   reduced version of synthdata.cpp from eSpeak project                  *
 *   Copyright (C) 2005, 2006 by Jonathan Duddington                       *
 *   jsd@clara.co.uk                                                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

//#include "StdAfx.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
//#include <wctype.h>
#include <string.h>
/*
#include "speak_lib.h"
#include "speech.h"
#include "voice.h"
#include "phoneme.h"
#include "synthesize.h"
#include "translate.h"
*/

#include "mcu_otherdef.h"
#include "mcu_phoneme.h"
#include "mcu_synthesize.h"

extern const char *WordToString(unsigned int word);

#ifdef MCU_USE_MANY_TABLES

// copy the current phoneme table into here
int n_phoneme_tab;
MCU_PHONEME_TAB *MCU_phoneme_tab[MCU_N_PHONEME_TAB];


unsigned int *MCU_phoneme_index=NULL;
char *MCU_spects_data=NULL;
unsigned char *MCU_wavefile_data=NULL;
static unsigned char *MCU_phoneme_tab_data = NULL;

#else

#include "mcu_phontab.c"
#include "mcu_phondata.c"
#define		MCU_N_PHONEME_TAB	MCU_NUM_PHONEMES_EN
int MCU_n_phoneme_tab = MCU_N_PHONEME_TAB;

MCU_PHONEME_TAB* MCU_phoneme_tab = (MCU_PHONEME_TAB*) phoneme_en;


unsigned char * MCU_wavefile_data = (unsigned char *) phoneme_data;
char *MCU_spects_data = (char*) phoneme_data;

#endif

#ifdef MCU_USE_MANY_TABLES
int n_phoneme_tables;
PHONEME_TAB_LIST phoneme_tab_list[N_PHONEME_TABS];
static int phoneme_tab_number = 0;
#endif

int MCU_wavefile_ix;              // a wavefile to play along with the synthesis
int MCU_wavefile_amp;
int MCU_wavefile_ix2;
int MCU_wavefile_amp2;

static int seq_len_adjust;
int MCU_vowel_transition[4];
int MCU_vowel_transition0;
int MCU_vowel_transition1;

void MCU_FormantTransitions(MCU_frameref_t *seq, int* n_frames,
							MCU_PHONEME_TAB *this_ph, MCU_PHONEME_TAB *other_ph, int which);
int MCU_FormantTransition2(MCU_frameref_t *seq, int* n_frames, unsigned int data1,
						   unsigned int data2, MCU_PHONEME_TAB *other_ph, int which);

#ifdef MCU_USE_MANY_TABLES
const char *PhonemeTabName(void)
{//=============================
	return(phoneme_tab_list[phoneme_tab_number].name);
}


static int ReadPhFile(char **ptr, const char *fname)
{//=================================================
	FILE *f_in;
	char *p;
	unsigned int  length;
	char buf[200];

	sprintf(buf,"%s%c%s",path_home,PATHSEP,fname);
	length = GetFileLength(buf);
	
	if((f_in = fopen(buf,"rb")) == NULL)
	{
		fprintf(stderr,"Can't read data file: '%s'\n",buf);
		return(1);
	}

	if(*ptr != NULL)
		Free(*ptr);
		
	if((p = Alloc(length)) == NULL)
	{
		fclose(f_in);
		return(-1);
	}
	if(fread(p,1,length,f_in) != length)
	{
		fclose(f_in);
		return(-1);
	}
	*ptr = p;
	fclose(f_in);
	return(0);
}  //  end of ReadPhFile


int LoadPhData()
{//=============
	int ix;
	int n_phonemes;
	unsigned char *p;

	if(ReadPhFile((char **)(&phoneme_tab_data),"phontab") != 0)
		return(-1);
	if(ReadPhFile((char **)(&phoneme_index),"phonindex") != 0)
		return(-1);
	if(ReadPhFile((char **)(&spects_data),"phondata") != 0)
		return(-1);
   wavefile_data = (unsigned char *)spects_data;

	// set up phoneme tables
	p = phoneme_tab_data;
	n_phoneme_tables = p[0];
	p+=4;

	for(ix=0; ix<n_phoneme_tables; ix++)
	{
		n_phonemes = p[0];
		phoneme_tab_list[ix].n_phonemes = p[0];
		phoneme_tab_list[ix].includes = p[1];
		p += 4;
		memcpy(phoneme_tab_list[ix].name,p,N_PHONEME_TAB_NAME);
		p += N_PHONEME_TAB_NAME;
		phoneme_tab_list[ix].phoneme_tab_ptr = (PHONEME_TAB *)p;
		p += (n_phonemes * sizeof(PHONEME_TAB));
	}

	if(phoneme_tab_number >= n_phoneme_tables)
		phoneme_tab_number = 0;

	return(0);
}  //  end of LoadPhData


void FreePhData(void)
{//==================
	Free(phoneme_tab_data);
	Free(phoneme_index);
	Free(spects_data);
}
#endif

int MCU_LookupPh(const char *string)
{//=============================
	int  ix;
	unsigned char c;
	unsigned int  mnem;

	// Pack up to 4 characters into a word
	mnem = 0;
	for(ix=0; ix<4; ix++)
	{
		if(string[ix]==0) break;
		c = string[ix];
		mnem |= (c << (ix*8));
	}

	for(ix=0; ix<MCU_n_phoneme_tab; ix++)
	{
/*		if(MCU_phoneme_tab[ix] == NULL)
			continue;*/
		if(MCU_phoneme_tab[ix].mnemonic == mnem)
			return(ix);
	}
	return(0);
}




static unsigned int MCU_LookupSound2(int index, unsigned int other_phcode, int control)
{//================================================================================
// control=1  get formant transition data only

	int code;
	unsigned int value, value2;
	
	while((value = phoneme_index[index++]) != 0)
	{
		if((code = (value & 0xff)) == other_phcode)
		{
			while(((value2 = phoneme_index[index]) != 0) && ((value2 & 0xff) < 8))
			{
				switch(value2 & 0xff)
				{
				case 0:
					// next entry is a wavefile to be played along with the synthesis
					if(control==0)
					{
						MCU_wavefile_ix = value2 >> 8;
					}
					break;
				case 1:
					if(control==0)
						seq_len_adjust = value2 >> 8;
					break;
				case 2:
					if(control==0)
						seq_len_adjust = -(value2 >> 8);
					break;
				case 3:
					if(control==0)
					{
						MCU_wavefile_amp = value2 >> 8;
					}
					break;
				case 4:
					// formant transition data, 2 words
					MCU_vowel_transition[0] = value2 >> 8;
					MCU_vowel_transition[1] = phoneme_index[index++ + 1];
					break;
				case 5:
					// formant transition data, 2 words
					MCU_vowel_transition[2] = value2 >> 8;
					MCU_vowel_transition[3] = phoneme_index[index++ + 1];
					break;
				}
				index++;
			}
			return(value >> 8);
		}
		else
		if((code == 4) || (code == 5))
		{
			// formant transition data, ignore next word of data
			index++;
		}
	}
	return(3);   // not found
}  //  end of LookupSound2


unsigned int MCU_LookupSound(MCU_PHONEME_TAB *this_ph, MCU_PHONEME_TAB *other_ph,
							 int which, int *match_level, int control)
{//============================================================================================================
	// follows,  1 other_ph preceeds this_ph,   2 other_ph follows this_ph
   // control:  1= get formant transition data only
	int spect_list;
	int spect_list2;
	int s_list;
	unsigned char virtual_ph;
	int  result;
	int  level=0;
	unsigned int  other_code;
	unsigned int  other_virtual;
	
	if(control==0)
	{
		MCU_wavefile_ix = 0;
		MCU_wavefile_amp = 32;
		seq_len_adjust = 0;
	}
	memset(MCU_vowel_transition,0,sizeof(MCU_vowel_transition));
	
	other_code = other_ph->code;
	if(MCU_phoneme_tab[other_code].type == phPAUSE)
		other_code = phonPAUSE_SHORT;       // use this version of Pause for matching

	if(which==1)
	{
		spect_list = this_ph->after;
		virtual_ph = this_ph->start_type;
		spect_list2 = MCU_phoneme_tab[virtual_ph].after;
		other_virtual = other_ph->end_type;
	}
	else
	{
		spect_list = this_ph->before;
		virtual_ph = this_ph->end_type;
		spect_list2 = MCU_phoneme_tab[virtual_ph].before;
		other_virtual = other_ph->start_type;
	}

	result = 3;
	// look for ph1-ph2 combination
	if((s_list = spect_list) != 0)
	{
		if((result = MCU_LookupSound2(s_list,other_code,control)) != 3)
		{
			level = 2;
		}
		else
		if(other_virtual != 0)
		{
			if((result = MCU_LookupSound2(spect_list,other_virtual,control)) != 3)
			{
				level = 1;
			}
		}
	}
	// not found, look in a virtual phoneme if one is given for this phoneme
	if((result==3) && (virtual_ph != 0) && ((s_list = spect_list2) != 0))
	{
		if((result = MCU_LookupSound2(s_list,other_code,control)) != 3)
		{
			level = 1;
		}
		else
		if(other_virtual != 0)
		{
			if((result = MCU_LookupSound2(spect_list2,other_virtual,control)) != 3)
			{
				level = 1;
			}
		}
	}

	if(match_level != NULL)
		*match_level = level;
	
	if(result==0)
		return(0);   // NULL was given in the phoneme source

	// note: values = 1 indicates use the default for this phoneme, even though we found a match
	// which set a secondary reference 
	if(result >= 4)
	{
		// values 1-3 can be used for special codes
		// 1 = DFT from the phoneme source file
		return(result);
	}
	
	// no match found for other_ph, return the default
	return(MCU_LookupSound2(this_ph->spect,phonPAUSE,control));

}  //  end of LookupSound



MCU_frameref_t *MCU_LookupSpect(MCU_PHONEME_TAB *this_ph, MCU_PHONEME_TAB *prev_ph, MCU_PHONEME_TAB *next_ph,
			int which, int *match_level, int *n_frames, MCU_PHONEME_LIST *plist)
{//=========================================================================================================
	int  ix;
	int  nf;
	int  nf1;
	int  seq_break;
	MCU_frameref_t *frames;
	int  length1;
	int  length_std;
	int  length_factor;
	MCU_SPECT_SEQ *seq;
	MCU_SPECT_SEQ *seq2;
	MCU_PHONEME_TAB *next2_ph;
	static MCU_frameref_t frames_buf[N_SEQ_FRAMES];
	
	MCU_PHONEME_TAB *other_ph;
	if(which == 1)
		other_ph = prev_ph;
	else
		other_ph = next_ph;

	if((ix = MCU_LookupSound(this_ph,other_ph,which,match_level,0)) == 0)
		return(NULL);

	// TK: added size check to avoid crash
	if( ix >= PHONEME_DATA_SIZE ) {
	  //MIOS32_MIDI_SendDebugMessage("bad ix: %d\n", ix);
	  ix = 0;
	}

	seq = (MCU_SPECT_SEQ *)(&MCU_spects_data[ix]);

	nf = seq->n_frames;

	seq_break = 0;
	length1 = 0;
	for(ix=0; ix<nf; ix++)
	{
		frames_buf[ix].frame = &seq->frame[ix];
		frames_buf[ix].frflags = seq->frame[ix].frflags;
		frames_buf[ix].length = seq->frame[ix].length;
		if(seq->frame[ix].frflags & FRFLAG_VOWEL_CENTRE)
			seq_break = ix;
	}
	
	frames = &frames_buf[0];
	if(seq_break > 0)
	{
		if(which==1)
		{
			nf = seq_break + 1;
		}
		else
		{
			frames = &frames_buf[seq_break];  // body of vowel, skip past initial frames
			nf -= seq_break;
		}
	}
	
	// do we need to modify a frame for blending with a consonant?
	if(this_ph->type == phVOWEL)
	{
		if(which==2)
		{
			// lookup formant transition for the following phoneme

			if(*match_level == 0)
			{
				MCU_LookupSound(next_ph,this_ph,1,NULL,1);
				seq_len_adjust += MCU_FormantTransition2(frames,&nf,MCU_vowel_transition[2],
					MCU_vowel_transition[3],next_ph,which);
			}
			else
			if(next_ph->phflags == phVOWEL2)
			{
				// not really a consonant, rather a coloured vowel
				if(MCU_LookupSound(next_ph,this_ph,1,NULL,1) == 0)
				{
					next2_ph = &MCU_phoneme_tab[plist[2].ph];
					MCU_LookupSound(next2_ph,next_ph,1,NULL,1);
					seq_len_adjust += MCU_FormantTransition2(frames,&nf,
						MCU_vowel_transition[2], MCU_vowel_transition[3], next2_ph,which);
				}
			}
		}
		else
		{
			if(*match_level == 0)
				seq_len_adjust = MCU_FormantTransition2(frames,&nf,MCU_vowel_transition0,
				MCU_vowel_transition1,prev_ph,which);
		}
//		FormantTransitions(frames,nf,this_ph,other_ph,which);
	}

	nf1 = nf - 1;
	for(ix=0; ix<nf1; ix++)
		length1 += frames[ix].length;


	if((MCU_wavefile_ix != 0) && ((MCU_wavefile_ix & 0x800000)==0))
	{
		// a secondary reference has been returned, which is not a wavefile
		// add these spectra to the main sequence
		seq2 = (MCU_SPECT_SEQ *)(&MCU_spects_data[MCU_wavefile_ix]);
	
		// first frame of the addition just sets the length of the last frame of the main seq
		nf--;
		for(ix=0; ix<seq2->n_frames; ix++)
		{
			frames[nf].length = seq2->frame[ix].length;
			if(ix > 0)
				frames[nf].frame = &seq2->frame[ix];
			nf++;
		}
		MCU_wavefile_ix = 0;
	}
	
	if((this_ph->type == phVOWEL) && (length1 > 0))
	{
		if(which==2)
		{
			// adjust the length of the main part to match the standard length specified for the vowel
			//   less the front part of the vowel and any added suffix
	
			length_std = this_ph->std_length + seq_len_adjust - 45;
			if(plist->synthflags & SFLAG_LENGTHEN)
				length_std += MCU_phoneme_tab[phonLENGTHEN].std_length;  // phoneme was followed by an extra : symbol

// can adjust vowel length for stressed syllables here


			length_factor = (length_std * 256)/ length1;
			
			for(ix=0; ix<nf1; ix++)
			{
				frames[ix].length = (frames[ix].length * length_factor)/256;
			}
		}
		else
		{
			// front of a vowel
			if(*match_level == 0)
			{
				// allow very short vowels to have shorter front parts
				if(this_ph->std_length < 130)
					frames[0].length = (frames[0].length * this_ph->std_length)/130;
			}

			if(seq_len_adjust != 0)
			{
				length_std = 0;
				for(ix=0; ix<nf1; ix++)
				{
					length_std += frames[ix].length;
				}
				length_factor = ((length_std + seq_len_adjust) * 256)/length_std;
				for(ix=0; ix<nf1; ix++)
				{
					frames[ix].length = (frames[ix].length * length_factor)/256;
				}
			}
		}
	}
	
	*n_frames = nf;
	return(frames);
}  //  end of LookupSpect


unsigned char *MCU_LookupEnvelope(int ix)
{//================================
	if(ix==0)
		return(NULL);
	return((unsigned char *)&MCU_spects_data[phoneme_index[ix]]);
}

#ifdef MCU_USE_MANY_TABLES

static void SetUpPhonemeTable(int number)
{//======================================
	int ix;
	int includes;
	int ph_code;
	MCU_PHONEME_TAB *phtab;

	if((includes = phoneme_tab_list[number].includes) > 0)
	{
		// recursively include base phoneme tables
		SetUpPhonemeTable(includes-1);
	}

	// now add the phonemes from this table
	phtab = phoneme_tab_list[number].phoneme_tab_ptr;
	for(ix=0; ix<phoneme_tab_list[number].n_phonemes; ix++)
	{
		ph_code = phtab[ix].code;
		phoneme_tab[ph_code] = &phtab[ix];
		if(ph_code > n_phoneme_tab)
			n_phoneme_tab = ph_code;
	}
}  // end of SetUpPhonemeTable


void SelectPhonemeTable(int number)
{//================================
	n_phoneme_tab = 0;
	SetUpPhonemeTable(number);  // recursively for included phoneme tables
	n_phoneme_tab++;
}  //  end of SelectPhonemeTable


int LookupPhonemeTable(const char *name)
{//=====================================
	int ix;

	for(ix=0; ix<n_phoneme_tables; ix++)
	{
		if(strcmp(name,phoneme_tab_list[ix].name)==0)
		{
			phoneme_tab_number = ix;
			break;
		}
	}
	if(ix == n_phoneme_tables)
		return(-1);

	return(ix);
}


int SelectPhonemeTableName(const char *name)
{//=========================================
// Look up a phoneme set by name, and select it if it exists
// Returns the phoneme table number
	int ix;

	if((ix = LookupPhonemeTable(name)) == -1)
		return(-1);

	SelectPhonemeTable(ix);
	return(ix);
}  //  end of DelectPhonemeTableName
#endif
