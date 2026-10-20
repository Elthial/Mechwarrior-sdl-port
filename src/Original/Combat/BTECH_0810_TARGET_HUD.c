#include "mechwarrior/combat_cockpit.h"
#include "mechwarrior/combat_startup.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_render.h"
#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_orders.h"
#include "mechwarrior/combat_damage.h"
#include <stdlib.h>

/*0810:6BCA. Cached damage panel followed by a live projected target box.
 *53C4 is the last two bytes of689E's ten-byte scratch, not separate storage. */
void BtechUpdateTargetPanelAndReticle(uint16_t targetUnitSlot)
{
    if (!playerMechState || playerMechState->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
    uint16_t layout=mechCockpitLayoutIndices[playerMechState->templateIndex];
    if (layout>=COMBAT_COCKPIT_LAYOUTS) abort();
    const BtechScreenRectangle *panel=&cockpitLayouts[layout].targetPanel;
    screenPolygonNeedsClip=0;drawPolygonFilled=1;
    if (!combatTickCounter) cachedTargetPanelSlot=0;
    drawPageSelector=backPageSelector;
    if (!targetUnitSlot)
    {
        if (cachedTargetPanelSlot)
        {
            cachedTargetPanelSlot=0;drawPolygonFilled=1;drawFillColour=Combat_Pal_EGA_LightGreen;drawLineColour=Combat_Pal_EGA_LightGreen;
            BtechDrawStyledRectangle((uint16_t)(panel->x+1),(uint16_t)(panel->y+1),panel->width,panel->height);
            BtechCopyHudRectangleIfReady((uint16_t)(panel->x+1),(uint16_t)(panel->y+1),panel->width,panel->height);
        }
        return;
    }
    if (targetUnitSlot>COMBAT_SPECIAL_TARGET_SLOT) abort();
    uint8_t *dirty=targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT?&specialTargetDamagePanelDirty:&targetDamagePanelDirty[targetUnitSlot];
    if (targetUnitSlot!=cachedTargetPanelSlot || *dirty || cockpitFullRedrawPending)
    {
        BtechDrawTargetDamagePanel(targetUnitSlot,panel);
        cachedTargetPanelSlot=targetUnitSlot;*dirty=0;
    }
    screenPolygonNeedsClip=1;drawPolygonFilled=0;
    if (hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES || combatLandscapeIndex>=COMBAT_LANDSCAPES) abort();
    drawFillColour=landscapeCameraColourRows[hudColourSchemeIndex][combatLandscapeIndex].hudReticleColour;
    BtechPooledWorldObject *target=targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT?specialTargetObject:unitBindings[targetUnitSlot].torso;
    if (!target || !unitBindings[0].torso) abort();
    uint32_t distance=BtechApproximatePlanarDistance(&unitBindings[0].torso->object.prefix.position,&target->object.prefix.position);
    int16_t scale;
    if (targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT) scale=0x09C4;
    else
    {
        BtechMechState *mech=unitBindings[targetUnitSlot].mech;
        if (!mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
        scale=(int16_t)mechTemplates[mech->templateIndex].displayScale;
    }
    uint32_t numerator=BtechShiftDwordLeft((uint32_t)(int32_t)scale,(uint8_t)viewScaleShift);
    int16_t ratio=(int16_t)BtechDivideSignedDword(numerator,distance);
    int16_t width=(int16_t)(uint16_t)((ratio-(ratio<0))/2+8);
    int16_t halfWidth=(int16_t)((width-(width<0))/2);
    /* NativeC958+slot8 aliases the existing C968 special projected object. */
    BtechPooledWorldObject *projected=targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT?unknownWorldObject:unitLastDrawParts[targetUnitSlot];
    if (!projected) abort();
    uint16_t x=(uint16_t)(projected->object.projectedScreenA-halfWidth);
    uint16_t y=(uint16_t)(projected->object.projectedScreenB-width);
    uint16_t height=(uint16_t)((uint16_t)width*2);
    if (targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT) {x-=halfWidth;width=(int16_t)(uint16_t)((uint16_t)width*2);}
    if ((int16_t)targetUnitSlot<COMBAT_FRIENDLY_SLOTS && (targetUnitSlot!=COMBAT_SPECIAL_TARGET_SLOT || sideObjectiveRules[0]!=OBJECTIVE_SPECIAL_TARGET))
    {
        BtechDrawStyledRectangle(x,y,(uint16_t)width,height);
        BtechDrawStyledRectangle((uint16_t)(x-2),(uint16_t)(y-2),(uint16_t)(width+4),(uint16_t)(height+4));
    }
    else BtechDrawStyledRectangle(x,y,(uint16_t)width,height);
    screenPolygonNeedsClip=0;
}

/*0810:6DBA. Armour+internal structure condition drives authored diagram boxes.
 * Rear centre torso remains real stored armour but has no diagram region. */
void BtechDrawTargetDamagePanel(uint16_t targetUnitSlot,const BtechScreenRectangle *panel)
{
    if (!panel || hudColourSchemeIndex>=COMBAT_HUD_COLOUR_SCHEMES) abort();
    uint16_t diagram;
    if (targetUnitSlot==COMBAT_SPECIAL_TARGET_SLOT)
    {
        int16_t condition=(int16_t)specialTargetDamageRemaining;
        const BtechHudColourScheme *colours=&hudColourSchemes[hudColourSchemeIndex];
        uint16_t colour=condition==SPECIAL_TARGET_INITIAL_DAMAGE?colours->undamaged:
            condition>SPECIAL_TARGET_INITIAL_DAMAGE/2?colours->aboveHalfCondition:condition?colours->damaged:colours->destroyed;
        drawFillColour=(uint8_t)colour;drawLineColour=(uint8_t)colour;
        BtechDrawStyledRectangle((uint16_t)(panel->x+5),(uint16_t)(panel->y+1),(uint16_t)(panel->width-10),(uint16_t)(panel->height-2));
        diagram=COMBAT_SPECIAL_TARGET_SLOT;
    }
    else
    {
        if (targetUnitSlot>=COMBAT_MECH_SLOTS) abort();
        BtechMechState *mech=unitBindings[targetUnitSlot].mech;
        if (!mech || mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
        const BtechMechTemplate *chassis=&mechTemplates[mech->templateIndex];
        drawLineColour=Combat_Pal_EGA_LightGrey;drawFillColour=Combat_Pal_EGA_LightGrey;
        BtechDrawStyledRectangle((uint16_t)(panel->x+5),(uint16_t)(panel->y+1),(uint16_t)(panel->width-10),(uint16_t)(panel->height-2));
        for (uint16_t armor=0;armor<COMBAT_ARMOR_LOCATIONS;++armor)
        {
            if (armor==COMBAT_ARMOR_REAR_CENTER_TORSO) continue;
            uint16_t body=armorToBodyLocations[armor];if (body>=COMBAT_BODY_LOCATIONS) abort();
            int16_t remaining=(int16_t)(uint16_t)(mech->currentArmor[armor]+mech->currentInternalStructure[body]);
            int16_t maximum=(int16_t)(uint16_t)(chassis->maximumArmor[armor]+chassis->maximumInternalStructure[body]);
            const BtechHudColourScheme *colours=&hudColourSchemes[hudColourSchemeIndex];
            int16_t halfMaximum=(int16_t)((maximum-(maximum<0))/2);
            uint16_t colour=remaining==maximum?colours->undamaged:remaining>halfMaximum?colours->aboveHalfCondition:remaining?colours->damaged:colours->destroyed;
            if (mech->templateIndex>=COMBAT_MECH_TEMPLATES) abort();
            const BtechScreenRectangle *region=&targetDiagramRegions[mech->templateIndex][armor];
            drawFillColour=(uint8_t)colour;drawLineColour=(uint8_t)colour;
            const BtechHudGaugeAnchor *anchor=&targetDiagramAnchors[mech->templateIndex];
            BtechDrawStyledRectangle((uint16_t)(region->x-anchor->x+panel->x),(uint16_t)(region->y-anchor->y+panel->y),region->width,region->height);
        }
        diagram=mech->templateIndex;
    }
    if (!smallMechPictures || diagram>=smallMechPictures->imageCount) abort();
    BtechDrawBitmap(&smallMechPictures->images[diagram],(int16_t)(uint16_t)(panel->x+1),(int16_t)(uint16_t)(panel->y+1));
    BtechCopyHudRectangleIfReady(panel->x,panel->y,(uint16_t)(panel->width+2),(uint16_t)(panel->height+2));
}
