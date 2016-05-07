#include "PlayerbotClassAI.h"
#include "Common.h"

PlayerbotClassAI::PlayerbotClassAI(Player* const master, Player* const bot, PlayerbotAI* const ai)
{
    m_master = master;
    m_bot = bot;
    m_ai = ai;

    m_MinHealthPercentTank   = 80;
    m_MinHealthPercentHealer = 60;
    m_MinHealthPercentDPS    = 30;
    m_MinHealthPercentMaster = m_MinHealthPercentDPS;

    ClearWait();
}
PlayerbotClassAI::~PlayerbotClassAI() {}

CombatManeuverReturns PlayerbotClassAI::DoFirstCombatManeuver(Unit *) { return RETURN_NO_ACTION_OK; }
CombatManeuverReturns PlayerbotClassAI::DoNextCombatManeuver(Unit *) { return RETURN_NO_ACTION_OK; }

CombatManeuverReturns PlayerbotClassAI::DoFirstCombatManeuverPVE(Unit *) { return RETURN_NO_ACTION_OK; }
CombatManeuverReturns PlayerbotClassAI::DoNextCombatManeuverPVE(Unit *) { return RETURN_NO_ACTION_OK; }
CombatManeuverReturns PlayerbotClassAI::DoFirstCombatManeuverPVP(Unit *) { return RETURN_NO_ACTION_OK; }
CombatManeuverReturns PlayerbotClassAI::DoNextCombatManeuverPVP(Unit *) { return RETURN_NO_ACTION_OK; }

bool PlayerbotClassAI::IsNewItemAnUpgrade(Item const *pNewItem, Item const *pCurrentItem) 
{ 
	DEBUG_LOG("[PlayerbotAI]: Warning: Using PlayerbotClassAI::IsNewItemAnUpgrade() rather than class specific function");
	return false; 
}

void PlayerbotClassAI::DoNonCombatActions()
{
    DEBUG_LOG("[PlayerbotAI]: Warning: Using PlayerbotClassAI::DoNonCombatActions() rather than class specific function");
}

bool PlayerbotClassAI::EatDrinkBandage(bool bMana, unsigned char foodPercent, unsigned char drinkPercent, unsigned char bandagePercent)
{
    Item* drinkItem = nullptr;
    Item* foodItem = nullptr;
    if (bMana && m_ai->GetManaPercent() < drinkPercent)
        drinkItem = m_ai->FindDrink();
    if (m_ai->GetHealthPercent() < foodPercent)
        foodItem = m_ai->FindFood();
    if (drinkItem || foodItem)
    {
        if (drinkItem)
        {
            m_ai->TellMaster("I could use a drink.");
            m_ai->UseItem(drinkItem);
        }
        if (foodItem)
        {
            m_ai->TellMaster("I could use some food.");
            m_ai->UseItem(foodItem);
        }
        return true;
    }

    if (m_ai->GetHealthPercent() < bandagePercent && !m_bot->HasAura(RECENTLY_BANDAGED))
    {
        Item* bandageItem = m_ai->FindBandage();
        if (bandageItem)
        {
            m_ai->TellMaster("I could use first aid.");
            m_ai->UseItem(bandageItem);
            return true;
        }
    }

    return false;
}

bool PlayerbotClassAI::CanPull()
{
    DEBUG_LOG("[PlayerbotAI]: Warning: Using PlayerbotClassAI::CanPull() rather than class specific function");
    return false;
}

bool PlayerbotClassAI::CastHoTOnTank()
{
    DEBUG_LOG("[PlayerbotAI]: Warning: Using PlayerbotClassAI::CastHoTOnTank() rather than class specific function");
    return false;
}

CombatManeuverReturns PlayerbotClassAI::HealPlayer(Player* target) {
    if (!m_ai)  return RETURN_NO_ACTION_ERROR;
    if (!m_bot) return RETURN_NO_ACTION_ERROR;

    if (!target) return RETURN_NO_ACTION_INVALIDTARGET;
    if (target->IsInDuel()) return RETURN_NO_ACTION_INVALIDTARGET;

    return RETURN_NO_ACTION_OK;
}

// Please note that job_type JOB_MANAONLY is a cumulative restriction. JOB_TANK | JOB_HEAL means both; JOB_TANK | JOB_MANAONLY means tanks with powertype MANA (paladins, druids)
CombatManeuverReturns PlayerbotClassAI::Buff(bool (*BuffHelper)(PlayerbotAI*, uint32, Unit*), uint32 spellId, uint32 type, bool bMustBeOOC)
{
    if (!m_ai)  return RETURN_NO_ACTION_ERROR;
    if (!m_bot) return RETURN_NO_ACTION_ERROR;
    if (!m_bot->isAlive() || m_bot->IsInDuel()) return RETURN_NO_ACTION_ERROR;
    if (bMustBeOOC && m_bot->isInCombat()) return RETURN_NO_ACTION_ERROR;

    if (spellId == 0) return RETURN_NO_ACTION_OK;

    // First, fill the list of targets
    if (m_bot->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player *groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || !groupMember->isAlive() || groupMember->IsInDuel())
                continue;
            JOB_TYPE job = GetTargetJob(groupMember);
            if (job & type && (!(job & JOB_MANAONLY) || groupMember->getClass() == CLASS_DRUID || groupMember->GetPowerType() == POWER_MANA))
            {
                if (BuffHelper(m_ai, spellId, groupMember))
                    return RETURN_CONTINUE;
            }
        }
    }
    else
    {
        if (m_master && !m_master->IsInDuel()
            && (!(GetTargetJob(m_master) & JOB_MANAONLY) || m_master->getClass() == CLASS_DRUID || m_master->GetPowerType() == POWER_MANA))
            if (BuffHelper(m_ai, spellId, m_master))
                return RETURN_CONTINUE;
        // Do not check job or power type - any buff you have is always useful to self
        if (BuffHelper(m_ai, spellId, m_bot))
            return RETURN_CONTINUE;
    }

    return RETURN_NO_ACTION_OK;
}

/**
 * GetHealTarget()
 * return Unit* Returns unit to be healed. First checks 'critical' Healer(s), next Tank(s), next Master (if different from:), next DPS.
 * If none of the healths are low enough (or multiple valid targets) against these checks, the lowest health is healed. Having a target
 * returned does not guarantee it's worth healing, merely that the target does not have 100% health.
 *
 * return NULL If NULL is returned, no healing is required. At all.
 *
 * Will need extensive re-write for co-operation amongst multiple healers. As it stands, multiple healers would all pick the same 'ideal'
 * healing target.
 */
Player* PlayerbotClassAI::GetHealTarget(JOB_TYPE type)
{
    if (!m_ai)  return nullptr;
    if (!m_bot) return nullptr;
    if (!m_bot->isAlive() || m_bot->IsInDuel()) return nullptr;

    // define seperately for sorting purposes - DO NOT CHANGE ORDER!
    std::vector<heal_priority> targets;

    // First, fill the list of targets
    if (m_bot->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player *groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || !groupMember->isAlive() || groupMember->IsInDuel())
                continue;
            JOB_TYPE job = GetTargetJob(groupMember);
            if (job & type)
                targets.push_back( heal_priority(groupMember, (groupMember->GetHealth() * 100 / groupMember->GetMaxHealth()), job) );
        }
    }
    else
    {
        targets.push_back( heal_priority(m_bot, m_bot->GetHealthPercent(), GetTargetJob(m_bot)) );
        if (m_master && !m_master->IsInDuel())
            targets.push_back( heal_priority(m_master, (m_master->GetHealth() * 100 / m_master->GetMaxHealth()), GetTargetJob(m_master)) );
    }

    // Sorts according to type: Healers first, tanks next, then master followed by DPS, thanks to the order of the TYPE enum
    std::sort(targets.begin(), targets.end());

    uint8 uCount = 0,i = 0;
    // x is used as 'target found' variable; i is used as the targets iterator throughout all 4 types.
    int16 x = -1;

    // Try to find a healer in need of healing (if multiple, the lowest health one)
    while (true)
    {
        // This works because we sorted it above
        if ( (uCount + i) >= targets.size() || !(targets.at(uCount).type & JOB_HEAL)) break;
        uCount++;
    }

    // We have uCount healers in the targets, check if any qualify for priority healing
    for (; uCount > 0; uCount--, i++)
    {
        if (targets.at(i).hp <= m_MinHealthPercentHealer)
            if (x == -1 || targets.at(x).hp > targets.at(i).hp)
                x = i;
    }
    if (x > -1) return targets.at(x).p;

    // Try to find a tank in need of healing (if multiple, the lowest health one)
    while (true)
    {
        if ( (uCount + i) >= targets.size() || !(targets.at(uCount).type & JOB_TANK)) break;
        uCount++;
    }

    for (; uCount > 0; uCount--, i++)
    {
        if (targets.at(i).hp <= m_MinHealthPercentTank)
            if (x == -1 || targets.at(x).hp > targets.at(i).hp)
                x = i;
    }
    if (x > -1) return targets.at(x).p;

    // Try to find master in need of healing (lowest health one first)
    if (m_MinHealthPercentMaster != m_MinHealthPercentDPS)
    {
        while (true)
        {
            if ( (uCount + i) >= targets.size() || !(targets.at(uCount).type & JOB_MASTER)) break;
            uCount++;
        }

        for (; uCount > 0; uCount--, i++)
        {
            if (targets.at(i).hp <= m_MinHealthPercentMaster)
                if (x == -1 || targets.at(x).hp > targets.at(i).hp)
                    x = i;
        }
        if (x > -1) return targets.at(x).p;
    }

    // Try to find anyone else in need of healing (lowest health one first)
    while (true)
    {
        if ( (uCount + i) >= targets.size() ) break;
        uCount++;
    }

    for (; uCount > 0; uCount--, i++)
    {
        if (targets.at(i).hp <= m_MinHealthPercentDPS)
            if (x == -1 || targets.at(x).hp > targets.at(i).hp)
                x = i;
    }
    if (x > -1) return targets.at(x).p;

    // Nobody is critical, find anyone hurt at all, return lowest (let the healer sort out if it's worth healing or not)
    for (i = 0, uCount = targets.size(); uCount > 0; uCount--, i++)
    {
        if (targets.at(i).hp < 100)
            if (x == -1 || targets.at(x).hp > targets.at(i).hp)
                x = i;
    }
    if (x > -1) return targets.at(x).p;

    return nullptr;
}

Player* PlayerbotClassAI::GetResurrectionTarget(JOB_TYPE type, bool bMustBeOOC)
{
    if (!m_ai)  return nullptr;
    if (!m_bot) return nullptr;
    if (!m_bot->isAlive() || m_bot->IsInDuel()) return nullptr;
    if (bMustBeOOC && m_bot->isInCombat()) return nullptr;

    // First, fill the list of targets
    if (m_bot->GetGroup())
    {
        // define seperately for sorting purposes - DO NOT CHANGE ORDER!
        std::vector<heal_priority> targets;

        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player *groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || groupMember->isAlive())
                continue;
            JOB_TYPE job = GetTargetJob(groupMember);
            if (job & type)
                targets.push_back( heal_priority(groupMember, 0, job) );
        }

        // Sorts according to type: Healers first, tanks next, then master followed by DPS, thanks to the order of the TYPE enum
        std::sort(targets.begin(), targets.end());

        if (targets.size())
            return targets.at(0).p;
    }
    else if (!m_master->isAlive())
        return m_master;

    return nullptr;
}

JOB_TYPE PlayerbotClassAI::GetTargetJob(Player* target)
{
    // is a bot
    if (target->GetPlayerbotAI())
    {
        if (target->GetPlayerbotAI()->IsHealer())
            return JOB_HEAL;
        if (target->GetPlayerbotAI()->IsTank())
            return JOB_TANK;
        return JOB_DPS;
    }

    // figure out what to do with human players - i.e. figure out if they're tank, DPS or healer
    uint32 uSpec = target->GetSpec();
    switch (target->getClass())
    {
        case CLASS_PALADIN:
            if (uSpec == PALADIN_SPEC_HOLY)
                return JOB_HEAL;
            if (uSpec == PALADIN_SPEC_PROTECTION)
                return JOB_TANK;
            return (m_master == target) ? JOB_MASTER : JOB_DPS;
        case CLASS_DRUID:
            if (uSpec == DRUID_SPEC_RESTORATION)
                return JOB_HEAL;
            // Feral can be used for both Tank or DPS... play it safe and assume tank. If not... he best be good at threat management or he'll ravage the healer's mana
            else if (uSpec == DRUID_SPEC_FERAL)
                return JOB_TANK;
            return (m_master == target) ? JOB_MASTER : JOB_DPS;
        case CLASS_PRIEST:
            // Since Discipline can be used for both healer or DPS assume DPS
            if (uSpec == PRIEST_SPEC_HOLY)
                return JOB_HEAL;
            return (m_master == target) ? JOB_MASTER : JOB_DPS;
        case CLASS_SHAMAN:
            if (uSpec == SHAMAN_SPEC_RESTORATION)
                return JOB_HEAL;
            return (m_master == target) ? JOB_MASTER : JOB_DPS;
        case CLASS_WARRIOR:
            if (uSpec == WARRIOR_SPEC_PROTECTION)
                return JOB_TANK;
            return (m_master == target) ? JOB_MASTER : JOB_DPS;
        case CLASS_MAGE:
        case CLASS_WARLOCK:
        case CLASS_ROGUE:
        case CLASS_HUNTER:
        default:
            return (m_master == target) ? JOB_MASTER : JOB_DPS;
    }
}

CombatManeuverReturns PlayerbotClassAI::CastSpellNoRanged(uint32 nextAction, Unit *pTarget)
{
    if (!m_ai)  return RETURN_NO_ACTION_ERROR;
    if (!m_bot) return RETURN_NO_ACTION_ERROR;

    if (nextAction == 0)
        return RETURN_NO_ACTION_OK; // Asked to do nothing so... yeh... Dooone.

    if (pTarget != nullptr)
        return (m_ai->CastSpell(nextAction, *pTarget) ? RETURN_CONTINUE : RETURN_NO_ACTION_ERROR);
    else
        return (m_ai->CastSpell(nextAction) ? RETURN_CONTINUE : RETURN_NO_ACTION_ERROR);
}

CombatManeuverReturns PlayerbotClassAI::CastSpellWand(uint32 nextAction, Unit *pTarget, uint32 SHOOT)
{
    if (!m_ai)  return RETURN_NO_ACTION_ERROR;
    if (!m_bot) return RETURN_NO_ACTION_ERROR;

    if (SHOOT > 0 && m_bot->FindCurrentSpellBySpellId(SHOOT) && m_bot->GetWeaponForAttack(RANGED_ATTACK, true, true))
    {
        if (nextAction == SHOOT)
            // At this point we're already shooting and are asked to shoot. Don't cause a global cooldown by stopping to shoot! Leave it be.
            return RETURN_CONTINUE; // ... We're asked to shoot and are already shooting so... Task accomplished?

        // We are shooting but wish to cast a spell. Stop 'casting' shoot.
        m_bot->InterruptNonMeleeSpells(true, SHOOT);
        // ai->TellMaster("Interrupting auto shot.");
    }

    // We've stopped ranged (if applicable), if no nextAction just return
    if (nextAction == 0)
        return RETURN_CONTINUE; // Asked to do nothing so... yeh... Dooone.

    if (nextAction == SHOOT)
    {
        if (SHOOT > 0 && m_ai->GetCombatStyle() == PlayerbotAI::COMBAT_RANGED && !m_bot->FindCurrentSpellBySpellId(SHOOT) && m_bot->GetWeaponForAttack(RANGED_ATTACK, true, true))
            return (m_ai->CastSpell(SHOOT, *pTarget) ? RETURN_CONTINUE : RETURN_NO_ACTION_ERROR);
        else
            // Do Melee attack
            return RETURN_NO_ACTION_UNKNOWN; // We're asked to shoot and aren't.
    }

    if (pTarget != nullptr)
        return (m_ai->CastSpell(nextAction, *pTarget) ? RETURN_CONTINUE : RETURN_NO_ACTION_ERROR);
    else
        return (m_ai->CastSpell(nextAction) ? RETURN_CONTINUE : RETURN_NO_ACTION_ERROR);
}

float PlayerbotClassAI::GetItemScore(const ItemPrototype* pItemProto)
{
	float score = 0;

	for (int i = 0; i < MAX_ITEM_MOD; i++) {
		// Get values of the items for this mod
		uint32 value = pItemProto->GetStatValue((ItemModType)i);

		// If this is health, we need to divide by the units of health per stamina so we get an accurate value of the two. Otherwise health
		// will be overvalued. Same for mana.
		if (i == ITEM_MOD_HEALTH) {
			value = value / 10;
		}
		else if (i == ITEM_MOD_MANA) {
			value = value / 15;
		}

		// Calculate the score
		score += (value * m_statWeights[i]);
	}

	return score;
}

float PlayerbotClassAI::GetItemEnchantmentScore(const Item* pItem)
{
	float score = 0;

	// Calculate enchantments applied to the item
	for (uint32 i = PERM_ENCHANTMENT_SLOT; i < MAX_ENCHANTMENT_SLOT; ++i)
	{
		// Don't count temp enchantment slots

		uint32 enchantmentId = pItem->GetEnchantmentId(EnchantmentSlot(i));
		SpellItemEnchantmentEntry const* pEnchant = sSpellItemEnchantmentStore.LookupEntry(enchantmentId);
		if (!pEnchant) {
			//DEBUG_LOG("[PlayerbotAI]: Info: No enchantment found in Slot %u", i);
			continue;
		}			

		for (int s = 0; s < 3; ++s)
		{
			uint32 enchant_display_type = pEnchant->type[s];
			uint32 enchant_amount = pEnchant->amount[s];
			uint32 enchant_spell_id = pEnchant->spellid[s];

			//DEBUG_LOG("[PlayerbotAI]: Info: Enchant Display Type of %u found in slot %u", enchant_display_type, i);
			//DEBUG_LOG("[PlayerbotAI]: Info: Enchant Amount of %u found in slot %u", enchant_amount, i);
			//DEBUG_LOG("[PlayerbotAI]: Info: Enchant Spell Id of %u found in slot %u", enchant_spell_id, i);

			// check the spell
			if (enchant_spell_id > 0) {
				SpellEntry const* spellInfo = sSpellStore.LookupEntry(enchant_spell_id);
				if (spellInfo) {
					for (uint32 effIndex = 0; effIndex < MAX_EFFECT_INDEX; ++effIndex) {
						if (spellInfo->EffectApplyAuraName[effIndex] == SPELL_AURA_MOD_STAT) {
							// Get the value
							uint32 value = spellInfo->CalculateSimpleValue(SpellEffectIndex(effIndex));

							// Which stat is it?
							switch (spellInfo->EffectMiscValue[effIndex])
							{
								case STAT_STRENGTH:
									score += (value * m_statWeights[ITEM_MOD_STRENGTH]);
									break;
								case STAT_AGILITY:
									score += (value * m_statWeights[ITEM_MOD_AGILITY]);
									break;
								case STAT_STAMINA:
									score += (value * m_statWeights[ITEM_MOD_STAMINA]);
									break;
								case STAT_INTELLECT:
									score += (value * m_statWeights[ITEM_MOD_INTELLECT]);
									break;
								case STAT_SPIRIT:
									score += (value * m_statWeights[ITEM_MOD_SPIRIT]);
									break;
								default:
									break;
							}
						}
					}
				}
			}

			// checks enchant types
			switch (enchant_display_type)
			{
				case ITEM_ENCHANTMENT_TYPE_NONE:
					break;
				case ITEM_ENCHANTMENT_TYPE_COMBAT_SPELL:	// TODO: Add calculation for combat spells
					break;
				case ITEM_ENCHANTMENT_TYPE_DAMAGE:			// TODO: Add calculation for damage type enchantments
					/*if (item->GetSlot() == EQUIPMENT_SLOT_MAINHAND)
					HandleStatModifier(UNIT_MOD_DAMAGE_MAINHAND, TOTAL_VALUE, float(enchant_amount), apply);
					else if (item->GetSlot() == EQUIPMENT_SLOT_OFFHAND)
					HandleStatModifier(UNIT_MOD_DAMAGE_OFFHAND, TOTAL_VALUE, float(enchant_amount), apply);
					else if (item->GetSlot() == EQUIPMENT_SLOT_RANGED)
					HandleStatModifier(UNIT_MOD_DAMAGE_RANGED, TOTAL_VALUE, float(enchant_amount), apply); */
					break;
				case ITEM_ENCHANTMENT_TYPE_EQUIP_SPELL:		// TODO: Add calculation for equip type enchantments
				{
					//if (enchant_spell_id)
					//{
					//	if (apply)
					//		CastSpell(this, enchant_spell_id, true, item);
					//	else
					//		RemoveAurasDueToItemSpell(item, enchant_spell_id);
					//}
					break;
				}
				case ITEM_ENCHANTMENT_TYPE_RESISTANCE:		// TODO: Add calculation for resists
					//HandleStatModifier(UnitMods(UNIT_MOD_RESISTANCE_START + enchant_spell_id), TOTAL_VALUE, float(enchant_amount), apply);
					break;
				case ITEM_ENCHANTMENT_TYPE_STAT:
				{
					uint32 value = enchant_amount;

					// If this is health, we need to divide by the units of health per stamina so we get an accurate value of the two. Otherwise health
					// will be overvalued. Same for mana.
					if (enchant_spell_id == ITEM_MOD_HEALTH) {
						value = value / 10;
					}
					else if (enchant_spell_id == ITEM_MOD_MANA) {
						value = value / 15;
					}

					score += (value * m_statWeights[enchant_spell_id]);

					break;
				}
				case ITEM_ENCHANTMENT_TYPE_TOTEM:           // Shaman Rockbiter Weapon
				{
					if (m_bot->getClass() == CLASS_SHAMAN)
					{
						// TODO: Handle totem type enchantments
					}
					break;
				}
				default:
					break;
			}                                               
		}                                                   
	}

	return score;
}
