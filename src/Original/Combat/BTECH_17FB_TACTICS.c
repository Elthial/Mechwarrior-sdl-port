#include "mechwarrior/port_diagnostics.h"
#include "mechwarrior/combat_tactics.h"
#include "mechwarrior/combat_import.h"
#include "mechwarrior/combat_ballistics.h"
#include "mechwarrior/combat_startup.h"
#include <stdlib.h>

/*17FB:2518. Signed WORD heuristic, not Battle Value. Pass two deliberately
 * reads the TRANSPOSED distance matrix. Failed selection leaves target stale. */
void BtechScoreAndSelectTacticalTarget(void)
{
    if (!activeUnitControl || activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: !activeUnitControl || activeUnitControl->unitSlot>=COMBAT_MECH_SLOTS");
    BtechUnitControlState *active=activeUnitControl;
    uint8_t eligible[COMBAT_MECH_SLOTS],anyEligible=0;
    int16_t maximumDistanceLow=0,maximumValueDifference=0,bestScore=0;
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        BtechUnitControlState *candidate=&unitControls[slot];
        eligible[slot]=unitBindings[slot].mech && candidate->sideIndex!=active->sideIndex
            && candidate->positionKnowledgeCode!=COMBAT_POSITION_UNKNOWN && candidate->conditionScoreOrExitCode>COMBAT_CONDITION_SCORE_MINIMUM;
        if (!eligible[slot]) continue;
        anyEligible=1;
        int32_t distance=unitDistances[active->unitSlot][slot];
        if (distance>=maximumDistanceLow) maximumDistanceLow=(int16_t)distance;
        int16_t difference=active->conditionScoreOrExitCode-candidate->conditionScoreOrExitCode;
        int16_t magnitude;
        if ((uint16_t)difference==0x8000) magnitude=INT16_MAX;
        else if (active->conditionScoreOrExitCode<candidate->conditionScoreOrExitCode)
            magnitude=(int16_t)(candidate->conditionScoreOrExitCode-active->conditionScoreOrExitCode);
        else magnitude=difference;
        if (magnitude>maximumValueDifference) maximumValueDifference=magnitude;
    }
    if (!anyEligible) {active->tacticalState=COMBAT_TACTICAL_UNENGAGED;return;}
    for (uint16_t slot=0;slot<COMBAT_MECH_SLOTS;++slot)
    {
        if (!eligible[slot]) continue;
        BtechUnitControlState *candidate=&unitControls[slot];
        int16_t candidateValue=candidate->targetSelectionCount?0:candidate->conditionScoreOrExitCode;
        int16_t difference=(int16_t)(active->conditionScoreOrExitCode-candidateValue),score=0;
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
    else active->tacticalState=TACTICAL_AVOIDING;
}

/*17FB:2741. Either pre-existing latch prevents updates. Both may be set;
 * objective-exit testing can mutate a latch before the survivor fallback. */
void BtechUpdateSideCompletionFlags(void)
{
    if (sideCompletionFlags[0] || sideCompletionFlags[1]) return;
    /* Side 0 tests side 1, and vice versa; the callee can also set its own latch. */
    for (uint16_t benefitingSideIndex=0;benefitingSideIndex<2;++benefitingSideIndex)
        if (BtechTestSideEliminationOrExitObjective(benefitingSideIndex<1)
            || (sideObjectiveRules[benefitingSideIndex]==OBJECTIVE_SPECIAL_TARGET && !specialTargetDamageRemaining)) sideCompletionFlags[benefitingSideIndex]=1;
    if (sideCompletionFlags[1] || sideCompletionFlags[0]) return;
    sideCompletionFlags[1]=1;sideCompletionFlags[0]=1;
    for (uint16_t survivingUnitSlot=0;survivingUnitSlot<COMBAT_MECH_SLOTS;++survivingUnitSlot)
        if (unitBindings[survivingUnitSlot].mech && unitControls[survivingUnitSlot].conditionScoreOrExitCode>COMBAT_CONDITION_SCORE_MINIMUM)
            sideCompletionFlags[unitControls[survivingUnitSlot].sideIndex<1]=0;
}

/*17FB:27F2. Withdrawal success sets THIS side but returns not-eliminated.
 * Only -1 is skipped. Boundary index is signed negation after WORD add2. */
uint8_t BtechTestSideEliminationOrExitObjective(uint16_t sideIndex)
{
    if (sideIndex>=2) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: sideIndex>=2");
    uint8_t noMechsStillInBattle=1;
    uint16_t sideObjectiveRule=sideObjectiveRules[sideIndex];
    uint8_t allowedExitBoundaryMask=scenarioSelectors.sideBoundaryMasks[sideIndex];
    for (uint16_t unitSlot=0;unitSlot<COMBAT_MECH_SLOTS;++unitSlot)
    {
        BtechUnitControlState *evaluatedUnitControl=&unitControls[unitSlot];
        if (!unitBindings[unitSlot].mech || evaluatedUnitControl->sideIndex!=sideIndex || evaluatedUnitControl->conditionScoreOrExitCode==COMBAT_MECH_DISABLED_VALUE) continue;
        int16_t exitBoundaryIndex=0-(uint16_t)(evaluatedUnitControl->conditionScoreOrExitCode+COMBAT_EXIT_CODE_OFFSET);
        if (exitBoundaryIndex<0) noMechsStillInBattle=0;
        else
        {
            if (exitBoundaryIndex>=TACTICAL_BOUNDARY_CODES) /* Port failsafe; not an original DOS exit. */ MW_PORT_FAILSAFE("Rejected condition: exitBoundaryIndex>=TACTICAL_BOUNDARY_CODES"); /* Adjacent authored0EB6 table alias. */
            if ((allowedExitBoundaryMask&boundaryMaskByCode[exitBoundaryIndex]) && sideObjectiveRule==OBJECTIVE_WITHDRAW)
            {
                sideCompletionFlags[sideIndex]=1;
                noMechsStillInBattle=0;
            }
        }
    }
    return noMechsStillInBattle;
}
