#ifndef SCRIPTEDANIMATION_H
#define SCRIPTEDANIMATION_H

#include "DataStream.h"
#include "AnimationFactory.h"
#include "Palette.h"
#include "SpriteCover.h"
#include "Map.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

//scripted animation flags 
#define S_ANI_PLAYONCE        8        //(same as area animation)

#define IE_VVC_TRANSPARENT	0x00000002
#define IE_VVC_BLENDED		0x00000008
#define IE_VVC_MIRRORX    	0x00000010
#define IE_VVC_MIRRORY   	0x00000020
#define IE_VVC_TINT     	0x00030000   //2 bits need to be set for tint
#define IE_VVC_GREYSCALE	0x00080000
#define IE_VVC_DARKEN           0x00100000   //this is unsure
#define IE_VVC_GLOWING  	0x00200000
#define IE_VVC_RED_TINT		0x02000000

#define IE_VVC_LOOP		0x00000001
#define IE_VVC_BAM		0x00000008
#define IE_VVC_NOCOVER		0x00000040

#define IE_VVC_UNUSED           0xe0000000U

//phases
#define P_NOTINITED -1
#define P_ONSET   0
#define P_HOLD    1
#define P_RELEASE 2

class GEM_EXPORT ScriptedAnimation {
public:
	ScriptedAnimation();
	~ScriptedAnimation(void);
	ScriptedAnimation(DataStream* stream, bool autoFree = true);
	void Init();
	void LoadAnimationFactory(AnimationFactory *af, int gettwin = 0);
	void Override(ScriptedAnimation *templ);
	//there are 3 phases: start, hold, release
	//it will usually cycle in the 2. phase
	//the anims could also be used 'orientation based' if FaceTarget is
	//set to 5, 9, 16
	Animation* anims[3*MAX_ORIENT];
	//there is only one palette
	Palette *palette;
	ieResRef sounds[3];
	ieResRef PaletteName;
	Color Tint;
	int Fade;
	ieDword Transparency;
	ieDword SequenceFlags;
	int Dither;
	//these are signed
	int XPos, YPos, ZPos;
	ieDword FrameRate;
	ieDword FaceTarget;
	unsigned char Orientation, NewOrientation;
	ieDword Duration;
	bool justCreated;
	ieResRef ResName;
	int Phase;
	SpriteCover* cover;
	ScriptedAnimation *twin;
public:
	//draws the next frame of the videocell
	bool Draw(Region &screen, Point &Pos, Color &tint, Map *area, int dither, int orientation);
	//sets phase (0-2)
	void SetPhase(int arg);
	//sets sound for phase (p_onset, p_hold, p_release)
	void SetSound(int arg, const ieResRef sound);
	//sets the animation to play only once
	void PlayOnce();
	//sets gradient colour slot to gradient
	void SetPalette(int gradient, int start=-1);
	//sets complete palette to ResRef
	void SetFullPalette(const ieResRef PaletteResRef);
	//sets complete palette to own name+index
	void SetFullPalette(int idx);
	//sets spritecover
	void SetSpriteCover(SpriteCover* c) { delete cover; cover = c; }
	/* get stored SpriteCover */
	SpriteCover* GetSpriteCover() const { return cover; }
	ieDword GetSequenceDuration(ieDword multiplier);
	/* sets default duration if it wasn't set yet */
	void SetDefaultDuration(unsigned int duration);
	/* sets up the direction of the vvc */
	void SetOrientation(int orientation);
	/* transforms vvc to blended */
	void SetBlend();
	/* sets fade effect at end of animation (pst feature) */
	void SetFade(ieByte initial, int speed);
	/* alters palette with rgb factor */
	void AlterPalette(const RGBModifier &rgb);
	/* returns possible twin after altering it to become underlay */
	ScriptedAnimation *DetachTwin();
private:
	void PrepareAnimation(Animation *anim, ieDword Transparency);
	void PreparePalette();
	bool HandlePhase(Sprite2D *&frame);
	void GetPaletteCopy();
};

#endif
