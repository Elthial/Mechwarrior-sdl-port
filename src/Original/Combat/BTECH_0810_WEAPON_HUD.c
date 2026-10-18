#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_ballistics.h"
#include <stdlib.h>

/*0810:6931. Ten physical mount rows, including holes and the second-bank gap.
 * The marker resets Y from the anchor only for populated mounts, as native. */
void BtechDrawWeaponStatusList(uint16_t selectedMountIndex,uint16_t targetUnitSlot)
{
    if (!playerMechState || playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layout=mechCockpitLayoutIndices[playerMechState->templateIndex];
    if (layout>=COMBAT_COCKPIT_LAYOUTS) abort();
    const BtechCockpitLayout *cockpit=&cockpitLayouts[layout];
    textTransparentBackground=0;textBackgroundColour=8;
    uint16_t rowY=(uint16_t)cockpit->weaponListY;
    uint16_t mount;
    for (mount=0;mount<COMBAT_WEAPON_MOUNTS;++mount,rowY+=8)
    {
        if (mount==5) rowY+=3;
        if (playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
        uint16_t weaponKind=(uint16_t)mechTemplates[playerMechState->templateIndex].weaponMounts[mount].weaponKind;
        if (weaponKind==COMBAT_WEAPON_NONE) continue;
        if (weaponKind>=COMBAT_WEAPON_KINDS || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
        int16_t ammoFamily=weaponDefinitions[weaponKind].ammoFamily;
        textForegroundColour=(uint8_t)(playerMechState->weapons[mount].reloadTicksRemaining?
            hudColourSchemes[hudColourSchemeIndex].weaponReloading:hudColourSchemes[hudColourSchemeIndex].weaponReady);
        if (ammoFamily!=-1 && (uint16_t)ammoFamily>=COMBAT_AMMO_FAMILIES) abort();
        if ((ammoFamily!=-1 && !playerMechState->ammoSalvosRemaining[ammoFamily]) || !playerMechState->weapons[mount].operational)
            textForegroundColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].weaponUnavailable;
        BtechDrawNearText(weaponLabels[weaponKind],cockpit->weaponListX,(int16_t)rowY);
        textForegroundColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].ammoText;
        const char *ammoText=ammoFamily==-1?energyAmmoPlaceholder:
            BtechFormatSignedDecimalField((int16_t)playerMechState->ammoSalvosRemaining[ammoFamily],3,' ');
        BtechDrawNearText(ammoText,(int16_t)(uint16_t)(cockpit->weaponListX+0x23),(int16_t)rowY);
        int16_t rangeBucket=targetUnitSlot?BtechClassifyWeaponRange(0,targetUnitSlot,weaponKind):4;
        if ((uint16_t)rangeBucket>=5) abort();
        textForegroundColour=(uint8_t)hudColourSchemes[hudColourSchemeIndex].rangeBuckets[rangeBucket];
        uint16_t rangeClass=weaponDefinitions[weaponKind].rangeClass;
        if (rangeClass>=4) abort();
        BtechDrawNearText(weaponRangeClassLabels[rangeClass],(int16_t)(uint16_t)(cockpit->weaponListX+0x39),(int16_t)rowY);
        uint16_t markerX=(uint16_t)(cockpit->weaponListX-0x0A);
        rowY=(uint16_t)(cockpit->weaponListY+mount*8+(mount>4?3:0));
        drawFillColour=(uint8_t)(mount==selectedMountIndex?hudColourSchemes[hudColourSchemeIndex].selectedMount:
            hudColourSchemes[hudColourSchemeIndex].unselectedMount);
        drawLineColour=drawFillColour;drawPolygonFilled=1;
        BtechDrawStyledRectangle(markerX,rowY,6,5);
        drawPolygonFilled=0;
        drawFillColour=playerMountFireFlags[mount] && mountGroupMode?0x0C:8;
        BtechDrawStyledRectangle((uint16_t)(markerX-1),(uint16_t)(rowY-1),8,7);
    }
    BtechPresentHudRectangleIfReady((uint16_t)(cockpit->weaponListX-0x0A),(uint16_t)(cockpit->weaponListY-1),0x4A,(uint16_t)(mount*8+3));
}
