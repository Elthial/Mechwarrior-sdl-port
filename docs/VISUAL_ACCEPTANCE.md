# Checking gameplay presentation

Screenshots complement gameplay checks; they do not prove that repairs, payment,
damage or mission results were applied correctly. Compare the same situation in
the original and port, recording company state and actions as well as images.

## Campaign coverage

Check distinct planet hubs, Bar/recruitment, crew assignments, contracts and
negotiation, damaged/functional Mechs, component/all repairs, rearming, trading,
travel/arrival and save/load. Compare the visible information with changes to
C-bills, ammunition, Mech condition, wages, assignments and the calendar.

## Combat coverage

Check briefing, cockpit, target selection, movement, torso rotation, weapons,
heat, damage, command screen and map. Include different chassis and landscapes,
jump-capable Mechs, impacts, destruction, retreat and mission completion.
Compare motion/effects over sequences, not just a stationary frame.

## Captures and their limits

F12 records actual SDL output with supporting state. Keep raw comparison sets
under ignored local capture directories. Do not upload assets or saves. The
documentation gallery contains selected real captures, not matched-original
goldens or proof that all acceptance cases passed.

For exact graphics comparison use equal native framebuffer dimensions and
palettes without filtering, CRT effects or reference-image resizing. A deliberately
different simulation state cannot be fixed with a generous pixel tolerance.
Audio needs timing/listening checks as well as recorded samples.

Ask before taking control of the shared desktop for a live session. Detached
off-screen UI checks and analysis of existing captures do not take user input.
Current playable limits are in [runtime status](RUNTIME_STATUS.md); detailed
previous capture records are historical evidence.
