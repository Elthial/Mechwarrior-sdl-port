# Campaign gameplay: running the mercenary company

`MwRunCampaign()` runs the campaign. It brings together the company, current
world, services, events and contracts; it is not the real-time combat loop.

## Arriving on a world

`MwFinishPlanetArrivalContinuation()` completes the move to the destination,
applies its local conditions and refreshes the Mech market, available contracts
and recruits. `MwEnterCampaignPlanetHub()` then presents the world and its
services. `MwApplyCurrentPlanetState()` supplies the House allegiance, scenery,
facilities, market conditions, ammunition prices and negotiation limit.

[Planets and arrival](gameplay/CAMPAIGN_PLANET_SCHEMA.md) explains those methods.

## Preparing the lance

Use the Mech Complex to inspect, repair and rearm company-owned Mechs. Repair
quotes belong to the selected Mech's condition; rearming replenishes ammunition
rather than repairing systems. The Bar provides recruitment, while the crew
screen assigns MechWarriors to owned Mechs. The starting Jenner is damaged.

- [Repair, rearm, trading and expenses](gameplay/CAMPAIGN_REPAIR_MECHANICS.md).
- [Recruitment and Mech assignments](gameplay/PILOT_SCHEMAS.md).
- [Campaign condition versus combat damage](gameplay/MECH_CONDITION_LOADOUT.md).

## Taking a contract

The House office offers mercenary work. Contract methods handle the objective,
payment, salvage and advance separately. Negotiation precedes acceptance;
deployment checks follow it. An accepted contract is not a guarantee that the
company is ready to deploy.

[Contracts and deployment](gameplay/SCENARIO_UNIT_SCHEMAS.md) names the methods
for browsing, negotiating, accepting and deploying.

## Between missions

Travel spends company funds and advances time. The campaign calendar drives
wages and refreshes opportunities. Save/load preserves the company's campaign
state. Combat returns its results to this campaign rather than replacing the
company with a new test roster. Full-campaign acceptance is still incomplete;
read [current status](RUNTIME_STATUS.md) before relying on every mission path.
