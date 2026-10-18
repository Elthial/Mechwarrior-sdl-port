#include "mechwarrior/combat_scene.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/*17FB:2B1A. Match enemy parts by POSITION, not object identity. The native
 * rewrite consumes six slots even for unknown chassis and short list tails. */
void BtechOrderMechRenderParts(BtechCameraContext *camera,BtechWorldObjectPrefix **objects,int16_t count)
{
    for (int16_t index=0;index<count;++index)
    {
        if (index>=COMBAT_WORLD_OBJECTS || !objects[index]) abort();
        if ((int16_t)objects[index]->resourceId<(int16_t)firstMechShapeResourceId) continue;
        for (uint16_t slot=1;slot<COMBAT_MECH_SLOTS;++slot)
        {
            BtechUnitBinding *binding=&unitBindings[slot];
            if (!binding->torso) continue;
            const BtechPosition3D *position=&binding->torso->object.prefix.position;
            const BtechPosition3D *candidate=&objects[index]->position;
            if (position->positionA!=candidate->positionA || position->positionB!=candidate->positionB || position->positionC!=candidate->positionC) continue;
            if (!camera || !camera->viewObject) abort();
            uint32_t deltaA=position->positionA-camera->viewObject->position.positionA;
            uint32_t deltaB=position->positionB-camera->viewObject->position.positionB;
            uint16_t heading=BtechCalculateVectorHeadingFar(deltaA,deltaB);
            int16_t relativeHeading=(int16_t)(uint16_t)(heading-binding->torso->object.prefix.orientationC+0x8000);
            if (!binding->mech) abort();
            int16_t chassis=(int16_t)binding->mech->templateIndex;
            if (index+5>=COMBAT_WORLD_OBJECTS) abort(); /* Native following-owner overwrite, not a count-tail guard. */
            if (chassis>=0 && chassis<COMBAT_MECH_TEMPLATES)
            {
                if (relativeHeading>=0)
                {
                    objects[index]=binding->rightArm?&binding->rightArm->object.prefix:NULL;
                    objects[index+1]=binding->rightLeg?&binding->rightLeg->object.prefix:NULL;
                    objects[index+4]=binding->positionedLeftLeg?&binding->positionedLeftLeg->object.prefix:NULL;
                    objects[index+5]=binding->leftArm?&binding->leftArm->object.prefix:NULL;
                }
                else
                {
                    objects[index]=binding->leftArm?&binding->leftArm->object.prefix:NULL;
                    objects[index+1]=binding->positionedLeftLeg?&binding->positionedLeftLeg->object.prefix:NULL;
                    objects[index+4]=binding->rightLeg?&binding->rightLeg->object.prefix:NULL;
                    objects[index+5]=binding->rightArm?&binding->rightArm->object.prefix:NULL;
                }
                uint16_t magnitude=relativeHeading==INT16_MIN?INT16_MAX:(relativeHeading<0?0-(uint16_t)relativeHeading:(uint16_t)relativeHeading);
                if (magnitude<0x4000)
                {
                    objects[index+2]=&binding->torso->object.prefix;
                    objects[index+3]=binding->head?&binding->head->object.prefix:NULL;
                }
                else
                {
                    objects[index+2]=binding->head?&binding->head->object.prefix:NULL;
                    objects[index+3]=&binding->torso->object.prefix;
                }
            }
            for (uint16_t part=0;part<5;++part)
            {if (!objects[index+part]) abort();objects[index+part]->renderFlags&=(uint8_t)~OBJECT_LAST_MECH_DRAW_PART;}
            if (!objects[index+5]) abort();
            objects[index+5]->renderFlags|=OBJECT_LAST_MECH_DRAW_PART;
            unitLastDrawParts[slot]=(BtechPooledWorldObject *)objects[index+5];
            index+=5;break;
        }
    }
}
