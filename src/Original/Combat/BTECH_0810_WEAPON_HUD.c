#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_ballistics.h"
#include <stdlib.h>

enum { WEAPON_HUD_ROW_HEIGHT=8, WEAPON_HUD_ROWS_PER_BANK=5, WEAPON_HUD_FIRST_BANK_LAST_ROW=4,
    WEAPON_HUD_BANK_GAP=3, WEAPON_HUD_AMMO_COLUMN=0x23, WEAPON_HUD_RANGE_COLUMN=0x39,
    WEAPON_HUD_AMMO_DIGITS=3, WEAPON_HUD_MARKER_INSET=0x0A,
    WEAPON_HUD_MARKER_WIDTH=6, WEAPON_HUD_MARKER_HEIGHT=5,
    WEAPON_HUD_OUTER_MARKER_WIDTH=8, WEAPON_HUD_OUTER_MARKER_HEIGHT=7,
    WEAPON_HUD_COPY_WIDTH=0x4A };

/*0810:6931. Ten physical mount rows, including holes and the second-bank gap.
 * The marker resets Y from the anchor only for populated mounts, as native. */
void BtechDrawWeaponStatusList(uint16_t selectedMountIndex,uint16_t targetUnitSlot)
{
    if (!playerMechState || playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layout=mechCockpitLayoutIndices[playerMechState->templateIndex];
    if (layout>=COMBAT_COCKPIT_LAYOUTS) abort();
    const BtechCockpitLayout *cockpit=&cockpitLayouts[layout];
    textTransparentBackground=false;textBackgroundColour=Combat_Pal_EGA_DarkGrey;
    uint16_t rowY=cockpit->weaponListY;
    uint16_t mount;
    for (mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount,rowY+=WEAPON_HUD_ROW_HEIGHT)
    {
        if (mount==WEAPON_HUD_ROWS_PER_BANK) rowY+=WEAPON_HUD_BANK_GAP;
        if (playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
        uint16_t weaponKind=mechTemplates[playerMechState->templateIndex].weaponMounts[mount].weaponKind;
        if (weaponKind==COMBAT_WEAPON_NONE) continue;
        if (weaponKind>=COMBAT_WEAPON_KINDS || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
        int16_t ammoFamily=weaponDefinitions[weaponKind].ammoFamily;
        textForegroundColour=(uint8_t)(playerMechState->weapons[mount].reloadTicksRemaining?
            hudColourSchemes[hudColourSchemeIndex].weaponReloading:hudColourSchemes[hudColourSchemeIndex].weaponReady);
        if (ammoFamily!=(int16_t)COMBAT_AMMO_NONE && (uint16_t)ammoFamily>=COMBAT_AMMO_FAMILIES) abort();
        if ((ammoFamily!=(int16_t)COMBAT_AMMO_NONE && !playerMechState->ammoSalvosRemaining[ammoFamily]) || !playerMechState->weapons[mount].operational)
            textForegroundColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].weaponUnavailable;
        BtechDrawNearText(weaponLabels[weaponKind],cockpit->weaponListX,(int16_t)rowY);
        textForegroundColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].ammoText;
        const char *ammoText=ammoFamily==(int16_t)COMBAT_AMMO_NONE?energyAmmoPlaceholder:
            BtechFormatSignedDecimalField((int16_t)playerMechState->ammoSalvosRemaining[ammoFamily],WEAPON_HUD_AMMO_DIGITS,' ');
        BtechDrawNearText(ammoText,(int16_t)(cockpit->weaponListX+WEAPON_HUD_AMMO_COLUMN),(int16_t)rowY);
        int16_t rangeBucket=targetUnitSlot?BtechClassifyWeaponRange(0,targetUnitSlot,weaponKind):4;
        if ((uint16_t)rangeBucket>=5) abort();
        textForegroundColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].rangeBuckets[rangeBucket];
        uint16_t rangeClass=weaponDefinitions[weaponKind].rangeClass;
        if (rangeClass>=4) abort();
        BtechDrawNearText(weaponRangeClassLabels[rangeClass],(int16_t)(cockpit->weaponListX+WEAPON_HUD_RANGE_COLUMN),(int16_t)rowY);
        uint16_t markerX=cockpit->weaponListX-WEAPON_HUD_MARKER_INSET;
        rowY=(uint16_t)(cockpit->weaponListY+mount*WEAPON_HUD_ROW_HEIGHT+(mount>WEAPON_HUD_FIRST_BANK_LAST_ROW?WEAPON_HUD_BANK_GAP:0));
        drawFillColour=(uint8_t)(mount==selectedMountIndex?hudColourSchemes[hudColourSchemeIndex].selectedMount:
            hudColourSchemes[hudColourSchemeIndex].unselectedMount);
        drawLineColour=drawFillColour;drawPolygonFilled=true;
        BtechDrawStyledRectangle(markerX,rowY,WEAPON_HUD_MARKER_WIDTH,WEAPON_HUD_MARKER_HEIGHT);
        drawPolygonFilled=false;
        drawFillColour=playerMountFireFlags[mount] && mountGroupMode?Combat_Pal_EGA_LightRed:Combat_Pal_EGA_DarkGrey;
        BtechDrawStyledRectangle((uint16_t)(markerX-1),(uint16_t)(rowY-1),WEAPON_HUD_OUTER_MARKER_WIDTH,WEAPON_HUD_OUTER_MARKER_HEIGHT);
    }
    BtechPresentHudRectangleIfReady((uint16_t)(cockpit->weaponListX-WEAPON_HUD_MARKER_INSET),(uint16_t)(cockpit->weaponListY-1),WEAPON_HUD_COPY_WIDTH,(uint16_t)(mount*WEAPON_HUD_ROW_HEIGHT+WEAPON_HUD_BANK_GAP));
}
