/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003-2004 The GemRB Project
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
 * $Id$
 *
 */

#include "ACMImp.h"

ACMImp::ACMImp()
{
}

bool ACMImp::Open(DataStream* stream, bool autofree)
{
    if (!stream) {
		return false;
	}
	char Signature[4];
	stream->Read( Signature, 4 );
	stream->Seek( 0, GEM_STREAM_START );
#ifdef HAS_VORBIS_SUPPORT
	if(strnicmp(Signature, "oggs", 4) == 0) {
		SoundReader = CreateSoundReader(stream, SND_READER_OGG, stream->Size(), autofree) ;
	} //ogg
#endif
	if(strnicmp(Signature, "RIFF", 4) == 0) {
		SoundReader = CreateSoundReader(stream, SND_READER_WAV, stream->Size(), autofree) ;
	} //wav
	if (*( unsigned int * ) Signature == IP_ACM_SIG) {
		SoundReader = CreateSoundReader(stream, SND_READER_ACM, stream->Size(), autofree) ;
	} //acm
	if (memcmp( Signature, "WAVC", 4 ) == 0) {
		SoundReader = CreateSoundReader(stream, SND_READER_ACM, stream->Size(), autofree) ;
	} //wavc

	if (SoundReader)
        return true ;

	return false;
}

ACMImp::~ACMImp()
{
    delete SoundReader ;
}

int ACMImp::get_channels()
{
    return SoundReader->get_channels() ;
}

int ACMImp::get_length()
{
    return SoundReader->get_length() ;
}

int ACMImp::get_samplerate()
{
    return SoundReader->get_samplerate() ;
}

int ACMImp::read_samples(short* memory, int cnt)
{
    return SoundReader->read_samples(memory, cnt) ;
}

void ACMImp::rewind()
{
    SoundReader->rewind() ;
}
