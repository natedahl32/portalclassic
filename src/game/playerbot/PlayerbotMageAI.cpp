
#include "PlayerbotMageAI.h"

class PlayerbotAI;

PlayerbotMageAI::PlayerbotMageAI(Player* const master, Player* const bot, PlayerbotAI* const ai) : PlayerbotClassAI(master, bot, ai)
{
    ARCANE_MISSILES         = m_ai->initSpell(ARCANE_MISSILES_1);
    ARCANE_EXPLOSION        = m_ai->initSpell(ARCANE_EXPLOSION_1);
    COUNTERSPELL            = m_ai->initSpell(COUNTERSPELL_1);
    ARCANE_POWER            = m_ai->initSpell(ARCANE_POWER_1);
    DAMPEN_MAGIC            = m_ai->initSpell(DAMPEN_MAGIC_1);
    AMPLIFY_MAGIC           = m_ai->initSpell(AMPLIFY_MAGIC_1);
    MAGE_ARMOR              = m_ai->initSpell(MAGE_ARMOR_1);
    ARCANE_INTELLECT        = m_ai->initSpell(ARCANE_INTELLECT_1);
    ARCANE_BRILLIANCE       = m_ai->initSpell(ARCANE_BRILLIANCE_1);
    MANA_SHIELD             = m_ai->initSpell(MANA_SHIELD_1);
    CONJURE_WATER           = m_ai->initSpell(CONJURE_WATER_1);
    CONJURE_FOOD            = m_ai->initSpell(CONJURE_FOOD_1);
    FIREBALL                = m_ai->initSpell(FIREBALL_1);
    FIRE_BLAST              = m_ai->initSpell(FIRE_BLAST_1);
    FLAMESTRIKE             = m_ai->initSpell(FLAMESTRIKE_1);
    SCORCH                  = m_ai->initSpell(SCORCH_1);
    POLYMORPH               = m_ai->initSpell(POLYMORPH_1);
    PYROBLAST               = m_ai->initSpell(PYROBLAST_1);
    BLAST_WAVE              = m_ai->initSpell(BLAST_WAVE_1);
    COMBUSTION              = m_ai->initSpell(COMBUSTION_1);
    FIRE_WARD               = m_ai->initSpell(FIRE_WARD_1);
    FROSTBOLT               = m_ai->initSpell(FROSTBOLT_1);
    FROST_NOVA              = m_ai->initSpell(FROST_NOVA_1);
    BLIZZARD                = m_ai->initSpell(BLIZZARD_1);
    CONE_OF_COLD            = m_ai->initSpell(CONE_OF_COLD_1);
    ICE_BARRIER             = m_ai->initSpell(ICE_BARRIER_1);
    FROST_WARD              = m_ai->initSpell(FROST_WARD_1);
    FROST_ARMOR             = m_ai->initSpell(FROST_ARMOR_1);
    ICE_ARMOR               = m_ai->initSpell(ICE_ARMOR_1);
    ICE_BLOCK               = m_ai->initSpell(ICE_BLOCK_1);
    COLD_SNAP               = m_ai->initSpell(COLD_SNAP_1);

    // RANGED COMBAT
    SHOOT                   = m_ai->initSpell(SHOOT_2);

    RECENTLY_BANDAGED       = 11196; // first aid check

    // racial
    ESCAPE_ARTIST           = m_ai->initSpell(ESCAPE_ARTIST_ALL); // gnome
    PERCEPTION              = m_ai->initSpell(PERCEPTION_ALL); // human
    BERSERKING              = m_ai->initSpell(BERSERKING_ALL); // troll
    WILL_OF_THE_FORSAKEN    = m_ai->initSpell(WILL_OF_THE_FORSAKEN_ALL); // undead

	// Create stat weights for mage, not based on spec (no basis behind these, just guesstimates)
	m_statWeights[ITEM_MOD_STAMINA] = 0.45f;
	m_statWeights[ITEM_MOD_SPIRIT] = 0.3f;
	m_statWeights[ITEM_MOD_INTELLECT] = 0.9f;
	m_statWeights[ITEM_MOD_STRENGTH] = 0.05f;
	m_statWeights[ITEM_MOD_AGILITY] = 0.05f;
	m_statWeights[ITEM_MOD_MANA] = 0.6f;
	m_statWeights[ITEM_MOD_HEALTH] = 0.45f;
}

PlayerbotMageAI::~PlayerbotMageAI() {}

CombatManeuverReturns PlayerbotMageAI::DoFirstCombatManeuver(Unit* pTarget)
{
    // There are NPCs in BGs and Open World PvP, so don't filter this on PvP scenarios (of course if PvP targets anyone but tank, all bets are off anyway)
    // Wait until the tank says so, until any non-tank gains aggro or X seconds - whichever is shortest
    if (m_ai->GetCombatOrder() & PlayerbotAI::ORDERS_TEMP_WAIT_TANKAGGRO)
{
        if (m_WaitUntil > m_ai->CurrentTime() && m_ai->GroupTankHoldsAggro())
    {
            return RETURN_NO_ACTION_OK; // wait it out
    }
        else
    {
            m_ai->ClearGroupCombatOrder(PlayerbotAI::ORDERS_TEMP_WAIT_TANKAGGRO);
            }
            }

    if (m_ai->GetCombatOrder() & PlayerbotAI::ORDERS_TEMP_WAIT_OOC)
            {
        if (m_WaitUntil > m_ai->CurrentTime() && !m_ai->IsGroupInCombat())
            return RETURN_NO_ACTION_OK; // wait it out
        else
            m_ai->ClearGroupCombatOrder(PlayerbotAI::ORDERS_TEMP_WAIT_OOC);
            }

    switch (m_ai->GetScenarioType())
            {
        case PlayerbotAI::SCENARIO_PVP_DUEL:
        case PlayerbotAI::SCENARIO_PVP_BG:
        case PlayerbotAI::SCENARIO_PVP_ARENA:
        case PlayerbotAI::SCENARIO_PVP_OPENWORLD:
            return DoFirstCombatManeuverPVP(pTarget);
        case PlayerbotAI::SCENARIO_PVE:
        case PlayerbotAI::SCENARIO_PVE_ELITE:
        case PlayerbotAI::SCENARIO_PVE_RAID:
        default:
            return DoFirstCombatManeuverPVE(pTarget);
                break;
            }

    return RETURN_NO_ACTION_ERROR;
            }

CombatManeuverReturns PlayerbotMageAI::DoFirstCombatManeuverPVE(Unit* /*pTarget*/)
            {
    return RETURN_NO_ACTION_OK;
            }

CombatManeuverReturns PlayerbotMageAI::DoFirstCombatManeuverPVP(Unit* /*pTarget*/)
            {
    return RETURN_NO_ACTION_OK;
            }

CombatManeuverReturns PlayerbotMageAI::DoNextCombatManeuver(Unit *pTarget)
            {
    // Face enemy, make sure bot is attacking
    if (!m_bot->HasInArc(M_PI_F, pTarget))
        m_bot->SetFacingTo(m_bot->GetAngle(pTarget));

    switch (m_ai->GetScenarioType())
            {
        case PlayerbotAI::SCENARIO_PVP_DUEL:
        case PlayerbotAI::SCENARIO_PVP_BG:
        case PlayerbotAI::SCENARIO_PVP_ARENA:
        case PlayerbotAI::SCENARIO_PVP_OPENWORLD:
            return DoNextCombatManeuverPVP(pTarget);
        case PlayerbotAI::SCENARIO_PVE:
        case PlayerbotAI::SCENARIO_PVE_ELITE:
        case PlayerbotAI::SCENARIO_PVE_RAID:
        default:
            return DoNextCombatManeuverPVE(pTarget);
                break;
            }

    return RETURN_NO_ACTION_ERROR;
            }

CombatManeuverReturns PlayerbotMageAI::DoNextCombatManeuverPVE(Unit *pTarget)
            {
    if (!m_ai)  return RETURN_NO_ACTION_ERROR;
    if (!m_bot) return RETURN_NO_ACTION_ERROR;

    Unit* pVictim = pTarget->getVictim();
    bool meleeReach = m_bot->CanReachWithMeleeAttack(pTarget);

    uint32 spec = m_bot->GetSpec();

    if (m_ai->GetCombatStyle() != PlayerbotAI::COMBAT_RANGED && !meleeReach)
        m_ai->SetCombatStyle(PlayerbotAI::COMBAT_RANGED);
    // if can't shoot OR have no ranged (wand) equipped
    else if(m_ai->GetCombatStyle() != PlayerbotAI::COMBAT_MELEE && (SHOOT == 0 || !m_bot->GetWeaponForAttack(RANGED_ATTACK, true, true)))
        m_ai->SetCombatStyle(PlayerbotAI::COMBAT_MELEE);

    //Used to determine if this bot is highest on threat
    Unit *newTarget = m_ai->FindAttacker((PlayerbotAI::ATTACKERINFOTYPE) (PlayerbotAI::AIT_VICTIMSELF | PlayerbotAI::AIT_HIGHESTTHREAT), m_bot);
    if (newTarget) // TODO: && party has a tank
            {
        // Insert instant threat reducing spell (if a mage has one)

        // Have threat, can't quickly lower it. 3 options remain: Stop attacking, lowlevel damage (wand), keep on keeping on.
        if (newTarget->GetHealthPercent() > 25)
            {
            // If elite, do nothing and pray tank gets aggro off you
            // TODO: Is there an IsElite function? If so, find it and insert.
            //if (newTarget->IsElite())
            //    return;

            // Not an elite. You could insert FEAR here but in any PvE situation that's 90-95% likely
            // to worsen the situation for the group. ... So please don't.
            CastSpell(SHOOT, pTarget);
            return RETURN_CONTINUE;
            }
            }

    switch (spec)
            {
        case MAGE_SPEC_FROST:
            if (ICE_BLOCK > 0 && m_ai->In_Reach(m_bot,ICE_BLOCK) && pVictim == m_bot && !m_bot->HasAura(ICE_BLOCK, EFFECT_INDEX_0) && CastSpell(ICE_BLOCK, m_bot))
                return RETURN_CONTINUE;
            if (ICE_BARRIER > 0 && m_ai->In_Reach(m_bot,ICE_BARRIER) && pVictim == m_bot && !m_bot->HasAura(ICE_BARRIER, EFFECT_INDEX_0) && m_ai->GetHealthPercent() < 50 && CastSpell(ICE_BARRIER, m_bot))
                return RETURN_CONTINUE;
            if (BLIZZARD > 0 && m_ai->In_Reach(pTarget,BLIZZARD) && m_ai->GetAttackerCount() >= 5 && CastSpell(BLIZZARD, pTarget))
            {
                m_ai->SetIgnoreUpdateTime(8);
                return RETURN_CONTINUE;
            }
            if (CONE_OF_COLD > 0 && meleeReach && !pTarget->HasAura(CONE_OF_COLD, EFFECT_INDEX_0) && CastSpell(CONE_OF_COLD, pTarget))
                return RETURN_CONTINUE;
            if (FROSTBOLT > 0 && m_ai->In_Reach(pTarget,FROSTBOLT) && !pTarget->HasAura(FROSTBOLT, EFFECT_INDEX_0) && CastSpell(FROSTBOLT, pTarget))
                return RETURN_CONTINUE;
            if (FROST_WARD > 0 && m_ai->In_Reach(m_bot,FROST_WARD) && !m_bot->HasAura(FROST_WARD, EFFECT_INDEX_0) && CastSpell(FROST_WARD, m_bot))
                return RETURN_CONTINUE;
            if (FROST_NOVA > 0 && meleeReach && !pTarget->HasAura(FROST_NOVA, EFFECT_INDEX_0) && CastSpell(FROST_NOVA, pTarget))
                return RETURN_CONTINUE;
            if (COLD_SNAP > 0 && m_ai->In_Reach(m_bot,COLD_SNAP) && CastSpell(COLD_SNAP, m_bot))
                return RETURN_CONTINUE;

            if (FROSTBOLT > 0 && m_ai->In_Reach(pTarget,FROSTBOLT))
                return CastSpell(FROSTBOLT, pTarget);
                break;

        case MAGE_SPEC_FIRE:
            if (FIRE_WARD > 0 && m_ai->In_Reach(m_bot,FIRE_WARD) && !m_bot->HasAura(FIRE_WARD, EFFECT_INDEX_0) && CastSpell(FIRE_WARD, m_bot))
                return RETURN_CONTINUE;
            if (COMBUSTION > 0 && m_ai->In_Reach(m_bot,COMBUSTION) && !m_bot->HasAura(COMBUSTION, EFFECT_INDEX_0) && CastSpell(COMBUSTION, m_bot))
                return RETURN_CONTINUE;
            if (FIREBALL > 0 && m_ai->In_Reach(pTarget,FIREBALL) && CastSpell(FIREBALL, pTarget))
                return RETURN_CONTINUE;
            if (FIRE_BLAST > 0 && m_ai->In_Reach(pTarget,FIRE_BLAST) && CastSpell(FIRE_BLAST, pTarget))
                return RETURN_CONTINUE;
            if (FLAMESTRIKE > 0 && m_ai->In_Reach(pTarget,FLAMESTRIKE) && CastSpell(FLAMESTRIKE, pTarget))
                return RETURN_CONTINUE;
            if (SCORCH > 0 && m_ai->In_Reach(pTarget,SCORCH) && CastSpell(SCORCH, pTarget))
                return RETURN_CONTINUE;
            if (PYROBLAST > 0 && m_ai->In_Reach(pTarget,PYROBLAST) && !pTarget->HasAura(PYROBLAST, EFFECT_INDEX_0) && CastSpell(PYROBLAST, pTarget))
                return RETURN_CONTINUE;
            if (BLAST_WAVE > 0 && m_ai->GetAttackerCount() >= 3 && meleeReach && CastSpell(BLAST_WAVE, pTarget))
                return RETURN_CONTINUE;

            if (FIREBALL > 0 && m_ai->In_Reach(pTarget,FIREBALL))
                return CastSpell(FIREBALL, pTarget);
                break;

        case MAGE_SPEC_ARCANE:
            if (ARCANE_POWER > 0 && m_ai->In_Reach(pTarget,ARCANE_POWER) && CastSpell(ARCANE_POWER, pTarget))
                return RETURN_CONTINUE;
            if (ARCANE_MISSILES > 0 && m_ai->In_Reach(pTarget,ARCANE_MISSILES) && CastSpell(ARCANE_MISSILES, pTarget))
            {
                m_ai->SetIgnoreUpdateTime(3);
                return RETURN_CONTINUE;
            }
            if (ARCANE_EXPLOSION > 0 && m_ai->GetAttackerCount() >= 3 && meleeReach && CastSpell(ARCANE_EXPLOSION, pTarget))
                return RETURN_CONTINUE;
            if (COUNTERSPELL > 0 && pTarget->IsNonMeleeSpellCasted(true) && CastSpell(COUNTERSPELL, pTarget))
                return RETURN_CONTINUE;
            if (SLOW > 0 && m_ai->In_Reach(pTarget,SLOW) && !pTarget->HasAura(SLOW, EFFECT_INDEX_0) && CastSpell(SLOW, pTarget))
                return RETURN_CONTINUE;
            if (MANA_SHIELD > 0 && m_ai->GetHealthPercent() < 70 && pVictim == m_bot && !m_bot->HasAura(MANA_SHIELD, EFFECT_INDEX_0) && CastSpell(MANA_SHIELD, m_bot))
                return RETURN_CONTINUE;

            if (FIREBALL > 0 && m_ai->In_Reach(pTarget,FIREBALL))
                return CastSpell(FIREBALL, pTarget);
                break;
            }

    // No spec due to low level OR no spell found yet
    if (FROSTBOLT > 0 && m_ai->In_Reach(pTarget,FROSTBOLT) && !pTarget->HasAura(FROSTBOLT, EFFECT_INDEX_0))
        return CastSpell(FROSTBOLT, pTarget);
    if (FIREBALL > 0 && m_ai->In_Reach(pTarget,FIREBALL)) // Very low levels
        return CastSpell(FIREBALL, pTarget);

    return RETURN_NO_ACTION_ERROR; // What? Not even Fireball is available?
} // end DoNextCombatManeuver

CombatManeuverReturns PlayerbotMageAI::DoNextCombatManeuverPVP(Unit* pTarget)
            {
    if (FIREBALL && m_ai->In_Reach(pTarget,FIREBALL) && m_ai->CastSpell(FIREBALL))
        return RETURN_CONTINUE;

    return DoNextCombatManeuverPVE(pTarget); // TODO: bad idea perhaps, but better than the alternative
    }

void PlayerbotMageAI::DoNonCombatActions()
{
    Player* master = GetMaster();

    if (!m_bot || !master)
        return;

    // Buff armor
    if (MAGE_ARMOR)
    {
        if (m_ai->SelfBuff(MAGE_ARMOR))
            return;
    }
    else if (ICE_ARMOR)
    {
        if (m_ai->SelfBuff(ICE_ARMOR))
            return;
    }
    else if (FROST_ARMOR)
        if (m_ai->SelfBuff(FROST_ARMOR))
                return;

    // buff group
    if (m_bot->GetGroup() && m_ai->HasSpellReagents(ARCANE_BRILLIANCE) && Buff(&PlayerbotMageAI::BuffHelper, ARCANE_BRILLIANCE) & RETURN_CONTINUE)
            return;
    else if (Buff(&PlayerbotMageAI::BuffHelper, ARCANE_INTELLECT, JOB_MANAONLY) & RETURN_CONTINUE)
        return;

    // TODO: The beauty of a mage is not only its ability to supply itself with water, but to share its water
    // So, conjure at *least* 1.25 stacks, ready to trade a stack and still have some left for self
    if (m_ai->FindDrink() == nullptr && CONJURE_WATER && m_ai->CastSpell(CONJURE_WATER, *m_bot))
    {
        m_ai->TellMaster("I'm conjuring some water.");
        m_ai->SetIgnoreUpdateTime(3);
        return;
    }
    if (m_ai->FindFood() == nullptr && CONJURE_FOOD && m_ai->CastSpell(CONJURE_FOOD, *m_bot))
    {
        m_ai->TellMaster("I'm conjuring some food.");
        m_ai->SetIgnoreUpdateTime(3);
        return;
    }

    if (EatDrinkBandage())
        return;
} // end DoNonCombatActions

// TODO: this and priest's BuffHelper are identical and thus could probably go in PlayerbotClassAI.cpp somewhere
bool PlayerbotMageAI::BuffHelper(PlayerbotAI* ai, uint32 spellId, Unit *target)
{
    if (!ai)          return false;
    if (spellId == 0) return false;
    if (!target)      return false;

    Pet* pet = target->GetPet();
    if (pet && !pet->HasAuraType(SPELL_AURA_MOD_UNATTACKABLE) && ai->Buff(spellId, pet))
        return true;

    if (ai->Buff(spellId, target))
        return true;

        return false;
}

// Return to UpdateAI the spellId usable to neutralize a target with creaturetype
uint32 PlayerbotMageAI::Neutralize(uint8 creatureType)
{
    if (!m_bot)         return 0;
    if (!m_ai)          return 0;
    if (!creatureType)  return 0;

    if (creatureType != CREATURE_TYPE_HUMANOID && creatureType != CREATURE_TYPE_BEAST)
    {
        m_ai->TellMaster("I can't polymorph that target.");
        return 0;
    }

    if (POLYMORPH)
        return POLYMORPH;
    else
        return 0;

    return 0;
}

bool PlayerbotMageAI::IsNewItemAnUpgrade(ItemPrototype const *pNewProto, ItemPrototype const *pCurrentProto)
{
	float newScore = 0;
	float currentScore = 0;

	// TODO: Move this to a common method, it is the same for all classes
	// Loop through all mods on the item and calculate score
	for (int i = 0; i < MAX_ITEM_MOD; i++) {
		// Get values of the items for this mod
		uint32 newVal = pNewProto->GetStatValue((ItemModType)i);
		uint32 currentVal = pCurrentProto->GetStatValue((ItemModType)i);

		// If this is health, we need to divide by the units of health per stamina so we get an accurate value of the two. Otherwise health
		// will be overvalued. Same for mana.
		if (i == ITEM_MOD_HEALTH) {
			newVal = newVal / 10;
			currentVal = currentVal / 10;
		}
		else if (i == ITEM_MOD_MANA) {
			newVal = newVal / 15;
			currentVal = currentVal / 15;
		}

		// Calculate the score
		newScore += (newVal * m_statWeights[i]);
		currentScore += (newVal * m_statWeights[i]);
	}

	// TODO: Calculate spell effects on items, such as +crit% and spellpower.
	// TODO: Calculate damage modifiers on items

	// Calculate DPS of a weapon
	if (pNewProto->Class == ITEM_CLASS_WEAPON && pCurrentProto->Class == ITEM_CLASS_WEAPON) {
		// Only care about wands DPS, caster weapons are stat sticks
		if (pNewProto->SubClass == ITEM_SUBCLASS_WEAPON_WAND && pCurrentProto->SubClass == ITEM_SUBCLASS_WEAPON_WAND) {
			newScore += (pNewProto->getDPS() * 0.09f);
			currentScore += (pNewProto->getDPS() * 0.09f);
		}
	}

	return newScore > currentScore;
}
