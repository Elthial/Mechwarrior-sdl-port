# Contracts, deployment and combat forces

A contract joins the mercenary company's campaign to a combat mission. It
specifies an objective and negotiated terms; the combat scenario supplies the
battlefield and forces. A planet record is not a complete mission definition.

## Contract methods

| Method | Gameplay purpose |
| --- | --- |
| `MwRequestMissionOfferContinuation()` | Requests an available mercenary contract. |
| `MwNextMissionOfferContinuation()` | Browses another offer. |
| `MwDrawMissionContractBriefingContinuation()` | Presents the offered objective and contract briefing. |
| `MwEditContractPaymentRequestContinuation()` | Changes the requested payment. |
| `MwEditContractSalvageRequestContinuation()` | Changes the requested salvage terms. |
| `MwEditContractAdvanceRequestContinuation()` | Changes the requested advance. |
| `MwNegotiateAndAcceptContractContinuation()` | Resolves negotiation and acceptance. |
| `MwCreditAcceptedContractAdvanceContinuation()` | Credits the accepted advance to company funds. |
| `MwCheckAssignedMechDeploymentReadiness()` | Checks the commander's and assigned Mechs' readiness for deployment. |
| `MwAttemptContractDeployment()` | Attempts to deploy under the accepted contract. |

Payment, salvage and advance are distinct terms. Local House conditions affect
negotiation opportunities. Failure to deploy can have contract consequences;
the port must not bypass readiness simply to reach a test battle.

## Mission preparation and return

`BtechDecodeCampaignImport()` reads the company's combat handoff.
`BtechPrepareCampaignCombatStage()` prepares the mission using the imported
company state and scenario. `BtechPrepareEnemyMech()` prepares opposing Mechs.
`BtechExportCombatResultsAndTransition()` returns combat results to the campaign.

`BtechUnitControlState` holds a combatant's orders, target, remembered enemy
position and manoeuvre state. That is different from its chassis specification
or current armour. The special mission target is not an additional Mech slot:
destroying an objective building and destroying an enemy BattleMech follow
different gameplay paths.
