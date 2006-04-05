/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2005 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/EFFImporter/EFFImp.cpp,v 1.7 2006/04/05 16:34:30 avenger_teambg Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../Core/Interface.h"
#include "../Core/AnimationMgr.h"
#include "EFFImp.h"

EFFImp::EFFImp(void)
{
	str = NULL;
	autoFree = false;
}

EFFImp::~EFFImp(void)
{
	if (str && autoFree) {
		delete( str );
	}
}

bool EFFImp::Open(DataStream* stream, bool autoFree)
{
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	char Signature[8];
	str->Read( Signature, 8 );
	if (strncmp( Signature, "EFF V2.0", 8 ) == 0) {
		version = 20;
	} else {
		version = 1;
	}
	str->Seek( -8, GEM_CURRENT_POS );
	return true;
}

Effect* EFFImp::GetEffect(Effect *fx)
{
	if (version == 1) {
		return GetEffectV1( fx );
	}
	else {
		// Skip over Signature1
		str->Seek( 8, GEM_CURRENT_POS );
		return GetEffectV20( fx );
	}
}

Effect* EFFImp::GetEffectV1(Effect *fx)
{
	ieByte tmpByte;
	ieWord tmpWord;

	memset( fx, 0, sizeof( Effect ) );

	str->ReadWord( &tmpWord );
	fx->Opcode = tmpWord;
	str->Read( &tmpByte, 1 );
	fx->Target = tmpByte;
	str->Read( &tmpByte, 1 );
	fx->Power = tmpByte;
	str->ReadDword( &fx->Parameter1 );
	str->ReadDword( &fx->Parameter2 );
	str->Read( &tmpByte, 1 );
	fx->TimingMode = tmpByte;
	str->Read( &tmpByte, 1 );
	fx->Resistance = tmpByte;
	str->ReadDword( &fx->Duration );
	str->Read( &tmpByte, 1 );
	fx->Probability1 = tmpByte;
	str->Read( &tmpByte, 1 );
	fx->Probability2 = tmpByte;
	str->ReadResRef( fx->Resource );
	str->ReadDword( &fx->DiceThrown );
	str->ReadDword( &fx->DiceSides );
	str->ReadDword( &fx->SavingThrowType );
	str->ReadDword( &fx->SavingThrowBonus );
	str->ReadDword( &fx->IsVariable );

	return fx;
}

Effect* EFFImp::GetEffectV20(Effect *fx)
{
	memset( fx, 0, sizeof( Effect ) );

	str->Seek(8, GEM_CURRENT_POS);
	str->ReadDword( &fx->Opcode );
	str->ReadDword( &fx->Target );
	str->ReadDword( &fx->Power );
	str->ReadDword( &fx->Parameter1 );
	str->ReadDword( &fx->Parameter2 );
	str->ReadDword( &fx->TimingMode );
	str->ReadDword( &fx->Duration );
	str->ReadWord( &fx->Probability1 );
	str->ReadWord( &fx->Probability2 );
	str->ReadResRef( fx->Resource );
	str->ReadDword( &fx->DiceThrown );
	str->ReadDword( &fx->DiceSides );
	str->ReadDword( &fx->SavingThrowType );
	str->ReadDword( &fx->SavingThrowBonus );
	str->ReadDword( &fx->IsVariable ); //if this field was set to 1, this is a variable	
	str->ReadDword( &fx->PrimaryType );
	str->Seek( 12, GEM_CURRENT_POS );
	str->ReadDword( &fx->Resistance );
	str->ReadDword( &fx->Parameter3 );
	str->ReadDword( &fx->Parameter4 );
	str->Seek( 8, GEM_CURRENT_POS );
	str->ReadResRef( fx->Resource2 );
	str->ReadResRef( fx->Resource3 );	
	str->Seek( 20, GEM_CURRENT_POS );//unsure fields, should be resource4, but also effect center
	str->ReadResRef( fx->Source );
	str->Seek( 12, GEM_CURRENT_POS );
	//Variable simply overwrites the resource fields (Keep them grouped)
	if (fx->IsVariable) {
		str->ReadResRef( fx->Resource );
		str->ReadResRef( fx->Resource+8 );
		str->ReadResRef( fx->Resource+16 );
		str->ReadResRef( fx->Resource+24 );
	} else {
		str->Seek( 32, GEM_CURRENT_POS);
	}
	str->Seek( 8, GEM_CURRENT_POS );
	str->ReadDword( &fx->SecondaryType );
	str->Seek( 60, GEM_CURRENT_POS );

	return fx;
}
