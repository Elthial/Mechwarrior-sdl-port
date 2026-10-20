#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/*17FB:2518. Signed WORD heuristic, not Battle Value. Pass two deliberately
 * reads the TRANSPOSED distance matrix. Failed selection leaves target stale. */
void BtechScoreAndSelectTacticalTarget(void)
{
    if (!activeUnitControl || activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) abort();
    BtechUnitControlState *active=activeUnitControl;
    uint8_t eligible[COMBAT_MECH_SLOTS],anyEligible=0;
    int16_t maximumDistanceLow=0,maximumValueDifference=0,bestScore=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechUnitControlState *candidate=&unitControls[slot];
        eligible[slot]=unitBindings[slot].mech && candidate->sideIndex!=active->sideIndex
            && candidate->positionKnowledgeCode!=COMBAT_POSITION_UNKNOWN && candidate->combatValueOrBoundaryCode>0;
        if (!eligible[slot]) continue;
        anyEligible=1;
        int32_t distance=unitDistances[active->unitSlot][slot];
        if (distance>=maximumDistanceLow) maximumDistanceLow=(int16_t)distance;
        int16_t difference=active->combatValueOrBoundaryCode-candidate->combatValueOrBoundaryCode;
        int16_t magnitude;
        if ((uint16_t)difference==0x8000) magnitude=INT16_MAX;
        else if (active->combatValueOrBoundaryCode<candidate->combatValueOrBoundaryCode)
            magnitude=(int16_t)(candidate->combatValueOrBoundaryCode-active->combatValueOrBoundaryCode);
        else magnitude=difference;
        if (magnitude>maximumValueDifference) maximumValueDifference=magnitude;
    }
    if (!anyEligible) {active->controlState=COMBAT_TACTICAL_UNENGAGED;return;}
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!eligible[slot]) continue;
        BtechUnitControlState *candidate=&unitControls[slot];
        int16_t candidateValue=candidate->targetSelectionCount?0:candidate->combatValueOrBoundaryCode;
        int16_t difference=(int16_t)(active->combatValueOrBoundaryCode-candidateValue),score=0;
        if (difference>TACTICAL_TARGET_VALUE_FLOOR)
        {
            int16_t distanceHeadroom=maximumDistanceLow-(uint16_t)unitDistances[slot][active->unitSlot];
            int16_t magnitude=(uint16_t)difference==0x8000?INT16_MAX:difference<0?-difference:difference;
            int16_t valueHeadroom=maximumValueDifference-magnitude;
            uint16_t shift=difference<0?TACTICAL_STRONGER_VALUE_SHIFT:TACTICAL_WEAKER_VALUE_SHIFT;
            uint16_t preference=(uint16_t)BtechShiftSignedDwordRight((uint32_t)valueHeadroom,shift);
            preference-=(uint16_t)(candidate->targetSelectionCount<<TACTICAL_SELECTION_PENALTY_SHIFT);
            preference+=(uint16_t)BtechShiftSignedDwordRight((uint32_t)distanceHeadroom,TACTICAL_DISTANCE_SCORE_SHIFT);
            score=(int16_t)(preference+TACTICAL_SCORE_BASE);
            if (candidate->positionKnowledgeCode==COMBAT_POSITION_REMEMBERED)
                score=(int16_t)(((uint16_t)score>>1)|((uint16_t)score&0x8000));
        }
        if (score>bestScore) {bestScore=score;active->targetUnitSlot=slot;}
    }
    if (bestScore>0) ++unitControls[active->targetUnitSlot].targetSelectionCount;
    else active->controlState=TACTICAL_AVOIDING;
}

/*17FB:2741. Either pre-existing latch prevents updates. Both may be set;
 * objective-exit testing can mutate a latch before the survivor fallback. */
void BtechUpdateSideCompletionFlags(void)
{
    if (sideCompletionFlags[0] || sideCompletionFlags[1]) return;
    for (uint16_t side=0;side<2;++side)
        if (BtechTestSideEliminationOrExitObjective(side<1)
            || (sideObjectiveRules[side]==OBJECTIVE_SPECIAL_TARGET && !specialTargetDamageRemaining)) sideCompletionFlags[side]=1;
    if (sideCompletionFlags[1] || sideCompletionFlags[0]) return;
    sideCompletionFlags[1]=1;sideCompletionFlags[0]=1;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
        if (unitBindings[slot].mech && unitControls[slot].combatValueOrBoundaryCode>0)
            sideCompletionFlags[unitControls[slot].sideIndex<1]=0;
}

/*17FB:27F2. Withdrawal success sets THIS side but returns not-eliminated.
 * Only -1 is skipped. Boundary index is signed negation after WORD add2. */
uint8_t BtechTestSideEliminationOrExitObjective(uint16_t sideIndex)
{
    if (sideIndex>=2) abort();
    uint8_t noUnitsRemaining=1;
    uint16_t rule=sideObjectiveRules[sideIndex];
    uint8_t allowedBoundaryMask=scenarioSelectors.sideBoundaryMasks[sideIndex];
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechUnitControlState *control=&unitControls[slot];
        if (!unitBindings[slot].mech || control->sideIndex!=sideIndex || control->combatValueOrBoundaryCode==COMBAT_MECH_DISABLED_VALUE) continue;
        int16_t boundary=0-(uint16_t)(control->combatValueOrBoundaryCode+2);
        if (boundary<0) noUnitsRemaining=0;
        else
        {
            if (boundary>=TACTICAL_BOUNDARY_CODES) abort(); /* Adjacent authored0EB6 table alias. */
            if ((allowedBoundaryMask&boundaryMaskByCode[boundary]) && rule==OBJECTIVE_WITHDRAW)
            {
                sideCompletionFlags[sideIndex]=1;
                noUnitsRemaining=0;
            }
        }
    }
    return noUnitsRemaining;
}
