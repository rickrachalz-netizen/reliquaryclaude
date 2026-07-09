#include "RLGameplayTags.h"

namespace RLTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Primary,   "Ability.Primary",   "Bread-and-butter attack, low/no cost.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Secondary, "Ability.Secondary", "Heavier hit on a short cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Utility,   "Ability.Utility",   "Mobility or defensive tool.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Special,   "Ability.Special",   "Big payoff ability, long cooldown.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Ability_Essence,   "Ability.Essence",   "Active granted by the major-slot essence.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Damage,          "Combat.Damage",          "Generic damage event.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Damage_Critical, "Combat.Damage.Critical", "Damage event that rolled a crit.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Killed,          "Combat.Killed",          "Fired on the victim when health hits zero.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Melee_Hit,   "Event.Melee.Hit",   "Sent by the RL Melee Hit anim notify at a swing's impact frame.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Event_Melee_Combo, "Event.Melee.Combo", "Sent when the ability input repeats mid-swing; buffers the next combo stage.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Dead,         "State.Dead",         "Actor is dead; blocks ability activation.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Invulnerable, "State.Invulnerable", "Ignores incoming damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_InCombat,     "State.InCombat",     "Recently dealt or received damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Exposed,      "State.Exposed",      "Stacking: primary finisher marks; Mortal Strike consumes for bonus damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_MortalWounds, "State.MortalWounds", "Healing received reduced (applied by Mortal Strike).");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_BladestormEmpowered, "State.BladestormEmpowered", "Bladestorm's final tick empowers the next Secondary.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_CrushedArmor,        "State.CrushedArmor",        "Armor crushed by Reckless Abandon: takes increased damage.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(State_Demoralized,         "State.Demoralized",         "Demoralizing Shout: deals reduced damage.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Warrior, "Class.Warrior", "Warrior base class.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Rogue,   "Class.Rogue",   "Rogue base class.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Mage,    "Class.Mage",    "Mage base class.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy,         "Enemy",         "Any hostile creature.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Elite,   "Enemy.Elite",   "Empowered variant with bonus stats and drops.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_Boss,    "Enemy.Boss",    "Challenge-altar and end-of-run bosses.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Enemy_WildGod, "Enemy.WildGod", "The final boss barring escape from the realm.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Set_Ironwood, "Set.Ironwood", "Ironwood gear set.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Set_Feywood,  "Set.Feywood",  "Feywood gear set.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Set_GodBone,  "Set.GodBone",  "God-Bone gear set, forged from end-of-run boss drops.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Set_Duskheart, "Set.Duskheart", "Duskheart gear set: Oakheart Helm + Duskiron Longsword.");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Damage,  "SetByCaller.Damage",  "Damage magnitude passed by abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Healing, "SetByCaller.Healing", "Healing magnitude passed by abilities.");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(SetByCaller_Mana,    "SetByCaller.Mana",    "Mana cost/restore magnitude passed by abilities.");
}
