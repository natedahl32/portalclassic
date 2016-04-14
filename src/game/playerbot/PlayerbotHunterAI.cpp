// an improved Hunter by rrtn & Runsttren :)

#include "PlayerbotHunterAI.h"
#include "PlayerbotMgr.h"

class PlayerbotAI;

PlayerbotHunterAI::PlayerbotHunterAI(Player* const master, Player* const bot, PlayerbotAI* const ai) : PlayerbotClassAI(master, bot, ai)
{
    // PET CTRL
    PET_SUMMON                    = m_ai->initSpell(CALL_PET_1);
    PET_DISMISS                   = m_ai->initSpell(DISMISS_PET_1);
    PET_REVIVE                    = m_ai->initSpell(REVIVE_PET_1);
    PET_MEND                      = m_ai->initSpell(MEND_PET_1);
    PET_FEED                      = 1539;
    BESTIAL_WRATH                 = m_ai->initSpell(BESTIAL_WRATH_1);

    INTIMIDATION                  = m_ai->initSpell(INTIMIDATION_1); // (generic)

    // PET SKILLS must be initialized by pets
    SONIC_BLAST                   = 0; // bat
    DEMORALIZING_SCREECH          = 0;
    BAD_ATTITUDE                  = 0; // crocolisk

    // RANGED COMBAT
    AUTO_SHOT                     = m_ai->initSpell(AUTO_SHOT_1);
    HUNTERS_MARK                  = m_ai->initSpell(HUNTERS_MARK_1);
    ARCANE_SHOT                   = m_ai->initSpell(ARCANE_SHOT_1);
    CONCUSSIVE_SHOT               = m_ai->initSpell(CONCUSSIVE_SHOT_1);
    DISTRACTING_SHOT              = m_ai->initSpell(DISTRACTING_SHOT_1);
    MULTI_SHOT                    = m_ai->initSpell(MULTISHOT_1);
    SERPENT_STING                 = m_ai->initSpell(SERPENT_STING_1);
    SCORPID_STING                 = m_ai->initSpell(SCORPID_STING_1);
    WYVERN_STING                  = m_ai->initSpell(WYVERN_STING_1);
    VIPER_STING                   = m_ai->initSpell(VIPER_STING_1);
    AIMED_SHOT                    = m_ai->initSpell(AIMED_SHOT_1);
    VOLLEY                        = m_ai->initSpell(VOLLEY_1);
    BLACK_ARROW                   = m_ai->initSpell(BLACK_ARROW_1);

    // MELEE
    RAPTOR_STRIKE                 = m_ai->initSpell(RAPTOR_STRIKE_1);
    WING_CLIP                     = m_ai->initSpell(WING_CLIP_1);
    MONGOOSE_BITE                 = m_ai->initSpell(MONGOOSE_BITE_1);
    DISENGAGE                     = m_ai->initSpell(DISENGAGE_1);
    DETERRENCE                    = m_ai->initSpell(DETERRENCE_1);

    // TRAPS
    FREEZING_TRAP                 = m_ai->initSpell(FREEZING_TRAP_1);
    IMMOLATION_TRAP               = m_ai->initSpell(IMMOLATION_TRAP_1);
    FROST_TRAP                    = m_ai->initSpell(FROST_TRAP_1);
    EXPLOSIVE_TRAP                = m_ai->initSpell(EXPLOSIVE_TRAP_1);

    // BUFFS
    ASPECT_OF_THE_HAWK            = m_ai->initSpell(ASPECT_OF_THE_HAWK_1);
    ASPECT_OF_THE_MONKEY          = m_ai->initSpell(ASPECT_OF_THE_MONKEY_1);
    RAPID_FIRE                    = m_ai->initSpell(RAPID_FIRE_1);
    TRUESHOT_AURA                 = m_ai->initSpell(TRUESHOT_AURA_1);

    RECENTLY_BANDAGED             = 11196; // first aid check

    // racial
    STONEFORM                     = m_ai->initSpell(STONEFORM_ALL); // dwarf
    SHADOWMELD                    = m_ai->initSpell(SHADOWMELD_ALL);
    BLOOD_FURY                    = m_ai->initSpell(BLOOD_FURY_ALL); // orc
    WAR_STOMP                     = m_ai->initSpell(WAR_STOMP_ALL); // tauren
    BERSERKING                    = m_ai->initSpell(BERSERKING_ALL); // troll

    m_petSummonFailed = false;
    m_rangedCombat = true;

	// Create stat weights for hunter, not based on spec (no basis behind these, just guesstimates)
	m_statWeights[ITEM_MOD_STAMINA] = 0.45f;
	m_statWeights[ITEM_MOD_SPIRIT] = 0.01f;
	m_statWeights[ITEM_MOD_INTELLECT] = 0.01f;
	m_statWeights[ITEM_MOD_STRENGTH] = 0.6f;
	m_statWeights[ITEM_MOD_AGILITY] = 0.9f;
	m_statWeights[ITEM_MOD_MANA] = 0.1f;
	m_statWeights[ITEM_MOD_HEALTH] = 0.45f;
}

PlayerbotHunterAI::~PlayerbotHunterAI() {}

CombatManeuverReturns PlayerbotHunterAI::DoFirstCombatManeuver(Unit* pTarget)
{
    Player *m_bot = GetPlayerBot();
    m_has_ammo = m_bot->HasItemCount( m_bot->GetUInt32Value(PLAYER_AMMO_ID), 1 );
    //DEBUG_LOG("current ammo (%u)",m_bot->GetUInt32Value(PLAYER_AMMO_ID));
    m_bot->setAttackTimer(RANGED_ATTACK,0);
    if (!m_has_ammo)
    {
       m_ai->FindAmmo();
       //DEBUG_LOG("new ammo (%u)",m_bot->GetUInt32Value(PLAYER_AMMO_ID));
       m_has_ammo = m_bot->HasItemCount( m_bot->GetUInt32Value(PLAYER_AMMO_ID), 1 );
    }
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

CombatManeuverReturns PlayerbotHunterAI::DoFirstCombatManeuverPVE(Unit* /*pTarget*/)
{
    return RETURN_NO_ACTION_OK;
}

bool PlayerbotHunterAI::HasPet(Player* bot)
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT * FROM character_pet WHERE owner = '%u' AND (slot = '%u' OR slot = '%u')", bot->GetGUIDLow(), PET_SAVE_AS_CURRENT, PET_SAVE_NOT_IN_SLOT);

    if (result)
        return true;  //hunter has current pet
    else
        return false;  //hunter either has no pet or stabled
} // end HasPet

CombatManeuverReturns PlayerbotHunterAI::DoFirstCombatManeuverPVP(Unit* /*pTarget*/)
{
    return RETURN_NO_ACTION_OK;
}

CombatManeuverReturns PlayerbotHunterAI::DoNextCombatManeuver(Unit *pTarget)
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

CombatManeuverReturns PlayerbotHunterAI::DoNextCombatManeuverPVE(Unit *pTarget)
{
    if (!m_ai)    return RETURN_NO_ACTION_ERROR;
    if (!m_bot)   return RETURN_NO_ACTION_ERROR;
    if (!pTarget) return RETURN_NO_ACTION_ERROR;

    Unit* pVictim = pTarget->getVictim();

    // check for pet and heal if neccessary
    Pet *pet = m_bot->GetPet();
    // TODO: clarify/simplify: !pet->getDeathState() != ALIVE
    if (pet && PET_MEND > 0 && pet->isAlive() && pet->GetHealthPercent() < 50 && pVictim != m_bot && !pet->HasAura(PET_MEND, EFFECT_INDEX_0) && m_ai->CastSpell(PET_MEND, *m_bot))
    {
        m_ai->TellMaster("healing pet.");
        return RETURN_CONTINUE;
    }
    else if (pet && INTIMIDATION > 0 && pVictim == pet && !pet->HasAura(INTIMIDATION, EFFECT_INDEX_0) && m_ai->CastSpell(INTIMIDATION, *m_bot))
        return RETURN_CONTINUE;

    // racial traits
    if (m_bot->getRace() == RACE_ORC && !m_bot->HasAura(BLOOD_FURY, EFFECT_INDEX_0))
        m_ai->CastSpell(BLOOD_FURY, *m_bot);
    else if (m_bot->getRace() == RACE_TROLL && !m_bot->HasAura(BERSERKING, EFFECT_INDEX_0))
        m_ai->CastSpell(BERSERKING, *m_bot);

    // check if ranged combat is possible
    bool meleeReach = m_bot->CanReachWithMeleeAttack(pTarget);

    if (meleeReach || !m_has_ammo)
    {
        // switch to melee combat (target in melee range, out of ammo)
        m_rangedCombat = false;
        m_ai->SetCombatStyle(PlayerbotAI::COMBAT_MELEE);
        if (!m_bot->GetUInt32Value(PLAYER_AMMO_ID))
            m_ai->TellMaster("Out of ammo!");

        // become monkey (increases dodge chance)...
        if (ASPECT_OF_THE_MONKEY > 0 && !m_bot->HasAura(ASPECT_OF_THE_MONKEY, EFFECT_INDEX_0))
            m_ai->CastSpell(ASPECT_OF_THE_MONKEY, *m_bot);

    }
    else if (!meleeReach)
    {
        // switch to ranged combat
        m_rangedCombat = true;
        m_ai->SetCombatStyle(PlayerbotAI::COMBAT_RANGED);

        // increase ranged attack power...
        if (ASPECT_OF_THE_HAWK > 0 && !m_bot->HasAura(ASPECT_OF_THE_HAWK, EFFECT_INDEX_0))
            m_ai->CastSpell(ASPECT_OF_THE_HAWK, *m_bot);

        // m_ai->TellMaster("target dist %f",m_bot->GetCombatDistance(pTarget,true));
        if (AUTO_SHOT > 0)
        {
            if (m_bot->isAttackReady(RANGED_ATTACK))
                m_bot->CastSpell(pTarget, AUTO_SHOT, true);

            m_bot->setAttackTimer(RANGED_ATTACK,500);

            const SpellEntry* spellInfo = sSpellStore.LookupEntry(AUTO_SHOT);
            if (!spellInfo)
                return RETURN_CONTINUE;

            if (m_ai->CheckBotCast(spellInfo) != SPELL_CAST_OK)
                m_bot->InterruptNonMeleeSpells(true, AUTO_SHOT);
        }
    }

    // damage spells
    if (m_ai->GetCombatStyle() == PlayerbotAI::COMBAT_RANGED)
    {
        if (HUNTERS_MARK > 0 && m_ai->In_Reach(pTarget,HUNTERS_MARK) && !pTarget->HasAura(HUNTERS_MARK, EFFECT_INDEX_0) && m_ai->CastSpell(HUNTERS_MARK, *pTarget))
            return RETURN_CONTINUE;
        else if (RAPID_FIRE > 0 && m_ai->In_Reach(pTarget,RAPID_FIRE) && !m_bot->HasAura(RAPID_FIRE, EFFECT_INDEX_0) && m_ai->CastSpell(RAPID_FIRE, *m_bot))
            return RETURN_CONTINUE;
        else if (MULTI_SHOT > 0 && m_ai->In_Reach(pTarget,MULTI_SHOT) && m_ai->GetAttackerCount() >= 3 && m_ai->CastSpell(MULTI_SHOT, *pTarget))
            return RETURN_CONTINUE;
        else if (ARCANE_SHOT > 0 && m_ai->In_Reach(pTarget,ARCANE_SHOT) && m_ai->CastSpell(ARCANE_SHOT, *pTarget))
            return RETURN_CONTINUE;
        else if (CONCUSSIVE_SHOT > 0 && m_ai->In_Reach(pTarget,CONCUSSIVE_SHOT) && !pTarget->HasAura(CONCUSSIVE_SHOT, EFFECT_INDEX_0) && m_ai->CastSpell(CONCUSSIVE_SHOT, *pTarget))
            return RETURN_CONTINUE;
        else if (VIPER_STING > 0 && m_ai->In_Reach(pTarget,VIPER_STING) && pTarget->GetPower(POWER_MANA) > 0 && m_ai->GetManaPercent() < 70 && !pTarget->HasAura(VIPER_STING, EFFECT_INDEX_0) && m_ai->CastSpell(VIPER_STING, *pTarget))
            return RETURN_CONTINUE;
        else if (SERPENT_STING > 0 && m_ai->In_Reach(pTarget,SERPENT_STING) && !pTarget->HasAura(SERPENT_STING, EFFECT_INDEX_0) && !pTarget->HasAura(SCORPID_STING, EFFECT_INDEX_0) &&  !pTarget->HasAura(VIPER_STING, EFFECT_INDEX_0) && m_ai->CastSpell(SERPENT_STING, *pTarget))
            return RETURN_CONTINUE;
        else if (SCORPID_STING > 0 && m_ai->In_Reach(pTarget,SCORPID_STING) && !pTarget->HasAura(WYVERN_STING, EFFECT_INDEX_0) && !pTarget->HasAura(SCORPID_STING, EFFECT_INDEX_0) && !pTarget->HasAura(SERPENT_STING, EFFECT_INDEX_0) && !pTarget->HasAura(VIPER_STING, EFFECT_INDEX_0) && m_ai->CastSpell(SCORPID_STING, *pTarget))
            return RETURN_CONTINUE;
        else if (VOLLEY > 0 && m_ai->In_Reach(pTarget,VOLLEY) && m_ai->GetAttackerCount() >= 3 && m_ai->CastSpell(VOLLEY, *pTarget))
            return RETURN_CONTINUE;
        else if (BLACK_ARROW > 0 && m_ai->In_Reach(pTarget,BLACK_ARROW) && !pTarget->HasAura(BLACK_ARROW, EFFECT_INDEX_0) && m_ai->CastSpell(BLACK_ARROW, *pTarget))
            return RETURN_CONTINUE;
        else if (AIMED_SHOT > 0 && m_ai->In_Reach(pTarget,AIMED_SHOT) && m_ai->CastSpell(AIMED_SHOT, *pTarget))
            return RETURN_CONTINUE;
        else
            return RETURN_NO_ACTION_OK;
    }
    else
    {
        if (RAPTOR_STRIKE > 0 && m_ai->In_Reach(pTarget,RAPTOR_STRIKE) && m_ai->CastSpell(RAPTOR_STRIKE, *pTarget))
            return RETURN_CONTINUE;
        else if (EXPLOSIVE_TRAP > 0 && !pTarget->HasAura(EXPLOSIVE_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(IMMOLATION_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(FROST_TRAP, EFFECT_INDEX_0) && m_ai->CastSpell(EXPLOSIVE_TRAP, *pTarget))
            return RETURN_CONTINUE;
        else if (WING_CLIP > 0 && m_ai->In_Reach(pTarget,WING_CLIP) && !pTarget->HasAura(WING_CLIP, EFFECT_INDEX_0) && m_ai->CastSpell(WING_CLIP, *pTarget))
            return RETURN_CONTINUE;
        else if (IMMOLATION_TRAP > 0 && !pTarget->HasAura(IMMOLATION_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(EXPLOSIVE_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(FROST_TRAP, EFFECT_INDEX_0) && m_ai->CastSpell(IMMOLATION_TRAP, *pTarget))
            return RETURN_CONTINUE;
        else if (MONGOOSE_BITE > 0 && m_ai->Impulse() && m_ai->CastSpell(MONGOOSE_BITE, *pTarget))
            return RETURN_CONTINUE;
        else if (FROST_TRAP > 0 && !pTarget->HasAura(FROST_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(IMMOLATION_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(EXPLOSIVE_TRAP, EFFECT_INDEX_0) && m_ai->CastSpell(FROST_TRAP, *pTarget))
            return RETURN_CONTINUE;
        else if (DETERRENCE > 0 && pVictim == m_bot && m_bot->GetHealthPercent() < 50 && !m_bot->HasAura(DETERRENCE, EFFECT_INDEX_0) && m_ai->CastSpell(DETERRENCE, *m_bot))
            return RETURN_CONTINUE;
        else if (m_bot->getRace() == RACE_TAUREN && !pTarget->HasAura(WAR_STOMP, EFFECT_INDEX_0) && m_ai->CastSpell(WAR_STOMP, *pTarget))
            return RETURN_CONTINUE;
//        else if (m_bot->getRace() == RACE_DWARF && m_bot->HasAuraState(AURA_STATE_DEADLY_POISON) && m_ai->CastSpell(STONEFORM, *m_bot))
//            return RETURN_CONTINUE;
        else if (m_bot->getRace() == RACE_NIGHTELF && pVictim == m_bot && m_ai->GetHealthPercent() < 25 && !m_bot->HasAura(SHADOWMELD, EFFECT_INDEX_0) && m_ai->CastSpell(SHADOWMELD, *m_bot))
            return RETURN_CONTINUE;

        /*else if(FREEZING_TRAP > 0 && !pTarget->HasAura(FREEZING_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(ARCANE_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(EXPLOSIVE_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(BEAR_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(IMMOLATION_TRAP, EFFECT_INDEX_0) && !pTarget->HasAura(FROST_TRAP, EFFECT_INDEX_0) && m_ai->CastSpell(FREEZING_TRAP,*pTarget) )
            out << " > Freezing Trap"; // this can trap your bots too
           else if(DISENGAGE > 0 && pVictim && m_ai->CastSpell(DISENGAGE,*pTarget) )
            out << " > Disengage!"; // attempt to return to ranged combat*/
    }

    return RETURN_NO_ACTION_OK;
} // end DoNextCombatManeuver

CombatManeuverReturns PlayerbotHunterAI::DoNextCombatManeuverPVP(Unit* pTarget)
{
    if (m_ai->CastSpell(RAPTOR_STRIKE))
        return RETURN_CONTINUE;

    return DoNextCombatManeuverPVE(pTarget); // TODO: bad idea perhaps, but better than the alternative
}

void PlayerbotHunterAI::DoNonCombatActions()
{
    if (!m_ai)  return;
    if (!m_bot) return;

    if (!m_rangedCombat || m_ai->GetCombatStyle() == PlayerbotAI::COMBAT_MELEE)
    {
        m_rangedCombat = true;
        m_ai->SetCombatStyle(PlayerbotAI::COMBAT_RANGED);
    }

    // buff group
    if (TRUESHOT_AURA > 0 && !m_bot->HasAura(TRUESHOT_AURA, EFFECT_INDEX_0))
        m_ai->CastSpell(TRUESHOT_AURA, *m_bot);

    // buff myself
    if (ASPECT_OF_THE_HAWK > 0 && !m_bot->HasAura(ASPECT_OF_THE_HAWK, EFFECT_INDEX_0))
        m_ai->CastSpell(ASPECT_OF_THE_HAWK, *m_bot);

	//create water
	if (m_ai->FindDrink() == nullptr && m_bot->getLevel() == 60)
	{
		if (Item* pItem = m_bot->StoreNewItemInInventorySlot(CRYSTAL_WATER, 20))
			m_bot->SendNewItem(pItem, 20, true, false);
		
		return;
	}

    // hp/mana check
    if (EatDrinkBandage())
        return;

    // check for pet
    if (PET_SUMMON > 0 && !m_petSummonFailed && HasPet(m_bot))
    {
        // we can summon pet, and no critical summon errors before
        Pet *pet = m_bot->GetPet();
        if (!pet)
        {
            // summon pet
            if (PET_SUMMON > 0 && m_ai->CastSpell(PET_SUMMON, *m_bot))
                m_ai->TellMaster("summoning pet.");
            else
            {
                m_petSummonFailed = true;
                m_ai->TellMaster("summon pet failed!");
            }
        }
        else if (!(pet->isAlive()))
        {
            if (PET_REVIVE > 0 && m_ai->CastSpell(PET_REVIVE, *m_bot))
                m_ai->TellMaster("reviving pet.");
        }
        else if (pet->GetHealthPercent() < 50)
        {
            if (PET_MEND > 0 && pet->isAlive() && !pet->HasAura(PET_MEND, EFFECT_INDEX_0) && m_ai->CastSpell(PET_MEND, *m_bot))
                m_ai->TellMaster("healing pet.");
        }
        else if (pet->GetHappinessState() != HAPPY) // if pet is hungry
        {
            Unit *caster = (Unit *) m_bot;
            // list out items in main backpack
            for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
            {
                Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                if (pItem)
                {
                    const ItemPrototype* const pItemProto = pItem->GetProto();
                    if (!pItemProto)
                        continue;

                    if (pet->HaveInDiet(pItemProto)) // is pItem in pets diet
                    {
                        // DEBUG_LOG ("[PlayerbotHunterAI]: DoNonCombatActions - Food for pet: %s",pItemProto->Name1);
                        caster->CastSpell(caster, 23355, true); // pet feed visual
                        uint32 count = 1; // number of items used
                        int32 benefit = pet->GetCurrentFoodBenefitLevel(pItemProto->ItemLevel); // nutritional value of food
                        m_bot->DestroyItemCount(pItem, count, true); // remove item from inventory
                        m_bot->CastCustomSpell(m_bot, PET_FEED, &benefit, nullptr, nullptr, true); // feed pet
                        m_ai->TellMaster("feeding pet.");
                        m_ai->SetIgnoreUpdateTime(10);
                        return;
                    }
                }
            }
            // list out items in other removable backpacks
            for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
            {
                const Bag* const pBag = (Bag *) m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);
                if (pBag)
                    for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
                    {
                        Item* const pItem = m_bot->GetItemByPos(bag, slot);
                        if (pItem)
                        {
                            const ItemPrototype* const pItemProto = pItem->GetProto();
                            if (!pItemProto)
                                continue;

                            if (pet->HaveInDiet(pItemProto)) // is pItem in pets diet
                            {
                                // DEBUG_LOG ("[PlayerbotHunterAI]: DoNonCombatActions - Food for pet: %s",pItemProto->Name1);
                                caster->CastSpell(caster, 23355, true); // pet feed visual
                                uint32 count = 1; // number of items used
                                int32 benefit = pet->GetCurrentFoodBenefitLevel(pItemProto->ItemLevel); // nutritional value of food
                                m_bot->DestroyItemCount(pItem, count, true); // remove item from inventory
                                m_bot->CastCustomSpell(m_bot, PET_FEED, &benefit, nullptr, nullptr, true); // feed pet
                                m_ai->TellMaster("feeding pet.");
                                m_ai->SetIgnoreUpdateTime(10);
                                return;
                            }
                        }
                    }
            }
            if (pet->HasAura(PET_MEND, EFFECT_INDEX_0) && !pet->HasAura(PET_FEED, EFFECT_INDEX_0))
                m_ai->TellMaster("..no pet food!");
            m_ai->SetIgnoreUpdateTime(7);
        }
    }
} // end DoNonCombatActions

bool PlayerbotHunterAI::IsNewItemAnUpgrade(ItemPrototype const *pNewProto, ItemPrototype const *pCurrentProto)
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
		currentScore += (currentVal * m_statWeights[i]);
	}

	// TODO: Calculate spell effects on items, such as +crit% and spellpower.
	// TODO: Calculate damage modifiers on items

	// Calculate DPS of a weapon
	if (pNewProto->Class == ITEM_CLASS_WEAPON && pCurrentProto->Class == ITEM_CLASS_WEAPON) {
		newScore += (pNewProto->getDPS() * 0.9f);
		currentScore += (pCurrentProto->getDPS() * 0.9f);
	}

	return newScore > currentScore;
}
