/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 * $Id$
 *
 */

/**
 * @file Item.h
 * Declares Item, class for all things your PCs can pick, carry and wear
 * and many they can't.
 * @author The GemRB Project
 */

#ifndef ITEM_H
#define ITEM_H

#include "../../includes/ie_types.h"

#include "AnimationMgr.h"
#include "EffectQueue.h"

class Projectile;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

// Item Flags bits
// !!! Keep these synchronized with GUIDefines.h !!!
#define IE_ITEM_CRITICAL     0x00000001
#define IE_ITEM_TWO_HANDED   0x00000002
#define IE_ITEM_MOVABLE      0x00000004
#define IE_ITEM_DISPLAYABLE  0x00000008
#define IE_ITEM_CURSED       0x00000010
#define IE_ITEM_NOT_COPYABLE 0x00000020
#define IE_ITEM_MAGICAL      0x00000040
#define IE_ITEM_BOW	  0x00000080
#define IE_ITEM_SILVER       0x00000100
#define IE_ITEM_COLD_IRON    0x00000200
#define IE_ITEM_STOLEN       0x00000400
#define IE_ITEM_CONVERSABLE  0x00000800
#define IE_ITEM_PULSATING    0x00001000
#define IE_ITEM_UNSELLABLE   ( IE_ITEM_CRITICAL | IE_ITEM_STOLEN )

//Extended header recharge flags
#define IE_ITEM_USESTRENGTH  1
#define IE_ITEM_RECHARGE     0x800
#define IE_ITEM_IGNORESHIELD 0x10000
#define IE_ITEM_KEEN	 0x20000

//item use locations (weapons are not listed in equipment list)
#define ITEM_LOC_WEAPON    1   //this is a weapon slot (uses thac0 etc)
#define ITEM_LOC_EQUIPMENT 3   //this slot appears on equipment list
//other item locations appear useless

//attack types
#define ITEM_AT_MELEE      1
#define ITEM_AT_PROJECTILE 2
#define ITEM_AT_MAGIC      3
#define ITEM_AT_BOW	4

//projectile qualifiers
#define PROJ_ARROW  1
#define PROJ_BOLT   2
#define PROJ_BULLET 4

//charge depletion flags
#define CHG_BREAK   1
#define CHG_NOSOUND 2
#define CHG_NONE    3

/**
 * @class ITMExtHeader
 * Header for special effects and uses of an Item.
 */

class GEM_EXPORT ITMExtHeader {
public:
	ITMExtHeader();
	~ITMExtHeader();
	ieByte AttackType;
	ieByte IDReq;
	ieByte Location;
	ieByte unknown1;
	ieResRef UseIcon;
	ieByte Target;
	ieByte TargetNumber;
	ieWord Range;
	ieWord ProjectileType; //it is not sure where this value is used
	ieWord Speed;
	ieWord THAC0Bonus;
	ieWord DiceSides;
	ieWord DiceThrown;
	ieWordSigned DamageBonus; //this must be signed!!!
	ieWord DamageType;
	ieWord FeatureCount;
	ieWord FeatureOffset;
	ieWord Charges;
	ieWord ChargeDepletion;
	ieDword RechargeFlags; //this is a bitfield with many bits
	ieWord ProjectileAnimation;
	ieWord MeleeAnimation[3];
	//this value is set in projectiles and launchers too
	int ProjectileQualifier; //this is a derived value determined on load time
	Effect *features;
};

/**
 * @class Item
 * Class for all things your PCs can pick, carry and wear and many they can't.
 */

class GEM_EXPORT Item {
public:
	Item();
	~Item();

	ITMExtHeader *ext_headers;
	Effect *equipping_features;
	ieResRef Name; //the resref of the item itself!

	ieStrRef ItemName;
	ieStrRef ItemNameIdentified;
	ieResRef ReplacementItem;
	ieDword Flags;
	ieWord ItemType;
	ieDword UsabilityBitmask;
	char AnimationType[2];
	ieWord MinLevel; //minlevel is actually just one byte, but who cares
	ieByte MinStrength;
	ieByte unknown2;
	ieByte MinStrengthBonus;
	//kit1
	ieByte MinIntelligence;
	//kit2
	ieByte MinDexterity;
	//kit3
	ieByte MinWisdom;
	//kit4
	ieByte MinConstitution;
	ieByte WeaProf;
	ieByte MinCharisma;
	ieByte unknown3;
	ieDword KitUsability;
	ieDword Price;
	ieWord StackAmount;
	ieResRef ItemIcon;
	ieWord LoreToID;
	ieResRef GroundIcon;
	ieDword Weight;
	ieStrRef ItemDesc;
	ieStrRef ItemDescIdentified;
	ieResRef DescriptionIcon;
	ieDword Enchantment;
	ieDword ExtHeaderOffset;
	ieWord ExtHeaderCount;
	ieDword FeatureBlockOffset;
	ieWord EquippingFeatureOffset;
	ieWord EquippingFeatureCount;

	// PST only
	ieResRef Dialog;
	ieStrRef DialogName;
	ieWord WieldColor;

	// PST and IWD2 only
	char unknown[26];
	// flag items to mutually exclusive to equip
	ieDword ItemExcl;
public:
	ieStrRef GetItemName(bool identified) const
	{
		if(identified) {
			if((int) ItemNameIdentified>=0) return ItemNameIdentified;
			return ItemName;
		}
		if((int) ItemName>=0) {
			return ItemName;
		}
		return ItemNameIdentified;
	};
	ieStrRef GetItemDesc(bool identified) const
	{
		if(identified) {
			if((int) ItemDescIdentified>=0) return ItemDescIdentified;
			return ItemDesc;
		}
		if((int) ItemDesc>=0) {
			return ItemDesc;
		}
		return ItemDescIdentified;
	}

	int UseCharge(ieWord *Charges, int header) const;

	//returns the requested extended header
	ITMExtHeader *GetExtHeader(unsigned int which) const
	{
		if(ExtHeaderCount<=which) {
			return NULL;
		}
		return ext_headers+which;
	}
	ieDword GetWieldedGradient() const
	{
		return WieldColor;
	}

	//-1 will return the equipping feature block
	EffectQueue *GetEffectBlock(int usage) const;
	//returns a projectile created from an extended header
	Projectile *GetProjectile(int header) const;
	//Builds an equipping glow effect from gradient colour
	//this stuff is not item specific, could be moved elsewhere
	Effect *BuildGlowEffect(int gradient) const;
	//returns the average damage of the weapon (doesn't check for special effects)
	int GetDamagePotential(bool ranged, ITMExtHeader *&header) const;
	//returns the weapon header
	ITMExtHeader *GetWeaponHeader(bool ranged) const;

};

#endif // ! ITEM_H
