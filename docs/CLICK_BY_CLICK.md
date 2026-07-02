# RELIQUARY — Click-by-Click Editor Walkthrough

Zero-assumed-knowledge instructions for the fiddly parts of `EDITOR_SETUP.md`. Follow top to bottom; each section is independent.

**Two rules that apply everywhere:**
- Editor names drop the C++ prefix: `URLGameInstance` appears as `RLGameInstance`, `ARLChallengeAltar` as `RLChallengeAltar`.
- After changing any Blueprint, click **Compile** (top-left of the Blueprint window, the button with the gear/checkmark), then **Save**. Do it every time. If Compile shows a red ✗, read the error at the bottom.

---

## A. HUD: bind widgets to the Get* functions

**Goal:** WBP_RunHUD shows the run timer, location, health/mana bars, mana count — and actually appears on screen.

### A1. Open the widget
1. Content Browser → `Content/UI` → double-click **WBP_RunHUD2** (or WBP_RunHUD — whichever you reparented).
2. Top-right corner of the window has two tabs: **Designer** and **Graph**. Click **Designer**.

### A2. Add a text block for the run timer
1. In the **Palette** panel (left side), type `text` in the search box.
2. Drag **Text** (the plain one, not Rich Text) onto the dark canvas area. Drop it near the top-center.
3. With it still selected, look at the **Details** panel (right side).
4. Find the row **Content → Text** (it says "Text Block"). At the far right of that row is a dropdown that says **Bind**.
5. Click **Bind**. A menu opens. Under **Functions** you'll see the functions from the C++ parent — click **GetRunTimerText**.

That's it — the text now live-updates every frame. It reads "Text Block" in the Designer (bindings only evaluate in game) — that's normal.

### A3. Repeat for the other text fields
Drag three more **Text** blocks and bind them the same way:
- one → Bind → **GetLocationName** (zone name)
- one → Bind → **GetHeroTitle** (name + level + class)
- one → Bind → **GetDangerLabel** (RoR2-style danger meter)

### A4. Excess mana (needs a conversion step)
`GetExcessMana` returns a number, not text, so it won't appear in the Bind list directly:
1. Drag another **Text** block onto the canvas.
2. Details → Content → Text → **Bind** → this time click **+ Create Binding** (top of the menu).
3. You're now in a little function graph with a purple **Return Node**.
4. Right-click on empty graph space → search `get excess mana` → click **Get Excess Mana**.
5. Drag a wire from that node's **Return Value** pin into the Return Node's **Return Value** pin. Unreal auto-inserts a green **ToText (Integer)** conversion node. Done.
6. Click the **Designer** tab to go back. Repeat the same trick with **GetRunResourceTotal** on another text block if you want a resource counter.

### A5. Health and mana bars
1. Palette → search `progress` → drag **Progress Bar** onto the canvas (bottom-left is traditional). Stretch it wide using the corner handles.
2. Details → **Progress → Percent** → **Bind** → **GetHealthFraction**.
3. To make it red: Details → **Appearance → Fill Color and Opacity** → click the color swatch → pick red.
4. Drag a second Progress Bar under it → Bind Percent → **GetManaFraction** → color it blue.
5. **Compile → Save.**

### A6. Actually put the HUD on screen (nothing spawns it yet!)
1. Content Browser → `Content/ThirdPerson/Blueprints` → double-click **BP_ThirdPersonCharacter** (your character BP).
2. Click the **Event Graph** tab.
3. Find the red **Event BeginPlay** node (if it's not there: right-click empty space → type `beginplay` → add it).
4. Right-click empty space → type `create widget` → click **Create Widget**.
5. On the new node, set the **Class** dropdown to **WBP_RunHUD2**.
6. Wire BeginPlay's white execution pin → Create Widget's input execution pin.
7. Drag off Create Widget's **Return Value** pin → release → type `add to viewport` → click **Add to Viewport**.
8. **Compile → Save.** Press Play — the HUD should be there.

---

## B. Input: the five ability keys

**Goal:** LMB / RMB / LShift / R / E fire Primary / Secondary / Utility / Special / Interact.

### B1. Create the five Input Action assets
1. Content Browser → `Content/Input` — you'll see the existing ones (IA_Jump, IA_Move...). If they're in an `Actions` subfolder, go in there. Stay in the same folder they use.
2. Right-click empty space in the Content Browser → **Input → Input Action**. Name it exactly `IA_Primary`.
3. Repeat four times: `IA_Secondary`, `IA_Utility`, `IA_Special`, `IA_Interact`.
4. You don't need to open them — the defaults (Digital/bool) are correct.

### B2. Add them to the mapping context
1. In `Content/Input`, double-click **IMC_Default** (the Input Mapping Context that already holds IA_Jump etc.).
2. In the **Mappings** section, click the **+** next to the word "Mappings". A new empty entry appears at the bottom.
3. The new entry has a dropdown that says **None** — click it, pick **IA_Primary**.
4. Under it there's a key selector row also saying **None**. Click *that* dropdown → in the search box type `left mouse` → pick **Left Mouse Button**.
   (Alternative: click the little keyboard icon next to the dropdown, then press the key you want — works for keyboard keys, not always for mouse buttons.)
5. Repeat the +, action, key for the rest:
   | Action | Key |
   |---|---|
   | IA_Secondary | Right Mouse Button |
   | IA_Utility | Left Shift |
   | IA_Special | R |
   | IA_Interact | E |
6. **Save.** Nothing else to register — since these live in the same IMC that already drives movement, the character picks them up automatically.

> Heads-up: LMB also rotates the camera in the template (IA_MouseLook or similar may be bound to it). If attacking also spins the camera weirdly, open IMC_Default and change the look mapping to only use **Mouse XY 2D-Axis / Mouse2D**, not the mouse *button*.

### B3. Tell the character which action is which
1. Open **BP_ThirdPersonCharacter** → click **Class Defaults** in the toolbar (top).
2. In the Details panel, search box at top: type `abilities`. The **Input|Abilities** category appears with five empty dropdowns.
3. Set: Primary Ability Action = **IA_Primary**, Secondary = **IA_Secondary**, Utility = **IA_Utility**, Special = **IA_Special**, Interact = **IA_Interact**.
4. **Compile → Save.** Press Play, click LMB near a tree — it should shatter into pickups.

---

## C. Enemy AI: chase and bite, no Behavior Tree needed

**Goal:** enemies run at you and call `DealTouchDamage` in melee range. We'll skip StateTree/BT entirely — a 6-node Blueprint loop does the job. Do this once in BP_Wolf, then copy-paste the nodes into the other enemy BPs.

### C1. One-time prerequisites
1. Your run map needs a **NavMeshBoundsVolume**: in the L_Run level, top toolbar → the cube-with-plus icon (**Quickly add to project**) → **Volumes → NavMeshBoundsVolume**. Drag it into the world, then in its Details set **Brush Settings → X/Y/Z** big enough to cover the whole playable ground (e.g. 20000 × 20000 × 3000).
2. Press **P** in the viewport — walkable ground turns green. No green = AI can't move. Save the level.

### C2. Set up the pawn to be AI-possessed
1. Open **BP_Wolf** → **Class Defaults**.
2. Details search: `possess` → set **Auto Possess AI** = **Placed in World or Spawned**.
3. Search `ai controller` → **AI Controller Class** should say **AIController** (the default). Leave it.

### C3. Build the chase loop (Event Graph)
Open the **Event Graph** tab and build this — node by node:

1. Right-click empty space → type `custom event` → **Add Custom Event...** → name it `Chase`.
2. Right-click → type `is dead` → add **Is Dead** (from the enemy parent class).
3. Right-click → type `branch` → add **Branch**. Wire: `Chase` exec → Branch input. Wire Is Dead's red **Return Value** → Branch's **Condition**. (If dead: do nothing — leave True unwired. Everything else comes off **False**.)
4. Right-click → type `get player pawn` → add **Get Player Pawn** (leave Player Index = 0).
5. Right-click → type `ai moveto` → add **AI MoveTo** (the one with multiple output pins: On Success / On Fail).
   - Wire Branch **False** → AI MoveTo input exec.
   - Wire Get Player Pawn **Return Value** → AI MoveTo **Target Actor**.
   - Set **Acceptance Radius** = `150`.
   - Leave **Pawn** empty or wire a `Get a reference to self` node (right-click → `self`) into it.
6. Right-click → type `deal touch damage` → add **Deal Touch Damage**.
   - Wire AI MoveTo **On Success** → Deal Touch Damage.
   - Wire Get Player Pawn **Return Value** → its **Target** pin.
7. Right-click → type `delay` → add **Delay**, set Duration = `1.0`.
   - Wire Deal Touch Damage exec → Delay.
   - Also wire AI MoveTo **On Fail** → the *same* Delay node (two wires into one input is fine).
8. Drag off Delay's **Completed** → release → type `chase` → pick **Chase** (your custom event). The loop is closed.
9. Finally: find/add red **Event BeginPlay** → wire it → **Chase**.

**Compile → Save.** The wolf now: runs to you → bites (once per second) → repeats; if it can't reach you it retries every second; stops forever when dead.

### C4. Copy to the other enemies
In BP_Wolf's graph: drag a selection box around all the Chase nodes → Ctrl+C. Open BP_Goblin → Event Graph → Ctrl+V → re-add Event BeginPlay wire → repeat C2's Auto Possess setting. Do the same for every enemy and boss BP.

---

## D. Ability Blueprints: filling the kit

**Goal:** create the nine GA_* assets that `Data/Classes.csv` points at. Names and folder must match **exactly** — `/Game/Abilities/GA_WarriorCleave` means the asset `GA_WarriorCleave` inside a folder called `Abilities` directly under Content.

### D1. Make the folder
Content Browser → click **Content** (root) → right-click empty space → **New Folder** → name it exactly `Abilities`.

### D2. The pattern — three flavors

Every ability is one of three recipes. Here's each, click by click.

**Flavor 1 — "Bigger melee hit" (no graph work at all):**
1. In `Content/Abilities`: right-click → **Blueprint Class** → in the search box of the "Pick Parent Class" window type `RLMeleeAttack` → select **RLMeleeAttackAbility** → name the asset (see table below).
2. Open it → **Class Defaults** → set these in the Details panel:
   - **Action Tag**: click the dropdown → expand **Ability** → tick the right one (Secondary/Utility/Special).
   - **Base Damage → Value**, **Power Coefficient**, **Range**, **Radius**, **Mana Cost**: from the table.
   - For spell-school novas, also set **Damage School = Spell**.
3. Compile → Save. Done — the parent class already does the damage sweep.

**Flavor 2 — "Projectile" (also no graph work):**
Same as Flavor 1 but pick parent **RLProjectileAbility**. Set Action Tag, Base Damage, Power Coefficient, Mana Cost.

**Flavor 3 — "Movement" (small graph):**
1. Right-click → Blueprint Class → parent **RLGameplayAbility** → name it.
2. Class Defaults: set **Action Tag** = Ability.Utility. Leave damage fields alone.
3. **Event Graph** → right-click → type `activateability` → add red **Event ActivateAbility**.
4. Right-click → `commit ability` → add **Commit Ability**. Wire ActivateAbility → Commit Ability.
5. Right-click → `get avatar actor from actor info` → add it.
6. Drag off its Return Value → type `cast to character` → add **Cast To Character**. Wire Commit Ability exec → Cast node.
7. **For a dash/charge:** drag off **As Character** → type `launch character` → add **Launch Character**, tick **XY Override**. For the velocity: drag off As Character → `get actor forward vector` → drag off that → `multiply` (vector × float), type `2500` in the float box → wire the result into **Launch Velocity**.
   **For a blink:** instead, drag off As Character → `add actor world offset`, and wire Forward Vector × `600` into **Delta Location**, tick **Sweep** (so you can't blink through walls).
8. Right-click → `end ability` → add **End Ability**. Wire Launch Character (or Add Actor World Offset) → End Ability.
9. Compile → Save.

### D3. The nine assets

| Asset name (exact) | Parent | Action Tag | Key settings |
|---|---|---|---|
| GA_WarriorCleave | RLMeleeAttackAbility | Ability.Secondary | BaseDamage 20, Coefficient 2.0, Range 250, Radius 250 |
| GA_WarriorCharge | RLGameplayAbility | Ability.Utility | Flavor 3, Launch 2500 |
| GA_WarriorAvatar | RLMeleeAttackAbility | Ability.Special | BaseDamage 30, Coefficient 3.5, Range 0, Radius 450, ManaCost 20 |
| GA_RogueFan | RLProjectileAbility | Ability.Secondary | Damage School **Physical**, BaseDamage 15, Coefficient 1.5 |
| GA_RogueDash | RLGameplayAbility | Ability.Utility | Flavor 3, Launch 3000 |
| GA_RogueDeathmark | RLMeleeAttackAbility | Ability.Special | BaseDamage 25, Coefficient 4.0, Range 220, Radius 150 |
| GA_MageNova | RLMeleeAttackAbility | Ability.Secondary | Damage School **Spell**, BaseDamage 18, Coefficient 1.8, Range 0, Radius 500, ManaCost 20 |
| GA_MageBlink | RLGameplayAbility | Ability.Utility | Flavor 3, blink 600, ManaCost 10 |
| GA_MageStarfall | RLProjectileAbility | Ability.Special | BaseDamage 30, Coefficient 3.0, ManaCost 40 |

No re-import needed — Classes.csv references these by path and finds them at runtime once the names match. Montages/VFX are optional garnish later (the melee parent exposes an **OnMeleeHit** event you can hang particles on).

> Cooldowns are intentionally skipped for now — abilities are spam-limited by mana only. Proper GAS cooldowns need a Gameplay Effect + tags per slot; say the word when you want that pass and I'll wire it into the C++ base instead so you don't have to click it out nine times.

---

## E. The five UI screens

First, a prerequisite the setup doc glossed over:

### E0. The scatter spawns C++ altars — make BP children so the UI events exist
The events (`OnChoiceRequested`, `OnBoonsOffered`, `OnCraftingOpened`) can only be *implemented* in Blueprint subclasses:

1. In `Content` make a folder `World`.
2. Right-click → Blueprint Class → parent **RLChallengeAltar** → name `BP_ChallengeAltar`. Open it, select the **Mesh** component (Components panel, top-left), Details → **Static Mesh** → pick any placeholder mesh (search `cube` or use a rock from Fab). Compile, Save.
3. Repeat for parent **RLUpgradeAltar** → `BP_UpgradeAltar`, and parent **RLBankingCrate** → `BP_BankingCrate`. Give each a mesh.
4. Also make `BP_CraftingStation` (parent **RLCraftingStation**) with a mesh, and place one in **L_Lobby** (drag it from the Content Browser into the level).
5. Now point the scatter at them: open **L_Run**, click your **RLZoneScatterVolume** actor in the Outliner (top-right list). In its Details: **Challenge Altar Class** = BP_ChallengeAltar, **Upgrade Altar Class** = BP_UpgradeAltar, **Banking Crate Class** = BP_BankingCrate. Save the level.

### E1. A reusable 4-node "open a menu" pattern
Every screen below ends with the same plumbing. When I say **"OPEN-MENU nodes"**, build this chain after Create Widget:
1. **Add to Viewport** (drag off the widget's Return Value).
2. Right-click → **Get Player Controller** (index 0).
3. Drag off it → **Set Show Mouse Cursor** → tick the checkbox (true).
4. Drag off Get Player Controller again → **Set Input Mode Game And UI**.

And when I say **"CLOSE-MENU nodes"** (used on buttons that dismiss a screen):
1. **Remove from Parent** (right-click → type it; it acts on the widget itself).
2. Get Player Controller → **Set Show Mouse Cursor** = unticked (false).
3. Get Player Controller → **Set Input Mode Game Only**.

### E2. Altar choice (extract or push deeper)
**Make the widget:**
1. `Content/UI` → right-click → **User Interface → Widget Blueprint** → parent **UserWidget** → name `WBP_AltarChoice`.
2. Designer: Palette → drag in a **Canvas Panel** first (new widgets start empty). Then drag in two **Button**s, side by side in the center. Drag a **Text** *onto each button* (it becomes the button's label). Select each Text and set its Text in Details: `Extract to Base Camp` and `Push Deeper`.
3. In the **Hierarchy** panel (left), click the first Button → in Details rename it (top field) to `Btn_Extract`. Rename the other `Btn_Continue`.
4. Make a variable to remember which altar opened us: go to the **Graph** tab → in **My Blueprint** panel (left), next to **Variables** click **+** → name it `Altar` → in Details set **Variable Type** → search `RLChallengeAltar` → pick **RL Challenge Altar → Object Reference**. Tick **Instance Editable** AND **Expose on Spawn** (both in Details).
5. Compile. Back in **Designer**, select Btn_Extract → Details → scroll to the bottom **Events** section → click **+** next to **On Clicked**. You land in the graph with a green event node.
6. Drag your **Altar** variable from My Blueprint into the graph (choose **Get**). Drag off it → type `choose extract` → add **Choose Extract**. Wire OnClicked → Choose Extract. After it, add the **CLOSE-MENU nodes**.
7. Same for Btn_Continue → **Choose Continue** → CLOSE-MENU nodes. Compile, Save.

**Make the altar open it:**
1. Open `BP_ChallengeAltar` → Event Graph → right-click → type `choice requested` → add **Event On Choice Requested**.
2. **Create Widget** → Class = WBP_AltarChoice. Notice the node now has an **Altar** input pin (that's Expose on Spawn working) — right-click graph → add **Get a reference to self** → wire **Self** into **Altar**.
3. Add the **OPEN-MENU nodes**. Compile, Save.

### E3. Boon choice (three offers)
**Widget** `WBP_BoonChoice`:
1. New Widget Blueprint, Canvas Panel, **three** Buttons in a row, a Text on each. Rename buttons `Btn_0`, `Btn_1`, `Btn_2`; rename each label Text `Txt_0`, `Txt_1`, `Txt_2` — and for the three Texts tick **Is Variable** in Details (top-right checkbox) so the graph can set them.
2. Variables (both **Instance Editable + Expose on Spawn**):
   - `Altar` — type **RL Upgrade Altar → Object Reference**
   - `BoonIds` — type **Name**, then click the little icon right of the type dropdown and pick the **grid (Array)** icon.
3. **Event Construct** (Graph tab; add via right-click if missing) — set the three labels:
   - Right-click → **Get Data Table Row**. In the node, set **Data Table** = `DT_Boons`. Drag your **BoonIds** variable in (Get) → drag off it → **Get (a copy)** with index `0` → wire into **Row Name**.
   - Drag off the node's **Out Row** → **Break RLBoonRow**.
   - Right-click → **Format Text** → in its Format box type `{name} — {cost} mana`. Wire Break's **Display Name** → `name`, **Mana Cost** → `cost` (a conversion node appears).
   - Drag **Txt_0** into the graph (Get) → drag off → **Set Text** → wire Format Text's result in. Wire Construct exec → GetDataTableRow → SetText.
   - Duplicate that whole chain twice (select nodes, Ctrl+W) for index `1` → Txt_1 and index `2` → Txt_2, chaining the exec pins.
4. Buttons: Designer → Btn_0 → Events → **On Clicked +**. In the graph: drag **Altar** in → drag off → **Purchase Boon**. Wire: **Purchaser** = right-click → **Get Player Pawn** (index 0); **Choice Index** = `0`. Then CLOSE-MENU nodes. Repeat for Btn_1 (index 1) and Btn_2 (index 2). Compile, Save.

**Altar side:** open `BP_UpgradeAltar` → Event Graph → add **Event On Boons Offered** (it has a **Boon Ids** output pin) → **Create Widget** (WBP_BoonChoice) → wire **Self** → Altar pin and the event's **Boon Ids** → BoonIds pin → OPEN-MENU nodes. Compile, Save.

### E4. Crafting
Two widgets: a row, and the screen that stacks rows.

**Row widget** `WBP_RecipeRow`:
1. New Widget Blueprint. Drag in ONE **Button**, with a **Text** child (tick the Text's **Is Variable**, name it `Txt_Label`).
2. Variable `RecipeId` — type **Name**, Instance Editable + Expose on Spawn.
3. **Event Construct** → **Get Data Table Row** (Data Table = `DT_Recipes`, Row Name = RecipeId) → **Break RLRecipeRow** → drag off **Result Item Id** → another **Get Data Table Row** (Data Table = `DT_Items`) → **Break RLItemRow** → wire **Display Name** → **Set Text** on Txt_Label. Chain the execs from Construct.
4. Button **On Clicked** → right-click → type `craft` → add **Craft** (from RLCrafting Library; it takes the world context automatically) → wire **RecipeId** into its Recipe Id pin. After it: drag off the Craft node's exec → right-click → **Can Craft** (same library, RecipeId wired) → drag the result into a **Set Is Enabled** on the Button (drag the Button from Hierarchy into the graph first, Get, then drag off → Set Is Enabled). Now a row greys out when you run out of mats. Compile, Save.

**Screen widget** `WBP_Crafting`:
1. New Widget Blueprint, Canvas Panel → drag in a **Scroll Box** (center it, make it big) — tick **Is Variable**, name `List`. Add a **Button**+Text labelled `Close` → its On Clicked → CLOSE-MENU nodes.
2. **Event Construct** → right-click → **Get Craftable Recipes** (from RLCrafting Library) → drag off its **Out Recipe Ids** array pin → **For Each Loop**. Inside the loop body: **Create Widget** (Class = WBP_RecipeRow, wire **Array Element** → its RecipeId pin) → drag **List** in (Get) → drag off → **Add Child** → wire the created widget into **Content**. Compile, Save.

**Station side:** open `BP_CraftingStation` → Event Graph → **Event On Crafting Opened** → Create Widget (WBP_Crafting) → OPEN-MENU nodes. Compile, Save. (You placed the station in L_Lobby in step E0.4 — walk up and press **E**.)

### E5. Hero creation (main menu)
1. Open **Content/Maps/WBP_MainMenu** → Designer. Add three Buttons with Text labels: `Warrior`, `Rogue`, `Mage` (plus a `Continue` button if you have an old save).
2. Warrior's **On Clicked**: right-click → **Get Game Instance** → drag off → **Cast to BP_RLGameInstance** (or `Cast to RLGameInstance` — either works since it's reparented).
3. Drag off **As BP RL Game Instance** → type `create hero` → **Create Hero**. Fill the inputs: **Hero Name** = anything (`Hero`), **Hero Class** = Warrior, **Spec Id** = `Juggernaut` (typed exactly — it's a row name from Specs.csv).
4. After Create Hero → right-click → **Open Level (by Name)** → Level Name = `L_Lobby`.
5. Duplicate the chain for the other two buttons: Rogue / `Shadowdancer`, Mage / `Pyromancer`.
6. `Continue` button: Cast to RLGameInstance → **Select Hero** (Hero Index 0) → Open Level `L_Lobby`.

(If clicks don't register on the menu, the menu level needs the cursor: in the Level Blueprint of L_MainMenu — toolbar → Blueprints icon → Open Level Blueprint — BeginPlay → Get Player Controller → Set Show Mouse Cursor true → Set Input Mode UI Only.)

### E6. Talents & Essences (minimal base-camp version)
A full tree UI is a project of its own; this is the functional stub — one button per talent you care about:

1. New Widget Blueprint `WBP_Talents`. Canvas Panel. Add a Text bound (via **Create Binding**, like A4) to a chain of: Get Game Instance → Cast to RLGameInstance → **Get Available Talent Points** → Return (shows unspent points). Add a Close button (CLOSE-MENU nodes).
2. Add a Button labelled e.g. `Might +` . **On Clicked** → Get Game Instance → Cast to RLGameInstance → **Spend Talent Point** with **Talent Id** = `Jug_Might` (typed exactly — row names are in `Data/Talents.csv`, first column).
3. After Spend Talent Point → right-click → **Get Player Character** → drag off → **Cast to BP_ThirdPersonCharacter** → drag off → **Refresh Hero Build**. This is the step that makes the stats actually land on your character.
4. Duplicate the button + chain for each talent you want clickable (change the label and the Talent Id).
5. Essences, same widget or a second one: a Button per essence — Cast to RLGameInstance → **Socket Essence** (Essence Id = `HeartOfTheFallen`, Slot Index = `0`) → Refresh Hero Build. And an upgrade Button → **Upgrade Essence** (Essence Id = `HeartOfTheFallen`) → Refresh Hero Build. Essence row names are in `Data/Essences.csv`.
6. Open it with a key: in **BP_ThirdPersonCharacter**'s Event Graph, right-click → search `keyboard t` → add the **T** keyboard event → **Pressed** → Create Widget (WBP_Talents) → OPEN-MENU nodes. (Fine for now; a proper toggle can come later.)

---

## F. Gotchas checklist

- **Stuck with a mouse cursor / character won't move after closing a menu** → some button is missing its CLOSE-MENU nodes (Set Input Mode Game Only + hide cursor).
- **Widget is invisible in Designer but you swear you bound it** → bindings only evaluate in Play. Press Play to verify.
- **Enemies stand still** → press P in the level; no green under them = NavMeshBoundsVolume missing/too small. Also check Auto Possess AI (C2).
- **Ability keys do nothing** → three-part checklist: IA assets exist → they're mapped in IMC_Default → they're assigned in the character's Class Defaults (B3). Also the class kit only exists after a hero exists (create one via the main menu, E5).
- **GA_ ability never fires** → asset name/folder must match Classes.csv exactly: `Content/Abilities/GA_WarriorCleave`. Check spelling.
- **Craft/boon names show as raw IDs or blanks** → the Get Data Table Row node's Data Table pin isn't set to the right DT asset.
