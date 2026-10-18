# Mech repair, rearm and company expenses

The Mech Complex handles the condition of company-owned BattleMechs. Repair,
ammunition replenishment and buying/selling are separate transactions.

## Repair methods

| Method | Gameplay purpose |
| --- | --- |
| `MwOpenOwnedMechList()` | Opens the company's owned-Mech list. |
| `MwOpenSelectedMechDetailContinuation()` | Opens the selected Mech's condition and service screen. |
| `MwOpenMechComponentRepairMenuCallback()` | Presents damaged systems and weapons for individual repair. |
| `MwQuoteRepairComponent()` | Calculates the repair quote for a selected damaged component. |
| `MwQuoteMechRepairTotal()` | Calculates the total quote for the Mech's outstanding damage. |
| `MwRepairSelectedMechComponentCallback()` | Attempts the selected repair, checks available C-bills and applies its cost and elapsed time. |
| `MwRepairAllSelectedMechComponentsCallback()` | Attempts all quoted repairs, refunds the quoted cost of unfinished work and reports whether repair is complete. |
| `MwResetRepairAttemptFailures()` | Clears remembered failed repair attempts when re-entering the planet hub. |

Repair availability depends on local market conditions. A failed repair is not
proof that a component is permanently destroyed. Individual repairs and Repair
All do not follow identical time-handling rules; the port preserves that game
behaviour rather than imposing a new workshop simulation.

## Rearming and trading

`MwQuoteSelectedMechRearm()` quotes the selected Mech's ammunition needs;
`MwRearmSelectedMech()` performs the rearm transaction. Neither repairs an
engine, actuator or weapon. `MwCalculateMarketPurchasePrice()` prices a Mech
for purchase, while `MwCalculateSelectedUnitNetPrice()` determines the selected
unit's net trading price. Company ownership, ammunition reserves and C-bills
remain separate records.

## Travel and wages

`MwConfirmPlanetTravelContinuation()` checks and deducts the travel quote.
`MwAdvancePaidTravelAndJoinPlanetArrivalContinuation()` advances the journey
towards arrival. `MwAdvanceCampaignTimeAndChargeWages()` updates the calendar,
charges MechWarrior wages and applies the due market, recruitment and contract
refreshes. Owning a DropShip adds its own condition/upkeep considerations;
transport selection does not make interstellar travel free of consequences.
