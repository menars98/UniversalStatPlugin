# RumbleCore — UniversalStats Plugin Wiki

A lightweight, GAS-free stats & effects system for Unreal Engine. Tag-driven, Blueprint-friendly, and built with multiplayer replication in mind.

---

## Table of Contents

1. [Installation](#installation)
2. [Demo Map](#demo-map)
3. [Architecture Overview](#architecture-overview)
4. [Core Components](#core-components)
   - [UniversalStatsComponent](#universalstatscomponent)
   - [UniversalCueComponent](#universalcuecomponent)
5. [Data Assets](#data-assets)
   - [UniversalStatPreset](#universalstatpreset)
   - [UniversalStatClampPreset](#universalstatclamppreset)
   - [UniversalStatTriggerPreset](#universalstattriggerpreset)
   - [UniversalCuePreset](#universalcuepreset)
6. [Effects System](#effects-system)
   - [UUniversalStatEffect](#uuniversalstateffect)
   - [Effect Duration Types](#effect-duration-types)
   - [Effect Stacking](#effect-stacking)
   - [Applying & Removing Effects](#applying--removing-effects)
7. [Visual / Audio Cue System](#visual--audio-cue-system)
   - [AUniversalGameplayCue](#auniversalgameplaycue)
   - [Cue Types](#cue-types)
8. [Delegates & Events](#delegates--events)
9. [UI Listener](#ui-listener)
10. [Replication](#replication)
11. [Quick Start Guide](#quick-start-guide)
12. [Design Notes & Known Limitations](#design-notes--known-limitations)

---

## Installation

1. **Download** this repository as a `.zip` (click **Code → Download ZIP** on GitHub).
2. **Create a `Plugins` folder** in the root of your Unreal Engine project directory (next to your `.uproject` file) if one doesn't already exist.
3. **Extract** the downloaded folder into `YourProject/Plugins/` so the structure looks like:
   ```
   YourProject/
   ├── Content/
   ├── Source/
   ├── Plugins/
   │   └── RumbleCore/
   │       ├── RumbleCore.uplugin
   │       ├── Source/
   │       └── Content/
   └── YourProject.uproject
   ```
4. **Right-click** your `.uproject` file and select **Generate Visual Studio project files**.
5. **Open the solution** in Visual Studio (or Rider) and **compile** the project. Unreal will detect and build the plugin automatically.
6. After launching the editor, go to **Edit → Plugins**, search for **RumbleCore**, and confirm it is enabled.

> **Unreal Engine version:** This plugin targets UE5. UE4 is not supported.

---

## Demo Map

**A showcase map is included in the plugin content — no setup required to try the system.**

Open it from the Content Browser at:
```
Plugins → RumbleCore Content → UniversalStatContents → Map → RC_ShowcaseMap
```

The map features several pre-built scenarios so you can experience all major systems before writing a single line of code:

| Scenario | What it demonstrates |
|---|---|
| **Dash Mechanic** | Stamina drain on input, cooldown recovery, clamp rules in action |
| **Poison Swamp** | `HasDuration` effect with periodic damage, `State.Poisoned` tag, looping Cue actor with VFX fade-out |
| **Spike Trap** | Instant effect, `Event.Character.Dead` trigger, camera shake via `OnCueExecuted` |
| **Armor & Poison Resistance Shrine** | Stacking buff (`AddStack`), `OnCueUpdated` driving material intensity, max stack cap |
| **Lethal Laser** (Immortality) | PreStatChange override in Action! Prevents lethal damage from dropping health below 1 when the State.Immortal tag is active.

> If you don't see the plugin content in the browser, enable **Show Plugin Content** in the Content Browser filter dropdown.

---

## Architecture Overview

```
Actor
├── UUniversalStatsComponent   ← Stat values, active effects, tags, delegates
└── UUniversalCueComponent     ← Listens to delegates → spawns/destroys visual cues
        │
        └── AUniversalGameplayCue (Actor)   ← Blueprint-implementable visual/audio logic

Data Assets (assigned in the component's Details panel):
  UUniversalStatPreset         ← Initial stat values
  UUniversalStatClampPreset    ← Min/max rules per stat
  UUniversalStatTriggerPreset  ← Fires gameplay events when thresholds are crossed
  UUniversalCuePreset          ← Maps gameplay tags → cue actors / particles / sounds
```

The system is intentionally GAS-free. All communication happens through **GameplayTags** and **multicast delegates**. Authority checks (`HasAuthority()`) gate all write operations, so it is safe to use in multiplayer out of the box.

---

## Core Components

### UniversalStatsComponent

`UUniversalStatsComponent : UActorComponent`

The heart of the plugin. Manages stat values, active effects, granted tags, and fires all events.

#### Properties

| Property | Type | Description |
|---|---|---|
| `StatPresets` | `TArray<UUniversalStatPreset*>` | Loaded on BeginPlay (server only). Later presets override earlier ones for duplicate stats. |
| `ClampPreset` | `UUniversalStatClampPreset*` | Applied automatically on every stat write. |
| `TriggerPreset` | `UUniversalStatTriggerPreset*` | Evaluated after every stat change to fire gameplay events. |
| `BaseTags` | `FGameplayTagContainer` | Innate tags always present on this actor (immune, undead, etc.). |

#### Key Functions

**Initialization**

```cpp
// Called automatically on BeginPlay if StatPresets is not empty.
// Can also be called manually (server only).
void InitializeFromPresets();

// Add a new stat at runtime.
void AddStat(FGameplayTag StatTag, float InitialValue);
```

**Reading Stats**

```cpp
float GetStatValue(FGameplayTag StatTag) const;
bool  HasStat(FGameplayTag StatTag) const;
TMap<FGameplayTag, float> GetAllCurrentStats() const;
```

**Writing Stats**

All write functions are `BlueprintAuthorityOnly` — they are no-ops on clients.

```cpp
void SetStatValue(FGameplayTag StatTag, float NewValue);
void ModifyStat(FGameplayTag StatTag, float DeltaValue);   // Adds delta to current value
```

`ModifyStat` passes the desired value through `PreStatChange` (override hook) and then `ApplyClampRules` before committing. `SetStatValue` skips `PreStatChange` but still clamps and broadcasts.

**Max Change Helper**

```cpp
// Adjusts AffectedStat when its MaxStat changes.
// Proportional: keeps the same percentage (e.g. 80/100 HP → 80/120 HP becomes 96/120)
// Delta: adds the flat difference (e.g. MaxHP 100→120: HP also +20)
void AdjustStatForMaxChange(
    FGameplayTag AffectedStatTag,
    FGameplayTag MaxStatTag,
    float NewMaxValue,
    EStatAdjustmentType AdjustmentType
);
```

**Override Hook**

```cpp
// BlueprintNativeEvent — override in Blueprint or C++ to intercept and modify any
// incoming stat value before clamping is applied.
float PreStatChange(FGameplayTag StatTag, float AttemptedValue);
```

**Tag Management**

```cpp
bool HasActiveTag(FGameplayTag TagToCheck) const;        // Checks effects + BaseTags
void AddLooseTag(FGameplayTag TagToAdd);                 // Adds to BaseTags
void RemoveLooseTag(FGameplayTag TagToRemove);
FGameplayTagContainer GetAllActiveTags() const;          // BaseTags + all GrantedTags from effects
```

**Effect Helpers**

```cpp
// Returns the raw base magnitude a given effect would apply to a given stat.
static float GetEffectModifierMagnitude(TSubclassOf<UUniversalStatEffect> EffectClass, FGameplayTag StatTag);

// Returns false if ANY negative modifier in the effect exceeds the current stat value.
bool CanAffordEffect(TSubclassOf<UUniversalStatEffect> EffectClass) const;
```

---

### UniversalCueComponent

`UUniversalCueComponent : UActorComponent`

Sits on the same actor as `UniversalStatsComponent`. On `BeginPlay` it automatically binds to the stats component's delegates. No manual wiring required.

#### Properties

| Property | Type | Description |
|---|---|---|
| `CuePreset` | `UUniversalCuePreset*` | Maps tags to visual/audio cues. |

#### Internal Behavior

- **`HandleGameplayEvent`** — Fires for `Instant` cues. Spawns a `AUniversalGameplayCue` actor and calls `OnCueExecuted`, or spawns Niagara/Sound directly (lightweight mode).
- **`HandleTagGranted`** — Fires when a tag is added. Spawns and attaches a looping cue actor, calls `OnCueAdded`, and stores it in `ActiveLoopingCues`.
- **`HandleTagRemoved`** — Fires when a tag is removed. Finds the matching actor from `ActiveLoopingCues`, calls `OnCueRemoved`, and cleans up.
- **`HandleTagUpdated`** — Fires when a stacking effect updates. Calls `OnCueUpdated` with the new stack count on the active cue actor.

---

## Data Assets

### UniversalStatPreset

`UUniversalStatPreset : UDataAsset`

Defines the starting stat values for a character, enemy type, or item.

```
StatPresets Array (on the component)
  └── Preset A: HP=100, MaxHP=100, Stamina=50
  └── Preset B: Strength=20, Dexterity=15
```

Multiple presets can be stacked. If two presets define the same stat tag, the later entry in the array wins.

| Field | Description |
|---|---|
| `InitialStats` | Array of `(FGameplayTag, float)` pairs. |
| `PresetName` | Display name for editor organization. |

---

### UniversalStatClampPreset

`UUniversalStatClampPreset : UDataAsset`

Per-stat min/max rules. Applied automatically inside `ModifyStat` and `SetStatValue`.

Each entry in `StatClampRules` is a `FStatClampRule`:

| Field | Description |
|---|---|
| `bEnabled` | Toggle without deleting the rule. |
| `bHasMinimum` / `MinValue` | Stat cannot go below this value. |
| `bMaxBoundToAnotherStat` / `MaxBoundaryStatTag` | Dynamic max: clamp against the current value of another stat (e.g. HP clamped to MaxHP). |
| `bHasAbsoluteMax` / `AbsoluteMaxValue` | Static max (only used when not bound to another stat). |
| `bLogClampEvents` | Prints a verbose log when a clamp fires. |

**Example:** Keeping `Stat.Health` between 0 and `Stat.Health.Max`:

```
StatClampRules:
  Key: Stat.Health
  Value:
    bHasMinimum = true,  MinValue = 0
    bMaxBoundToAnotherStat = true,  MaxBoundaryStatTag = Stat.Health.Max
```

---

### UniversalStatTriggerPreset

`UUniversalStatTriggerPreset : UDataAsset`

Fires a `FOnGameplayEventSignature` delegate when a stat crosses a threshold. Uses **edge triggering** — the event only fires once when the condition transitions from unmet → met, not every tick while it remains met.

Each entry in `TriggerRules` is a `FStatTriggerRule`:

| Field | Description |
|---|---|
| `StatToWatch` | Which stat to observe. |
| `Condition` | `LessThanOrEqual`, `GreaterThanOrEqual`, or `EqualTo`. |
| `Threshold` | The value to compare against. |
| `EventTagToFire` | The gameplay event tag broadcast via `OnGameplayEvent`. |

**Example:** Fire `Event.Character.Dead` when Health reaches 0:

```
TriggerRules:
  StatToWatch = Stat.Health
  Condition   = LessThanOrEqual
  Threshold   = 0
  EventTagToFire = Event.Character.Dead
```

The payload (`FStatEventPayload`) carries `TriggeringStat` and `TriggeringValue`.

---

### UniversalCuePreset

`UUniversalCuePreset : UDataAsset`

Maps gameplay tags to visual and audio feedback.

**Instant Cues** (`InstantCues`) — triggered by gameplay events:

| Field | Description |
|---|---|
| `TriggerTag` | The event tag to listen for. |
| `CueClass` | Spawns an actor, calls `OnCueExecuted(Target, Magnitude)`. |
| `ParticleEffect` | Spawned at actor location via NiagaraFunctionLibrary. |
| `SoundEffect` | Played at actor location via GameplayStatics. |

**Looping Cues** (`LoopingCues`) — triggered by tag grants/removes:

| Field | Description |
|---|---|
| `TriggerTag` | The granted tag to listen for. |
| `CueClass` | Spawned when tag is granted, destroyed when tag is removed. |

---

## Effects System

### UUniversalStatEffect

`UUniversalStatEffect : UObject` — `Abstract`, `Blueprintable`

Create a Blueprint subclass for each effect. All configuration lives in the class defaults (CDO), not in instances.

#### Core Properties

| Property | Description |
|---|---|
| `DurationType` | `Instant`, `HasDuration`, or `Infinite`. |
| `Duration` | Seconds (only for `HasDuration`). |
| `Period` | If > 0, modifiers re-apply every N seconds (e.g. DoT). |
| `Modifiers` | Array of `FStatModifierInfo` — which stat and how much (positive = buff, negative = debuff). |
| `AssetTag` | Identity tag for this effect (e.g. `Effect.Debuff.Poison`). |
| `GrantedTags` | State tags added while the effect is active. |
| `GameplayCues` | Visual/audio tags forwarded to the Cue component. |
| `ApplicationChance` | 0.0–1.0 probability the effect applies. |
| `TargetMustHaveTags` | Effect is blocked if the target lacks ANY of these. |
| `TargetMustNotHaveTags` | Effect is blocked if the target has ANY of these (immunity). |
| `RemoveEffectsWithTags` | On application, removes all active effects matching these tags. |
| `StackingType` | `None`, `RefreshDuration`, or `AddStack`. |
| `MaxStacks` | Maximum stack count (only for `AddStack`). |

#### Custom Magnitude Calculation

Override `CalculateModifierMagnitude` in Blueprint (or C++) to make the effect scale with level, instigator stats, or anything else:

```cpp
// Default implementation just returns BaseMagnitude unchanged.
float CalculateModifierMagnitude_Implementation(
    UUniversalStatsComponent* TargetComponent,
    AActor* Instigator,
    float EffectLevel,
    FGameplayTag StatTag,
    float BaseMagnitude
) const;
```

---

### Effect Duration Types

| Type | Behavior |
|---|---|
| `Instant` | Modifiers applied once immediately. No active effect entry is kept. |
| `HasDuration` | Modifiers applied immediately (or per period). Effect expires after `Duration` seconds. Tags are removed on expiry. |
| `Infinite` | Modifiers applied immediately (or per period). Lives until explicitly removed. |

> **Note:** Duration-based effects currently do not roll back flat stat modifiers when they expire (marked as TODO in the source). Roll-back is only performed by `RemoveEffectsWithTag` / `RemoveEffectsByClass`. Plan accordingly for permanent buffs — prefer tracking them as a separate "max" stat or implement roll-back in `PreStatChange`.

---

### Effect Stacking

| Type | Behavior |
|---|---|
| `None` | Each application creates an independent effect entry. |
| `RefreshDuration` | If an effect with the same `AssetTag` already exists, its duration is reset instead of adding a second copy. `OnCueUpdated` is broadcast. |
| `AddStack` | Stack count is incremented (up to `MaxStacks`). Duration is also refreshed. `OnCueUpdated` is broadcast with the new stack count. |

Stacking lookup is done by matching `AssetTag`, so it must be set for stacking to work.

---

### Applying & Removing Effects

**Apply**

```cpp
// Server only. Returns a FGuid handle for the new active effect (invalid for Instant effects).
FGuid ApplyEffectToSelf(const FUniversalEffectSpec& EffectSpec);
```

`FUniversalEffectSpec` holds:
- `EffectClass` — the Blueprint subclass of `UUniversalStatEffect`
- `Level` — passed into `CalculateModifierMagnitude`
- `Instigator` — the actor who caused this effect (optional, also passed into magnitude calculation)

**Remove by Tag**

```cpp
// Removes all effects whose AssetTag or GrantedTags match TagToRemove.
// StacksToRemove = -1 means remove all stacks.
int32 RemoveEffectsWithTag(FGameplayTag TagToRemove, int32 StacksToRemove = -1);
```

**Remove by Class**

```cpp
// Supports parent class matching via IsA().
int32 RemoveEffectsByClass(TSubclassOf<UUniversalStatEffect> EffectClassToRemove, int32 StacksToRemove = -1);
```

**Clear All**

```cpp
// Removes every active effect and broadcasts all tag removals.
void ClearAllEffects();
```

---

## Visual / Audio Cue System

### AUniversalGameplayCue

`AUniversalGameplayCue : AActor` — `BlueprintNativeEvent` for all callbacks

Subclass this in Blueprint to implement the actual visual/audio logic (material tints, particle components, etc.).

### Cue Types

| Event | When Called | Parameters |
|---|---|---|
| `OnCueAdded(TargetActor)` | A looping/duration tag is granted | The actor the effect is on |
| `OnCueRemoved(TargetActor)` | A looping/duration tag is removed | The actor the effect was on |
| `OnCueExecuted(TargetActor, Magnitude)` | An instant event fires | The actor + triggering value |
| `OnCueUpdated(TargetActor, NewStackCount)` | A stacking effect updates | The actor + new stack count |

**Looping Cue Lifecycle:**

```
Tag Granted
  → HandleTagGranted
    → SpawnActor(CueClass)
    → AttachToActor (follows the character)
    → OnCueAdded(TargetActor)
    → Stored in ActiveLoopingCues[Tag]

Tag Removed
  → HandleTagRemoved
    → OnCueRemoved(TargetActor)   ← restore materials, stop loops here
    → ActiveLoopingCues.Remove(Tag)
    (Actor destruction is your responsibility in OnCueRemoved or via a timer)
```

> The actor is not automatically destroyed on tag removal. Call `Destroy()` (or set a dissolve timer) inside your `OnCueRemoved` Blueprint implementation.

---

## Delegates & Events

All delegates are `DYNAMIC_MULTICAST` — bindable from both C++ and Blueprint.

| Delegate | Signature | When |
|---|---|---|
| `OnStatChanged` | `(OwningComp, StatTag, OldValue, NewValue)` | After any stat value change (server + clients via RepNotify) |
| `OnGameplayEvent` | `(OwningComp, EventTag, Payload)` | When a trigger threshold is crossed or an Instant cue fires |
| `OnTagGranted` | `(OwningComp, Tag)` | When an effect grants a tag or `AddLooseTag` is called |
| `OnTagRemoved` | `(OwningComp, Tag)` | When an effect expires/is removed or `RemoveLooseTag` is called |
| `OnTagUpdated` | `(OwningComp, Tag, NewStackCount)` | When a stacking effect's stack count changes |

---

## UI Listener

`UUniversalStatListener : UBlueprintAsyncActionBase`

A Blueprint async node for watching stat values in the UI without polling.

```
// Single stat
ListenForStatChange(StatsComponent, Stat.Health)
  → OnStatChanged pin fires with (AsyncTask, StatTag, OldValue, NewValue)

// Multiple stats at once
ListenForStatsChange(StatsComponent, [Stat.Health, Stat.Mana])
  → Same pin, fires for any watched stat
```

Call `EndTask()` when the widget is destroyed to unbind and allow GC.

---

## Replication

- `ReplicatedStats` (`TArray<FUniversalStatEntry>`) is replicated with `DOREPLIFETIME`.
- On the server, `StatsCache` (a `TMap`) is the live working copy. After every write, `UpdateArrayFromCache` syncs the replicated array.
- On clients, `OnRep_Stats` fires, updates `StatsCache`, and broadcasts `OnStatChanged` **only for stats whose values actually changed** — avoiding spurious UI updates.
- All write functions guard with `HasAuthority()`. UI and visual systems react to `OnStatChanged` and tag delegates, which fire on both server and clients.
- `ActiveEffects` is **not replicated** — effect application must be triggered server-side. The resulting stat changes and tag events propagate to clients automatically.

---

## Quick Start Guide

### 1. Add Dependencies

In your `.uproject` or plugin `uplugin`, add `GameplayTags` to the dependency modules. RumbleCore already declares the log category `LogRumbleCore`.

### 2. Set Up Gameplay Tags

Define your stat tags in `Project Settings → Gameplay Tags`:

```
Stat.Health
Stat.Health.Max
Stat.Mana
Stat.Stamina
State.Poisoned
Effect.Debuff.Poison
Event.Character.Dead
Cue.Hit
```

### 3. Create Data Assets

Right-click in Content Browser → Miscellaneous → Data Asset:

- `UUniversalStatPreset` — set initial values
- `UUniversalStatClampPreset` — set HP clamp (min 0, max = `Stat.Health.Max`)
- `UUniversalStatTriggerPreset` — fire `Event.Character.Dead` when Health ≤ 0

### 4. Add Components

On your Character Blueprint:
- Add `UniversalStatsComponent` → assign your data assets in Details
- Add `UniversalCueComponent` → assign your `CuePreset`

> **Recommended:** Instead of adding `UniversalStatsComponent` directly, create a **Blueprint child class** of it first (`BP_HeroStatsComponent`, for example), then add that child to your character. This lets you override `PreStatChange` and other `BlueprintNativeEvent` functions directly in Blueprint — without touching C++. If you add the base class directly, those override points are still available but require a C++ subclass to use.

### 5. Create an Effect

Create a new Blueprint class inheriting `UUniversalStatEffect`. In Class Defaults set:

```
DurationType = HasDuration
Duration     = 5.0
Period       = 1.0
Modifiers    = [ (Stat.Health, -10) ]
AssetTag     = Effect.Debuff.Poison
GrantedTags  = [ State.Poisoned ]
GameplayCues = [ Cue.Poison.Looping ]
```

### 6. Apply the Effect

```cpp
FUniversalEffectSpec Spec(UBP_PoisonEffect::StaticClass(), 1.f, InstigatorActor);
StatsComponent->ApplyEffectToSelf(Spec);
```

Or from Blueprint using the `Make FUniversalEffectSpec` node, then `ApplyEffectToSelf`.

### 7. React to Events

In Blueprint, bind `OnGameplayEvent` on the stats component. Check `EventTag == Event.Character.Dead` → play death animation.

---

## Design Notes & Known Limitations

### Cue Actor Lifetime — Full VFX Control by Design

`OnCueRemoved` does **not** automatically call `Destroy()` on the cue actor. This is an **intentional design decision**, not a limitation.

If the engine forcibly destroyed the actor the moment a tag was removed, a VFX artist would have no way to play a fade-out, dissolve, or wind-down animation — the effect would simply blink out of existence. By handing the lifetime decision to Blueprint, the system gives full creative control to the artist:

- Fade out over 0.5 seconds → set a dissolve timeline in `OnCueRemoved`, call `Destroy()` at the end
- Detach and let particles finish naturally → detach from actor, set a lifespan
- Hard cut → call `Destroy()` immediately

The `ActiveLoopingCues` map entry is always cleaned up by the C++ side regardless of what Blueprint does with the actor.

---

### `ActiveEffects` Not Replicated — Lightweight by Design

`ActiveEffects` lives only on the server. This is a conscious **network optimization** decision.

Epic's GAS replicates its equivalent array using `FFastArraySerializer`, a heavy system that tracks per-element deltas and generates significant boilerplate. UniversalStats takes a different philosophy: **the server is the authority, clients only need the results**.

When the server applies an effect, the consequences — stat value changes and tag grants — propagate to clients automatically through `ReplicatedStats` (RepNotify) and tag delegates. A client rendering a health bar or a poison VFX does not need to know *why* the value changed, only *that* it changed. This keeps bandwidth and CPU overhead minimal, which matters at scale in multiplayer.

---

###  `EqualTo` Trigger Condition — Use with Care

The `EqualTo` condition in `UUniversalStatTriggerPreset` uses `FMath::IsNearlyEqual` internally. This is the correct approach — floating point values in memory are rarely bit-identical to a round number (e.g. `100.0` may be stored as `99.9999998`), so a raw `==` comparison would almost never fire.

However, in practice, RPG stat triggers are almost always threshold-based rather than point-exact. **Prefer `LessThanOrEqual` (e.g. `Health <= 0`) over `EqualTo` for reliability**, especially for values modified through replication or float arithmetic chains.

`EqualTo` is safe to use for integer-equivalent stats that are always set directly via `SetStatValue` with a known exact value.
