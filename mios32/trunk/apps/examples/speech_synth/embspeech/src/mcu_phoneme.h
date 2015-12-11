/***************************************************************************
 *   Copyright (C) 2005,2006 by Jonathan Duddington                        *
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


// phoneme types
#define phPAUSE   0
#define phSTRESS  1
#define phVOWEL   2
#define phLIQUID  3
#define phSTOP    4
#define phVSTOP   5
#define phFRICATIVE 6
#define phVFRICATIVE 7
#define phNASAL   8
#define phVIRTUAL 9
#define phINVALID 15


// phoneme properties
//   bits 16-19 give place of articulation (not currently used)
#define phWAVE     0x01
#define phUNSTRESSED 0x02
#define phFORTIS   0x08
#define phVOICED   0x10
#define phSIBILANT 0x20
#define phNOLINK   0x40
#define phTRILL    0x80
#define phVOWEL2   0x100   // liquid that is considered a vowel
#define phPALATAL  0x200
#define phLONG     0x1000

#define phALTERNATIVE    0x0c00   // bits 10,11  specifying use of alternative_ph
#define phBEFOREVOWEL    0x0000
#define phBEFOREVOWELPAUSE  0x0400
#define phBEFORENOTVOWEL 0x0c00
#define phSWITCHVOICING  0x0800

// fixed phoneme code numbers, these can be used from the program code
#define phonCONTROL     1
#define phonSTRESS_U    2
#define phonSTRESS_D    3
#define phonSTRESS_2    4
#define phonSTRESS_3    5
#define phonSTRESS_P    6
#define phonSTRESS_TONIC 7
#define phonSTRESS_PREV 8
#define phonPAUSE       9
#define phonPAUSE_SHORT 10
#define phonPAUSE_NOLINK 11
#define phonLENGTHEN    12
#define phonSCHWA       13
#define phonSCHWA_SHORT 14
#define phonEND_WORD    15
#define phonSONORANT    16
#define phonDEFAULTTONE 17
#define phonCAPITAL     18
#define phonGLOTTALSTOP 19
#define phonSYLLABIC    20
#define phonSWITCH      21
#define phonX1          22      // a language specific action


// place of articulation
#define phPLACE        0xf0000
#define phPLACE_pla    0x60000

#ifdef MCU_USE_MANY_TABLES

#define N_PHONEME_TABS      50     // number of phoneme tables

#define N_PHONEME_TAB_NAME  32     // must be multiple of 4
#endif

#define N_PHONEME_TAB      256     // max phonemes in a phoneme table
// main table of phonemes, index by phoneme number (1-254)
typedef struct {
	unsigned int mnemonic;        // 1st char is in the l.s.byte
	unsigned int phflags;         // bits 28-30 reduce_to level,  bits 16-19 place of articulation
                                 // bits 10-11 alternative ph control

	unsigned short std_length;    // for vowels, in mS;  for phSTRESS, the stress/tone type
	unsigned short  spect;
	unsigned short  before;
	unsigned short  after;

	unsigned char  code;          // the phoneme number
	unsigned char  type;          // phVOWEL, phPAUSE, phSTOP etc
	unsigned char  start_type;
	unsigned char  end_type;
	
	unsigned char  length_mod;     // a length_mod group number, used to access length_mod_tab
	unsigned char  reduce_to;      // change to this phoneme if unstressed
	unsigned char  alternative_ph; // change to this phoneme if a vowel follows/doesn't follow
	unsigned char  link_out;       // insert linking phoneme if a vowel follows
	
}MCU_PHONEME_TAB;

/*

// Several phoneme tables may be loaded into memory. phoneme_tab points to
// one for the current voice
extern int n_phoneme_tab;
extern PHONEME_TAB *phoneme_tab[N_PHONEME_TAB];

typedef struct {
	char name[N_PHONEME_TAB_NAME];
	PHONEME_TAB *phoneme_tab_ptr;
	int n_phonemes;
	int includes;           // also include the phonemes from this other phoneme table
} PHONEME_TAB_LIST;



// table of phonemes to be replaced with different phonemes, for the current voice
#define N_REPLACE_PHONEMES   60
typedef struct {
	unsigned char old_ph;
	unsigned char new_ph;
	char type;   // 0=always replace, 1=only at end of word
} REPLACE_PHONEMES;

extern int n_replace_phonemes;
extern REPLACE_PHONEMES replace_phonemes[N_REPLACE_PHONEMES];



char *EncodePhonemes(char *p, char *outptr, char *bad_phoneme);
void DecodePhonemes(const char *inptr, char *outptr);
const char *PhonemeTabName(void);
int LookupPh(const char *string);

*/
