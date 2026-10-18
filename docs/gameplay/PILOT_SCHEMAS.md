# MechWarriors, recruitment and assignments

A MechWarrior has an identity, Gunnery and Piloting ability, wages and an
assignment to a company-owned Mech. The crew roster is not the Mech inventory:
removing a pilot does not remove the BattleMech they were assigned to.

## Recruitment and company methods

| Method | Gameplay purpose |
| --- | --- |
| `MwEnterBarContinuation()` | Opens the Bar's drinks and recruitment choices. |
| `MwOrderDrinkContinuation()` | Handles ordering a drink and the associated Bar interaction. |
| `MwOpenRecruitInterviewContinuation()` | Presents the available recruit for an interview. |
| `MwHireSelectedRecruitContinuation()` | Hires the selected MechWarrior into the company roster. |
| `MwRejectSelectedRecruitContinuation()` | Declines the selected recruit. |
| `MwEnterCrewMenuContinuation()` | Opens the company's crew roster. |
| `MwBeginPilotMechAssignmentContinuation()` | Starts choosing an owned BattleMech for the selected MechWarrior. |
| `MwConfirmPilotMechAssignmentContinuation()` | Confirms the chosen Mech assignment. |
| `MwTransferPreviousMechToConflictingPilot()` | Gives the displaced MechWarrior the previous assignment when an assignment swap is required. |
| `MwDismissSelectedPilotContinuation()` | Dismisses the selected MechWarrior. |
| `MwAdvancePilotSkillFromExperience()` | Applies the game's experience-based skill progression. |

MechWarrior identity, roster position and owned-Mech selection are different
identifiers. A lance deployed to combat is assembled from assignments; changing
a menu row must not be confused with changing a pilot's identity or chassis.

`MwAdvanceCampaignTimeAndChargeWages()` handles due wages as campaign time
passes. `MwRefreshAvailablePilotRecruits()` refreshes recruitment opportunities.
