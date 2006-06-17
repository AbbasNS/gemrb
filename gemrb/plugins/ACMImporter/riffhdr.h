/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/riffhdr.h,v 1.6 2006/06/17 07:51:44 avenger_teambg Exp $
 *
 */

#ifndef _ACM_LAB_RIFF_HEADER_H
#define _ACM_LAB_RIFF_HEADER_H

#include <stdio.h>

typedef struct {
	char riff_sig[4];
	unsigned int total_len_m8;
	char wave_sig[8];
	unsigned int formatex_len;
	unsigned short wFormatTag, nChannels;
	unsigned int nSamplesPerSec, nAvgBytesPerSec;
	unsigned short nBlockAlign, wBitsPerSample;
	char data_sig[4];
	unsigned int raw_data_len;
} RIFF_HEADER;

typedef struct {
	char wavc_sig[4];
	char wavc_ver[4];
	int uncompressed;
	int compressed;
	int headersize;
	short channels;
	short bits;
	short samplespersec;
	unsigned short unknown;
} WAVC_HEADER;

void write_riff_header(void* memory, int samples, int channels,
	int samplerate);
void write_wavc_header(FILE* foutp, int samples, int channels,
	int compressed, int samplerate);

#endif //_ACM_LAB_RIFF_HEADER_H
