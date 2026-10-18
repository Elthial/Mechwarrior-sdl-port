#ifndef MECHWARRIOR_CAMPAIGN_CONTINUATIONS_H
#define MECHWARRIOR_CAMPAIGN_CONTINUATIONS_H

/* Original MW_MAIN.EXE code offsets in segment 081B. These WORDs transport
 * native JMP/shared-entry destinations through the C dispatcher: they are not
 * AX return values, campaign outcome codes, data addresses or new procedures.
 * Keep distinct entries distinct even when they share one C procedure body. */
enum MwCampaignContinuation
{
    CAMPAIGN_CONTINUE_CLEANUP = 0x015F, /* Shared MwRunCampaign shutdown tail. */
    CAMPAIGN_CONTINUE_UI = 0x089B, /* MwCampaignUiEventLoop: resume menu input. */
    CAMPAIGN_CONTINUE_COPY_PROTECTION = 0x166D, /* Protection/new-campaign setup entry. */
    CAMPAIGN_CONTINUE_INITIALIZE_COMPANY = 0x181A, /* Same setup body, after protection. */
    CAMPAIGN_CONTINUE_PLANET_HUB = 0x1909, /* Enter hub and process arrival/events. */
    CAMPAIGN_CONTINUE_HUB_MENU = 0x1A0D, /* Hub menu after event dispatch; no re-entry work. */
    CAMPAIGN_CONTINUE_CANCEL_CONTRACT = 0x1B87, /* Deployment-refusal cancellation. */
    CAMPAIGN_CONTINUE_DEPLOYMENT = 0x1C3E, /* Resolve deployment or export combat exchange. */
    CAMPAIGN_CONTINUE_EVENT_DEPLOYMENT = 0x1CCC, /* Event combat, without refusal penalty. */
    CAMPAIGN_CONTINUE_AFTER_ACTION = 0x1CD4, /* Debrief inside MwRunCampaign. */
    CAMPAIGN_CONTINUE_CREW_MENU = 0x260C, /* Rebuild the company's crew screen. */
    CAMPAIGN_CONTINUE_PILOT_ASSIGNMENT_INPUT = 0x2911, /* Poll the selected pilot's Mech assignment. */
    CAMPAIGN_CONTINUE_CONFIRM_PILOT_ASSIGNMENT = 0x2948, /* Accept assignment or resolve a conflict. */
    CAMPAIGN_CONTINUE_MECH_COMPLEX = 0x2DA6, /* Root Mech Complex menu. */
    CAMPAIGN_CONTINUE_OWNED_MECH_LIST = 0x2E28, /* List company-owned machines. */
    CAMPAIGN_CONTINUE_MARKET_MECH_LIST = 0x2F32, /* List dealer stock. */
    CAMPAIGN_CONTINUE_OPEN_OWNED_MECH = 0x3063, /* Open selected owned-unit detail. */
    CAMPAIGN_CONTINUE_RETURN_TO_OWNED_MECH = 0x3091, /* Restore detail after a modal operation. */
    CAMPAIGN_CONTINUE_OWNED_MECH_DETAIL = 0x30A2, /* Draw selected machine's condition and services. */
    CAMPAIGN_CONTINUE_OWNED_MECH_SERVICE_MENU = 0x3291, /* Shared cancel tail; skip detail drawing. */
    CAMPAIGN_CONTINUE_COMPONENT_REPAIR_MENU = 0x38FC, /* Select a damaged component for repair. */
    CAMPAIGN_CONTINUE_MARKET_MECH_DETAIL = 0x4353, /* Dealer detail after the entry's POP. */
    CAMPAIGN_CONTINUE_CONTRACT_OFFICE_REDRAW = 0x4AA8, /* Redraw without charging another day. */
    CAMPAIGN_CONTINUE_MISSION_BRIEFING = 0x4B55, /* Draw the negotiating mission offer. */
    CAMPAIGN_CONTINUE_CREDIT_CONTRACT_ADVANCE = 0x55A5, /* Credit accepted advance C-bills. */
    CAMPAIGN_CONTINUE_ADJUST_ENEMY_FORCE = 0x55D8, /* Adjust the accepted contract's opposition. */
    CAMPAIGN_CONTINUE_SELECT_TRAVEL_DESTINATION = 0x569F, /* Select planet and quote travel. */
    CAMPAIGN_CONTINUE_TRAVEL_MAP = 0x56E7, /* Resume map menu without saving the marker again. */
    CAMPAIGN_CONTINUE_PAID_TRAVEL = 0x5EA0, /* Charge travel and advance the campaign calendar. */
    CAMPAIGN_CONTINUE_PLANET_ARRIVAL = 0x5F15, /* Finish transport and publish the arrival. */
    CAMPAIGN_CONTINUE_NEXT_EVENT = 0x6290, /* Resume dispatcher after its suspended event call. */
    CAMPAIGN_CONTINUE_VICTORY_EVENT_RETURN = 0x629F, /* Return from fixed event to the campaign victory flow. */
    CAMPAIGN_CONTINUE_EVENT_QUIT = 0x683D, /* Stop music/set launcher result, then shared cleanup. */
    CAMPAIGN_CONTINUE_RESTART = 0x6855, /* Restart campaign, retaining its native frame contract. */
    CAMPAIGN_CONTINUE_EVENT_COMBAT = 0x6913, /* Prepare the story event's combat scenario. */
    CAMPAIGN_CONTINUE_EVENT_COMBAT_RETURN = 0x692B, /* Inherited STC/RET; event caller ignores carry. */
    CAMPAIGN_CONTINUE_BAR_BACKGROUND = 0x6950, /* Return to Bar without restarting music. */
    CAMPAIGN_CONTINUE_BAR_REFRESH = 0x698F, /* Refresh Bar offers/menu without redrawing scenery. */
    CAMPAIGN_CONTINUE_OPTIONS = 0x6CFA, /* Enter options with scenery setup. */
    CAMPAIGN_CONTINUE_OPTIONS_REDRAW = 0x6D23, /* Options redraw after confirmation NO. */
    CAMPAIGN_CONTINUE_QUIT_FROM_OPTIONS = 0x6D4E, /* Confirmation YES: POP/JMP shared cleanup. */
    CAMPAIGN_CONTINUE_OPTIONS_FROM_SUBMENU = 0x6E6A, /* Native PUSH/JMP options re-entry. */
    CAMPAIGN_CONTINUE_RESTART_FROM_OPTIONS = 0x70B7 /* Confirmation YES: balance frame before restart. */
};

#endif
