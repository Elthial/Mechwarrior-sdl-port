# Planets, markets and arrival

Each campaign world has a name, description, Great House allegiance, map
position, market conditions, scenery, available facilities and mission
eligibility. Travel also uses the world's contribution to journey time.

## Methods and responsibilities

| Method | Gameplay purpose |
| --- | --- |
| `MwApplyCurrentPlanetState()` | Applies the current world's House allegiance, scenery, facilities and market conditions. Recalculates ammunition prices and sets the contract-negotiation limit. |
| `MwFinishPlanetArrivalContinuation()` | Makes the destination the current world, applies its conditions, refreshes Mechs for sale, contract offers and available recruits, then returns to the planet hub after the transport sequence. |
| `MwEnterCampaignPlanetHub()` | Presents the current world and its available destinations: Mech Complex, Bar, company facilities and House office or contract deployment. Processes pending campaign notifications and events. |
| `MwRefreshMarketUnitStock()` | Replenishes the local Mech market using the current market conditions. |
| `MwRefreshMissionOffers()` | Generates the available mercenary contracts. |
| `MwRefreshAvailablePilotRecruits()` | Refreshes the MechWarriors available for hire. |

Applying a world's conditions does not itself refresh every local service;
arrival performs those additional steps. The scenery used behind the planet
hub is not the mission's terrain tile or world-object layout.

## Choosing a destination

`MwFindPlanetAtTravelMarker()` identifies the world under the travel cursor.
`MwCalculatePlanetTravelDistance()` determines campaign-map distance to the
chosen world. `MwDrawTravelDestinationDetails()` presents the destination's
description, population and travel information. These use campaign worlds,
not combat positions or BattleMech navigation waypoints.

Travel payment, elapsed time and arrival are explained in
[Travel and the mercenary company's finances](CAMPAIGN_REPAIR_MECHANICS.md).
