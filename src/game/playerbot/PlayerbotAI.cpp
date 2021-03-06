#include "Common.h"
#include "Database/DatabaseEnv.h"
#include "../ItemPrototype.h"
#include "../World.h"
#include "../SpellMgr.h"
#include "../GridNotifiers.h"
#include "../GridNotifiersImpl.h"
#include "../CellImpl.h"
#include "PlayerbotAI.h"
#include "PlayerbotMgr.h"
#include "PlayerbotDruidAI.h"
#include "PlayerbotHunterAI.h"
#include "PlayerbotMageAI.h"
#include "PlayerbotPaladinAI.h"
#include "PlayerbotPriestAI.h"
#include "PlayerbotRogueAI.h"
#include "PlayerbotShamanAI.h"
#include "PlayerbotWarlockAI.h"
#include "PlayerbotWarriorAI.h"
#include "../Player.h"
#include "../ObjectMgr.h"
#include "../Chat.h"
#include "../Guild.h"
#include "WorldPacket.h"
#include "../Spell.h"
#include "../Unit.h"
#include "../SpellAuras.h"
#include "../SharedDefines.h"
#include "Log.h"
#include "../GossipDef.h"
#include "../MotionMaster.h"
#include "../AuctionHouseMgr.h"
#include "../Mail.h"
#include "../Language.h"
#include "../LootMgr.h"

// returns a float in range of..
float rand_float(float low, float high)
{
    return (rand() / (static_cast<float> (RAND_MAX) + 1.0)) * (high - low) + low;
}

// ChatHandler already implements some useful commands the master can call on bots
// These commands are protected inside the ChatHandler class so this class provides access to the commands
// we'd like to call on our bots
class PlayerbotChatHandler : protected ChatHandler
{
public:
    explicit PlayerbotChatHandler(Player* pMasterPlayer) : ChatHandler(pMasterPlayer) {}
    bool revive(Player& botPlayer) { return HandleReviveCommand((char *) botPlayer.GetName()); }
    bool teleport(Player& botPlayer) { return HandleNamegoCommand((char *) botPlayer.GetName()); }
    void sysmessage(const char *str) { SendSysMessage(str); }
    bool dropQuest(char *str) { return HandleQuestRemoveCommand(str); }
};

PlayerbotAI::PlayerbotAI(PlayerbotMgr* const mgr, Player* const bot) :
m_mgr(mgr), m_bot(bot), m_classAI(0), m_ignoreAIUpdatesUntilTime(CurrentTime()),
m_combatOrder(ORDERS_NONE), m_ScenarioType(SCENARIO_PVE),
    m_TimeDoneEating(0), m_TimeDoneDrinking(0),
    m_CurrentlyCastingSpellId(0), m_spellIdCommand(0),
    m_targetGuidCommand(ObjectGuid()),
    m_taxiMaster(ObjectGuid())
{

    // set bot state and needed item list
    m_botState = BOTSTATE_NORMAL;
    SetQuestNeedItems();
    SetQuestNeedCreatures();

    // reset some pointers
    m_targetChanged = false;
    m_targetType = TARGET_NORMAL;
    m_targetCombat = 0;
    m_targetAssist = 0;
    m_targetProtect = 0;
	m_primaryProfession1 = 0;
	m_primaryProfession2 = 0;

    // set collection options
    m_collectionFlags = 0;
    if (m_mgr->m_confCollectCombat)
        SetCollectFlag(COLLECT_FLAG_COMBAT);
    if (m_mgr->m_confCollectQuest)
        SetCollectFlag(COLLECT_FLAG_QUEST);
    if (m_mgr->m_confCollectProfession)
        SetCollectFlag(COLLECT_FLAG_PROFESSION);
    if (m_mgr->m_confCollectLoot)
        SetCollectFlag(COLLECT_FLAG_LOOT);
    if (m_mgr->m_confCollectSkin && m_bot->HasSkill(SKILL_SKINNING))
        SetCollectFlag(COLLECT_FLAG_SKIN);
    if (m_mgr->m_confCollectObjects)
        SetCollectFlag(COLLECT_FLAG_NEAROBJECT);

	// If the bot is in the same group as the master when summoned, then follow the master
	// Otherwise, stay where they are until they join the masters group
	if (m_bot->IsInSameGroupWith(GetMaster()))
		SetMovementOrder(MOVEMENT_FOLLOW, GetMaster());
	else
		SetMovementOrder(MOVEMENT_STAY);
    BotDataRestore();
    m_DelayAttackInit = CurrentTime();

    // get class specific ai
    ReloadAI();

	// Get primary profession of bot
	if (m_bot->HasSpell(ALCHEMY_1))
		m_primaryProfession1 = ALCHEMY_1;
	else if (m_bot->HasSpell(BLACKSMITHING_1))
		m_primaryProfession1 = BLACKSMITHING_1;
	else if (m_bot->HasSpell(ENCHANTING_1))
		m_primaryProfession1 = ENCHANTING_1;
	else if (m_bot->HasSpell(ENGINEERING_1))
		m_primaryProfession1 = ENGINEERING_1;
	else if (m_bot->HasSpell(LEATHERWORKING_1))
		m_primaryProfession1 = LEATHERWORKING_1;
	else if (m_bot->HasSpell(TAILORING_1))
		m_primaryProfession1 = TAILORING_1;
	else if (m_bot->HasSpell(HERB_GATHERING_1))
		m_primaryProfession1 = HERB_GATHERING_1;
	else if (m_bot->HasSpell(MINING_1))
		m_primaryProfession1 = MINING_1;
	else if (m_bot->HasSpell(SKINNING_1))
		m_primaryProfession1 = SKINNING_1;

	// Get secondary profession of bot
	if (m_bot->HasSpell(ALCHEMY_1) && m_primaryProfession1 != ALCHEMY_1)
		m_primaryProfession2 = ALCHEMY_1;
	else if (m_bot->HasSpell(BLACKSMITHING_1) && m_primaryProfession1 != BLACKSMITHING_1)
		m_primaryProfession2 = BLACKSMITHING_1;
	else if (m_bot->HasSpell(ENCHANTING_1) && m_primaryProfession1 != ENCHANTING_1)
		m_primaryProfession2 = ENCHANTING_1;
	else if (m_bot->HasSpell(ENGINEERING_1) && m_primaryProfession1 != ENGINEERING_1)
		m_primaryProfession2 = ENGINEERING_1;
	else if (m_bot->HasSpell(LEATHERWORKING_1) && m_primaryProfession1 != LEATHERWORKING_1)
		m_primaryProfession2 = LEATHERWORKING_1;
	else if (m_bot->HasSpell(TAILORING_1) && m_primaryProfession1 != TAILORING_1)
		m_primaryProfession2 = TAILORING_1;
	else if (m_bot->HasSpell(HERB_GATHERING_1) && m_primaryProfession1 != HERB_GATHERING_1)
		m_primaryProfession2 = HERB_GATHERING_1;
	else if (m_bot->HasSpell(MINING_1) && m_primaryProfession1 != MINING_1)
		m_primaryProfession2 = MINING_1;
	else if (m_bot->HasSpell(SKINNING_1) && m_primaryProfession1 != SKINNING_1)
		m_primaryProfession2 = SKINNING_1;
}

PlayerbotAI::~PlayerbotAI()
{
    if (m_classAI) delete m_classAI;
}

Player* PlayerbotAI::GetMaster() const
{
    return m_mgr->GetMaster();
}

bool PlayerbotAI::CanReachWithSpellAttack(Unit* target)
{
    bool inrange = false;
    float dist = m_bot->GetCombatDistance(target,false);

    for (SpellRanges::iterator itr = m_spellRangeMap.begin(); itr != m_spellRangeMap.end(); ++itr)
    {
        uint32 spellId = itr->first;

        // ignore positive spells
        if (IsPositiveSpell(spellId))
            continue;

        // ignore active auras
        if (target->HasAura(spellId, EFFECT_INDEX_0))
            continue;

        const SpellEntry* spellInfo = sSpellStore.LookupEntry(spellId);
        if (!spellInfo)
            continue;

        // ignore non-ranged spells
        if (!spellInfo->HasAttribute(SPELL_ATTR_RANGED))
            continue;

        float maxrange = itr->second;

        // DEBUG_LOG("[%s] spell (%s) : dist (%f) < maxrange (%f)",m_bot->GetName(), spellInfo->SpellName[0], dist, maxrange);

        if (dist < maxrange)
        {
            inrange = true;
            break;
        }
    }
    return inrange;
}

bool PlayerbotAI::In_Reach(Unit* Target, uint32 spellId)
{
    if (!Target)
        return false;

    float range = 0;
    float dist = m_bot->GetCombatDistance(Target,false);
    SpellRanges::iterator it;
    it = m_spellRangeMap.find(spellId);
    (it != m_spellRangeMap.end()) ? range = it->second : range = 0;

    // DEBUG_LOG("spell (%u) : range (%f)", spellId, range);

    if (dist > range)
        return false;

    return true;
}

// finds spell ID for matching substring args
// in priority of full text match, spells not taking reagents, and highest rank
uint32 PlayerbotAI::getSpellId(const char* args, bool master) const
{
    if (!*args)
        return 0;

    std::string namepart = args;
    std::wstring wnamepart;

    if (!Utf8toWStr(namepart, wnamepart))
        return 0;

    // converting string that we try to find to lower case
    wstrToLower(wnamepart);

    int loc = 0;
    if (master)
        loc = GetMaster()->GetSession()->GetSessionDbcLocale();
    else
        loc = m_bot->GetSession()->GetSessionDbcLocale();

    uint32 foundSpellId = 0;
    bool foundExactMatch = false;
    bool foundMatchUsesNoReagents = false;

    for (PlayerSpellMap::iterator itr = m_bot->GetSpellMap().begin(); itr != m_bot->GetSpellMap().end(); ++itr)
    {
        uint32 spellId = itr->first;

        if (itr->second.state == PLAYERSPELL_REMOVED || itr->second.disabled || IsPassiveSpell(spellId))
            continue;

        const SpellEntry* pSpellInfo = sSpellStore.LookupEntry(spellId);
        if (!pSpellInfo)
            continue;

        const std::string name = pSpellInfo->SpellName[loc];
        if (name.empty() || !Utf8FitTo(name, wnamepart))
            continue;

        bool isExactMatch = (name.length() == wnamepart.length()) ? true : false;
        bool usesNoReagents = (pSpellInfo->Reagent[0] <= 0) ? true : false;

        // if we already found a spell
        bool useThisSpell = true;
        if (foundSpellId > 0)
        {
            if (isExactMatch && !foundExactMatch) {}
            else if (usesNoReagents && !foundMatchUsesNoReagents) {}
            else if (spellId > foundSpellId) {}
            else
                useThisSpell = false;
        }
        if (useThisSpell)
        {
            foundSpellId = spellId;
            foundExactMatch = isExactMatch;
            foundMatchUsesNoReagents = usesNoReagents;
        }
    }

    return foundSpellId;
}


uint32 PlayerbotAI::getPetSpellId(const char* args) const
{
    if (!*args)
        return 0;

    Pet* pet = m_bot->GetPet();
    if (!pet)
        return 0;

    std::string namepart = args;
    std::wstring wnamepart;

    if (!Utf8toWStr(namepart, wnamepart))
        return 0;

    // converting string that we try to find to lower case
    wstrToLower(wnamepart);

    int loc = GetMaster()->GetSession()->GetSessionDbcLocale();

    uint32 foundSpellId = 0;
    bool foundExactMatch = false;
    bool foundMatchUsesNoReagents = false;

    for (PetSpellMap::iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
    {
        uint32 spellId = itr->first;

        if (itr->second.state == PETSPELL_REMOVED || IsPassiveSpell(spellId))
            continue;

        const SpellEntry* pSpellInfo = sSpellStore.LookupEntry(spellId);
        if (!pSpellInfo)
            continue;

        const std::string name = pSpellInfo->SpellName[loc];
        if (name.empty() || !Utf8FitTo(name, wnamepart))
            continue;

        bool isExactMatch = (name.length() == wnamepart.length()) ? true : false;
        bool usesNoReagents = (pSpellInfo->Reagent[0] <= 0) ? true : false;

        // if we already found a spell
        bool useThisSpell = true;
        if (foundSpellId > 0)
        {
            if (isExactMatch && !foundExactMatch) {}
            else if (usesNoReagents && !foundMatchUsesNoReagents) {}
            else if (spellId > foundSpellId) {}
            else
                useThisSpell = false;
        }
        if (useThisSpell)
        {
            foundSpellId = spellId;
            foundExactMatch = isExactMatch;
            foundMatchUsesNoReagents = usesNoReagents;
        }
    }

    return foundSpellId;
}


uint32 PlayerbotAI::initSpell(uint32 spellId)
{
    // Check if bot knows this spell
    if (!m_bot->HasSpell(spellId))
        return 0;

    uint32 next = 0;
    SpellChainMapNext const& nextMap = sSpellMgr.GetSpellChainNext();
    for (SpellChainMapNext::const_iterator itr = nextMap.lower_bound(spellId); itr != nextMap.upper_bound(spellId); ++itr)
    {
        // Work around buggy chains
        if (sSpellStore.LookupEntry(spellId)->SpellIconID != sSpellStore.LookupEntry(itr->second)->SpellIconID)
            continue;

        SpellChainNode const* node = sSpellMgr.GetSpellChainNode(itr->second);
        // If next spell is a requirement for this one then skip it
        if (node->req == spellId)
            continue;
        if (node->prev == spellId)
        {
            next = initSpell(itr->second);
            break;
        }
    }
    if (next == 0)
    {
        const SpellEntry* const pSpellInfo = sSpellStore.LookupEntry(spellId);
        DEBUG_LOG ("[PlayerbotAI]: initSpell - Playerbot spell init: %s is %u", pSpellInfo->SpellName[0], spellId);

        // Add spell to spellrange map
        Spell *spell = new Spell(m_bot, pSpellInfo, false);
        SpellRangeEntry const* srange = sSpellRangeStore.LookupEntry(pSpellInfo->rangeIndex);
        float range = GetSpellMaxRange(srange);
        if (Player* modOwner = m_bot->GetSpellModOwner())
            modOwner->ApplySpellMod(pSpellInfo->Id, SPELLMOD_RANGE, range, spell);
        m_spellRangeMap.insert(std::pair<uint32, float>(spellId, range));
        delete spell;
    }
    return (next == 0) ? spellId : next;
}


// Pet spells do not form chains like player spells.
// One of the options to initialize a spell is to use spell icon id
uint32 PlayerbotAI::initPetSpell(uint32 spellIconId)
{
    Pet * pet = m_bot->GetPet();

    if (!pet)
        return 0;

    for (PetSpellMap::iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
    {
        const uint32 spellId = itr->first;

        if (itr->second.state == PETSPELL_REMOVED || IsPassiveSpell(spellId))
            continue;

        const SpellEntry* const pSpellInfo = sSpellStore.LookupEntry(spellId);
        if (!pSpellInfo)
            continue;

        if (pSpellInfo->SpellIconID == spellIconId)
            return spellId;
    }

    // Nothing found
    return 0;
}

/*
 * Send a list of equipment that is in bot's inventor that is currently unequipped.
 * This is called when the master is inspecting the bot.
 */

void PlayerbotAI::SendNotEquipList(Player& /*player*/)
{
    // find all unequipped items and put them in
    // a vector of dynamically created lists where the vector index is from 0-18
    // and the list contains Item* that can be equipped to that slot
    // Note: each dynamically created list in the vector must be deleted at end
    // so NO EARLY RETURNS!
    // see enum EquipmentSlots in Player.h to see what equipment slot each index in vector
    // is assigned to. (The first is EQUIPMENT_SLOT_HEAD=0, and last is EQUIPMENT_SLOT_TABARD=18)
    std::list<Item*>* equip[19];
    for (uint8 i = 0; i < 19; ++i)
        equip[i] = nullptr;

    // list out items in main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!pItem)
            continue;

        uint16 dest;
        uint8 msg = m_bot->CanEquipItem(NULL_SLOT, dest, pItem, !pItem->IsBag());
        if (msg != EQUIP_ERR_OK)
            continue;

        // the dest looks like it includes the old loc in the 8 higher bits
        // so casting it to a uint8 strips them
        int8 equipSlot = uint8(dest);
        if (!(equipSlot >= 0 && equipSlot < 19))
            continue;

        // create a list if one doesn't already exist
        if (equip[equipSlot] == nullptr)
            equip[equipSlot] = new std::list<Item*>;

        std::list<Item*>* itemListForEqSlot = equip[equipSlot];
        itemListForEqSlot->push_back(pItem);
    }

    // list out items in other removable backpacks
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        const Bag* const pBag = (Bag *) m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);
        if (pBag)
            for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                Item* const pItem = m_bot->GetItemByPos(bag, slot);
                if (!pItem)
                    continue;

                uint16 dest;
                uint8 msg = m_bot->CanEquipItem(NULL_SLOT, dest, pItem, !pItem->IsBag());
                if (msg != EQUIP_ERR_OK)
                    continue;

                int8 equipSlot = uint8(dest);
                if (!(equipSlot >= 0 && equipSlot < 19))
                    continue;

                // create a list if one doesn't already exist
                if (equip[equipSlot] == nullptr)
                    equip[equipSlot] = new std::list<Item*>;

                std::list<Item*>* itemListForEqSlot = equip[equipSlot];
                itemListForEqSlot->push_back(pItem);
            }
    }

    TellMaster("Here's all the items in my inventory that I can equip.");
    ChatHandler ch(GetMaster());

    const std::string descr[] = { "head", "neck", "shoulders", "body", "chest",
                                  "waist", "legs", "feet", "wrists", "hands", "finger1", "finger2",
                                  "trinket1", "trinket2", "back", "mainhand", "offhand", "ranged",
                                  "tabard" };

    // now send client all items that can be equipped by slot
    for (uint8 equipSlot = 0; equipSlot < 19; ++equipSlot)
    {
        if (equip[equipSlot] == nullptr)
            continue;
        std::list<Item*>* itemListForEqSlot = equip[equipSlot];
        std::ostringstream out;
        out << descr[equipSlot] << ": ";
        for (std::list<Item*>::iterator it = itemListForEqSlot->begin(); it != itemListForEqSlot->end(); ++it)
        {
            const ItemPrototype* const pItemProto = (*it)->GetProto();

            std::string itemName = pItemProto->Name1;
            ItemLocalization(itemName, pItemProto->ItemId);

            out << " |cffffffff|Hitem:" << pItemProto->ItemId
                << ":0:0:0:0:0:0:0" << "|h[" << itemName
                << "]|h|r";
        }
        ch.SendSysMessage(out.str().c_str());

        delete itemListForEqSlot; // delete list of Item*
    }
}

/*
* Send a list of equipment that is the bot currently has equipped and would be upgrading with
* the new item passed in.
*/

void PlayerbotAI::SendUpgradingItems(ItemPrototype const *newItem)
{
	// Get the items we would be upgrading and link them back to our master
	std::list<const Item*> existing = GetExistingItemsInSlotByProto(newItem);

	ChatHandler ch(GetMaster());
	std::ostringstream out;
	// Check if nothing is currently equipped
	if (existing.size() == 0) {
		TellMaster("Upgrade for me. I have nothing equipped currently!");
		return;
	}
		
	for (std::list<const Item*>::const_iterator iterator = existing.begin(), end = existing.end(); iterator != end; ++iterator) {
		const Item* const pItem = *iterator;
		//const ItemPrototype* const pItemProto = *iterator;

		std::string itemName = pItem->GetProto()->Name1;
		ItemLocalization(itemName, pItem->GetProto()->ItemId);

		out << " |cffffffff|Hitem:" << pItem->GetProto()->ItemId
			<< ":0:0:0:0:0:0:0" << "|h[" << itemName
			<< "]|h|r";
	}
	TellMaster("Upgrades the following items:");
	ch.SendSysMessage(out.str().c_str());
}

void PlayerbotAI::SendQuestNeedList()
{
    std::ostringstream out;

    for (BotNeedItem::iterator itr = m_needItemList.begin(); itr != m_needItemList.end(); ++itr)
    {
        ItemPrototype const* pItemProto = sObjectMgr.GetItemPrototype(itr->first);
        if(pItemProto)
        {
            std::string itemName = pItemProto->Name1;
            ItemLocalization(itemName, pItemProto->ItemId);

            out << " " << itr->second << "x|cffffffff|Hitem:" << pItemProto->ItemId
                << ":0:0:0:0:0:0:0" << "|h[" << itemName
                << "]|h|r";
        }
    }

    for (BotNeedItem::iterator itr = m_needCreatureOrGOList.begin(); itr != m_needCreatureOrGOList.end(); ++itr)
    {
        CreatureInfo const* cInfo = ObjectMgr::GetCreatureTemplate(itr->first);
        if (cInfo)
        {
            std::string creatureName = cInfo->Name;
            CreatureLocalization(creatureName, cInfo->Entry);
            out << " " << itr->second << "x|cFFFFFF00|Hcreature_entry:" << itr->first << "|h[" << creatureName << "]|h|r";
        }

        if (m_bot->HasQuestForGO(itr->first))
        {
            GameObjectInfo const* gInfo = ObjectMgr::GetGameObjectInfo(itr->first);
            if (gInfo)
            {
                std::string gameobjectName = gInfo->name;
                GameObjectLocalization(gameobjectName, gInfo->id);
                out << " " << itr->second << "x|cFFFFFF00|Hgameobject_entry:" << itr->first << "|h[" << gameobjectName << "]|h|r";
            }
        }
    }

    TellMaster("Here's a list of all things needed for quests:");
    if (!out.str().empty())
        TellMaster(out.str().c_str());
}

bool PlayerbotAI::IsItemUseful(uint32 itemid)
{
    const static uint32 item_weapon_skills[MAX_ITEM_SUBCLASS_WEAPON] =
    {
        SKILL_AXES,     SKILL_2H_AXES,  SKILL_BOWS,          SKILL_GUNS,      SKILL_MACES,
        SKILL_2H_MACES, SKILL_POLEARMS, SKILL_SWORDS,        SKILL_2H_SWORDS, 0,
        SKILL_STAVES,   0,              0,                   SKILL_UNARMED,   0,
        SKILL_DAGGERS,  SKILL_THROWN,   SKILL_ASSASSINATION, SKILL_CROSSBOWS, SKILL_WANDS,
        SKILL_FISHING
    };

    const static uint32 item_armor_skills[MAX_ITEM_SUBCLASS_ARMOR] =
    {
        0, SKILL_CLOTH, SKILL_LEATHER, SKILL_MAIL, SKILL_PLATE_MAIL, 0, SKILL_SHIELD, 0, 0, 0
    };

    ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(itemid);

	// Not true, if we are low enough level and have nothing equipped it can be useful. And if nothing else it's useful for selling.
    /*if (!pProto || pProto->Quality < ITEM_QUALITY_NORMAL)
        return false;*/

    // do we already have the max allowed of item if more than zero?
    if (pProto->MaxCount > 0 && m_bot->HasItemCount(itemid, pProto->MaxCount, true))
        return false;

    // quest related items
    if (pProto->StartQuest > 0 && HasCollectFlag(COLLECT_FLAG_QUEST))
        return true;

    switch (pProto->Class)
    {
        case ITEM_CLASS_WEAPON:
            if (pProto->SubClass >= MAX_ITEM_SUBCLASS_WEAPON)
                return false;
            else
                return m_bot->HasSkill(item_weapon_skills[pProto->SubClass]);
                break;
        case ITEM_CLASS_ARMOR:
            if (pProto->SubClass >= MAX_ITEM_SUBCLASS_ARMOR)
                return false;
            else
                return (m_bot->HasSkill(item_armor_skills[pProto->SubClass]));
                break;
        case ITEM_CLASS_QUEST:
            if (!HasCollectFlag(COLLECT_FLAG_QUEST))
                break;
        case ITEM_CLASS_KEY:
            return true;
        case ITEM_CLASS_TRADE_GOODS:
            if (!HasCollectFlag(COLLECT_FLAG_PROFESSION))
                break;

            switch (pProto->SubClass)
            {
                case ITEM_SUBCLASS_PARTS:
                case ITEM_SUBCLASS_EXPLOSIVES:
                case ITEM_SUBCLASS_DEVICES:
                    if (m_bot->HasSkill(SKILL_ENGINEERING))
                        return true;
                    break;
                case ITEM_SUBCLASS_CLOTH:
                    if (m_bot->HasSkill(SKILL_TAILORING))
                        return true;
                    break;
                case ITEM_SUBCLASS_LEATHER:
                    if (m_bot->HasSkill(SKILL_LEATHERWORKING))
                        return true;
                    break;
                case ITEM_SUBCLASS_METAL_STONE:
                    if ((m_bot->HasSkill(SKILL_BLACKSMITHING) ||
                        m_bot->HasSkill(SKILL_ENGINEERING) ||
                        m_bot->HasSkill(SKILL_MINING)))
                        return true;
                    break;
                case ITEM_SUBCLASS_MEAT:
                    if (m_bot->HasSkill(SKILL_COOKING))
                        return true;
                    break;
                case ITEM_SUBCLASS_HERB:
                    if ((m_bot->HasSkill(SKILL_HERBALISM) ||
                        m_bot->HasSkill(SKILL_ALCHEMY)))
                        return true;
                    break;
                case ITEM_SUBCLASS_ELEMENTAL:
                    return true;    // pretty much every profession uses these a bit
                case ITEM_SUBCLASS_ENCHANTING:
                    if (m_bot->HasSkill(SKILL_ENCHANTING))
                        return true;
                    break;
                default:
                    break;
            }
            break;
        case ITEM_CLASS_RECIPE:
        {
            if (!HasCollectFlag(COLLECT_FLAG_PROFESSION))
                break;

            // skip recipes that we have
            if (m_bot->HasSpell(pProto->Spells[2].SpellId))
                break;

            switch (pProto->SubClass)
            {
                case ITEM_SUBCLASS_LEATHERWORKING_PATTERN:
                    if (m_bot->HasSkill(SKILL_LEATHERWORKING))
                        return true;
                    break;
                case ITEM_SUBCLASS_TAILORING_PATTERN:
                    if (m_bot->HasSkill(SKILL_TAILORING))
                        return true;
                    break;
                case ITEM_SUBCLASS_ENGINEERING_SCHEMATIC:
                    if (m_bot->HasSkill(SKILL_ENGINEERING))
                        return true;
                    break;
                case ITEM_SUBCLASS_BLACKSMITHING:
                    if (m_bot->HasSkill(SKILL_BLACKSMITHING))
                        return true;
                    break;
                case ITEM_SUBCLASS_COOKING_RECIPE:
                    if (m_bot->HasSkill(SKILL_COOKING))
                        return true;
                    break;
                case ITEM_SUBCLASS_ALCHEMY_RECIPE:
                    if (m_bot->HasSkill(SKILL_ALCHEMY))
                        return true;
                    break;
                case ITEM_SUBCLASS_FIRST_AID_MANUAL:
                    if (m_bot->HasSkill(SKILL_FIRST_AID))
                        return true;
                    break;
                case ITEM_SUBCLASS_ENCHANTING_FORMULA:
                    if (m_bot->HasSkill(SKILL_ENCHANTING))
                        return true;
                    break;
                case ITEM_SUBCLASS_FISHING_MANUAL:
                    if (m_bot->HasSkill(SKILL_FISHING))
                        return true;
                    break;
                default:
                    break;
            }
        }
        default:
            break;
    }

    return false;
}

// Gets any existing items that are currently equipped in the same slot as the new item
std::list<const Item*> PlayerbotAI::GetExistingItemsInSlot(Item const *pNewItem)
{
	return GetExistingItemsInSlotByProto(pNewItem->GetProto());
}

// Gets any existing items that are currently equipped in the same slot as the new item
std::list<const Item*> PlayerbotAI::GetExistingItemsInSlotByProto(ItemPrototype const *pNewItemProto)
{
	std::list<EquipmentSlots> slots;
	switch (pNewItemProto->InventoryType)
	{
	case INVTYPE_HEAD:
		slots.push_back(EQUIPMENT_SLOT_HEAD);
		break;
	case INVTYPE_NECK:
		slots.push_back(EQUIPMENT_SLOT_NECK);
		break;
	case INVTYPE_SHOULDERS:
		slots.push_back(EQUIPMENT_SLOT_SHOULDERS);
		break;
	case INVTYPE_CHEST:
		slots.push_back(EQUIPMENT_SLOT_CHEST);
		break;
	case INVTYPE_WAIST:
		slots.push_back(EQUIPMENT_SLOT_WAIST);
		break;
	case INVTYPE_LEGS:
		slots.push_back(EQUIPMENT_SLOT_LEGS);
		break;
	case INVTYPE_FEET:
		slots.push_back(EQUIPMENT_SLOT_FEET);
		break;
	case INVTYPE_WRISTS:
		slots.push_back(EQUIPMENT_SLOT_WRISTS);
		break;
	case INVTYPE_HANDS:
		slots.push_back(EQUIPMENT_SLOT_HANDS);
		break;
	case INVTYPE_FINGER:
		slots.push_back(EQUIPMENT_SLOT_FINGER1);
		slots.push_back(EQUIPMENT_SLOT_FINGER2);
		break;
	case INVTYPE_TRINKET:
		slots.push_back(EQUIPMENT_SLOT_TRINKET1);
		slots.push_back(EQUIPMENT_SLOT_TRINKET2);
		break;
	case INVTYPE_WEAPON:
		slots.push_back(EQUIPMENT_SLOT_MAINHAND);
		slots.push_back(EQUIPMENT_SLOT_OFFHAND);
		break;
	case INVTYPE_WEAPONMAINHAND:
	case INVTYPE_2HWEAPON:
		slots.push_back(EQUIPMENT_SLOT_MAINHAND);
		break;
	case INVTYPE_SHIELD:
	case INVTYPE_WEAPONOFFHAND:
		slots.push_back(EQUIPMENT_SLOT_OFFHAND);
		break;
	case INVTYPE_RANGED:
		slots.push_back(EQUIPMENT_SLOT_RANGED);
		break;
	case INVTYPE_CLOAK:
		slots.push_back(EQUIPMENT_SLOT_BACK);
		break;
	default:
		break;
	}

	std::list<const Item*> items;
	// loop through each equipment slot for this item and get the item(s) currently in that slot
	for (std::list<EquipmentSlots>::const_iterator iterator = slots.begin(), end = slots.end(); iterator != end; ++iterator) {
		EquipmentSlots slot = *iterator;

		// Get current item in this slot
		Item* pItemCurrent = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot); // EquipmentSlots
		if (pItemCurrent)
			items.push_back(pItemCurrent);
	}

	return items;
}

bool PlayerbotAI::IsItemAnUpgrade(Item* pItem)
{
	// If the item is not useful, it is not an upgrade
	if (!IsItemUseful(pItem->GetProto()->ItemId)) {
		DEBUG_LOG("Item is not useful to me.");
		return false;
	}

	// If the item is not armor or a weapon, we can't consider it as an upgrade
	switch (pItem->GetProto()->Class)
	{
		case ITEM_CLASS_WEAPON:
		case ITEM_CLASS_ARMOR:
			break;
		default:
			return false;
			break;
	}

	// Get the current items to compare
	std::list<const Item*> currentItems = GetExistingItemsInSlot(pItem);

	// If there are no items equipped it is an upgrade
	if (currentItems.size() == 0)
		return true;

	bool isUpgrade = false;

	// loop through each item and check it against the item given to us
	// if we find an upgrade we break immediately
	for (std::list<const Item*>::const_iterator iterator = currentItems.begin(), end = currentItems.end(); iterator != end; ++iterator) {
		const Item* pItemCurrent = *iterator;

		// If we have an item in this slot
		if (pItemCurrent)
		{
			// Check with class AI
			if (m_classAI->IsNewItemAnUpgrade(pItem, pItemCurrent)) {
				isUpgrade = true;
				break;
			}
		}

	}

	return isUpgrade;
}

void PlayerbotAI::ReloadAI()
{
    switch (m_bot->getClass())
    {
        case CLASS_PRIEST:
            if (m_classAI) delete m_classAI;
            m_combatStyle = COMBAT_RANGED;
            m_classAI = (PlayerbotClassAI *) new PlayerbotPriestAI(GetMaster(), m_bot, this);
            break;
        case CLASS_MAGE:
            if (m_classAI) delete m_classAI;
            m_combatStyle = COMBAT_RANGED;
            m_classAI = (PlayerbotClassAI *) new PlayerbotMageAI(GetMaster(), m_bot, this);
            break;
        case CLASS_WARLOCK:
            if (m_classAI) delete m_classAI;
            m_combatStyle = COMBAT_RANGED;
            m_classAI = (PlayerbotClassAI *) new PlayerbotWarlockAI(GetMaster(), m_bot, this);
            break;
        case CLASS_WARRIOR:
            if (m_classAI) delete m_classAI;
            m_combatStyle = COMBAT_MELEE;
            m_classAI = (PlayerbotClassAI *) new PlayerbotWarriorAI(GetMaster(), m_bot, this);
            break;
        case CLASS_SHAMAN:
            if (m_classAI) delete m_classAI;
            if (m_bot->GetSpec() == SHAMAN_SPEC_ENHANCEMENT)
            {
            m_combatStyle = COMBAT_MELEE;
            }
            else
                m_combatStyle = COMBAT_RANGED;
            m_classAI = (PlayerbotClassAI *) new PlayerbotShamanAI(GetMaster(), m_bot, this);
            break;
        case CLASS_PALADIN:
            if (m_classAI) delete m_classAI;
            m_combatStyle = COMBAT_MELEE;
            m_classAI = (PlayerbotClassAI *) new PlayerbotPaladinAI(GetMaster(), m_bot, this);
            break;
        case CLASS_ROGUE:
            if (m_classAI) delete m_classAI;
            m_combatStyle = COMBAT_MELEE;
            m_classAI = (PlayerbotClassAI *) new PlayerbotRogueAI(GetMaster(), m_bot, this);
            break;
        case CLASS_DRUID:
            if (m_classAI) delete m_classAI;
            if (m_bot->GetSpec() == DRUID_SPEC_FERAL)
            {
            m_combatStyle = COMBAT_MELEE;
            }
            else
                m_combatStyle = COMBAT_RANGED;
            m_classAI = (PlayerbotClassAI *) new PlayerbotDruidAI(GetMaster(), m_bot, this);
            break;
        case CLASS_HUNTER:
            if (m_classAI) delete m_classAI;
            m_combatStyle = COMBAT_RANGED;
            m_classAI = (PlayerbotClassAI *) new PlayerbotHunterAI(GetMaster(), m_bot, this);
            break;
    }

    HERB_GATHERING      = initSpell(HERB_GATHERING_1);
    MINING              = initSpell(MINING_1);
    SKINNING            = initSpell(SKINNING_1);

	ALCHEMY				= initSpell(ALCHEMY_1);
	BLACKSMITHING		= initSpell(BLACKSMITHING_1);
	ENCHANTING			= initSpell(ENCHANTING_1);
	ENGINEERING			= initSpell(ENGINEERING_1);
	LEATHERWORKING		= initSpell(LEATHERWORKING_1);
	TAILORING			= initSpell(TAILORING_1);
}

void PlayerbotAI::SendOrders(Player& /*player*/)
{
    std::ostringstream out;

    if (!m_combatOrder)
        out << "Got no combat orders!";
    else if (m_combatOrder & ORDERS_TANK)
        out << "I TANK";
    else if (m_combatOrder & ORDERS_ASSIST)
        out << "I ASSIST " << (m_targetAssist ? m_targetAssist->GetName() : "unknown");
    else if (m_combatOrder & ORDERS_HEAL)
        out << "I HEAL and DISPEL";
    else if (m_combatOrder & ORDERS_NODISPEL)
        out << "I HEAL and WON'T DISPEL";
    else if (m_combatOrder & ORDERS_PASSIVE)
        out << "I'm PASSIVE";
    if ((m_combatOrder & ORDERS_PRIMARY) && (m_combatOrder & (ORDERS_PROTECT | ORDERS_RESIST)))
    {
        out << " and ";
    if (m_combatOrder & ORDERS_PROTECT)
        out << "I PROTECT " << (m_targetProtect ? m_targetProtect->GetName() : "unknown");
        if (m_combatOrder & ORDERS_RESIST)
        {
            if (m_combatOrder & ORDERS_RESIST_FIRE)
                out << "I RESIST FIRE";
            if (m_combatOrder & ORDERS_RESIST_NATURE)
                out << "I RESIST NATURE";
            if (m_combatOrder & ORDERS_RESIST_FROST)
                out << "I RESIST FROST";
            if (m_combatOrder & ORDERS_RESIST_SHADOW)
                out << "I RESIST SHADOW";
        }
    }
    out << ".";

    if (m_mgr->m_confDebugWhisper)
    {
        out << " " << (IsInCombat() ? "I'm in COMBAT! " : "Not in combat. ");
        out << "Current state is ";
        if (m_botState == BOTSTATE_NORMAL)
            out << "NORMAL";
        else if (m_botState == BOTSTATE_COMBAT)
            out << "COMBAT";
        else if (m_botState == BOTSTATE_DEAD)
            out << "DEAD";
        else if (m_botState == BOTSTATE_DEADRELEASED)
            out << "RELEASED";
        else if (m_botState == BOTSTATE_LOOTING)
            out << "LOOTING";
        else if (m_botState == BOTSTATE_FLYING)
            out << "FLYING";
        out << ". Movement order is ";
        if (m_movementOrder == MOVEMENT_NONE)
            out << "NONE";
        else if (m_movementOrder == MOVEMENT_FOLLOW)
            out << "FOLLOW " << (m_followTarget ? m_followTarget->GetName() : "unknown");
        else if (m_movementOrder == MOVEMENT_STAY)
            out << "STAY";
        out << ". Got " << m_attackerInfo.size() << " attacker(s) in list.";
        out << " Next action in " << (m_ignoreAIUpdatesUntilTime - time(0)) << "sec.";
    }

    TellMaster(out.str().c_str());
    if (m_DelayAttack)
        TellMaster("My combat delay is '%u'", m_DelayAttack);
}

// handle outgoing packets the server would send to the client
void PlayerbotAI::HandleBotOutgoingPacket(const WorldPacket& packet)
{
    switch (packet.GetOpcode())
    {
        case SMSG_DUEL_WINNER:
        {
            m_bot->HandleEmoteCommand(EMOTE_ONESHOT_APPLAUD);
            return;
        }
        case SMSG_DUEL_COMPLETE:
        {
            SetIgnoreUpdateTime(4);
            m_ScenarioType = SCENARIO_PVE;
            ReloadAI();
            m_bot->GetMotionMaster()->Clear(true);
            return;
        }
        case SMSG_DUEL_OUTOFBOUNDS:
        {
            m_bot->HandleEmoteCommand(EMOTE_ONESHOT_CHICKEN);
            return;
        }
        case SMSG_DUEL_REQUESTED:
        {
            SetIgnoreUpdateTime(0);
            WorldPacket p(packet);
            ObjectGuid flagGuid;
            p >> flagGuid;
            ObjectGuid playerGuid;
            p >> playerGuid;
            Player* const pPlayer = ObjectAccessor::FindPlayer(playerGuid);
            if (canObeyCommandFrom(*pPlayer))
            {
                m_bot->GetMotionMaster()->Clear(true);
                WorldPacket* const packet = new WorldPacket(CMSG_DUEL_ACCEPTED, 8);
                *packet << flagGuid;
                m_bot->GetSession()->QueuePacket(packet); // queue the packet to get around race condition

                // follow target in casting range
                float angle = rand_float(0, M_PI_F);
                float dist = rand_float(4, 10);

                m_bot->GetMotionMaster()->Clear(true);
                m_bot->GetMotionMaster()->MoveFollow(pPlayer, dist, angle);

                m_bot->SetSelectionGuid(ObjectGuid(playerGuid));
                SetIgnoreUpdateTime(4);
                m_ScenarioType = SCENARIO_PVP_DUEL;
            }
            return;
        }

        case SMSG_AUCTION_COMMAND_RESULT:
        {
            uint32 auctionId, Action, ErrorCode;
            std::string action[3] = {"Creating", "Cancelling", "Bidding"};
            std::ostringstream out;

            WorldPacket p(packet);
            p >> auctionId;
            p >> Action;
            p >> ErrorCode;
            p.resize(12);

            switch (ErrorCode)
            {
                case AUCTION_OK:
                {
                    out << "|cff1eff00|h" << action[Action] << " was successful|h|r";
                    break;
                }
                case AUCTION_ERR_DATABASE:
                {
                    out << "|cffff0000|hWhile" << action[Action] << ", an internal error occured|h|r";
                    break;
                }
                case AUCTION_ERR_NOT_ENOUGH_MONEY:
                {
                    out << "|cffff0000|hWhile " << action[Action] << ", I didn't have enough money|h|r";
                    break;
                }
                case AUCTION_ERR_ITEM_NOT_FOUND:
                {
                    out << "|cffff0000|hItem was not found!|h|r";
                    break;
                }
                case AUCTION_ERR_BID_OWN:
                {
                    out << "|cffff0000|hI cannot bid on my own auctions!|h|r";
                    break;
                }
            }
            TellMaster(out.str().c_str());
            return;
        }

        case SMSG_INVENTORY_CHANGE_FAILURE:
        {
            WorldPacket p(packet);
            uint8 err;
            p >> err;

            if (m_inventory_full)
                return;

            m_inventory_full = true;

            if (err != EQUIP_ERR_OK)
            {
                switch (err)
                {
                    case EQUIP_ERR_CANT_CARRY_MORE_OF_THIS:
                        TellMaster("I can't carry anymore of those.");
                        return;
                    case EQUIP_ERR_MISSING_REAGENT:
                        TellMaster("I'm missing some reagents for that.");
                        return;
                    case EQUIP_ERR_ITEM_LOCKED:
                        TellMaster("That item is locked.");
                        return;
                    case EQUIP_ERR_ALREADY_LOOTED:
                        TellMaster("That is already looted.");
                        return;
                    case EQUIP_ERR_INVENTORY_FULL:
                        TellMaster("My inventory is full.");
                        return;
                    case EQUIP_ERR_NOT_IN_COMBAT:
                        TellMaster("I can't use that in combat.");
                        return;
                    case EQUIP_ERR_LOOT_CANT_LOOT_THAT_NOW:
                        TellMaster("I can't get that now.");
                        return;
                    case EQUIP_ERR_BANK_FULL:
                        TellMaster("My bank is full.");
                        return;
                    case EQUIP_ERR_ITEM_NOT_FOUND:
                        TellMaster("I can't find the item.");
                        return;
                    case EQUIP_ERR_TOO_FAR_AWAY_FROM_BANK:
                        TellMaster("I'm too far from the bank.");
                        return;
                    default:
                        TellMaster("I can't use that.");
                        DEBUG_LOG ("[PlayerbotAI]: HandleBotOutgoingPacket - SMSG_INVENTORY_CHANGE_FAILURE: %u", err);
                        return;
                }
            }
        }

/*        case SMSG_CAST_FAILED:
        {
            WorldPacket p(packet);
            uint32 spellId;
            uint8 result;
            // uint8 castCount;

            p.hexlike();

            p >> spellId >> result;
            p.resize(5);

            SpellEntry const* spellInfo = sSpellStore.LookupEntry(spellId);
            if (!spellInfo)
                return;

            if (result != SPELL_CAST_OK)
            {
                switch (result)
                {
                    case SPELL_FAILED_INTERRUPTED:
                    {
                        DEBUG_LOG("spell %s interrupted (%u)",spellInfo->SpellName[0], result);
                        return;
                    }
                    case SPELL_FAILED_BAD_TARGETS:
                    {
                        // DEBUG_LOG("[%s]bad target (%u) for spellId (%u) & m_CurrentlyCastingSpellId (%u)",m_bot->GetName(),result,spellId,m_CurrentlyCastingSpellId);
                        Spell* const pSpell = GetCurrentSpell();
                        if (pSpell)
                            pSpell->cancel();
                        return;
                    }
                    default:
                        //DEBUG_LOG ("[%s] SMSG_CAST_FAIL: %s err (%u)", m_bot->GetName(), spellInfo->SpellName[0],result);
                        return;
                }
            }
            return;
        } */

/*        case SMSG_SPELL_FAILURE:
        {
            WorldPacket p(packet);
            uint8 castCount;
            uint32 spellId;
            ObjectGuid casterGuid;

            p >> casterGuid.ReadAsPacked();
            if (casterGuid != m_bot->GetObjectGuid())
                return;

            p >> castCount >> spellId;
            if (m_CurrentlyCastingSpellId == spellId)
            {
                SetIgnoreUpdateTime(0);
                m_CurrentlyCastingSpellId = 0;
            }
            return;
        }
*/
        // if a change in speed was detected for the master
        // make sure we have the same mount status
        case SMSG_FORCE_RUN_SPEED_CHANGE:
        {
            WorldPacket p(packet);
            ObjectGuid guid;

            p >> guid.ReadAsPacked();
            if (guid != GetMaster()->GetObjectGuid())
                return;
            if (GetMaster()->IsMounted() && !m_bot->IsMounted())
            {
                //Player Part
                if (!GetMaster()->GetAurasByType(SPELL_AURA_MOUNTED).empty())
                {
                    int32 master_speed1 = 0;
                    int32 master_speed2 = 0;
                    master_speed1 = GetMaster()->GetAurasByType(SPELL_AURA_MOUNTED).front()->GetSpellProto()->EffectBasePoints[1];
                    master_speed2 = GetMaster()->GetAurasByType(SPELL_AURA_MOUNTED).front()->GetSpellProto()->EffectBasePoints[2];

                    //Bot Part
                    uint32 spellMount = 0;
                    for (PlayerSpellMap::iterator itr = m_bot->GetSpellMap().begin(); itr != m_bot->GetSpellMap().end(); ++itr)
                    {
                        uint32 spellId = itr->first;
                        if (itr->second.state == PLAYERSPELL_REMOVED || itr->second.disabled || IsPassiveSpell(spellId))
                            continue;
                        const SpellEntry* pSpellInfo = sSpellStore.LookupEntry(spellId);
                        if (!pSpellInfo)
                            continue;

                        if (pSpellInfo->EffectApplyAuraName[0] == SPELL_AURA_MOUNTED)
                        {
                            if (pSpellInfo->EffectApplyAuraName[1] == SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED)
                            {
                                if (pSpellInfo->EffectBasePoints[1] == master_speed1)
                                {
                                    spellMount = spellId;
                                    break;
                                }
                            }
                            else if (pSpellInfo->EffectApplyAuraName[2] == SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED)
                                if (pSpellInfo->EffectBasePoints[2] == master_speed2)
                                {
                                    spellMount = spellId;
                                    break;
                                }
                        }
                    }
                    if (spellMount > 0) m_bot->CastSpell(m_bot, spellMount, false);
                }
            }
            else if (!GetMaster()->IsMounted() && m_bot->IsMounted())
            {
                WorldPacket emptyPacket;
                m_bot->GetSession()->HandleCancelMountAuraOpcode(emptyPacket);  //updated code
            }
            return;
        }

        // If the leader role was given to the bot automatically give it to the master
        // if the master is in the group, otherwise leave group
        case SMSG_GROUP_SET_LEADER:
        {
            WorldPacket p(packet);
            std::string name;
            p >> name;
            if (m_bot->GetGroup() && name == m_bot->GetName())
            {
                if (m_bot->GetGroup()->IsMember(GetMaster()->GetObjectGuid()))
                {
                    p.resize(8);
                    p << GetMaster()->GetObjectGuid();
                    m_bot->GetSession()->HandleGroupSetLeaderOpcode(p);
                }
                else
                {
                    p.clear(); // not really needed
                    m_bot->GetSession()->HandleGroupDisbandOpcode(p); // packet not used updated code
                }
            }
            return;
        }

        // If the master leaves the group, then the bot leaves too
        case SMSG_PARTY_COMMAND_RESULT:
        {
            WorldPacket p(packet);
            uint32 operation;
            p >> operation;
            std::string member;
            p >> member;
            uint32 result;
            p >> result;
            p.clear();
            if (operation == PARTY_OP_LEAVE)
                if (member == GetMaster()->GetName())
                    m_bot->GetSession()->HandleGroupDisbandOpcode(p);  // packet not used updated code
            return;
        }

        // Handle Group invites (auto accept if master is in group, otherwise decline & send message
        case SMSG_GROUP_INVITE:
        {
            if (m_bot->GetGroupInvite())
            {
                const Group* const grp = m_bot->GetGroupInvite();
                if (!grp)
                    return;

                Player* const inviter = sObjectMgr.GetPlayer(grp->GetLeaderGuid());
                if (!inviter)
                    return;

                WorldPacket p;
                if (!canObeyCommandFrom(*inviter))
                {
                    std::string buf = "I can't accept your invite unless you first invite my master ";
                    buf += GetMaster()->GetName();
                    buf += ".";
                    SendWhisper(buf, *inviter);
                    m_bot->GetSession()->HandleGroupDeclineOpcode(p); // packet not used
                }
                else
				{ 
					// set movement to follow master when joining the group
					SetMovementOrder(MOVEMENT_FOLLOW, GetMaster());
					m_bot->GetSession()->HandleGroupAcceptOpcode(p);  // packet not used
				}
                    
            }
            return;
        }

		case SMSG_GROUP_LIST:
		{
			// if bot is no longer in a group, set movement to stay
			if (!m_bot->IsInSameGroupWith(GetMaster()))
				SetMovementOrder(MOVEMENT_STAY);
			return;
		}

		// Handle guild invite (auto accept if master is the guild leader, otherwise decline & send message)
		case SMSG_GUILD_INVITE:
		{
			if (m_bot->GetGuildIdInvited())
			{
				const int guildId = m_bot->GetGuildIdInvited();
				if (!guildId)
				{
					TellMaster("No guild id found!");
					return;
				}

				const Guild* const guild = sGuildMgr.GetGuildById(guildId);
				if (!guild)
				{
					TellMaster("No guild returned from Guild Manager by the id");
					return;
				}
					
				Player* const leader = sObjectMgr.GetPlayer(guild->GetLeaderGuid());
				if (!leader)
					return;

				WorldPacket p;
				if (!canObeyCommandFrom(*leader))
				{
					// TODO: This is incorrect. We are actually sending a message to the leader and not the person who invited us.
					std::string buf = "I can't accept your invite unless my master is the Guild leader ";
					buf += GetMaster()->GetName();
					buf += ".";
					SendWhisper(buf, *leader);
					m_bot->GetSession()->HandleGuildDeclineOpcode(p); // packet not used
				}
				else
					m_bot->GetSession()->HandleGuildAcceptOpcode(p);  // packet not used
			}
			return;
		}

        // Handle when another player opens the trade window with the bot
        // also sends list of tradable items bot can trade if bot is allowed to obey commands from
        case SMSG_TRADE_STATUS:
        {
            if (m_bot->GetTrader() == nullptr)
                break;

            WorldPacket p(packet);
            uint32 status;
            p >> status;
            p.resize(4);

            //4 == TRADE_STATUS_TRADE_ACCEPT
            if (status == 4)
            {
                m_bot->GetSession()->HandleAcceptTradeOpcode(p);  // packet not used
                SetQuestNeedItems();
            }

            //1 == TRADE_STATUS_BEGIN_TRADE
            else if (status == 1)
            {
                m_bot->GetSession()->HandleBeginTradeOpcode(p); // packet not used

                if (!canObeyCommandFrom(*(m_bot->GetTrader())))
                {
                    SendWhisper("I'm not allowed to trade you any of my items, but you are free to give me money or items.", *(m_bot->GetTrader()));
                    return;
                }

                // list out items available for trade
                std::ostringstream out;

                out << "In my main backpack:";
                // list out items in main backpack
                for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
                {
                    const Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                    if (pItem)
                        MakeItemLink(pItem, out, true);
                }
                ChatHandler ch(m_bot->GetTrader());
                ch.SendSysMessage(out.str().c_str());

                // list out items in other removable backpacks
                for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
                {
                    const Bag* const pBag = (Bag *) m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);
                    if (pBag)
                    {
                        std::ostringstream outbag;
                        outbag << "In my ";
                        const ItemPrototype* const pBagProto = pBag->GetProto();
                        std::string bagName = pBagProto->Name1;
                        ItemLocalization(bagName, pBagProto->ItemId);
                        outbag << bagName << ":";

                        for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
                        {
                            const Item* const pItem = m_bot->GetItemByPos(bag, slot);
                            if (pItem)
                                MakeItemLink(pItem, outbag, true);
                        }
                        ch.SendSysMessage(outbag.str().c_str());
                    }
                }

                // calculate how much money bot has
                uint32 copper = m_bot->GetMoney();
                uint32 gold = uint32(copper / 10000);
                copper -= (gold * 10000);
                uint32 silver = uint32(copper / 100);
                copper -= (silver * 100);

                // send bot the message
                std::ostringstream whisper;
                whisper << "I have |cff00ff00" << gold
                        << "|r|cfffffc00g|r|cff00ff00" << silver
                        << "|r|cffcdcdcds|r|cff00ff00" << copper
                        << "|r|cffffd333c|r";
                SendWhisper(whisper.str().c_str(), *(m_bot->GetTrader()));
            }
            return;
        }

        case SMSG_SPELL_START:
        {

            WorldPacket p(packet);

            ObjectGuid castItemGuid;
            p >> castItemGuid.ReadAsPacked();
            ObjectGuid casterGuid;
            p >> casterGuid.ReadAsPacked();
            if (casterGuid != m_bot->GetObjectGuid())
                return;

            uint32 spellId;
            p >> spellId;
            uint16 castFlags;
            p >> castFlags;
            uint32 msTime;
            p >> msTime;

            // DEBUG_LOG("castItemGuid (%s) casterItemGuid(%s) spellId (%u) castFlags (%u) msTime (%u)",castItemGuid.GetString().c_str(),casterGuid.GetString().c_str(), spellId, castFlags, msTime);

            const SpellEntry* const pSpellInfo = sSpellStore.LookupEntry(spellId);
            if (!pSpellInfo)
                return;

            if (pSpellInfo->AuraInterruptFlags & AURA_INTERRUPT_FLAG_NOT_SEATED)
                return;

            uint32 CastingTime = !IsChanneledSpell(pSpellInfo) ? GetSpellCastTime(pSpellInfo) : GetSpellDuration(pSpellInfo);

            SetIgnoreUpdateTime((msTime / 1000) + 1);

            return;
        }

        case SMSG_SPELL_GO:
        {
            WorldPacket p(packet);

            ObjectGuid castItemGuid;
            p >> castItemGuid.ReadAsPacked();
            ObjectGuid casterGuid;
            p >> casterGuid.ReadAsPacked();
            if (casterGuid != m_bot->GetObjectGuid())
                return;

            uint32 spellId;
            p >> spellId;
            uint16 castFlags;
            p >> castFlags;

            return;
        }

        // if someone tries to resurrect, then accept
        case SMSG_RESURRECT_REQUEST:
        {
            if (!m_bot->isAlive())
            {
                WorldPacket p(packet);
                ObjectGuid guid;
                p >> guid;

                WorldPacket* const packet = new WorldPacket(CMSG_RESURRECT_RESPONSE, 8 + 1);
                *packet << guid;
                *packet << uint8(1);                        // accept
                m_bot->GetSession()->QueuePacket(packet);   // queue the packet to get around race condition

                // set back to normal
                SetState(BOTSTATE_NORMAL);
                SetIgnoreUpdateTime(0);
            }
            return;
        }

        case SMSG_LOOT_RESPONSE:
        {
            WorldPacket p(packet); // (8+1+4+1+1+4+4+4+4+4+1)
            ObjectGuid guid;
            uint8 loot_type;

            p >> guid;      // 8 corpse guid
            p >> loot_type; // 1 loot type

            // Create the loot object and check it exists
            Loot* loot = sLootMgr.GetLoot(m_bot, guid);
            if (!loot)
            {
                sLog.outError("PLAYERBOT Debug Error cannot get loot object info in SMSG_LOOT_RESPONSE!");
                return;
            }

            // Pickup money
            if (loot->GetGoldAmount())
                loot->SendGold(m_bot);

            // Pick up the items
            // Get the list of items first and iterate it
            LootItemList lootList;
            loot->GetLootItemsListFor(m_bot, lootList);

            for (LootItemList::const_iterator lootItr = lootList.begin(); lootItr != lootList.end(); ++lootItr)
            {
                LootItem* lootItem = *lootItr;

                // Skip non lootable items
                if (lootItem->GetSlotTypeForSharedLoot(m_bot, loot) != LOOT_SLOT_NORMAL)
                    continue;

                // If bot is skinning or has collect all orders: autostore all items
                // else bot has order to only loot quest or useful items
                if (loot_type == LOOT_SKINNING || HasCollectFlag(COLLECT_FLAG_LOOT) || (loot_type == LOOT_CORPSE && (IsInQuestItemList(lootItem->itemId) || IsItemUseful(lootItem->itemId))))
                {
                    // item may be blocked by roll system or already looted or another cheating possibility
                    if (lootItem->isBlocked || lootItem->GetSlotTypeForSharedLoot(m_bot, loot) == MAX_LOOT_SLOT_TYPE)
                    {
                        sLog.outError("PLAYERBOT debug Bot %s have no right to loot itemId(%u)", m_bot->GetGuidStr().c_str(), lootItem->itemId);
                    continue;
                    }

                    // Try to send the item to bot
                    InventoryResult result = loot->SendItem(m_bot, lootItem);

                    // If inventory is full: release loot
                    if (result == EQUIP_ERR_INVENTORY_FULL)
                    {
                        loot->Release(m_bot);
                        return;
                    }

                    ObjectGuid const& lguid = loot->GetLootGuid();

                    // Check that bot has either equiped or received the item
                    // then change item's loot state
                    if (result == EQUIP_ERR_OK && lguid.IsItem())
                {
                        if (Item* item = m_bot->GetItemByGuid(lguid))
                            item->SetLootState(ITEM_LOOT_CHANGED);
                    }
                }
            }

            // release loot
            loot->Release(m_bot);

            return;
        }

        case SMSG_LOOT_RELEASE_RESPONSE:
        {
            WorldPacket p(packet);
            ObjectGuid guid;

            p >> guid;

            if (guid == m_lootCurrent)
            {
                Creature *c = m_bot->GetMap()->GetCreature(m_lootCurrent);

                if (c && c->GetCreatureInfo()->SkinningLootId && !c->GetLootStatus() != CREATURE_LOOT_STATUS_LOOTED)
                {
                    uint32 reqSkill = c->GetCreatureInfo()->GetRequiredLootSkill();
                    // check if it is a leather skin and if it is to be collected (could be ore or herb)
                    if (m_bot->HasSkill(reqSkill) && ((reqSkill != SKILL_SKINNING) ||
                                                      (HasCollectFlag(COLLECT_FLAG_SKIN) && reqSkill == SKILL_SKINNING)))
                    {
                        // calculate skill requirement
                        uint32 skillValue = m_bot->GetPureSkillValue(reqSkill);
                        uint32 targetLevel = c->getLevel();
                        uint32 reqSkillValue = targetLevel < 10 ? 0 : targetLevel < 20 ? (targetLevel - 10) * 10 : targetLevel * 5;
                        if (skillValue >= reqSkillValue)
                        {
                            if (m_lootCurrent != m_lootPrev)    // if this wasn't previous loot try again
                            {
                                m_lootPrev = m_lootCurrent;
                                SetIgnoreUpdateTime();
                                return; // so that the DoLoot function is called again to get skin
                            }
                        }
                        else
                            TellMaster("My skill is %u but it requires %u", skillValue, reqSkillValue);
                    }
                }

                // if previous is current, clear
                if (m_lootPrev == m_lootCurrent)
                    m_lootPrev = ObjectGuid();
                // clear current target
                m_lootCurrent = ObjectGuid();
                // clear movement
                m_bot->GetMotionMaster()->Clear(false);
                m_bot->GetMotionMaster()->MoveIdle();
                SetIgnoreUpdateTime();
            }

            return;
        }

        case SMSG_LOOT_ROLL_WON:
        {
            WorldPacket p(packet);   // (8+4+4+4+4+8+1+1)
            ObjectGuid guid;
            uint32 itemid;

            p.read_skip<ObjectGuid>(); // creature guid what we're looting
            p.read_skip<uint32>();   // item slot in loot
            p >> itemid;             // the itemEntryId for the item that shall be rolled fo
            p.read_skip<uint32>();   // randomSuffix
            p.read_skip<uint32>();   // Item random property
            p >> guid;               // guid of the player who won
            p.read_skip<uint8>();    // rollnumber related to SMSG_LOOT_ROLL
            p.read_skip<uint8>();    // Rolltype related to SMSG_LOOT_ROLL

            if (m_bot->GetObjectGuid() != guid)
                return;

            SetState(BOTSTATE_DELAYED);

            /* ItemPrototype const *pProto = ObjectMgr::GetItemPrototype(itemid);
            if(pProto)
            {
                std::ostringstream out;
                out << "|cff009900" << "I won: |r";
                MakeItemLink(pProto,out);
                if (FindItem(itemid,true))
                    out << "and have the item";
                TellMaster(out.str().c_str());
            }*/
            return;
        }

        case SMSG_PARTYKILLLOG:
        {
            // reset AI delay so bots immediately respond to next combat target & or looting/skinning
            SetIgnoreUpdateTime(0);
            return;
        }

        case SMSG_ITEM_PUSH_RESULT:
        {
            WorldPacket p(packet);  // (8+4+4+4+1+4+4+4+4+4+4)
            ObjectGuid guid;

            p >> guid;              // 8 player guid
            if (m_bot->GetObjectGuid() != guid)
                return;

            uint8 bagslot;
            uint32 itemslot, itemid, count, totalcount;

            p.read_skip<uint32>();  // 4 0=looted, 1=from npc
            p.read_skip<uint32>();  // 4 0=received, 1=created
            p.read_skip<uint32>();  // 4 IsShowChatMessage
            p >> bagslot;           // 1 bagslot
            p >> itemslot;          // 4 item slot, but when added to stack: 0xFFFFFFFF
            p >> itemid;            // 4 item entry id
            p.read_skip<uint32>();  // 4 SuffixFactor
            p.read_skip<uint32>();  // 4 random item property id
            p >> count;             // 4 count of items
            p >> totalcount;        // 4 count of items in inventory

            if (IsInQuestItemList(itemid))
            {
                m_needItemList[itemid] = (m_needItemList[itemid] - count);
                if (m_needItemList[itemid] <= 0)
                    m_needItemList.erase(itemid);
            }

            return;
        }

            /* uncomment this and your bots will tell you all their outgoing packet opcode names */
               /*case SMSG_MONSTER_MOVE:
               case SMSG_UPDATE_WORLD_STATE:
               case SMSG_COMPRESSED_UPDATE_OBJECT:
               case MSG_MOVE_SET_FACING:
               case MSG_MOVE_STOP:
               case MSG_MOVE_HEARTBEAT:
               case MSG_MOVE_STOP_STRAFE:
               case MSG_MOVE_START_STRAFE_LEFT:
               case SMSG_UPDATE_OBJECT:
               case MSG_MOVE_START_FORWARD:
               case MSG_MOVE_START_STRAFE_RIGHT:
               case SMSG_DESTROY_OBJECT:
               case MSG_MOVE_START_BACKWARD:
               case MSG_MOVE_FALL_LAND:
               case MSG_MOVE_JUMP:
            return;*/

               default:
               {
            /*const char* oc = LookupOpcodeName(packet.GetOpcode());

                std::ostringstream out;
                out << "botout: " << oc;
                sLog.outError(out.str().c_str());

            TellMaster(oc);*/
               }
    }
}

uint8 PlayerbotAI::GetHealthPercent(const Unit& target) const
{
    return (static_cast<float> (target.GetHealth()) / target.GetMaxHealth()) * 100;
}

uint8 PlayerbotAI::GetHealthPercent() const
{
    return GetHealthPercent(*m_bot);
}

uint8 PlayerbotAI::GetManaPercent(const Unit& target) const
{
    return (static_cast<float> (target.GetPower(POWER_MANA)) / target.GetMaxPower(POWER_MANA)) * 100;
}

uint8 PlayerbotAI::GetManaPercent() const
{
    return GetManaPercent(*m_bot);
}

uint8 PlayerbotAI::GetBaseManaPercent(const Unit& target) const
{
    if (target.GetPower(POWER_MANA) >= target.GetCreateMana())
        return (100);
    else
        return (static_cast<float> (target.GetPower(POWER_MANA)) / target.GetCreateMana()) * 100;
}

uint8 PlayerbotAI::GetBaseManaPercent() const
{
    return GetBaseManaPercent(*m_bot);
}

uint8 PlayerbotAI::GetRageAmount(const Unit& target) const
{
    return (static_cast<float> (target.GetPower(POWER_RAGE)));
}

uint8 PlayerbotAI::GetRageAmount() const
{
    return GetRageAmount(*m_bot);
}

uint8 PlayerbotAI::GetEnergyAmount(const Unit& target) const
{
    return (static_cast<float> (target.GetPower(POWER_ENERGY)));
}

uint8 PlayerbotAI::GetEnergyAmount() const
{
    return GetEnergyAmount(*m_bot);
}

bool PlayerbotAI::HasAura(uint32 spellId, const Unit& player) const
{
    if (spellId <= 0)
        return false;

    for (Unit::SpellAuraHolderMap::const_iterator iter = player.GetSpellAuraHolderMap().begin(); iter != player.GetSpellAuraHolderMap().end(); ++iter)
    {
        if (iter->second->GetId() == spellId)
            return true;
    }
    return false;
}

bool PlayerbotAI::HasAura(const char* spellName) const
{
    return HasAura(spellName, *m_bot);
}

bool PlayerbotAI::HasAura(const char* spellName, const Unit& player) const
{
    uint32 spellId = getSpellId(spellName);
    return (spellId) ? HasAura(spellId, player) : false;
}

// looks through all items / spells that bot could have to get a mount
Item* PlayerbotAI::FindMount(uint32 matchingRidingSkill) const
{
    // list out items in main backpack

    Item* partialMatch = nullptr;

    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (pItem)
        {
            const ItemPrototype* const pItemProto = pItem->GetProto();
            if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK || pItemProto->RequiredSkill != SKILL_RIDING)
                continue;

            if (pItemProto->RequiredSkillRank == matchingRidingSkill)
                return pItem;

            else if (!partialMatch || (partialMatch && partialMatch->GetProto()->RequiredSkillRank < pItemProto->RequiredSkillRank))
                partialMatch = pItem;
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
                    if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK || pItemProto->RequiredSkill != SKILL_RIDING)
                        continue;

                    if (pItemProto->RequiredSkillRank == matchingRidingSkill)
                        return pItem;

                    else if (!partialMatch || (partialMatch && partialMatch->GetProto()->RequiredSkillRank < pItemProto->RequiredSkillRank))
                        partialMatch = pItem;
                }
            }
    }
    return partialMatch;
}

Item* PlayerbotAI::FindFood() const
{
    // list out items in main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (pItem)
        {
            const ItemPrototype* const pItemProto = pItem->GetProto();
            if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                continue;

            if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == ITEM_SUBCLASS_CONSUMABLE)
                // if is FOOD
                // this enum is no longer defined in mangos. Is it no longer valid?
                // according to google it was 11
                if (pItemProto->Spells[0].SpellCategory == 11)
                    return pItem;
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

                    if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                        continue;

                    // this enum is no longer defined in mangos. Is it no longer valid?
                    // according to google it was 11
                    if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == ITEM_SUBCLASS_CONSUMABLE)
                        // if is FOOD
                        // this enum is no longer defined in mangos. Is it no longer valid?
                        // according to google it was 11
                        // if (pItemProto->Spells[0].SpellCategory == SPELL_CATEGORY_FOOD)
                        if (pItemProto->Spells[0].SpellCategory == 11)
                            return pItem;
                }
            }
    }
    return nullptr;
}

Item* PlayerbotAI::FindDrink() const
{
    // list out items in main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (pItem)
        {
            const ItemPrototype* const pItemProto = pItem->GetProto();

            if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                continue;

            if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == ITEM_SUBCLASS_CONSUMABLE)
                // if (pItemProto->Spells[0].SpellCategory == SPELL_CATEGORY_DRINK)

                // this enum is no longer defined in mangos. Is it no longer valid?
                // according to google it was 59
                // if (pItemProto->Spells[0].SpellCategory == 59)
                if (pItemProto->Spells[0].SpellCategory == 59)
                    return pItem;
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

                    if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                        continue;

                    if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == ITEM_SUBCLASS_CONSUMABLE)
                        // if is WATER
                        // SPELL_CATEGORY_DRINK is no longer defined in an enum in mangos
                        // google says the valus is 59. Is this still valid?
                        // if (pItemProto->Spells[0].SpellCategory == SPELL_CATEGORY_DRINK)
                        if (pItemProto->Spells[0].SpellCategory == 59)
                            return pItem;
                }
            }
    }
    return nullptr;
}

Item* PlayerbotAI::FindBandage() const
{
    // list out items in main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (pItem)
        {
            const ItemPrototype* const pItemProto = pItem->GetProto();

            if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                continue;

            if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == ITEM_SUBCLASS_BANDAGE)
                return pItem;
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

                    if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                        continue;

                    if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == ITEM_SUBCLASS_BANDAGE)
                        return pItem;
                }
            }
    }
    return nullptr;
}
//Find Poison ...Natsukawa
Item* PlayerbotAI::FindPoison() const
{
    // list out items in main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (pItem)
        {
            const ItemPrototype* const pItemProto = pItem->GetProto();

            if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                continue;

            if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == 6)
                return pItem;
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

                    if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                        continue;

                    if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->SubClass == 6)
                        return pItem;
                }
            }
    }
    return nullptr;
}

Item* PlayerbotAI::FindConsumable(uint32 displayId) const
{
    // list out items in main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (pItem)
        {
            const ItemPrototype* const pItemProto = pItem->GetProto();

            if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                continue;

            if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->DisplayInfoID == displayId)
                return pItem;
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

                    if (!pItemProto || m_bot->CanUseItem(pItemProto) != EQUIP_ERR_OK)
                        continue;

                    if (pItemProto->Class == ITEM_CLASS_CONSUMABLE && pItemProto->DisplayInfoID == displayId)
                        return pItem;
                }
            }
    }
    return nullptr;
}

bool PlayerbotAI::FindAmmo() const
{
    for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item* pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (pItem)
        {
           const ItemPrototype* const pItemProto = pItem->GetProto();

           if (pItemProto->Class == ITEM_CLASS_PROJECTILE && m_bot->CheckAmmoCompatibility(pItemProto))
           {
                m_bot->SetAmmo(pItem->GetEntry());
                return true;
           }
        }
    }
    for (int i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        if (Bag* pBag = (Bag*)m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i))
        {
            for (uint32 j = 0; j < pBag->GetBagSize(); ++j)
            {
                Item* pItem = m_bot->GetItemByPos(i, j);
                if (pItem)
                {
                    const ItemPrototype* const pItemProto = pItem->GetProto();

                    if (pItemProto->Class == ITEM_CLASS_PROJECTILE && m_bot->CheckAmmoCompatibility(pItemProto))
                    {
                        m_bot->SetAmmo(pItem->GetEntry());
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

void PlayerbotAI::InterruptCurrentCastingSpell()
{
    //TellMaster("I'm interrupting my current spell!");
    WorldPacket* const packet = new WorldPacket(CMSG_CANCEL_CAST, 5);  //changed from thetourist suggestion
    *packet << m_CurrentlyCastingSpellId;
    *packet << m_targetGuidCommand;   //changed from thetourist suggestion
    m_CurrentlyCastingSpellId = 0;
    m_bot->GetSession()->QueuePacket(packet);
}

// intelligently sets a reasonable combat order for this bot
// based on its class / level / etc
void PlayerbotAI::Attack(Unit* forcedTarget)
{
    // set combat state, and clear looting, etc...
    if (m_botState != BOTSTATE_COMBAT)
    {
        SetState(BOTSTATE_COMBAT);
        m_lootCurrent = ObjectGuid();
        m_targetCombat = 0;
        m_DelayAttackInit = CurrentTime(); // Combat started, new start time to check CombatDelay for.
    }

    GetCombatTarget(forcedTarget);

    if (!m_targetCombat)
        return;

    m_bot->Attack(m_targetCombat, true);

    // add thingToAttack to loot list
    m_lootTargets.push_back(m_targetCombat->GetObjectGuid());
}

// intelligently sets a reasonable combat order for this bot
// based on its class / level / etc
void PlayerbotAI::GetCombatTarget(Unit* forcedTarget)
{
    // update attacker info now
    UpdateAttackerInfo();

    // check for attackers on protected unit, and make it a forcedTarget if any
    if (!forcedTarget && (m_combatOrder & ORDERS_PROTECT) && m_targetProtect)
    {
        Unit* newTarget = FindAttacker((ATTACKERINFOTYPE) (AIT_VICTIMNOTSELF | AIT_HIGHESTTHREAT), m_targetProtect);
        if (newTarget && newTarget != m_targetCombat)
        {
            forcedTarget = newTarget;
            m_targetType = TARGET_THREATEN;
            if (m_mgr->m_confDebugWhisper)
                TellMaster("Changing target to %s to protect %s", forcedTarget->GetName(), m_targetProtect->GetName());
        }
    }
    else if (forcedTarget)
    {
        if (m_mgr->m_confDebugWhisper)
            TellMaster("Changing target to %s by force!", forcedTarget->GetName());
        m_targetType = (m_combatOrder & ORDERS_TANK ? TARGET_THREATEN : TARGET_NORMAL);
    }

    // we already have a target and we are not forced to change it
    if (m_targetCombat && !forcedTarget)
        return;

    // forced to change target to current target == null operation
    if (forcedTarget && forcedTarget == m_targetCombat)
        return;

    // are we forced on a target?
    if (forcedTarget)
    {
        m_targetCombat = forcedTarget;
        m_targetChanged = true;
    }
    // do we have to assist someone?
    if (!m_targetCombat && (m_combatOrder & ORDERS_ASSIST) && m_targetAssist)
    {
        m_targetCombat = FindAttacker((ATTACKERINFOTYPE) (AIT_VICTIMNOTSELF | AIT_LOWESTTHREAT), m_targetAssist);
        if (m_mgr->m_confDebugWhisper && m_targetCombat)
            TellMaster("Attacking %s to assist %s", m_targetCombat->GetName(), m_targetAssist->GetName());
        m_targetType = (m_combatOrder & ORDERS_TANK ? TARGET_THREATEN : TARGET_NORMAL);
        m_targetChanged = true;
    }
    // are there any other attackers?
    if (!m_targetCombat)
    {
        m_targetCombat = FindAttacker();
        m_targetType = (m_combatOrder & ORDERS_TANK ? TARGET_THREATEN : TARGET_NORMAL);
        m_targetChanged = true;
    }
    // no attacker found anyway
    if (!m_targetCombat)
    {
        m_targetType = TARGET_NORMAL;
        m_targetChanged = false;
        return;
    }

    // if thing to attack is in a duel, then ignore and don't call updateAI for 6 seconds
    // this method never gets called when the bot is in a duel and this code
    // prevents bot from helping
    if (m_targetCombat->GetTypeId() == TYPEID_PLAYER && dynamic_cast<Player*> (m_targetCombat)->duel)
    {
        SetIgnoreUpdateTime(6);
        return;
    }

    m_bot->SetSelectionGuid((m_targetCombat->GetObjectGuid()));
    //SetIgnoreUpdateTime(1);

    if (m_bot->getStandState() != UNIT_STAND_STATE_STAND)
        m_bot->SetStandState(UNIT_STAND_STATE_STAND);
}

void PlayerbotAI::GetDuelTarget(Unit* forcedTarget)
{
    // set combat state, and clear looting, etc...
    if (m_botState != BOTSTATE_COMBAT)
    {
        SetState(BOTSTATE_COMBAT);
        m_targetChanged = true;
        m_targetCombat = forcedTarget;
        m_targetType = TARGET_THREATEN;
        m_combatStyle = COMBAT_MELEE;
    }
    m_bot->Attack(m_targetCombat, true);
}

void PlayerbotAI::DoNextCombatManeuver()
{
    if (!GetClassAI())
        return; // error, error...

    if (m_combatOrder == ORDERS_PASSIVE)
        return;

    // check for new targets
    if (m_ScenarioType == SCENARIO_PVP_DUEL)
        GetDuelTarget(GetMaster()); // TODO: Wow... wait... what? So not right.
    else
        Attack();

	//Removed, was causing a crash
	//when mob use some spell invisibility the bot action
	/*if (m_targetCombat->HasAuraType(SPELL_AURA_MOD_INVISIBILITY) && !(m_combatOrder & ORDERS_HEAL))
	{
		m_bot->AttackStop();
		m_bot->SetSelectionGuid(ObjectGuid());
		m_bot->InterruptNonMeleeSpells(true);
		return;
	}*/

    // clear orders if current target for attacks doesn't make sense anymore
    if (!m_targetCombat || m_targetCombat->isDead() || !m_targetCombat->IsInWorld() || !m_bot->IsHostileTo(m_targetCombat) || !m_bot->IsInMap(m_targetCombat))
    {
        m_bot->AttackStop();
        m_bot->SetSelectionGuid(ObjectGuid());
        MovementReset();
        m_bot->InterruptNonMeleeSpells(true);
        m_targetCombat = 0;
        m_targetChanged = false;
        m_targetType = TARGET_NORMAL;
        SetQuestNeedCreatures();
        if (GetCombatOrder() & ORDERS_TEMP)
        {
            if (GetCombatOrder() & ORDERS_TEMP_WAIT_TANKAGGRO)
                TellMaster("I was still waiting for the tank to gain aggro, but that doesn't make sense anymore...");
            if (GetCombatOrder() & ORDERS_TEMP_WAIT_OOC)
                TellMaster("I was still waiting OOC but that was way off...");
            ClearCombatOrder(ORDERS_TEMP);
        }
        return;
    }

    // new target -> DoFirstCombatManeuver
    if (m_targetChanged)
    {
        switch (GetClassAI()->DoFirstCombatManeuver(m_targetCombat))
        {
            case RETURN_CONTINUE: // true needed for rogue stealth attack
                break;

            case RETURN_NO_ACTION_ERROR:
                TellMaster("FirstCombatManeuver: No action performed due to error. Heading onto NextCombatManeuver.");
            case RETURN_FINISHED_FIRST_MOVES: // false default
            case RETURN_NO_ACTION_UNKNOWN:
            case RETURN_NO_ACTION_OK:
            default: // assume no action -> no return
            m_targetChanged = false;
    }
    }

    // do normal combat movement
    DoCombatMovement();

    if (!m_targetChanged)
    {
        // if m_targetChanged = false
        switch (GetClassAI()->DoNextCombatManeuver(m_targetCombat))
        {
            case RETURN_NO_ACTION_UNKNOWN:
            case RETURN_NO_ACTION_OK:
            case RETURN_CONTINUE:
            case RETURN_NO_ACTION_ERROR:
            default:
                return;
}
    }
}

void PlayerbotAI::DoCombatMovement()
{
    if (!m_targetCombat) return;

    bool meleeReach = m_bot->CanReachWithMeleeAttack(m_targetCombat);

    if (m_combatStyle == COMBAT_MELEE
        && !m_bot->hasUnitState(UNIT_STAT_CHASE)
        && ((m_movementOrder == MOVEMENT_STAY && meleeReach) || m_movementOrder != MOVEMENT_STAY)
        && GetClassAI()->GetWaitUntil() == 0 ) // Not waiting
    {
        // melee combat - chase target if in range or if we are not forced to stay
        m_bot->GetMotionMaster()->Clear(false);
        m_bot->GetMotionMaster()->MoveChase(m_targetCombat);
    }
    else if (m_combatStyle == COMBAT_RANGED
             && m_movementOrder != MOVEMENT_STAY
             && GetClassAI()->GetWaitUntil() == 0 ) // Not waiting
    {
        // ranged combat - just move within spell range
        if (!CanReachWithSpellAttack(m_targetCombat))
        {
            m_bot->GetMotionMaster()->Clear(false);
            m_bot->GetMotionMaster()->MoveChase(m_targetCombat);
        }
        else
            MovementClear();
    }
}

/*
 * IsGroupInCombat()
 *
 * return true if any member of the group is in combat or (error handling only) occupied in some way
 */
bool PlayerbotAI::IsGroupInCombat()
{
    if (!m_bot) return false;
    if (!m_bot->isAlive() || m_bot->IsInDuel()) return true; // Let's just say you're otherwise occupied
    if (m_bot->isInCombat()) return true;

    if (m_bot->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player* groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || !groupMember->isAlive() || groupMember->IsInDuel() || groupMember->isInCombat()) // all occupied in some way
                return true;
        }
    }

    return false;
}

Player* PlayerbotAI::GetGroupTank()
{
    if (!m_bot) return nullptr;

    if (m_bot->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player* groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || !groupMember->GetPlayerbotAI())
                continue;
            if (groupMember->GetPlayerbotAI()->IsTank())
                return groupMember;
        }
    }

    return nullptr;
}

void PlayerbotAI::SetGroupCombatOrder(CombatOrderType co)
{
    if (!m_bot) return;

    if (m_bot->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player* groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || !groupMember->GetPlayerbotAI())
                continue;
            groupMember->GetPlayerbotAI()->SetCombatOrder(co);
        }
    }
    else
        SetCombatOrder(co);
}

void PlayerbotAI::ClearGroupCombatOrder(CombatOrderType co)
{
    if (!m_bot) return;

    if (m_bot->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player* groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || !groupMember->GetPlayerbotAI())
                continue;
            groupMember->GetPlayerbotAI()->ClearCombatOrder(co);
        }
    }
    else
        ClearCombatOrder(co);
}

void PlayerbotAI::SetGroupIgnoreUpdateTime(uint8 t)
{
    if (!m_bot) return;

    if (m_bot->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player* groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || !groupMember->GetPlayerbotAI())
                continue;
            groupMember->GetPlayerbotAI()->SetIgnoreUpdateTime(t);
        }
    }
    else
        SetIgnoreUpdateTime(t);
}

bool PlayerbotAI::GroupHoTOnTank()
{
    if (!m_bot) return false;

    bool bReturn = false;

    if (m_bot->GetGroup())
    {
        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
        {
            Player* groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || !groupMember->GetPlayerbotAI())
                continue;
            if (groupMember->GetPlayerbotAI()->GetClassAI()->CastHoTOnTank())
                bReturn = true;
        }

        if (bReturn)
        {
            for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
            {
                Player* groupMember = sObjectMgr.GetPlayer(itr->guid);
                if (!groupMember || !groupMember->GetPlayerbotAI())
                    continue;
                groupMember->GetPlayerbotAI()->SetIgnoreUpdateTime(1);
            }
        }
    }
    else // No group
    {
        if (GetClassAI()->CastHoTOnTank())
        {
            SetIgnoreUpdateTime(1);
            return true;
        }
    }

    return bReturn;
}

bool PlayerbotAI::CanPull(Player &fromPlayer)
{
    if (!m_bot) return false;
    if (!GetClassAI()) return false;

    if (!m_bot->GetGroup() || fromPlayer.GetGroup() != m_bot->GetGroup())
    {
        SendWhisper("I can't pull - we're not in the same group.", fromPlayer);
        return false;
    }

    if (IsGroupInCombat()) // TODO: add raid support
    {
        SendWhisper("Unable to pull - the group is already in combat", fromPlayer);
        return false;
    }

    if ((GetCombatOrder() & ORDERS_TANK) == 0)
    {
        SendWhisper("I cannot pull as I do not have combat orders to tank.", fromPlayer);
        return false;
    }

    switch (m_bot->getClass())
    {
       case CLASS_PALADIN:
            if ( ((PlayerbotPaladinAI*)GetClassAI())->CanPull() == false)
            {
                SendWhisper("I cannot pull, I do not have the proper spell or it's not ready yet.", fromPlayer);
                return false;
            }
            break;

        case CLASS_DRUID:
            if ( ((PlayerbotDruidAI*)GetClassAI())->CanPull() == false)
            {
                SendWhisper("I cannot pull, I do not have the proper spell or it's not ready yet.", fromPlayer);
                return false;
            }
            break;

        case CLASS_WARRIOR:
            if ( ((PlayerbotWarriorAI*)GetClassAI())->CanPull() == false)
            {
                SendWhisper("I cannot pull, I do not have the proper weapon and/or ammo.", fromPlayer);
                return false;
            }
            break;

        default:
            SendWhisper("I cannot pull, I am not a tanking class.", fromPlayer);
            return false;
    }

    return true;
}

// This function assumes a "CanPull()" call was preceded (not doing so will result in odd behavior)
bool PlayerbotAI::CastPull()
{
    if (!m_bot) return false;
    if (!GetClassAI()) return false;
    if (!GetCurrentTarget()) return false;

    if ((GetCombatOrder() & ORDERS_TANK) == 0) return false;

    switch (m_bot->getClass())
    {
        case CLASS_PALADIN:
            return ((PlayerbotPaladinAI*)GetClassAI())->Pull();

        case CLASS_DRUID:
            return ((PlayerbotDruidAI*)GetClassAI())->Pull();

        case CLASS_WARRIOR:
            return ((PlayerbotWarriorAI*)GetClassAI())->Pull();

        default:
            return false;
    }

    return false;
}

bool PlayerbotAI::GroupTankHoldsAggro()
{
    if (!m_bot) return false;

    // update attacker info now
    UpdateAttackerInfo();

    if (m_bot->GetGroup())
    {
        Unit* newTarget = FindAttacker((ATTACKERINFOTYPE) (AIT_VICTIMNOTSELF), GetGroupTank());
        if (newTarget)
        {
            return false;
        }
    }
    else
        return false; // no group -> no group tank to hold aggro

    return true;
}

// Wrapper for the UpdateAI cast subfunction
// Each bot class neutralize function will return a spellId
// depending on the creatureType of the target
bool PlayerbotAI::CastNeutralize()
{
    if (!m_bot) return false;
    if (!GetClassAI()) return false;
    if (!m_targetGuidCommand) return false;

    Unit* pTarget = ObjectAccessor::GetUnit(*m_bot, m_targetGuidCommand);
    if (!pTarget) return false;

    Creature * pCreature = (Creature*) pTarget;
    if (!pCreature) return false;

    // Define the target's creature type, so the bot AI will now if
    // it can neutralize it
    uint8 creatureType = 0;
    creatureType = pCreature->GetCreatureInfo()->CreatureType;

    switch (m_bot->getClass())
    {
        default:
            return false;
    }

    // A spellId was found
    if (m_spellIdCommand != 0)
        return true;

    return false;
}

void PlayerbotAI::SetQuestNeedCreatures()
{
    // reset values first
    m_needCreatureOrGOList.clear();

    // run through accepted quests, get quest info and data
    for (int qs = 0; qs < MAX_QUEST_LOG_SIZE; ++qs)
    {
        uint32 questid = m_bot->GetQuestSlotQuestId(qs);
        if (questid == 0)
            continue;

        QuestStatusData &qData = m_bot->getQuestStatusMap()[questid];
        // only check quest if it is incomplete
        if (qData.m_status != QUEST_STATUS_INCOMPLETE)
            continue;

        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if (!qInfo)
            continue;

        // All creature/GO slain/casted (not required, but otherwise it will display "Creature slain 0/10")
        for (int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
        {
            if (!qInfo->ReqCreatureOrGOCount[i] || (qInfo->ReqCreatureOrGOCount[i] - qData.m_creatureOrGOcount[i]) <= 0)
                continue;
            m_needCreatureOrGOList[qInfo->ReqCreatureOrGOId[i]] = (qInfo->ReqCreatureOrGOCount[i] - qData.m_creatureOrGOcount[i]);
        }
    }
}

void PlayerbotAI::SetQuestNeedItems()
{
    // reset values first
    m_needItemList.clear();

    // run through accepted quests, get quest info and data
    for (int qs = 0; qs < MAX_QUEST_LOG_SIZE; ++qs)
    {
        uint32 questid = m_bot->GetQuestSlotQuestId(qs);
        if (questid == 0)
            continue;

        QuestStatusData &qData = m_bot->getQuestStatusMap()[questid];
        // only check quest if it is incomplete
        if (qData.m_status != QUEST_STATUS_INCOMPLETE)
            continue;

        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if (!qInfo)
            continue;

        // check for items we not have enough of
        for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; i++)
        {
            if (!qInfo->ReqItemCount[i] || (qInfo->ReqItemCount[i] - qData.m_itemcount[i]) <= 0)
                continue;
            m_needItemList[qInfo->ReqItemId[i]] = (qInfo->ReqItemCount[i] - qData.m_itemcount[i]);
        }
    }
}

void PlayerbotAI::SetState(BotState state)
{
    // DEBUG_LOG ("[PlayerbotAI]: SetState - %s switch state %d to %d", m_bot->GetName(), m_botState, state );
    m_botState = state;
}

uint8 PlayerbotAI::GetFreeBagSpace() const
{
    uint8 space = 0;
    for (uint8 i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
    {
        Item *pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (!pItem)
            ++space;
    }
    for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
    {
        Bag* pBag = (Bag *) m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
        if (pBag && pBag->GetProto()->BagFamily == BAG_FAMILY_NONE)
            space += pBag->GetFreeSlots();
    }
    return space;
}

void PlayerbotAI::DoFlight()
{
    DEBUG_LOG("[PlayerbotAI]: DoFlight - %s : %s", m_bot->GetName(), m_taxiMaster.GetString().c_str());

    Creature *npc = m_bot->GetNPCIfCanInteractWith(m_taxiMaster, UNIT_NPC_FLAG_FLIGHTMASTER);
    if (!npc)
    {
        DEBUG_LOG("[PlayerbotAI]: DoFlight - %s not found or you can't interact with it.", m_taxiMaster.GetString().c_str());
        return;
    }

    m_bot->ActivateTaxiPathTo(m_taxiNodes, npc);
}

void PlayerbotAI::DoLoot()
{
    // clear BOTSTATE_LOOTING if no more loot targets
    if (m_lootCurrent.IsEmpty() && m_lootTargets.empty())
    {
        // DEBUG_LOG ("[PlayerbotAI]: DoLoot - %s is going back to idle", m_bot->GetName());
        SetState(BOTSTATE_NORMAL);
        m_bot->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_LOOTING);
        m_inventory_full = false;
        return;
    }

    // set first in list to current
    if (m_lootCurrent.IsEmpty())
    {
        m_lootCurrent = m_lootTargets.front();
        m_lootTargets.pop_front();
    }

    WorldObject *wo = m_bot->GetMap()->GetWorldObject(m_lootCurrent);

    // clear invalid object or object that is too far from master
    if (!wo || GetMaster()->GetDistance(wo) > BOTLOOT_DISTANCE)
    {
        m_lootCurrent = ObjectGuid();
        return;
    }

    Creature *c = m_bot->GetMap()->GetCreature(m_lootCurrent);
    GameObject *go = m_bot->GetMap()->GetGameObject(m_lootCurrent);

    // clear creature or object that is not spawned or if not creature or object
    if ((c && c->IsDespawned()) || (go && !go->isSpawned()) || (!c && !go))
    {
        m_lootCurrent = ObjectGuid();
        return;
    }

    uint32 skillId = 0;

    if (c)
    {
        if (c->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE))
            skillId = c->GetCreatureInfo()->GetRequiredLootSkill();

        // not a lootable creature, clear it
        if (!c->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE) &&
            (!c->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE) ||
             (c->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE) && !m_bot->HasSkill(skillId))))
        {
            m_lootCurrent = ObjectGuid();
            // clear movement target, take next target on next update
            m_bot->GetMotionMaster()->Clear(false);
            m_bot->GetMotionMaster()->MoveIdle();
            return;
        }
    }

    if (m_bot->GetDistance(wo) > CONTACT_DISTANCE + wo->GetObjectBoundingRadius())
    {
        m_bot->GetMotionMaster()->MovePoint(wo->GetMapId(), wo->GetPositionX(), wo->GetPositionY(),wo->GetPositionZ());
        // give time to move to point before trying again
        SetIgnoreUpdateTime(1);
    }

    if (m_bot->GetDistance(wo) < INTERACTION_DISTANCE)
    {
        uint32 reqSkillValue = 0;
        uint32 SkillValue = 0;
        bool keyFailed = false;
        bool skillFailed = false;
        bool forceFailed = false;

        if (c)  // creature
        {
            if (c->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE) && !c->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE))
            {
                // loot the creature
                WorldPacket* const packet = new WorldPacket(CMSG_LOOT, 8);
                *packet << m_lootCurrent;
                m_bot->GetSession()->QueuePacket(packet);
                return; // no further processing is needed
                // m_lootCurrent is reset in SMSG_LOOT_RELEASE_RESPONSE after checking for skinloot
            }
            else if (c->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE))
                // not all creature skins are leather, some are ore or herb
                if (m_bot->HasSkill(skillId) && ((skillId != SKILL_SKINNING) ||
                                                 (HasCollectFlag(COLLECT_FLAG_SKIN) && skillId == SKILL_SKINNING)))
                {
                    // calculate skinning skill requirement
                    uint32 targetLevel = c->getLevel();
                    reqSkillValue = targetLevel < 10 ? 0 : targetLevel < 20 ? (targetLevel - 10) * 10 : targetLevel * 5;
                }

            // creatures cannot be unlocked or forced open
            keyFailed = true;
            forceFailed = true;
        }

        if (go) // object
        {
            // add this GO to our collection list if active and is chest/ore/herb
            if (go && HasCollectFlag(COLLECT_FLAG_NEAROBJECT) && go->GetGoType() == GAMEOBJECT_TYPE_CHEST)
            {
                m_collectObjects.push_back(go->GetEntry());
                m_collectObjects.sort();
                m_collectObjects.unique();
            }

            uint32 reqItem = 0;

            // check skill or lock on object
            uint32 lockId = go->GetGOInfo()->GetLockId();
            LockEntry const *lockInfo = sLockStore.LookupEntry(lockId);
            if (lockInfo)
                for (int i = 0; i < 8; ++i)
                {
                    if (lockInfo->Type[i] == LOCK_KEY_ITEM)
                    {
                        if (lockInfo->Index[i] > 0)
                        {
                            reqItem = lockInfo->Index[i];
                            if (m_bot->HasItemCount(reqItem, 1))
                                break;
                            continue;
                        }
                    }
                    else if (lockInfo->Type[i] == LOCK_KEY_SKILL)
                    {
                        switch (LockType(lockInfo->Index[i]))
                        {
                            case LOCKTYPE_OPEN:
                                if (CastSpell(3365))    // Opening
                                    return;
                                break;
                            case LOCKTYPE_CLOSE:
                                if (CastSpell(6233))    // Closing
                                    return;
                                break;
                            case LOCKTYPE_QUICK_OPEN:
                                if (CastSpell(6247))    // Opening
                                    return;
                                break;
                            case LOCKTYPE_QUICK_CLOSE:
                                if (CastSpell(6247))    // Closing
                                    return;
                                break;
                            case LOCKTYPE_OPEN_TINKERING:
                                if (CastSpell(6477))    // Opening
                                    return;
                                break;
                            case LOCKTYPE_OPEN_KNEELING:
                                if (CastSpell(22810))    // Opening; listed with 17667 and 22810
                                    return;
                                break;
                            case LOCKTYPE_OPEN_ATTACKING:
                                if (CastSpell(8386))    // Attacking
                                    return;
                                break;
                            case LOCKTYPE_SLOW_OPEN:
                                if (CastSpell(21651))   // Opening; also had 26868
                                    return;
                                break;
                            case LOCKTYPE_SLOW_CLOSE:
                                if (CastSpell(21652))   // Closing
                                    return;
                                break;
                            default:
                                if (SkillByLockType(LockType(lockInfo->Index[i])) > 0)
                                {
                                    skillId = SkillByLockType(LockType(lockInfo->Index[i]));
                                    reqSkillValue = lockInfo->Skill[i];
                                }
                        }
                    }
                }

            // use key on object if available
            if (reqItem > 0 && m_bot->HasItemCount(reqItem, 1))
            {
                UseItem(FindItem(reqItem), TARGET_FLAG_OBJECT, m_lootCurrent);
                m_lootCurrent = ObjectGuid();
                return;
            }
            else
                keyFailed = true;
        }

        // determine bot's skill value for object's required skill
        if (skillId != SKILL_NONE)
            SkillValue = uint32(m_bot->GetPureSkillValue(skillId));

        // bot has the specific skill or object requires no skill at all
        if ((m_bot->HasSkill(skillId) && skillId != SKILL_NONE) || (skillId == SKILL_NONE && go))
        {
            if (SkillValue >= reqSkillValue)
            {
                switch (skillId)
                {
                    case SKILL_MINING:
                        if (CastSpell(MINING))
                            return;
                        else
                            skillFailed = true;
                        break;
                    case SKILL_HERBALISM:
                        if (CastSpell(HERB_GATHERING))
                            return;
                        else
                            skillFailed = true;
                        break;
                    case SKILL_SKINNING:
                        if (c && HasCollectFlag(COLLECT_FLAG_SKIN) && CastSpell(SKINNING, *c))
                            return;
                        else
                            skillFailed = true;
                        break;
                    case SKILL_LOCKPICKING:
                        if (CastSpell(PICK_LOCK_1))
                            return;
                        else
                            skillFailed = true;
                        break;
                    case SKILL_NONE:
                        if (CastSpell(3365)) //Spell 3365 = Opening?
                            return;
                        else
                            skillFailed = true;
                        break;
                    default:
                        TellMaster("I'm not sure how to get that.");
                        skillFailed = true;
                        DEBUG_LOG ("[PlayerbotAI]:DoLoot Skill %u is not implemented", skillId);
                        break;
                }
            }
            else
            {
                TellMaster("My skill is not high enough. It requires %u, but mine is %u.",
                           reqSkillValue, SkillValue);
                skillFailed = true;
            }
        }
        else
        {
            TellMaster("I do not have the required skill.");
            skillFailed = true;
        }

        if (go) // only go's can be forced
        {
            // if pickable, check if a forcible item is available for the bot
            if (skillId == SKILL_LOCKPICKING && (m_bot->HasSkill(SKILL_BLACKSMITHING) ||
                                                 m_bot->HasSkill(SKILL_ENGINEERING)))
            {
                // check for skeleton keys appropriate for lock value
                if (m_bot->HasSkill(SKILL_BLACKSMITHING))
                {
                    Item *kItem = FindKeyForLockValue(reqSkillValue);
                    if (kItem)
                    {
                        TellMaster("I have a skeleton key that can open it!");
                        UseItem(kItem, TARGET_FLAG_OBJECT, m_lootCurrent);
                        return;
                    }
                    else
                    {
                        TellMaster("I have no skeleton keys that can open that lock.");
                        forceFailed = true;
                    }
                }

                // check for a charge that can blast it open
                if (m_bot->HasSkill(SKILL_ENGINEERING))
                {
                    Item *bItem = FindBombForLockValue(reqSkillValue);
                    if (bItem)
                    {
                        TellMaster("I can blast it open!");
                        UseItem(bItem, TARGET_FLAG_OBJECT, m_lootCurrent);
                        return;
                    }
                    else
                    {
                        TellMaster("I have nothing to blast it open with.");
                        forceFailed = true;
                    }
                }
            }
            else
                forceFailed = true;
        }

        // if all attempts failed in some way then clear because it won't get SMSG_LOOT_RESPONSE
        if (keyFailed && skillFailed && forceFailed)
        {
            DEBUG_LOG ("[PlayerbotAI]: DoLoot attempts failed on [%s]",
                       go ? go->GetGOInfo()->name : c->GetCreatureInfo()->Name);
            m_lootCurrent = ObjectGuid();
        }
            // clear movement target, take next target on next update
        m_bot->GetMotionMaster()->Clear(false);
            m_bot->GetMotionMaster()->MoveIdle();
        }
    }

void PlayerbotAI::AcceptQuest(Quest const *qInfo, Player *pGiver)
{
    if (!qInfo || !pGiver)
        return;

    uint32 quest = qInfo->GetQuestId();

    if (!pGiver->CanShareQuest(qInfo->GetQuestId()))
    {
        // giver can't share quest
        m_bot->ClearDividerGuid();
        return;
    }

    if (!m_bot->CanTakeQuest(qInfo, false))
    {
        // can't take quest
        m_bot->ClearDividerGuid();
        return;
    }

    if (m_bot->GetDividerGuid())
    {
        // send msg to quest giving player
        pGiver->SendPushToPartyResponse(m_bot, QUEST_PARTY_MSG_ACCEPT_QUEST);
        m_bot->ClearDividerGuid();
    }

    if (m_bot->CanAddQuest(qInfo, false))
    {
        m_bot->AddQuest(qInfo, pGiver);

        if (m_bot->CanCompleteQuest(quest))
            m_bot->CompleteQuest(quest);

        // build needed items if quest contains any
        for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; i++)
            if (qInfo->ReqItemCount[i] > 0)
            {
                SetQuestNeedItems();
                break;
            }

        // build needed creatures if quest contains any
        for (int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
            if (qInfo->ReqCreatureOrGOCount[i] > 0)
            {
                SetQuestNeedCreatures();
                break;
            }

        // Runsttren: did not add typeid switch from WorldSession::HandleQuestgiverAcceptQuestOpcode!
        // I think it's not needed, cause typeid should be TYPEID_PLAYER - and this one is not handled
        // there and there is no default case also.

        if (qInfo->GetSrcSpell() > 0)
            m_bot->CastSpell(m_bot, qInfo->GetSrcSpell(), true);
    }
}

void PlayerbotAI::Levelup() 
{
	// Learn all weapon skills if not already known
	switch (m_bot->getClass())
	{
		case CLASS_SHAMAN:
			if (!m_bot->HasSpell(196)) m_bot->addSpell(196, true, true, false, false);	// axes
			if (!m_bot->HasSpell(199)) m_bot->addSpell(199, true, true, false, false);	// 2 handed maces
			if (!m_bot->HasSpell(198)) m_bot->addSpell(198, true, true, false, false);	// maces
			if (!m_bot->HasSpell(197)) m_bot->addSpell(197, true, true, false, false);	// 2 handed axes
			if (!m_bot->HasSpell(1180)) m_bot->addSpell(1180, true, true, false, false);	// daggers
			if (!m_bot->HasSpell(227)) m_bot->addSpell(227, true, true, false, false);	// staves
			break;
		case CLASS_WARRIOR:
			if (!m_bot->HasSpell(196)) m_bot->addSpell(196, true, true, false, false);	// axes
			if (!m_bot->HasSpell(201)) m_bot->addSpell(201, true, true, false, false);	// swords
			if (!m_bot->HasSpell(202)) m_bot->addSpell(202, true, true, false, false);	// 2 handed swords
			if (!m_bot->HasSpell(199)) m_bot->addSpell(199, true, true, false, false);	// 2 handed maces
			if (!m_bot->HasSpell(197)) m_bot->addSpell(197, true, true, false, false);	// 2 handed axes
			if (!m_bot->HasSpell(198)) m_bot->addSpell(198, true, true, false, false);	// maces
			if (!m_bot->HasSpell(200)) m_bot->addSpell(200, true, true, false, false);	// polearms
			if (!m_bot->HasSpell(227)) m_bot->addSpell(227, true, true, false, false);	// staves
			if (!m_bot->HasSpell(1180)) m_bot->addSpell(1180, true, true, false, false);	// daggers
			if (!m_bot->HasSpell(264)) m_bot->addSpell(264, true, true, false, false);	// bows
			if (!m_bot->HasSpell(5011)) m_bot->addSpell(5011, true, true, false, false);	// crossbows
			if (!m_bot->HasSpell(266)) m_bot->addSpell(266, true, true, false, false);	// guns
			if (!m_bot->HasSpell(2567)) m_bot->addSpell(2567, true, true, false, false);	// thrown
			break;
		case CLASS_ROGUE:
			if (!m_bot->HasSpell(196)) m_bot->addSpell(196, true, true, false, false);	// axes
			if (!m_bot->HasSpell(201)) m_bot->addSpell(201, true, true, false, false);	// swords
			if (!m_bot->HasSpell(198)) m_bot->addSpell(198, true, true, false, false);	// maces
			if (!m_bot->HasSpell(1180)) m_bot->addSpell(1180, true, true, false, false);	// daggers
			if (!m_bot->HasSpell(264)) m_bot->addSpell(264, true, true, false, false);	// bows
			if (!m_bot->HasSpell(5011)) m_bot->addSpell(5011, true, true, false, false);	// crossbows
			if (!m_bot->HasSpell(266)) m_bot->addSpell(266, true, true, false, false);	// guns
			if (!m_bot->HasSpell(2567)) m_bot->addSpell(2567, true, true, false, false);	// thrown
			break;
		case CLASS_HUNTER:
			if (!m_bot->HasSpell(199)) m_bot->addSpell(199, true, true, false, false);	// 2 handed maces
			if (!m_bot->HasSpell(197)) m_bot->addSpell(197, true, true, false, false);	// 2 handed axes
			if (!m_bot->HasSpell(196)) m_bot->addSpell(196, true, true, false, false);	// axes
			if (!m_bot->HasSpell(201)) m_bot->addSpell(201, true, true, false, false);	// swords
			if (!m_bot->HasSpell(200)) m_bot->addSpell(200, true, true, false, false);	// polearms
			if (!m_bot->HasSpell(227)) m_bot->addSpell(227, true, true, false, false);	// staves
			if (!m_bot->HasSpell(1180)) m_bot->addSpell(1180, true, true, false, false);	// daggers
			if (!m_bot->HasSpell(264)) m_bot->addSpell(264, true, true, false, false);	// bows
			if (!m_bot->HasSpell(5011)) m_bot->addSpell(5011, true, true, false, false);	// crossbows
			if (!m_bot->HasSpell(266)) m_bot->addSpell(266, true, true, false, false);	// guns
			break;
		case CLASS_PRIEST:
			if (!m_bot->HasSpell(1180)) m_bot->addSpell(1180, true, true, false, false);	// daggers
			if (!m_bot->HasSpell(227)) m_bot->addSpell(227, true, true, false, false);	// staves
			break;
		case CLASS_MAGE:
			if (!m_bot->HasSpell(201)) m_bot->addSpell(201, true, true, false, false);	// swords
			if (!m_bot->HasSpell(227)) m_bot->addSpell(227, true, true, false, false);	// staves
			if (!m_bot->HasSpell(1180)) m_bot->addSpell(1180, true, true, false, false);	// daggers
			break;
		case CLASS_WARLOCK:
			if (!m_bot->HasSpell(201)) m_bot->addSpell(201, true, true, false, false);	// swords
			if (!m_bot->HasSpell(227)) m_bot->addSpell(227, true, true, false, false);	// staves
			if (!m_bot->HasSpell(1180)) m_bot->addSpell(1180, true, true, false, false);	// daggers
			break;
		case CLASS_DRUID:
			if (!m_bot->HasSpell(198)) m_bot->addSpell(198, true, true, false, false);	// maces
			if (!m_bot->HasSpell(199)) m_bot->addSpell(199, true, true, false, false);	// 2 handed maces
			if (!m_bot->HasSpell(200)) m_bot->addSpell(200, true, true, false, false);	// polearms
			if (!m_bot->HasSpell(1180)) m_bot->addSpell(1180, true, true, false, false);	// daggers
			if (!m_bot->HasSpell(227)) m_bot->addSpell(227, true, true, false, false);	// staves
			break;
		case CLASS_PALADIN:
			if (!m_bot->HasSpell(196)) m_bot->addSpell(196, true, true, false, false);	// axes
			if (!m_bot->HasSpell(197)) m_bot->addSpell(197, true, true, false, false);	// 2 handed axes
			if (!m_bot->HasSpell(201)) m_bot->addSpell(201, true, true, false, false);	// swords
			if (!m_bot->HasSpell(202)) m_bot->addSpell(202, true, true, false, false);	// 2 handed swords
			if (!m_bot->HasSpell(200)) m_bot->addSpell(200, true, true, false, false);	// polearms
			if (!m_bot->HasSpell(199)) m_bot->addSpell(199, true, true, false, false);	// 2 handed maces
			if (!m_bot->HasSpell(198)) m_bot->addSpell(198, true, true, false, false);	// maces
			break;
		default:
			break;
	}

	// Create random professions for the bot if they don't have any
	if ((m_primaryProfession1 == 0 || m_primaryProfession2 == 0) && m_bot->getLevel() >= 5)
	{
		int profs[9] = { ALCHEMY_1, BLACKSMITHING_1, ENCHANTING_1, ENGINEERING_1, HERB_GATHERING_1, LEATHERWORKING_1, MINING_1, SKINNING_1, TAILORING_1 };
		if (m_primaryProfession1 == 0)
		{
			int randIdx = rand() % 9;
			m_primaryProfession1 = profs[randIdx];
			m_bot->addSpell(m_primaryProfession1, true, true, false, false);
		}
		if (m_primaryProfession2 == 0)
		{
			// exclude the chosen primary1 profession
			int profs2[8];
			int prof2Idx = 0;
			for (int i = 0; i < 9; i++)
			{
				if (m_primaryProfession1 != profs[i])
				{
					profs2[prof2Idx] = profs[i];
					prof2Idx++;
				}
			}

			// Choose a second profession randomly
			int randIdx = rand() % 8;
			m_primaryProfession2 = profs2[randIdx];
			m_bot->addSpell(m_primaryProfession2, true, true, false, false);
		}
	}
	else {
		// Check level of bot and increase profressions by adding new spell if level appropriate
		if (m_bot->getLevel() >= 35) {
			uint32 alchUpgrades[3] = { ALCHEMY_2, ALCHEMY_3, ALCHEMY_4 };
			uint32 bsUpgrades[3] = { BLACKSMITHING_2, BLACKSMITHING_3, BLACKSMITHING_4 };
			uint32 enchantUpgrades[3] = { ENCHANTING_2, ENCHANTING_3, ENCHANTING_4 };
			uint32 engUpgrades[3] = { ENGINEERING_2, ENGINEERING_3, ENGINEERING_4 };
			uint32 lwUpgrades[3] = { LEATHERWORKING_2, LEATHERWORKING_3, LEATHERWORKING_4 };
			uint32 tailUpgrades[3] = { TAILORING_2, TAILORING_3, TAILORING_4 };
			uint32 herbUpgrades[3] = { HERB_GATHERING_2, HERB_GATHERING_3, HERB_GATHERING_4 };
			uint32 mineUpgrades[3] = { MINING_2, MINING_3, MINING_4 };
			uint32 skinUpgrades[3] = { SKINNING_2, SKINNING_3, SKINNING_4 };

			UpgradeProfession(ALCHEMY_1, alchUpgrades);
			UpgradeProfession(BLACKSMITHING_1, bsUpgrades);
			UpgradeProfession(ENCHANTING_1, enchantUpgrades);
			UpgradeProfession(ENGINEERING_1, engUpgrades);
			UpgradeProfession(LEATHERWORKING_1, lwUpgrades);
			UpgradeProfession(TAILORING_1, tailUpgrades);
			UpgradeProfession(HERB_GATHERING_1, herbUpgrades);
			UpgradeProfession(MINING_1, mineUpgrades);
			UpgradeProfession(SKINNING_1, skinUpgrades);
		}
		else if (m_bot->getLevel() >= 20) {
			uint32 alchUpgrades[3] = { ALCHEMY_2, ALCHEMY_3, 0 };
			uint32 bsUpgrades[3] = { BLACKSMITHING_2, BLACKSMITHING_3, 0 };
			uint32 enchantUpgrades[3] = { ENCHANTING_2, ENCHANTING_3, 0 };
			uint32 engUpgrades[3] = { ENGINEERING_2, ENGINEERING_3, 0 };
			uint32 lwUpgrades[3] = { LEATHERWORKING_2, LEATHERWORKING_3, 0 };
			uint32 tailUpgrades[3] = { TAILORING_2, TAILORING_3, 0 };
			uint32 herbUpgrades[3] = { HERB_GATHERING_2, HERB_GATHERING_3, 0 };
			uint32 mineUpgrades[3] = { MINING_2, MINING_3, 0 };
			uint32 skinUpgrades[3] = { SKINNING_2, SKINNING_3, 0 };

			UpgradeProfession(ALCHEMY_1, alchUpgrades);
			UpgradeProfession(BLACKSMITHING_1, bsUpgrades);
			UpgradeProfession(ENCHANTING_1, enchantUpgrades);
			UpgradeProfession(ENGINEERING_1, engUpgrades);
			UpgradeProfession(LEATHERWORKING_1, lwUpgrades);
			UpgradeProfession(TAILORING_1, tailUpgrades);
			UpgradeProfession(HERB_GATHERING_1, herbUpgrades);
			UpgradeProfession(MINING_1, mineUpgrades);
			UpgradeProfession(SKINNING_1, skinUpgrades);
		}
		else if (m_bot->getLevel() >= 10) {
			uint32 alchUpgrades[3] = { ALCHEMY_2, 0, 0 };
			uint32 bsUpgrades[3] = { BLACKSMITHING_2, 0, 0 };
			uint32 enchantUpgrades[3] = { ENCHANTING_2, 0, 0 };
			uint32 engUpgrades[3] = { ENGINEERING_2, 0, 0 };
			uint32 lwUpgrades[3] = { LEATHERWORKING_2, 0, 0 };
			uint32 tailUpgrades[3] = { TAILORING_2, 0, 0 };
			uint32 herbUpgrades[3] = { HERB_GATHERING_2, 0, 0 };
			uint32 mineUpgrades[3] = { MINING_2, 0, 0 };
			uint32 skinUpgrades[3] = { SKINNING_2, 0, 0 };

			UpgradeProfession(ALCHEMY_1, alchUpgrades);
			UpgradeProfession(BLACKSMITHING_1, bsUpgrades);
			UpgradeProfession(ENCHANTING_1, enchantUpgrades);
			UpgradeProfession(ENGINEERING_1, engUpgrades);
			UpgradeProfession(LEATHERWORKING_1, lwUpgrades);
			UpgradeProfession(TAILORING_1, tailUpgrades);
			UpgradeProfession(HERB_GATHERING_1, herbUpgrades);
			UpgradeProfession(MINING_1, mineUpgrades);
			UpgradeProfession(SKINNING_1, skinUpgrades);
		}
	}
	

	// Increase skills to max for current level
	m_bot->UpdateSkillsToMaxSkillsForLevel();

	// Apply all active talent spec for the bot
	ApplyActiveTalentSpec();
}

void PlayerbotAI::UpgradeProfession(uint32 profId, uint32 upgrades[3])
{
	if (m_primaryProfession1 == profId || m_primaryProfession2 == profId) {
		for (int i = 0; i < 3; i++) {
			if (upgrades[i] != 0) {
				if (!m_bot->HasSpell(upgrades[i])) m_bot->addSpell(upgrades[i], true, true, false, false);
			}
		}
	}
}

void PlayerbotAI::TurnInQuests(WorldObject *questgiver)
{
    ObjectGuid giverGUID = questgiver->GetObjectGuid();

	//TellMaster("Turning in quest!");
    /*if (!m_bot->IsInMap(questgiver))
        TellMaster("hey you are turning in quests without me!");
    else
    {*/
        m_bot->SetSelectionGuid(giverGUID);

        // auto complete every completed quest this NPC has
        m_bot->PrepareQuestMenu(giverGUID);
        QuestMenu& questMenu = m_bot->PlayerTalkClass->GetQuestMenu();
        for (uint32 iI = 0; iI < questMenu.MenuItemCount(); ++iI)
        {
            QuestMenuItem const& qItem = questMenu.GetItem(iI);
            uint32 questID = qItem.m_qId;
            Quest const* pQuest = sObjectMgr.GetQuestTemplate(questID);

            std::ostringstream out;
            std::string questTitle  = pQuest->GetTitle();
            QuestLocalization(questTitle, questID);

            QuestStatus status = m_bot->GetQuestStatus(questID);

            // if quest is complete, turn it in
			// playerbot update, even if quest is not complete allow it to be turned in anyway
            if (status == QUEST_STATUS_COMPLETE || status == QUEST_STATUS_INCOMPLETE)
            {
                // if bot hasn't already turned quest in
                if (!m_bot->GetQuestRewardStatus(questID))
                {
                    // auto reward quest if no choice in reward
                    if (pQuest->GetRewChoiceItemsCount() == 0)
                    {
						// Just reward quest
						m_bot->RewardQuest(pQuest, 0, questgiver, false);
						out << "Quest complete: |cff808080|Hquest:" << questID << ':' << pQuest->GetQuestLevel() << "|h[" << questTitle << "]|h|r";

						// As a bot we don't care if the quest can be rewarded or not
                        /*if (m_bot->CanRewardQuest(pQuest, false))
                        {
                            m_bot->RewardQuest(pQuest, 0, questgiver, false);
                            out << "Quest complete: |cff808080|Hquest:" << questID << ':' << pQuest->GetQuestLevel() << "|h[" << questTitle << "]|h|r";
                        }
                        else
                            out << "|cffff0000Unable to turn quest in:|r |cff808080|Hquest:" << questID << ':' << pQuest->GetQuestLevel() << "|h[" << questTitle << "]|h|r";*/
                    }

                    // auto reward quest if one item as reward
                    else if (pQuest->GetRewChoiceItemsCount() == 1)
                    {
                        int rewardIdx = 0;
                        ItemPrototype const *pRewardItem = sObjectMgr.GetItemPrototype(pQuest->RewChoiceItemId[rewardIdx]);
                        std::string itemName = pRewardItem->Name1;
                        ItemLocalization(itemName, pRewardItem->ItemId);

						m_bot->RewardQuest(pQuest, rewardIdx, questgiver, true);

						ItemLocalization(itemName, pRewardItem->ItemId);

						out << "Quest complete: "
							<< " |cff808080|Hquest:" << questID << ':' << pQuest->GetQuestLevel()
							<< "|h[" << questTitle << "]|h|r reward: |cffffffff|Hitem:"
							<< pRewardItem->ItemId << ":0:0:0:0:0:0:0" << "|h[" << itemName << "]|h|r";

                        /*if (m_bot->CanRewardQuest(pQuest, rewardIdx, false))
                        {
                            m_bot->RewardQuest(pQuest, rewardIdx, questgiver, true);

                            std::string itemName = pRewardItem->Name1;
                            ItemLocalization(itemName, pRewardItem->ItemId);

                            out << "Quest complete: "
                                << " |cff808080|Hquest:" << questID << ':' << pQuest->GetQuestLevel()
                                << "|h[" << questTitle << "]|h|r reward: |cffffffff|Hitem:"
                                << pRewardItem->ItemId << ":0:0:0:0:0:0:0" << "|h[" << itemName << "]|h|r";
                        }
                        else
                            out << "|cffff0000Unable to turn quest in:|r "
                                << "|cff808080|Hquest:" << questID << ':'
                                << pQuest->GetQuestLevel() << "|h[" << questTitle << "]|h|r"
                                << " reward: |cffffffff|Hitem:"
                                << pRewardItem->ItemId << ":0:0:0:0:0:0:0" << "|h[" << itemName << "]|h|r";*/
                    }

                    // else multiple rewards - auto pick based on whether or not an item is an upgrade
                    else
                    {
						// TODO: Auto pick reward based on IsAnUpgrade method
						// TOOD: If is not an upgrade, pick the item we could possibly use (random if multiple)
						// TODO: Otherwise just pick randomly since it doesn't really matter

						int rewardIdx = -1;
                        for (uint8 i = 0; i < pQuest->GetRewChoiceItemsCount(); ++i)
                        {
							// Is the item useful
							if (IsItemUseful(pQuest->RewChoiceItemId[i]))
							{
								ItemPrototype const * const pRewardItem = sObjectMgr.GetItemPrototype(pQuest->RewChoiceItemId[i]);

								// TODO: Fix this
								// Is the item an upgrade? If so, immediately choose this one (we are foregoing the possibility there is a better upgrade here, might be dangerous)
								/*if (IsItemAnUpgrade(pRewardItem))
								{
									rewardIdx = i;
									break;
								}*/
								
								// Not an upgrade but it is useful. Set the reward index to this but keep looping through looking for an upgrade
								rewardIdx = i;
							}
                            
                            // If this is the last rewarded item and we haven't chosen anything yet, choose this one
							if (i == (pQuest->GetRewChoiceItemsCount() - 1) && rewardIdx == -1)
								rewardIdx = i;
                        }

						// If a reward was chosen
						if (rewardIdx > -1)
						{
							ItemPrototype const *pRewardItem = sObjectMgr.GetItemPrototype(pQuest->RewChoiceItemId[rewardIdx]);
							std::string itemName = pRewardItem->Name1;
							ItemLocalization(itemName, pRewardItem->ItemId);

							m_bot->RewardQuest(pQuest, rewardIdx, questgiver, true);

							ItemLocalization(itemName, pRewardItem->ItemId);

							out << "Quest complete: "
								<< " |cff808080|Hquest:" << questID << ':' << pQuest->GetQuestLevel()
								<< "|h[" << questTitle << "]|h|r reward: |cffffffff|Hitem:"
								<< pRewardItem->ItemId << ":0:0:0:0:0:0:0" << "|h[" << itemName << "]|h|r";
						}
						else
						{
							TellMaster("No reward was chosen for quest!");
						}
                    }
                }
            }

			// We want bots to complete incomplete quests. This is so we don't have to do the quest multiple times for each bot
			// When we complete the quest, the bot should also.
            /*else if (status == QUEST_STATUS_INCOMPLETE)
                out << "|cffff0000Quest incomplete:|r "
                    << " |cff808080|Hquest:" << questID << ':' << pQuest->GetQuestLevel() << "|h[" << questTitle << "]|h|r";*/

            else if (status == QUEST_STATUS_AVAILABLE)
                out << "|cff00ff00Quest available:|r "
                    << " |cff808080|Hquest:" << questID << ':' << pQuest->GetQuestLevel() << "|h[" << questTitle << "]|h|r";

            if (!out.str().empty())
                TellMaster(out.str());
        }
    //}
}

bool PlayerbotAI::IsInCombat()
{
    Pet *pet;
    bool inCombat = false;
    inCombat |= m_bot->isInCombat();
    pet = m_bot->GetPet();
    if (pet)
        inCombat |= pet->isInCombat();
    inCombat |= GetMaster()->isInCombat();
    if (m_bot->GetGroup())
    {
        GroupReference *ref = m_bot->GetGroup()->GetFirstMember();
        while (ref)
        {
            inCombat |= ref->getSource()->isInCombat();
            pet = ref->getSource()->GetPet();
            if (pet)
                inCombat |= pet->isInCombat();
            ref = ref->next();
        }
    }
    return inCombat;
}

bool PlayerbotAI::IsRegenerating()
{
    Unit::SpellAuraHolderMap& auras = m_bot->GetSpellAuraHolderMap();
    for (Unit::SpellAuraHolderMap::iterator aura = auras.begin(); aura != auras.end(); aura++)
    {
        SpellEntry const* spell = aura->second->GetSpellProto();
        if (!spell)
            continue;
        if (spell->Category == 59 || spell->Category == 11){
            return true;
        }
    }
    if (m_bot->getStandState() != UNIT_STAND_STATE_STAND)
        m_bot->SetStandState(UNIT_STAND_STATE_STAND);
    return false;
}

void PlayerbotAI::UpdateAttackersForTarget(Unit *victim)
{
    HostileReference *ref = victim->getHostileRefManager().getFirst();
    while (ref)
    {
        ThreatManager *target = ref->getSource();
        ObjectGuid guid = target->getOwner()->GetObjectGuid();
        m_attackerInfo[guid].attacker = target->getOwner();
        m_attackerInfo[guid].victim = target->getOwner()->getVictim();
        m_attackerInfo[guid].threat = target->getThreat(victim);
        m_attackerInfo[guid].count = 1;
        //m_attackerInfo[guid].source = 1; // source is not used so far.
        ref = ref->next();
    }
}

void PlayerbotAI::UpdateAttackerInfo()
{
    // clear old list
    m_attackerInfo.clear();

    // check own attackers
    UpdateAttackersForTarget(m_bot);
    Pet *pet = m_bot->GetPet();
    if (pet)
        UpdateAttackersForTarget(pet);

    // check master's attackers
    UpdateAttackersForTarget(GetMaster());
    pet = GetMaster()->GetPet();
    if (pet)
        UpdateAttackersForTarget(pet);

    // check all group members now
    if (m_bot->GetGroup())
    {
        GroupReference *gref = m_bot->GetGroup()->GetFirstMember();
        while (gref)
        {
            if (gref->getSource() == m_bot || gref->getSource() == GetMaster())
            {
                gref = gref->next();
                continue;
            }

            UpdateAttackersForTarget(gref->getSource());
            pet = gref->getSource()->GetPet();
            if (pet)
                UpdateAttackersForTarget(pet);

            gref = gref->next();
        }
    }

    // get highest threat not caused by bot for every entry in AttackerInfoList...
    for (AttackerInfoList::iterator itr = m_attackerInfo.begin(); itr != m_attackerInfo.end(); ++itr)
    {
        if (!itr->second.attacker)
            continue;
        Unit *a = itr->second.attacker;
        float t = 0.00;
        std::list<HostileReference*>::const_iterator i = a->getThreatManager().getThreatList().begin();
        for (; i != a->getThreatManager().getThreatList().end(); ++i)
        {
            if ((*i)->getThreat() > t && (*i)->getTarget() != m_bot)
                t = (*i)->getThreat();
        }
        m_attackerInfo[itr->first].threat2 = t;
    }

    // DEBUG: output attacker info
    //sLog.outBasic( "[PlayerbotAI]: %s m_attackerInfo = {", m_bot->GetName() );
    //for( AttackerInfoList::iterator i=m_attackerInfo.begin(); i!=m_attackerInfo.end(); ++i )
    //    sLog.outBasic( "[PlayerbotAI]:     [%016I64X] { %08X, %08X, %.2f, %.2f, %d, %d }",
    //        i->first,
    //        (i->second.attacker?i->second.attacker->GetGUIDLow():0),
    //        (i->second.victim?i->second.victim->GetGUIDLow():0),
    //        i->second.threat,
    //        i->second.threat2,
    //        i->second.count,
    //        i->second.source );
    //sLog.outBasic( "[PlayerbotAI]: };" );
}

uint32 PlayerbotAI::EstRepairAll()
{
    uint32 TotalCost = 0;
    // equipped, backpack, bags itself
    for (int i = EQUIPMENT_SLOT_START; i < INVENTORY_SLOT_ITEM_END; ++i)
        TotalCost += EstRepair(((INVENTORY_SLOT_BAG_0 << 8) | i));

    // bank, buyback and keys not repaired

    // items in inventory bags
    for (int j = INVENTORY_SLOT_BAG_START; j < INVENTORY_SLOT_BAG_END; ++j)
        for (int i = 0; i < MAX_BAG_SIZE; ++i)
            TotalCost += EstRepair(((j << 8) | i));
    return TotalCost;
}

uint32 PlayerbotAI::EstRepair(uint16 pos)
{
    Item* item = m_bot->GetItemByPos(pos);

    uint32 TotalCost = 0;
    if (!item)
        return TotalCost;

    uint32 maxDurability = item->GetUInt32Value(ITEM_FIELD_MAXDURABILITY);
    if (!maxDurability)
        return TotalCost;

    uint32 curDurability = item->GetUInt32Value(ITEM_FIELD_DURABILITY);

    uint32 LostDurability = maxDurability - curDurability;
    if (LostDurability > 0)
    {
        ItemPrototype const *ditemProto = item->GetProto();

        DurabilityCostsEntry const *dcost = sDurabilityCostsStore.LookupEntry(ditemProto->ItemLevel);
        if (!dcost)
        {
            sLog.outError("RepairDurability: Wrong item lvl %u", ditemProto->ItemLevel);
            return TotalCost;
        }

        uint32 dQualitymodEntryId = (ditemProto->Quality + 1) * 2;
        DurabilityQualityEntry const *dQualitymodEntry = sDurabilityQualityStore.LookupEntry(dQualitymodEntryId);
        if (!dQualitymodEntry)
        {
            sLog.outError("RepairDurability: Wrong dQualityModEntry %u", dQualitymodEntryId);
            return TotalCost;
        }

        uint32 dmultiplier = dcost->multiplier[ItemSubClassToDurabilityMultiplierId(ditemProto->Class, ditemProto->SubClass)];
        uint32 costs = uint32(LostDurability * dmultiplier * double(dQualitymodEntry->quality_mod));

        if (costs == 0)                                 //fix for ITEM_QUALITY_ARTIFACT
            costs = 1;

        TotalCost = costs;
    }
    return TotalCost;
}

Unit* PlayerbotAI::FindAttacker(ATTACKERINFOTYPE ait, Unit* victim)
{
    // list empty? why are we here?
    if (m_attackerInfo.empty())
        return 0;

    // not searching something specific - return first in list
    if (!ait)
        return (m_attackerInfo.begin())->second.attacker;

    float t = ((ait & AIT_HIGHESTTHREAT) ? 0.00 : 9999.00);
    Unit *a = 0;
    AttackerInfoList::iterator itr = m_attackerInfo.begin();
    for (; itr != m_attackerInfo.end(); ++itr)
    {
        if ((ait & AIT_VICTIMSELF) && !(ait & AIT_VICTIMNOTSELF) && itr->second.victim != m_bot)
            continue;

        if (!(ait & AIT_VICTIMSELF) && (ait & AIT_VICTIMNOTSELF) && itr->second.victim == m_bot)
            continue;

        if ((ait & AIT_VICTIMNOTSELF) && victim && itr->second.victim != victim)
            continue;

        if (!(ait & (AIT_LOWESTTHREAT | AIT_HIGHESTTHREAT)))
        {
            a = itr->second.attacker;
            itr = m_attackerInfo.end(); // == break;
        }
        else
        {
            if ((ait & AIT_HIGHESTTHREAT) && /*(itr->second.victim==m_bot) &&*/ itr->second.threat >= t)
            {
                t = itr->second.threat;
                a = itr->second.attacker;
            }
            else if ((ait & AIT_LOWESTTHREAT) && /*(itr->second.victim==m_bot) &&*/ itr->second.threat <= t)
            {
                t = itr->second.threat;
                a = itr->second.attacker;
            }
        }
    }
    return a;
}

/**
* BotDataRestore()
* Restores autoequip - the toggle status for the 'equip auto' command.
* Restores gDelayAttack - the other attributes need a valid target. This function is to be called when the targets
* may or may not be online (such as upon login). See CombatOrderRestore() for full orders restore.
* Restores m_DelayAttack - the other attributes need a valid target. This function is to be called when the targets
*/
void PlayerbotAI::BotDataRestore()
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT combat_delay, active_spec FROM playerbot_saved_data WHERE guid = '%u'", m_bot->GetGUIDLow());

    if (!result)
    {
        sLog.outString();
        sLog.outString(">> [BotDataRestore()] Loaded `playerbot_saved_data`, found no match for guid %u.", m_bot->GetGUIDLow());
        m_DelayAttack = 0;
        return;
    }
    else
    {
        Field* fields = result->Fetch();
        m_DelayAttack = fields[0].GetUInt8();
		uint32 spec = fields[1].GetUInt32();
		if (spec > 0)
			SetActiveTalentSpec(GetTalentSpec((long)m_bot->getClass(), spec));
        delete result;
    }
}

/**
* CombatOrderRestore()
* Restores all saved attributes. This function is to be called when the targets are assumed to be online.
*/

void PlayerbotAI::CombatOrderRestore()
{
    QueryResult* result = CharacterDatabase.PQuery("SELECT combat_order,primary_target,secondary_target,pname,sname,combat_delay,auto_follow FROM playerbot_saved_data WHERE guid = '%u'", m_bot->GetGUIDLow());

    if (!result)
    {
        sLog.outString();
        sLog.outString(">> [CombatOrderRestore()] Loaded `playerbot_saved_data`, found no match for guid %u.", m_bot->GetGUIDLow());
        TellMaster("I have no orders");
        return;
    }

    Field* fields = result->Fetch();
    CombatOrderType combatOrders = (CombatOrderType)fields[0].GetUInt32();
    ObjectGuid PrimtargetGUID = ObjectGuid(fields[1].GetUInt64());
    ObjectGuid SectargetGUID = ObjectGuid(fields[2].GetUInt64());
    std::string pname = fields[3].GetString();
    std::string sname = fields[4].GetString();
    m_DelayAttack = fields[5].GetUInt8();
    gPrimtarget = ObjectAccessor::GetUnit(*m_bot->GetMap()->GetWorldObject(PrimtargetGUID), PrimtargetGUID);
    gSectarget = ObjectAccessor::GetUnit(*m_bot->GetMap()->GetWorldObject(SectargetGUID), SectargetGUID);
    delete result;

    //Unit* target = nullptr;
    //ObjectGuid NoTargetGUID = m_bot->GetObjectGuid();
    //target = ObjectAccessor::GetUnit(*m_bot, NoTargetGUID);

    if (combatOrders & ORDERS_PRIMARY) SetCombatOrder(combatOrders, gPrimtarget);
    if (combatOrders & ORDERS_SECONDARY) SetCombatOrder(combatOrders, gSectarget);
}

void PlayerbotAI::SetCombatOrderByStr(std::string str, Unit *target)
{
    CombatOrderType co;
    if (str == "tank")              co = ORDERS_TANK;
    else if (str == "assist")       co = ORDERS_ASSIST;
    else if (str == "heal")         co = ORDERS_HEAL;
    else if (str == "protect")      co = ORDERS_PROTECT;
    else if (str == "passive")      co = ORDERS_PASSIVE;
    else if (str == "pull")         co = ORDERS_TEMP_WAIT_TANKAGGRO;
    else if (str == "nodispel")     co = ORDERS_NODISPEL;
    else if (str == "resistfrost")  co = ORDERS_RESIST_FROST;
    else if (str == "resistnature") co = ORDERS_RESIST_NATURE;
    else if (str == "resistfire")   co = ORDERS_RESIST_FIRE;
    else if (str == "resistshadow") co = ORDERS_RESIST_SHADOW;
    else                            co = ORDERS_RESET;

    SetCombatOrder(co, target);
}

void PlayerbotAI::SetCombatOrder(CombatOrderType co, Unit* target)
{
    uint32 gTempTarget;
    std::string gname;
    if (target)
    {
        gTempTarget = target->GetGUIDLow();
        gname = target->GetName();
    }

    // reset m_combatOrder after ORDERS_PASSIVE
    if (m_combatOrder == ORDERS_PASSIVE)
    {
        m_combatOrder = ORDERS_NONE;
        m_targetAssist = 0;
        m_targetProtect = 0;
    }

    switch(co)
    {
        case ORDERS_ASSIST: // 2(10)
            {
                if (!target)
                {
                    TellMaster("The assist command requires a target.");
        return;
    }
                else m_targetAssist = target;
                break;
            }
        case ORDERS_PROTECT: // 10(10000)
            {
                if (!target)
                {
                    TellMaster("The protect command requires a target.");
                    return;
                }
                else m_targetProtect = target;
                break;
            }
        case ORDERS_PASSIVE: // 20(100000)
            {
                m_combatOrder = ORDERS_PASSIVE;
                m_targetAssist = 0;
                m_targetProtect = 0;
                return;
            }
        case ORDERS_RESET: // FFFF(11111111)
            {
        m_combatOrder = ORDERS_NONE;
        m_targetAssist = 0;
        m_targetProtect = 0;
                m_DelayAttackInit = CurrentTime();
                m_DelayAttack = 0;
                CharacterDatabase.DirectPExecute("UPDATE playerbot_saved_data SET combat_order = 0, primary_target = 0, secondary_target = 0, pname = '',sname = '', combat_delay = 0 WHERE guid = '%u'", m_bot->GetGUIDLow());
        TellMaster("Orders are cleaned!");
        return;
    }
        default:
            break;
    }

    // Do your magic
    if ((co & ORDERS_PRIMARY))
    {
        m_combatOrder = (CombatOrderType) (((uint32) m_combatOrder & (uint32) ORDERS_SECONDARY) | (uint32) co);
        if (target)
            CharacterDatabase.DirectPExecute("UPDATE playerbot_saved_data SET combat_order = '%u', primary_target = '%u', pname = '%s' WHERE guid = '%u'", (m_combatOrder & ~ORDERS_TEMP), gTempTarget, gname.c_str(), m_bot->GetGUIDLow());
    }
    else
    {
        m_combatOrder = (CombatOrderType)((uint32)m_combatOrder | (uint32)co);
        if (target)
            CharacterDatabase.DirectPExecute("UPDATE playerbot_saved_data SET combat_order = '%u', secondary_target = '%u', sname = '%s' WHERE guid = '%u'", (m_combatOrder & ~ORDERS_TEMP), gTempTarget, gname.c_str(), m_bot->GetGUIDLow());
}
}

void PlayerbotAI::ClearCombatOrder(CombatOrderType co)
{
     m_combatOrder = (CombatOrderType) ((uint32) m_combatOrder & (uint32) ~co);

     switch (co)
     {
     case ORDERS_NONE:
     case ORDERS_TANK:
     case ORDERS_ASSIST:
     case ORDERS_HEAL:
     case ORDERS_PASSIVE:
     case ORDERS_PRIMARY:
     case ORDERS_RESET:
     case ORDERS_SECONDARY:
         SetCombatOrder(ORDERS_RESET);
         return;

     default:
         return;
     }
 }

void PlayerbotAI::SetMovementOrder(MovementOrderType mo, Unit *followTarget)
{
    m_movementOrder = mo;
    m_followTarget = followTarget;
    MovementReset();
}

void PlayerbotAI::MovementReset()
{
    // stop moving...
    MovementClear();

    if (m_movementOrder == MOVEMENT_FOLLOW)
    {
        if (!m_followTarget)
            return;

        WorldObject* distTarget = m_followTarget;   // target to distance check

        // don't follow while in combat
        if (m_bot->isInCombat())
            return;

        Player* pTarget;                            // target is player
        if (m_followTarget->GetTypeId() == TYPEID_PLAYER)
            pTarget = ((Player*) m_followTarget);

        if (pTarget)
        {
            // check player for follow situations
			// if the follow target is dead, do not follow them
            if (pTarget->IsBeingTeleported() || pTarget->IsTaxiFlying() || !pTarget->isAlive())
                return;

            // use player's corpse as distance check target
            if (pTarget->GetCorpse())
                distTarget = pTarget->GetCorpse();
        }

        // is bot too far from the follow target
        if (!m_bot->IsWithinDistInMap(distTarget, 50))
        {
			TellMaster("I am trying to follow you master but you are too far away!");
            DoTeleport(*m_followTarget);
            return;
        }

        if (m_bot->isAlive())
        {
            float angle = rand_float(0, M_PI_F);
            float dist = rand_float(m_mgr->m_confFollowDistance[0], m_mgr->m_confFollowDistance[1]);
            m_bot->GetMotionMaster()->MoveFollow(m_followTarget, dist, angle);
        }
    }
}

void PlayerbotAI::MovementClear()
{
    // stop...
    m_bot->clearUnitState(UNIT_STAT_FOLLOW);
    m_bot->GetMotionMaster()->Initialize();

    // stand up...
    if (!m_bot->IsStandState())
        m_bot->SetStandState(UNIT_STAND_STATE_STAND);
}

void PlayerbotAI::PlaySound(uint32 soundid)
{
    WorldPacket data(SMSG_PLAY_SOUND, 4);
    data << soundid;
    GetMaster()->GetSession()->SendPacket(&data);
}

// PlaySound data from SoundEntries.dbc
void PlayerbotAI::Announce(AnnounceFlags msg)
{
    switch (m_bot->getRace())
    {
        case RACE_HUMAN:
            switch (msg)
            {
                case CANT_AFFORD: m_bot->getGender() == GENDER_MALE ? PlaySound(1908) : PlaySound(2032); break;
                case INVENTORY_FULL: m_bot->getGender() == GENDER_MALE ? PlaySound(1875) : PlaySound(1999); break;
                default: break;
            }
            break;
        case RACE_ORC:
            switch (msg)
            {
                case CANT_AFFORD: m_bot->getGender() == GENDER_MALE ? PlaySound(2319) : PlaySound(2374); break;
                case INVENTORY_FULL: m_bot->getGender() == GENDER_MALE ? PlaySound(2284) : PlaySound(2341); break;
                default: break;
            }
            break;
        case RACE_DWARF:
            switch (msg)
            {
                case CANT_AFFORD: m_bot->getGender() == GENDER_MALE ? PlaySound(1630) : PlaySound(1686); break;
                case INVENTORY_FULL: m_bot->getGender() == GENDER_MALE ? PlaySound(1581) : PlaySound(1654); break;
                default: break;
            }
            break;
        case RACE_NIGHTELF:
            switch (msg)
            {
                case CANT_AFFORD: m_bot->getGender() == GENDER_MALE ? PlaySound(2151) : PlaySound(2262); break;
                case INVENTORY_FULL: m_bot->getGender() == GENDER_MALE ? PlaySound(2118) : PlaySound(2229); break;
                default: break;
            }
            break;
        case RACE_UNDEAD:
            switch (msg)
            {
                case CANT_AFFORD: m_bot->getGender() == GENDER_MALE ? PlaySound(2096) : PlaySound(2207); break;
                case INVENTORY_FULL: m_bot->getGender() == GENDER_MALE ? PlaySound(2054) : PlaySound(2173); break;
                default: break;
            }
            break;
        case RACE_TAUREN:
            switch (msg)
            {
                case CANT_AFFORD: m_bot->getGender() == GENDER_MALE ? PlaySound(2463) : PlaySound(2462); break;
                case INVENTORY_FULL: m_bot->getGender() == GENDER_MALE ? PlaySound(2396) : PlaySound(2397); break;
                default: break;
            }
            break;
        case RACE_GNOME:
            switch (msg)
            {
                case CANT_AFFORD: m_bot->getGender() == GENDER_MALE ? PlaySound(1743) : PlaySound(1798); break;
                case INVENTORY_FULL: m_bot->getGender() == GENDER_MALE ? PlaySound(1708) : PlaySound(1709); break;
                default: break;
            }
            break;
        case RACE_TROLL:
            switch (msg)
            {
                case CANT_AFFORD: m_bot->getGender() == GENDER_MALE ? PlaySound(1853) : PlaySound(1963); break;
                case INVENTORY_FULL: m_bot->getGender() == GENDER_MALE ? PlaySound(1820) : PlaySound(1930); break;
                default: break;
            }
            break;
        default:
            break;
    }
}

bool PlayerbotAI::IsMoving()
{
    return (m_bot->GetMotionMaster()->GetCurrentMovementGeneratorType() == IDLE_MOTION_TYPE ? false : true);
}

// some possible things to use in AI
// GetRandomContactPoint
// GetPower, GetMaxPower
// HasSpellCooldown
// IsAffectedBySpellmod
// isMoving
// hasUnitState(FLAG) FLAG like: UNIT_STAT_ROOT, UNIT_STAT_CONFUSED, UNIT_STAT_STUNNED
// hasAuraType

void PlayerbotAI::UpdateAI(const uint32 /*p_time*/)
{
    if (m_bot->IsBeingTeleported() || m_bot->GetTrader())
        return;

    if (CurrentTime() < m_ignoreAIUpdatesUntilTime)
        return;

	// Removing the ignore update time. We want precision timing for our bots.
    // default updates occur every two seconds
    //SetIgnoreUpdateTime(2);

    if (!m_bot->isAlive())
    {
        if (m_botState == BOTSTATE_DEAD)
        {
            // become ghost
            if (m_bot->GetCorpse()) {
                // DEBUG_LOG ("[PlayerbotAI]: UpdateAI - %s already has a corpse...", m_bot->GetName());
                SetState(BOTSTATE_DEADRELEASED);
                return;
            }
            m_bot->SetBotDeathTimer();
            m_bot->BuildPlayerRepop();
            // relocate ghost
            WorldLocation loc;
            Corpse *corpse = m_bot->GetCorpse();
            corpse->GetPosition(loc);
            m_bot->TeleportTo(loc.mapid, loc.coord_x, loc.coord_y, loc.coord_z, m_bot->GetOrientation());
            // set state to released
            SetState(BOTSTATE_DEADRELEASED);

            return;
        }
        
        if (m_botState == BOTSTATE_DEADRELEASED)
        {
            // get bot's corpse
            Corpse *corpse = m_bot->GetCorpse();
            if (!corpse)
                // DEBUG_LOG ("[PlayerbotAI]: UpdateAI - %s has no corpse!", m_bot->GetName());
                return;
            // teleport ghost from graveyard to corpse
            // DEBUG_LOG ("[PlayerbotAI]: UpdateAI - Teleport %s to corpse...", m_bot->GetName());
            DoTeleport(*corpse);
            // check if we are allowed to resurrect now
            time_t resurrect_time = corpse->GetGhostTime() + m_bot->GetCorpseReclaimDelay(corpse->GetType() == CORPSE_RESURRECTABLE_PVP);
            if (resurrect_time > CurrentTime())
            {
                SetIgnoreUpdateTime( resurrect_time );
                // DEBUG_LOG ("[PlayerbotAI]: UpdateAI - %s has to wait for %d seconds to revive...", m_bot->GetName(), m_ignoreAIUpdatesUntilTime-CurrentTime() );
                return;
            }
            // resurrect now
            // DEBUG_LOG ("[PlayerbotAI]: UpdateAI - Reviving %s to corpse...", m_bot->GetName() );

            SetIgnoreUpdateTime(2);

            PlayerbotChatHandler ch(GetMaster());
            if (!ch.revive(*m_bot))
            {
                ch.sysmessage(".. could not be revived ..");
                return;
            }
            // set back to normal
            SetState(BOTSTATE_NORMAL);

            return;
        }

        // if (m_botState != BOTSTATE_DEAD && m_botState != BOTSTATE_DEADRELEASED)
        // DEBUG_LOG ("[PlayerbotAI]: UpdateAI - %s died and is not in correct state...", m_bot->GetName() );
        // clear loot list on death
        m_lootTargets.clear();
        m_lootCurrent = ObjectGuid();
        // clear combat orders
        m_bot->SetSelectionGuid(ObjectGuid());
        m_bot->GetMotionMaster()->Clear(true);
        // set state to dead
        SetState(BOTSTATE_DEAD);
        
		// wait 30sec
		// we don't want to sit around and wait for bots to revive, changing this to 2 seconds
        SetIgnoreUpdateTime(2);

        return;
    }

    // bot still alive
    if (!m_findNPC.empty())
        findNearbyCreature();

    // if we are casting a spell then interrupt it
    // make sure any actions that cast a spell set a proper m_ignoreAIUpdatesUntilTime!
    Spell* const pSpell = GetCurrentSpell();
    if (pSpell && !(pSpell->IsChannelActive() || pSpell->IsAutoRepeat()))
    {
        // DEBUG_LOG("spell (%s) is being interrupted",pSpell->m_spellInfo->SpellName[0]);
        InterruptCurrentCastingSpell();
        return;
    }

    // direct cast command from master
    if (m_spellIdCommand != 0)
    {
        Unit* pTarget = ObjectAccessor::GetUnit(*m_bot, m_targetGuidCommand);
        if (pTarget)
            CastSpell(m_spellIdCommand, *pTarget);
        m_spellIdCommand = 0;
        m_targetGuidCommand = ObjectGuid();

		return;
    }

    //if master is unmounted, unmount the bot
    if (!GetMaster()->IsMounted() && m_bot->IsMounted())
    {
        WorldPacket emptyPacket;
        m_bot->GetSession()->HandleCancelMountAuraOpcode(emptyPacket);  //updated code

		return;
    }

    // handle combat (either self/master/group in combat, or combat state and valid target)
    if (IsInCombat() || (m_botState == BOTSTATE_COMBAT && m_targetCombat) ||  m_ScenarioType == SCENARIO_PVP_DUEL)
    {
        //check if the bot is Mounted
        if (!m_bot->IsMounted())
        {
            if (!pSpell || !pSpell->IsChannelActive())
            {
                // DEBUG_LOG("m_DelayAttackInit (%li) + m_DelayAttack (%u) > time(%li)", m_DelayAttackInit, m_DelayAttack, CurrentTime());
                if (m_DelayAttackInit + m_DelayAttack > CurrentTime())
                    return SetIgnoreUpdateTime(0); // short bursts of delay

                return DoNextCombatManeuver();
            }
            else // channelling a spell
                return SetIgnoreUpdateTime(0);  // It's better to update AI more frequently during combat
        }

        return;
    }

        // bot was in combat recently - loot now
    if (m_botState == BOTSTATE_COMBAT)
    {
        if (GetCombatOrder() & ORDERS_TEMP)
        {
            if (GetCombatOrder() & ORDERS_TEMP_WAIT_TANKAGGRO)
                TellMaster("I was still waiting for the tank to gain aggro, but that doesn't make sense anymore...");
            if (GetCombatOrder() & ORDERS_TEMP_WAIT_OOC)
                TellMaster("I was still waiting OOC but I just got out of combat...");
            ClearCombatOrder(ORDERS_TEMP);
        }

        SetState(BOTSTATE_LOOTING);
        m_attackerInfo.clear();
        if (HasCollectFlag(COLLECT_FLAG_COMBAT))
            m_lootTargets.unique();
        else
            m_lootTargets.clear();

    }

	if (m_botState == BOTSTATE_LOOTING)
	{
		// If we are not in the same group as our master, don't try to loot anything. Avoids the bots
		// to start moving around and getting summoned even when told to stay.
		if (m_bot->IsInSameGroupWith(GetMaster()))
			return DoLoot();
		else
			SetState(BOTSTATE_NORMAL);
	}
        

    if (m_botState == BOTSTATE_FLYING)
    {
        /* std::ostringstream out;
            out << "Taxi: " << m_bot->GetName() << m_ignoreAIUpdatesUntilTime;
            TellMaster(out.str().c_str()); */
        DoFlight();
        SetState(BOTSTATE_NORMAL);
        SetIgnoreUpdateTime(0);

        return;
    }

    // if commanded to follow master and not already following master then follow master
    if (!m_bot->isInCombat() && !IsMoving())
        return MovementReset();

        // do class specific non combat actions
    if (GetClassAI() && !m_bot->IsMounted() && !IsRegenerating())
    {
        GetClassAI()->DoNonCombatActions();

        // have we been told to collect GOs
        if (HasCollectFlag(COLLECT_FLAG_NEAROBJECT))
        {
            findNearbyGO();
            // start looting if have targets
            if (!m_lootTargets.empty())
                SetState(BOTSTATE_LOOTING);
        }

        return;
    }
}

Spell* PlayerbotAI::GetCurrentSpell() const
{
    if (m_CurrentlyCastingSpellId == 0)
        return nullptr;

    Spell* const pSpell = m_bot->FindCurrentSpellBySpellId(m_CurrentlyCastingSpellId);
    return pSpell;
}

void PlayerbotAI::TellMaster(const std::string& text) const
{
    SendWhisper(text, *GetMaster());
}

void PlayerbotAI::TellMaster(const char *fmt, ...) const
{
    char temp_buf[1024];
    va_list ap;
    va_start(ap, fmt);
    (void) vsnprintf(temp_buf, 1024, fmt, ap);
    va_end(ap);
    std::string str = temp_buf;
    TellMaster(str);
}

void PlayerbotAI::SendWhisper(const std::string& text, Player& player) const
{
    WorldPacket data(SMSG_MESSAGECHAT, 200);
    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, text.c_str(),
        LANG_UNIVERSAL, m_bot->GetChatTag(), m_bot->GetObjectGuid(),
        m_bot->GetName(), player.GetObjectGuid());
    player.GetSession()->SendPacket(&data);
}

bool PlayerbotAI::canObeyCommandFrom(const Player& player) const
{
    return player.GetSession()->GetAccountId() == GetMaster()->GetSession()->GetAccountId();
}

bool PlayerbotAI::In_Range(Unit* Target, uint32 spellId)
{
    const SpellEntry* const pSpellInfo = sSpellStore.LookupEntry(spellId);
    if (!pSpellInfo)
        return false;

    SpellRangeEntry const* TempRange = GetSpellRangeStore()->LookupEntry(pSpellInfo->rangeIndex);

    //Spell has invalid range store so we can't use it
    if (!TempRange)
        return false;

    if (TempRange->minRange == (TempRange->maxRange == 0.0f))
        return true;

    //Unit is out of range of this spell
    if (!m_bot->IsInRange(Target, TempRange->minRange, TempRange->maxRange))
        return false;

    return true;
}

bool PlayerbotAI::CheckBotCast(const SpellEntry *sInfo )
{
    if (!sInfo)
        return false;

    // check DoLoot() spells before casting
    Spell* tmp_spell = new Spell(m_bot, sInfo, false);
    if (tmp_spell)
    {
        if (m_lootCurrent.IsCreature())
        {
            if (Creature* obj = m_bot->GetMap()->GetCreature(m_lootCurrent))
                tmp_spell->m_targets.setUnitTarget(obj);
        }
        else if (m_lootCurrent.IsGameObject())
        {
            if (GameObject* obj = m_bot->GetMap()->GetGameObject(m_lootCurrent))
                tmp_spell->m_targets.setGOTarget(obj);
        }

        SpellCastResult res = tmp_spell->CheckCast(false);
        // DEBUG_LOG("CheckBotCast SpellCastResult(%u)",res);
        switch(res)
        {
            case SPELL_CAST_OK:
                return true;
            case SPELL_FAILED_TRY_AGAIN:
                return true;
            default:
                return false;
        }
    }
    return false;
}

bool PlayerbotAI::CastSpell(const char* args)
{
    uint32 spellId = getSpellId(args);
    return (spellId) ? CastSpell(spellId) : false;
}

bool PlayerbotAI::CastSpell(uint32 spellId, Unit& target)
{
    ObjectGuid oldSel = m_bot->GetSelectionGuid();
    m_bot->SetSelectionGuid(target.GetObjectGuid());
    bool rv = CastSpell(spellId);
    m_bot->SetSelectionGuid(oldSel);
    return rv;
}

bool PlayerbotAI::CastSpell(uint32 spellId)
{
    // some AIs don't check if the bot doesn't have spell before using it
    // so just return false when this happens
    if (spellId == 0)
        return false;

    // check spell cooldown
    if (m_bot->HasSpellCooldown(spellId))
        return false;

    // see Creature.cpp 1738 for reference
    // don't allow bot to cast damage spells on friends
    const SpellEntry* const pSpellInfo = sSpellStore.LookupEntry(spellId);
    if (!pSpellInfo)
    {
        TellMaster("missing spell entry in CastSpell for spellid %u.", spellId);
        return false;
    }

    // Power check (stolen from: CreatureAI.cpp - CreatureAI::CanCastSpell)
    if (m_bot->GetPower((Powers)pSpellInfo->powerType) < Spell::CalculatePowerCost(pSpellInfo, m_bot))
        return false;

    // set target
    ObjectGuid targetGUID = m_bot->GetSelectionGuid();
    Unit* pTarget = ObjectAccessor::GetUnit(*m_bot, targetGUID);

    if (!pTarget)
        pTarget = m_bot;

    if (IsPositiveSpell(spellId))
    {
        if (pTarget && !m_bot->IsFriendlyTo(pTarget))
            pTarget = m_bot;
    }
    else
    {
        if (pTarget && m_bot->IsFriendlyTo(pTarget))
            return false;

        m_bot->SetInFront(pTarget);
    }

    float CastTime = 0.0f;

    // stop movement to prevent cancel spell casting
    SpellCastTimesEntry const * castTimeEntry = sSpellCastTimesStore.LookupEntry(pSpellInfo->CastingTimeIndex);
    // stop movement to prevent cancel spell casting
    if (castTimeEntry && castTimeEntry->CastTime)
    {
        CastTime = (castTimeEntry->CastTime / 1000);
        DEBUG_LOG ("[PlayerbotAI]: CastSpell - Bot movement reset for casting %s (%u)", pSpellInfo->SpellName[0], spellId);
        MovementClear();
    }

    uint16 target_type = TARGET_FLAG_UNIT;

    if (pSpellInfo->Effect[0] == SPELL_EFFECT_OPEN_LOCK)
        target_type = TARGET_FLAG_OBJECT;

    m_CurrentlyCastingSpellId = spellId;

    if (pSpellInfo->Effect[0] == SPELL_EFFECT_OPEN_LOCK ||
        pSpellInfo->Effect[0] == SPELL_EFFECT_SKINNING)
    {
        if (m_lootCurrent)
        {
            if (!CheckBotCast(pSpellInfo))
                return false;

            WorldPacket* const packet = new WorldPacket(CMSG_CAST_SPELL, 4+2+8);
            *packet << spellId;
            *packet << target_type;
            *packet << m_lootCurrent.WriteAsPacked();
            m_bot->GetSession()->QueuePacket(packet);       // queue the packet to get around race condition

      /*      if (target_type == TARGET_FLAG_OBJECT)
            {
                WorldPacket* const packetgouse = new WorldPacket(CMSG_GAMEOBJ_USE, 8);
                *packetgouse << m_lootCurrent;
                m_bot->GetSession()->QueuePacket(packetgouse);  // queue the packet to get around race condition

                GameObject *obj = m_bot->GetMap()->GetGameObject(m_lootCurrent);
                if (!obj)
                    return false;

                // add other go types here, i.e.:
                // GAMEOBJECT_TYPE_CHEST - loot quest items of chest
                if (obj->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER)
                {
                    TurnInQuests(obj);

                    // auto accept every available quest this NPC has
                    m_bot->PrepareQuestMenu(m_lootCurrent);
                    QuestMenu& questMenu = m_bot->PlayerTalkClass->GetQuestMenu();
                    for (uint32 iI = 0; iI < questMenu.MenuItemCount(); ++iI)
                    {
                        QuestMenuItem const& qItem = questMenu.GetItem(iI);
                        uint32 questID = qItem.m_qId;
                        if (!AddQuest(questID, obj))
                            TellMaster("Couldn't take quest");
                    }
                    m_lootCurrent = ObjectGuid();
                    m_bot->GetMotionMaster()->Clear(false);
                    m_bot->GetMotionMaster()->MoveIdle();
                }
            } */
            return true;
        }
        else
            return false;
    }
    else
    {
        // Check spell range
        if (!In_Range(pTarget, spellId))
            return false;

        // Check line of sight
        if (!m_bot->IsWithinLOSInMap(pTarget))
            return false;

        if (IsAutoRepeatRangedSpell(pSpellInfo))
            m_bot->CastSpell(pTarget, pSpellInfo, true);       // cast triggered spell
        else
            m_bot->CastSpell(pTarget, pSpellInfo, false);      // uni-cast spell
    }

    SetIgnoreUpdateTime(CastTime + 1);

    return true;
}

bool PlayerbotAI::CastPetSpell(uint32 spellId, Unit* target)
{
    if (spellId == 0)
        return false;

    Pet* pet = m_bot->GetPet();
    if (!pet)
        return false;

    if (pet->HasSpellCooldown(spellId))
        return false;

    const SpellEntry* const pSpellInfo = sSpellStore.LookupEntry(spellId);
    if (!pSpellInfo)
    {
        TellMaster("Missing spell entry in CastPetSpell()");
        return false;
    }

    // set target
    Unit* pTarget;
    if (!target)
    {
        ObjectGuid targetGUID = m_bot->GetSelectionGuid();
        pTarget = ObjectAccessor::GetUnit(*m_bot, targetGUID);
    }
    else
        pTarget = target;

    if (IsPositiveSpell(spellId))
    {
        if (pTarget && !m_bot->IsFriendlyTo(pTarget))
            pTarget = m_bot;
    }
    else
    {
        if (pTarget && m_bot->IsFriendlyTo(pTarget))
            return false;

        if (!pet->isInFrontInMap(pTarget, 10)) // distance probably should be calculated
            pet->SetFacingTo(pet->GetAngle(pTarget));
    }

    pet->CastSpell(pTarget, pSpellInfo, false);

    Spell* const pSpell = pet->FindCurrentSpellBySpellId(spellId);
    if (!pSpell)
        return false;

    return true;
}

// Perform sanity checks and cast spell
bool PlayerbotAI::Buff(uint32 spellId, Unit* target, void (*beforeCast)(Player *))
{
    if (spellId == 0)
        return false;

    SpellEntry const * spellProto = sSpellStore.LookupEntry(spellId);

    if (!spellProto)
        return false;

    if (!target)
        return false;

    // Select appropriate spell rank for target's level
    spellProto = sSpellMgr.SelectAuraRankForLevel(spellProto, target->getLevel());
    if (!spellProto)
        return false;

    // Check if spell will boost one of already existent auras
    bool willBenefitFromSpell = false;
    for (uint8 i = 0; i < MAX_EFFECT_INDEX; ++i)
    {
        if (spellProto->EffectApplyAuraName[i] == SPELL_AURA_NONE)
            break;

        bool sameOrBetterAuraFound = false;
        int32 bonus = m_bot->CalculateSpellDamage(target, spellProto, SpellEffectIndex(i));
        Unit::AuraList const& auras = target->GetAurasByType(AuraType(spellProto->EffectApplyAuraName[i]));
        for (Unit::AuraList::const_iterator it = auras.begin(); it != auras.end(); ++it)
            if ((*it)->GetModifier()->m_miscvalue == spellProto->EffectMiscValue[i] && (*it)->GetModifier()->m_amount >= bonus)
            {
                sameOrBetterAuraFound = true;
                break;
            }
        willBenefitFromSpell = willBenefitFromSpell || !sameOrBetterAuraFound;
    }

    if (!willBenefitFromSpell)
        return false;

    // Druids may need to shapeshift before casting
    if (beforeCast)
        (*beforeCast)(m_bot);

    return CastSpell(spellProto->Id, *target);
}

// Can be used for personal buffs like Mage Armor and Inner Fire
bool PlayerbotAI::SelfBuff(uint32 spellId)
{
    if (spellId == 0)
        return false;

    if (m_bot->HasAura(spellId))
        return false;

    return CastSpell(spellId, *m_bot);
}

// Checks if spell is single per target per caster and will make any effect on target
bool PlayerbotAI::CanReceiveSpecificSpell(uint8 spec, Unit* target) const
{
    if (IsSingleFromSpellSpecificPerTargetPerCaster(SpellSpecific(spec), SpellSpecific(spec)))
    {
        Unit::SpellAuraHolderMap holders = target->GetSpellAuraHolderMap();
        Unit::SpellAuraHolderMap::iterator it;
        for (it = holders.begin(); it != holders.end(); ++it)
            if ((*it).second->GetCasterGuid() == m_bot->GetObjectGuid() && GetSpellSpecific((*it).second->GetId()) == SpellSpecific(spec))
                return false;
    }
    return true;
}

Item* PlayerbotAI::FindItem(uint32 ItemId)
{
    // list out items equipped & in main backpack
    //INVENTORY_SLOT_ITEM_START = 23
    //INVENTORY_SLOT_ITEM_END = 39

    for (uint8 slot = EQUIPMENT_SLOT_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        // DEBUG_LOG ("[PlayerbotAI]: FindItem - [%s's]backpack slot = %u",m_bot->GetName(),slot); // 23 to 38 = 16
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);  // 255, 23 to 38
        if (pItem)
        {
            const ItemPrototype* const pItemProto = pItem->GetProto();
            if (!pItemProto)
                continue;

            if (pItemProto->ItemId == ItemId)   // have required item
                return pItem;
        }
    }
    // list out items in other removable backpacks
    //INVENTORY_SLOT_BAG_START = 19
    //INVENTORY_SLOT_BAG_END = 23

    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)  // 20 to 23 = 4
    {
        const Bag* const pBag = (Bag *) m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);   // 255, 20 to 23
        if (pBag)
            for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                DEBUG_LOG ("[PlayerbotAI]: FindItem - [%s's]bag[%u] slot = %u", m_bot->GetName(), bag, slot);  // 1 to bagsize = ?
                Item* const pItem = m_bot->GetItemByPos(bag, slot); // 20 to 23, 1 to bagsize
                if (pItem)
                {
                    const ItemPrototype* const pItemProto = pItem->GetProto();
                    if (!pItemProto)
                        continue;

                    if (pItemProto->ItemId == ItemId)        // have required item
                        return pItem;
                }
            }
    }
    return nullptr;
}

Item* PlayerbotAI::FindItemInBank(uint32 ItemId)
{
    // list out items in bank item slots

    for (uint8 slot = BANK_SLOT_ITEM_START; slot < BANK_SLOT_ITEM_END; slot++)
    {
        // sLog.outDebug("[%s's]backpack slot = %u",m_bot->GetName(),slot);
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (pItem)
        {
            const ItemPrototype* const pItemProto = pItem->GetProto();
            if (!pItemProto)
                continue;

            if (pItemProto->ItemId == ItemId)   // have required item
                return pItem;
        }
    }
    // list out items in bank bag slots

    for (uint8 bag = BANK_SLOT_BAG_START; bag < BANK_SLOT_BAG_END; ++bag)
    {
        const Bag* const pBag = (Bag *) m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);
        if (pBag)
            for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                // sLog.outDebug("[%s's]bag[%u] slot = %u", m_bot->GetName(), bag, slot);
                Item* const pItem = m_bot->GetItemByPos(bag, slot);
                if (pItem)
                {
                    const ItemPrototype* const pItemProto = pItem->GetProto();
                    if (!pItemProto)
                        continue;

                    if (pItemProto->ItemId == ItemId)        // have required item
                        return pItem;
                }
            }
    }
    return nullptr;
}

Item* PlayerbotAI::FindKeyForLockValue(uint32 reqSkillValue)
{
    if (reqSkillValue <= 25 && m_bot->HasItemCount(SILVER_SKELETON_KEY, 1))
        return FindItem(SILVER_SKELETON_KEY);
    if (reqSkillValue <= 125 && m_bot->HasItemCount(GOLDEN_SKELETON_KEY, 1))
        return FindItem(GOLDEN_SKELETON_KEY);
    if (reqSkillValue <= 200 && m_bot->HasItemCount(TRUESILVER_SKELETON_KEY, 1))
        return FindItem(TRUESILVER_SKELETON_KEY);
    if (reqSkillValue <= 300 && m_bot->HasItemCount(ARCANITE_SKELETON_KEY, 1))
        return FindItem(ARCANITE_SKELETON_KEY);

    return nullptr;
}

Item* PlayerbotAI::FindBombForLockValue(uint32 reqSkillValue)
{
    if (reqSkillValue <= 150 && m_bot->HasItemCount(SMALL_SEAFORIUM_CHARGE, 1))
        return FindItem(SMALL_SEAFORIUM_CHARGE);
    if (reqSkillValue <= 250 && m_bot->HasItemCount(LARGE_SEAFORIUM_CHARGE, 1))
        return FindItem(LARGE_SEAFORIUM_CHARGE);
    if (reqSkillValue <= 300 && m_bot->HasItemCount(POWERFUL_SEAFORIUM_CHARGE, 1))
        return FindItem(POWERFUL_SEAFORIUM_CHARGE);

    return nullptr;
}

bool PlayerbotAI::PickPocket(Unit* pTarget)
{
    if(!pTarget)
        return false;

    ObjectGuid markGuid = pTarget->GetObjectGuid();
    Creature *c = m_bot->GetMap()->GetCreature(markGuid);
    if(c)
    {
        Loot*& loot = c->loot;
        if (!loot)
            loot = new Loot(m_bot, c, LOOT_PICKPOCKETING);
        else
        {
            if (loot->GetLootType() != LOOT_PICKPOCKETING)
            {
                delete loot;
                loot = new Loot(m_bot, c, LOOT_PICKPOCKETING);
            }
        }

        if (loot->GetGoldAmount())
        {
            m_bot->ModifyMoney(loot->GetGoldAmount());

            if (m_mgr->m_confDebugWhisper)
            {
                std::ostringstream out;

                // calculate how much money bot loots
                uint32 copper = loot->GetGoldAmount();
                uint32 gold = uint32(copper / 10000);
                copper -= (gold * 10000);
                uint32 silver = uint32(copper / 100);
                copper -= (silver * 100);

                out << "|r|cff009900" << m_bot->GetName() << " loots: " << "|h|cffffffff[|r|cff00ff00" << gold
                    << "|r|cfffffc00g|r|cff00ff00" << silver
                    << "|r|cffcdcdcds|r|cff00ff00" << copper
                    << "|r|cff993300c"
                    << "|h|cffffffff]";

                TellMaster(out.str().c_str());
            }

            // send the money to the bot and remove it from the creature
            loot->SendGold(m_bot);
        }

        if (!loot->AutoStore(m_bot, false, NULL_BAG, NULL_SLOT))
            sLog.outDebug("PLAYERBOT Debug: Failed to get loot from pickpocketed NPC");

        // release the loot whatever happened
        loot->Release(m_bot);
    }
    return false; // ensures that the rogue only pick pockets target once
}

bool PlayerbotAI::HasSpellReagents(uint32 spellId)
{
    const SpellEntry* const pSpellInfo = sSpellStore.LookupEntry(spellId);
    if (!pSpellInfo)
        return false;

    if (m_bot->CanNoReagentCast(pSpellInfo))
        return true;

    for (uint32 i = 0; i < MAX_SPELL_REAGENTS; ++i)
    {
        if (pSpellInfo->Reagent[i] <= 0)
            continue;

        uint32 itemid = pSpellInfo->Reagent[i];
        uint32 count = pSpellInfo->ReagentCount[i];

        if (!m_bot->HasItemCount(itemid, count))
            return false;
    }

    return true;
}

// extracts all item ids in format below
// I decided to roll my own extractor rather then use the one in ChatHandler
// because this one works on a const string, and it handles multiple links
// |color|linkType:key:something1:...:somethingN|h[name]|h|r
void PlayerbotAI::extractItemIds(const std::string& text, std::list<uint32>& itemIds) const
{
    uint8 pos = 0;
    while (true)
    {
        int i = text.find("Hitem:", pos);
        if (i == -1)
            break;
        pos = i + 6;
        int endPos = text.find(':', pos);
        if (endPos == -1)
            break;
        std::string idC = text.substr(pos, endPos - pos);
        uint32 id = atol(idC.c_str());
        pos = endPos;
        if (id)
            itemIds.push_back(id);
    }
}

void PlayerbotAI::extractQuestIds(const std::string& text, std::list<uint32>& questIds) const
{
    uint8 pos = 0;
    while (true)
    {
        int i = text.find("Hquest:", pos);
        if (i == -1)
            break;
        pos = i + 7;
        int endPos = text.find(':', pos);
        if (endPos == -1)
            break;
        std::string idC = text.substr(pos, endPos - pos);
        uint32 id = atol(idC.c_str());
        pos = endPos;
        if (id)
            questIds.push_back(id);
    }
}

// Build an hlink for Weapon skills in Aqua
void PlayerbotAI::MakeWeaponSkillLink(const SpellEntry *sInfo, std::ostringstream &out, uint32 skillid)
{
    int loc = GetMaster()->GetSession()->GetSessionDbcLocale();
    out << "|cff00ffff|Hspell:" << sInfo->Id << "|h[" << sInfo->SpellName[loc] << " : " << m_bot->GetSkillValue(skillid) << " /" << m_bot->GetMaxSkillValue(skillid) << "]|h|r";
}

// Build an hlink for spells in White
void PlayerbotAI::MakeSpellLink(const SpellEntry *sInfo, std::ostringstream &out)
{
    int    loc = GetMaster()->GetSession()->GetSessionDbcLocale();
    out << "|cffffffff|Hspell:" << sInfo->Id << "|h[" << sInfo->SpellName[loc] << "]|h|r";
}

// Builds a hlink for an item, but since its
// only a ItemPrototype, we cant fill in everything
void PlayerbotAI::MakeItemLink(const ItemPrototype *item, std::ostringstream &out)
{
    // Color
    out << "|c";
    switch (item->Quality)
    {
        case ITEM_QUALITY_POOR:     out << "ff9d9d9d"; break;  //GREY
        case ITEM_QUALITY_NORMAL:   out << "ffffffff"; break;  //WHITE
        case ITEM_QUALITY_UNCOMMON: out << "ff1eff00"; break;  //GREEN
        case ITEM_QUALITY_RARE:     out << "ff0070dd"; break;  //BLUE
        case ITEM_QUALITY_EPIC:     out << "ffa335ee"; break;  //PURPLE
        case ITEM_QUALITY_LEGENDARY: out << "ffff8000"; break;  //ORANGE
        case ITEM_QUALITY_ARTIFACT: out << "ffe6cc80"; break;  //LIGHT YELLOW
        default:                    out << "ffff0000"; break;  //Don't know color, so red?
    }
    out << "|Hitem:";

    // Item Id
    out << item->ItemId << ":";

    // Permanent enchantment, gems, 4 unknowns, and reporter_level
    // ->new items wont have enchantments or gems so..
    out << "0:0:0:0:0:0:0:0:0";

    // Name
    std::string name = item->Name1;
    ItemLocalization(name, item->ItemId);
    out << "|h[" << name << "]|h|r";
}

// Builds a hlink for an item, includes everything
// |color|Hitem:item_id:perm_ench_id:gem1:gem2:gem3:0:0:0:0:reporter_level|h[name]|h|r
void PlayerbotAI::MakeItemLink(const Item *item, std::ostringstream &out, bool IncludeQuantity /*= true*/)
{
    const ItemPrototype *proto = item->GetProto();
    // Color
    out << "|c";
    switch (proto->Quality)
    {
        case ITEM_QUALITY_POOR:     out << "ff9d9d9d"; break;  //GREY
        case ITEM_QUALITY_NORMAL:   out << "ffffffff"; break;  //WHITE
        case ITEM_QUALITY_UNCOMMON: out << "ff1eff00"; break;  //GREEN
        case ITEM_QUALITY_RARE:     out << "ff0070dd"; break;  //BLUE
        case ITEM_QUALITY_EPIC:     out << "ffa335ee"; break;  //PURPLE
        case ITEM_QUALITY_LEGENDARY: out << "ffff8000"; break;  //ORANGE
        case ITEM_QUALITY_ARTIFACT: out << "ffe6cc80"; break;  //LIGHT YELLOW
        default:                    out << "ffff0000"; break;  //Don't know color, so red?
    }
    out << "|Hitem:";

    // Item Id
    out << proto->ItemId << ":";

    // Permanent enchantment
    out << item->GetEnchantmentId(PERM_ENCHANTMENT_SLOT) << ":";

    // Gems
    uint32 g1 = 0, g2 = 0, g3 = 0;
    out << g1 << ":" << g2 << ":" << g3 << ":";

    // Temp enchantment, Bonus Enchantment, Prismatic Enchantment?
    // Other stuff, don't know what it is
    out << "0:0:0:0:";

    // Reporter Level
    out << "0";

    // Name
    std::string name = proto->Name1;
    ItemLocalization(name, proto->ItemId);
    out << "|h[" << name << "]|h|r";

    // Stacked items
    if (item->GetCount() > 1 && IncludeQuantity)
        out << "x" << item->GetCount() << ' ';
}

void PlayerbotAI::extractAuctionIds(const std::string& text, std::list<uint32>& auctionIds) const
{
    uint8 pos = 0;
    while (true)
    {
        int i = text.find("Htitle:", pos);
        if (i == -1)
            break;
        pos = i + 7;
        int endPos = text.find('|', pos);
        if (endPos == -1)
            break;
        std::string idC = text.substr(pos, endPos - pos);
        uint32 id = atol(idC.c_str());
        pos = endPos;
        if (id)
            auctionIds.push_back(id);
    }
}

void PlayerbotAI::extractSpellId(const std::string& text, uint32 &spellId) const
{

    //   Link format
    //   |cffffffff|Hspell:" << spellId << ":" << "|h[" << pSpellInfo->SpellName[loc] << "]|h|r";
    //   cast |cff71d5ff|Hspell:686|h[Shadow Bolt]|h|r";
    //   012345678901234567890123456
    //        base = 16 >|  +7 >|

    uint8 pos = 0;

    int i = text.find("Hspell:", pos);
    if (i == -1)
        return;

    // DEBUG_LOG("[PlayerbotAI]: extractSpellId - first pos %u i %u",pos,i);
    pos = i + 7;     // start of window in text 16 + 7 = 23
    int endPos = text.find('|', pos);
    if (endPos == -1)
        return;

    // DEBUG_LOG("[PlayerbotAI]: extractSpellId - second endpos : %u pos : %u",endPos,pos);
    std::string idC = text.substr(pos, endPos - pos);     // 26 - 23
    spellId = atol(idC.c_str());
    pos = endPos;     // end
}

void PlayerbotAI::extractSpellIdList(const std::string& text, BotEntryList& m_spellsToLearn) const
{

    //   Link format
    //   |cffffffff|Hspell:" << spellId << ":" << "|h[" << pSpellInfo->SpellName[loc] << "]|h|r";
    //   cast |cff71d5ff|Hspell:686|h[Shadow Bolt]|h|r";
    //   012345678901234567890123456
    //        base = 16 >|  +7 >|

    uint8 pos = 0;
    while (true)
    {
        int i = text.find("Hspell:", pos);
        if (i == -1)
            break;

        // DEBUG_LOG("[PlayerbotAI]: extractSpellIdList - first pos %u i %u",pos,i);
        pos = i + 7;     // start of window in text 16 + 7 = 23
        int endPos = text.find('|', pos);
        if (endPos == -1)
            break;

        // DEBUG_LOG("[PlayerbotAI]: extractSpellIdList - second endpos : %u pos : %u",endPos,pos);
        std::string idC = text.substr(pos, endPos - pos);     // 26 - 23
        uint32 spellId = atol(idC.c_str());
        pos = endPos;     // end

        if (spellId)
            m_spellsToLearn.push_back(spellId);
    }
}

void PlayerbotAI::extractTalentIds(const std::string &text, std::list<talentPair> &talentIds) const
{
    // Link format:
    // |color|Htalent:talent_id:rank|h[name]|h|r
    // |cff4e96f7|Htalent:1396:4|h[Unleashed Fury]|h|r

    uint8 pos = 0;
    while (true)
    {
        int i = text.find("Htalent:", pos);
        if (i == -1)
            break;
        pos = i + 8;
        // DEBUG_LOG("extractTalentIds first pos %u i %u",pos,i);
        // extract talent_id
        int endPos = text.find(':', pos);
        if (endPos == -1)
            break;
        // DEBUG_LOG("extractTalentId second endpos : %u pos : %u",endPos,pos);
        std::string idC = text.substr(pos, endPos - pos);
        uint32 id = atol(idC.c_str());
        pos = endPos + 1;
        // extract rank
        endPos = text.find('|', pos);
        if (endPos == -1)
            break;
        // DEBUG_LOG("extractTalentId third endpos : %u pos : %u",endPos,pos);
        std::string rankC = text.substr(pos, endPos - pos);
        uint32 rank = atol(rankC.c_str());
        pos = endPos + 1;

        // DEBUG_LOG("extractTalentId second id : %u  rank : %u",id,rank);

        if (id)
            talentIds.push_back(std::pair<uint32, uint32>(id, rank));
    }
}

void PlayerbotAI::extractGOinfo(const std::string& text, BotObjectList& m_lootTargets) const
{

    //    Link format
    //    |cFFFFFF00|Hfound:" << guid << ':'  << entry << ':'  <<  "|h[" << gInfo->name << "]|h|r";
    //    |cFFFFFF00|Hfound:9582:1731|h[Copper Vein]|h|r

    uint8 pos = 0;
    while (true)
    {
        // extract GO guid
        int i = text.find("Hfound:", pos);     // base H = 11
        if (i == -1)     // break if error
            break;

        pos = i + 7;     //start of window in text 11 + 7 = 18
        int endPos = text.find(':', pos);     // end of window in text 22
        if (endPos == -1)     //break if error
            break;
        std::string guidC = text.substr(pos, endPos - pos);     // get string within window i.e guid 22 - 18 =  4
        uint32 guid = atol(guidC.c_str());     // convert ascii to long int

        // extract GO entry
        pos = endPos + 1;
        endPos = text.find(':', pos);     // end of window in text
        if (endPos == -1)     //break if error
            break;

        std::string entryC = text.substr(pos, endPos - pos);     // get string within window i.e entry
        uint32 entry = atol(entryC.c_str());     // convert ascii to float

        ObjectGuid lootCurrent = ObjectGuid(HIGHGUID_GAMEOBJECT, entry, guid);

        if (guid)
            m_lootTargets.push_back(lootCurrent);
    }
}

// extracts currency in #g#s#c format
uint32 PlayerbotAI::extractMoney(const std::string& text) const
{
    // if user specified money in ##g##s##c format
    std::string acum = "";
    uint32 copper = 0;
    for (uint8 i = 0; i < text.length(); i++)
    {
        if (text[i] == 'g')
        {
            copper += (atol(acum.c_str()) * 100 * 100);
            acum = "";
        }
        else if (text[i] == 'c')
        {
            copper += atol(acum.c_str());
            acum = "";
        }
        else if (text[i] == 's')
        {
            copper += (atol(acum.c_str()) * 100);
            acum = "";
        }
        else if (text[i] == ' ')
            break;
        else if (text[i] >= 48 && text[i] <= 57)
            acum += text[i];
        else
        {
            copper = 0;
            break;
        }
    }
    return copper;
}

// finds items in equipment and adds Item* to foundItemList
// also removes found item IDs from itemIdSearchList when found
void PlayerbotAI::findItemsInEquip(std::list<uint32>& itemIdSearchList, std::list<Item*>& foundItemList) const
{
    for (uint8 slot = EQUIPMENT_SLOT_START; itemIdSearchList.size() > 0 && slot < EQUIPMENT_SLOT_END; slot++)
    {
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!pItem)
            continue;

        for (std::list<uint32>::iterator it = itemIdSearchList.begin(); it != itemIdSearchList.end(); ++it)
        {
            if (pItem->GetProto()->ItemId != *it)
                continue;

            foundItemList.push_back(pItem);
            itemIdSearchList.erase(it);
            break;
        }
    }
}

// finds items in inventory and adds Item* to foundItemList
// also removes found item IDs from itemIdSearchList when found
void PlayerbotAI::findItemsInInv(std::list<uint32>& itemIdSearchList, std::list<Item*>& foundItemList) const
{

    // look for items in main bag
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; itemIdSearchList.size() > 0 && slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (!pItem)
            continue;

        for (std::list<uint32>::iterator it = itemIdSearchList.begin(); it != itemIdSearchList.end(); ++it)
        {
            if (pItem->GetProto()->ItemId != *it)
                continue;

            if (m_bot->GetTrader() && m_bot->GetTradeData()->HasItem(pItem->GetObjectGuid()))
                continue;

            foundItemList.push_back(pItem);
            itemIdSearchList.erase(it);
            break;
        }
    }

    // for all for items in other bags
    for (uint8 bag = INVENTORY_SLOT_BAG_START; itemIdSearchList.size() > 0 && bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        Bag* const pBag = (Bag *) m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);
        if (!pBag)
            continue;

        for (uint8 slot = 0; itemIdSearchList.size() > 0 && slot < pBag->GetBagSize(); ++slot)
        {
            Item* const pItem = m_bot->GetItemByPos(bag, slot);
            if (!pItem)
                continue;

            for (std::list<uint32>::iterator it = itemIdSearchList.begin(); it != itemIdSearchList.end(); ++it)
            {
                if (pItem->GetProto()->ItemId != *it)
                    continue;

                if (m_bot->GetTrader() && m_bot->GetTradeData()->HasItem(pItem->GetObjectGuid()))
                    continue;

                foundItemList.push_back(pItem);
                itemIdSearchList.erase(it);
                break;
            }
        }
    }
}

void PlayerbotAI::findNearbyGO()
{
    if (m_collectObjects.empty())
        return;

    std::list<GameObject*> tempTargetGOList;
    float radius = 20.0f;

    for (BotEntryList::iterator itr = m_collectObjects.begin(); itr != m_collectObjects.end(); itr++)
    {
        uint32 entry = *(itr);
        GameObjectInfo const * gInfo = ObjectMgr::GetGameObjectInfo(entry);

        uint32 lootid = gInfo->GetLootId();

        if(gInfo->GetLootId() > 0)
        {
            QueryResult *result;
            result = WorldDatabase.PQuery("SELECT item FROM gameobject_loot_template WHERE entry = '%u'",lootid);
            if (result)
            {
                Field *fields = result->Fetch();
                uint32 Item = fields[0].GetUInt32();
                if (!IsInQuestItemList(Item))    // quest item needed
                {
                    m_collectObjects.remove(entry); // remove gameobject from collect list
                    delete result;
                    return;
                }
                delete result;
            }
        }

        // search for GOs with entry, within range of m_bot
        MaNGOS::GameObjectEntryInPosRangeCheck go_check(*m_bot, entry, m_bot->GetPositionX(), m_bot->GetPositionY(), m_bot->GetPositionZ(), radius);
        MaNGOS::GameObjectListSearcher<MaNGOS::GameObjectEntryInPosRangeCheck> checker(tempTargetGOList, go_check);
        Cell::VisitGridObjects(m_bot, checker, radius);

        // no objects found, continue to next entry
        if (tempTargetGOList.empty())
            continue;

        // add any objects found to our lootTargets
        for (std::list<GameObject*>::iterator iter = tempTargetGOList.begin(); iter != tempTargetGOList.end(); iter++)
        {
            GameObject* go = (*iter);
            if (go->isSpawned())
                m_lootTargets.push_back(go->GetObjectGuid());
        }
    }
}

void PlayerbotAI::findNearbyCreature()
{
    std::list<Creature*> creatureList;
    float radius = 2.5;

    CellPair pair(MaNGOS::ComputeCellPair(m_bot->GetPositionX(), m_bot->GetPositionY()));
    Cell cell(pair);

    MaNGOS::AnyUnitInObjectRangeCheck go_check(m_bot, radius);
    MaNGOS::CreatureListSearcher<MaNGOS::AnyUnitInObjectRangeCheck> go_search(creatureList, go_check);
    TypeContainerVisitor<MaNGOS::CreatureListSearcher<MaNGOS::AnyUnitInObjectRangeCheck>, GridTypeMapContainer> go_visit(go_search);

    // Get Creatures
    cell.Visit(pair, go_visit, *(m_bot->GetMap()), *(m_bot), radius);

    // if (!creatureList.empty())
    //    TellMaster("Found %i Creatures.", creatureList.size());

    for (std::list<Creature*>::iterator iter = creatureList.begin(); iter != creatureList.end(); iter++)
    {
        Creature* currCreature = *iter;

        for (std::list<enum NPCFlags>::iterator itr = m_findNPC.begin(); itr != m_findNPC.end(); itr++)
        {
            uint32 npcflags = currCreature->GetUInt32Value(UNIT_NPC_FLAGS);

            if (!(*itr & npcflags))
                continue;

            WorldObject *wo = m_bot->GetMap()->GetWorldObject(currCreature->GetObjectGuid());

            if (m_bot->GetDistance(wo) > CONTACT_DISTANCE + wo->GetObjectBoundingRadius())
            {
                float x, y, z;
                wo->GetContactPoint(m_bot, x, y, z, wo->GetObjectBoundingRadius());
                m_bot->GetMotionMaster()->MovePoint(wo->GetMapId(), x, y, z, false);
                // give time to move to point before trying again
                SetIgnoreUpdateTime(1);
            }

            if (m_bot->GetDistance(wo) < INTERACTION_DISTANCE)
            {

                // DEBUG_LOG("%s is interacting with (%s)",m_bot->GetName(),currCreature->GetCreatureInfo()->Name);
                GossipMenuItemsMapBounds pMenuItemBounds = sObjectMgr.GetGossipMenuItemsMapBounds(currCreature->GetCreatureInfo()->GossipMenuId);

                // prepares quest menu when true
                bool canSeeQuests = currCreature->GetCreatureInfo()->GossipMenuId == m_bot->GetDefaultGossipMenuForSource(wo);

                // if canSeeQuests (the default, top level menu) and no menu options exist for this, use options from default options
                if (pMenuItemBounds.first == pMenuItemBounds.second && canSeeQuests)
                    pMenuItemBounds = sObjectMgr.GetGossipMenuItemsMapBounds(0);

                for (GossipMenuItemsMap::const_iterator it = pMenuItemBounds.first; it != pMenuItemBounds.second; it++)
                {
                    if (!(it->second.npc_option_npcflag & npcflags))
                        continue;

                    DEBUG_LOG("GOSSIP_OPTION_ (%u)",it->second.option_id);

                    switch (it->second.option_id)
                    {
                        case GOSSIP_OPTION_BANKER:
                        {
                            // Manage banking actions
                            if (!m_tasks.empty())
                                for (std::list<taskPair>::iterator ait = m_tasks.begin(); ait != m_tasks.end(); )
                                {
                                    switch (ait->first)
                                    {
                                        // withdraw items
                                        case WITHDRAW:
                                        {
                                            // TellMaster("Withdraw items");
                                            if (!Withdraw(ait->second))
                                                DEBUG_LOG("Withdraw: Couldn't withdraw (%u)", ait->second);
                                            break;
                                        }
                                        // deposit items
                                        case DEPOSIT:
                                        {
                                            // TellMaster("Deposit items");
                                            if (!Deposit(ait->second))
                                                DEBUG_LOG("Deposit: Couldn't deposit (%u)", ait->second);
                                            break;
                                        }
                                        default:
                                            break;
                                    }
                                    ait = m_tasks.erase(ait);
                                }
                            BankBalance();
                            break;
                        }
                        case GOSSIP_OPTION_TAXIVENDOR:
                        case GOSSIP_OPTION_GOSSIP:
                        case GOSSIP_OPTION_INNKEEPER:
                        case GOSSIP_OPTION_TRAINER:
                        case GOSSIP_OPTION_QUESTGIVER:
                        case GOSSIP_OPTION_VENDOR:
                        {
                            // Manage questgiver, trainer, innkeeper & vendor actions
                            if (!m_tasks.empty())
                                for (std::list<taskPair>::iterator ait = m_tasks.begin(); ait != m_tasks.end(); )
                                {
                                    switch (ait->first)
                                    {
                                        // take new quests
                                        case TAKE:
                                        {
                                            // TellMaster("Accepting quest");
                                            if (!AddQuest(ait->second, wo))
                                                DEBUG_LOG("AddQuest: Couldn't add quest (%u)", ait->second);
                                            break;
                                        }
                                        // list npc quests
                                        case LIST:
                                        {
                                            // TellMaster("Show available npc quests");
                                            ListQuests(wo);
                                            break;
                                        }
                                        // end quests
                                        case END:
                                        {
                                            // TellMaster("Turn in available quests");
                                            TurnInQuests(wo);
                                            break;
                                        }
                                        // sell items
                                        case SELL:
                                        {
                                            // TellMaster("Selling items");
                                            Sell(ait->second);
                                            break;
                                        }
                                        // repair items
                                        case REPAIR:
                                        {
                                            // TellMaster("Repairing items");
                                            Repair(ait->second, currCreature);
                                            break;
                                        }
                                        default:
                                            break;
                                    }
                                    ait = m_tasks.erase(ait);
                                }
                            break;
                        }
                        case GOSSIP_OPTION_AUCTIONEER:
                        {
                            // Manage auctioneer actions
                            if (!m_tasks.empty())
                                for (std::list<taskPair>::iterator ait = m_tasks.begin(); ait != m_tasks.end(); )
                                {
                                    switch (ait->first)
                                    {
                                        // add new auction item
                                        case ADD:
                                        {
                                            // TellMaster("Creating auction");
                                            AddAuction(ait->second, currCreature);
                                            break;
                                        }
                                        // cancel active auction
                                        case REMOVE:
                                        {
                                            // TellMaster("Cancelling auction");
                                            if (!RemoveAuction(ait->second))
                                                DEBUG_LOG("RemoveAuction: Couldn't remove auction (%u)", ait->second);
                                            break;
                                        }
                                        default:
                                            break;
                                    }
                                    ait = m_tasks.erase(ait);
                                }
                            ListAuctions();
                            break;
                        }
                        default:
                            break;
                    }

                    m_bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                }
            }
            itr = m_findNPC.erase(itr); // all done lets go home
            m_bot->GetMotionMaster()->Clear(false);
            m_bot->GetMotionMaster()->MoveIdle();
			// This is a fix for when the iterator returned from FindNPC is the last. If it is the end we break the loop or else
			// we might try to increment passed the last iteration. http://stackoverflow.com/questions/6167082/c-list-iterator-not-incrementable
			if (itr == m_findNPC.end())
				break;
        }
    }
}

// Playerbot wrapper to know if a target is elite or not
// This is used by the AI to switch from one action to another
// if creature is dangerous (elite)
bool PlayerbotAI::IsElite(Unit* pTarget) const
{
    if (!pTarget)
        return false;

    if (Creature * pCreature = (Creature*) pTarget)
    {
        return (pCreature->IsElite() || pCreature->IsWorldBoss());
        }

    return false;
        }

// Check if bot target has one of the following auras: Sap, Polymorph, Shackle Undead, Banish, Seduction, Freezing Trap, Hibernate
// This is used by the AI to prevent bots from attacking crowd control targets

static const uint32 uAurasIds[21] =
{
    118, 12824, 12825, 12826,   // polymorph
    28272, 28271,               // polymorph pig, turtle
    9484, 9485, 10955,          // shackle
    6358,                       // seduction
    710, 18647,                 // banish
    6770, 2070, 11297,          // sap
    3355, 14308, 14309,         // freezing trap (effect auras IDs, not spell IDs)
    2637, 18657, 18658          // hibernate
};

bool PlayerbotAI::IsNeutralized(Unit* pTarget)
{
    if (!pTarget)
        return false;

    for (uint8 i = 0; i < countof(uAurasIds); ++i)
    {
        if (pTarget->HasAura(uAurasIds[i], EFFECT_INDEX_0))
            return true;
    }

    return false;
}

bool PlayerbotAI::CanStore()
{
    uint32 totalused = 0;
    // list out items in main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
    {
        const Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (pItem)
            totalused++;
    }
    uint32 totalfree = 16 - totalused;
    // list out items in other removable backpacks
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        const Bag* const pBag = (Bag *) m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);
        if (pBag)
        {
            ItemPrototype const* pBagProto = pBag->GetProto();
            if (pBagProto->Class == ITEM_CLASS_CONTAINER && pBagProto->SubClass == ITEM_SUBCLASS_CONTAINER)
                totalfree =  totalfree + pBag->GetFreeSlots();
        }
    }
    return totalfree;
}

// use item on self
void PlayerbotAI::UseItem(Item *item)
{
    UseItem(item, TARGET_FLAG_SELF, ObjectGuid());
}

// use item on equipped item
void PlayerbotAI::UseItem(Item *item, uint8 targetInventorySlot)
{
    if (targetInventorySlot >= EQUIPMENT_SLOT_END)
        return;

    Item* const targetItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, targetInventorySlot);
    if (!targetItem)
        return;

    UseItem(item, TARGET_FLAG_ITEM, targetItem->GetObjectGuid());
}

// use item on unit
void PlayerbotAI::UseItem(Item *item, Unit *target)
{
    if (!target)
        return;

    UseItem(item, TARGET_FLAG_UNIT, target->GetObjectGuid());
}

// generic item use method
void PlayerbotAI::UseItem(Item *item, uint16 targetFlag, ObjectGuid targetGUID)
{
    if (!item)
        return;

    uint8 bagIndex = item->GetBagSlot();
    uint8 slot = item->GetSlot();
    uint8 spell_count = 1;
    ObjectGuid item_guid = item->GetObjectGuid();

    if (uint32 questid = item->GetProto()->StartQuest)
    {
        std::ostringstream report;

        Quest const* qInfo = sObjectMgr.GetQuestTemplate(questid);
        if (qInfo)
        {
            m_bot->GetMotionMaster()->Clear(true);
            WorldPacket* const packet = new WorldPacket(CMSG_QUESTGIVER_ACCEPT_QUEST, 8 + 4 + 4);
            *packet << item_guid;
            *packet << questid;
            *packet << uint32(0);
            m_bot->GetSession()->QueuePacket(packet); // queue the packet to get around race condition
            report << "|cffffff00Quest taken |r" << qInfo->GetTitle();
            TellMaster(report.str());
        }
        return;
    }

    uint32 spellId = 0;
    for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
    {
        if (item->GetProto()->Spells[i].SpellId > 0)
        {
            spellId = item->GetProto()->Spells[i].SpellId;
            break;
        }
    }

    SpellEntry const * spellInfo = sSpellStore.LookupEntry(spellId);
    if (!spellInfo)
    {
        TellMaster("Can't find spell entry for spell %u on item %u", spellId, item->GetEntry());
        return;
    }

    SpellCastTimesEntry const * castingTimeEntry = sSpellCastTimesStore.LookupEntry(spellInfo->CastingTimeIndex);
    if (!castingTimeEntry)
    {
        TellMaster("Can't find casting time entry for spell %u with index %u", spellId, spellInfo->CastingTimeIndex);
        return;
    }
    // stop movement to prevent cancel spell casting
    else if (castingTimeEntry && castingTimeEntry->CastTime)
    {
        DEBUG_LOG ("[PlayerbotAI]: UseItem - Bot movement reset for casting %s (%u)", spellInfo->SpellName[0], spellId);
        MovementClear();
    }

    m_CurrentlyCastingSpellId = spellId;

    WorldPacket* const packet = new WorldPacket(CMSG_USE_ITEM, 13);
    *packet << bagIndex;
    *packet << slot;
    *packet << spell_count;
    *packet << targetFlag;

    if (targetFlag & (TARGET_FLAG_UNIT | TARGET_FLAG_ITEM | TARGET_FLAG_OBJECT))
        *packet << targetGUID.WriteAsPacked();

    m_bot->GetSession()->QueuePacket(packet);
}

// submits packet to use an item
void PlayerbotAI::EquipItem(Item* src_Item)
{
    uint8 src_bagIndex = src_Item->GetBagSlot();
    uint8 src_slot = src_Item->GetSlot();

    DEBUG_LOG("PlayerbotAI::EquipItem: %s in srcbag = %u, srcslot = %u", src_Item->GetProto()->Name1, src_bagIndex, src_slot);

    uint16 dest;
    InventoryResult msg = m_bot->CanEquipItem(NULL_SLOT, dest, src_Item, !src_Item->IsBag());
    if (msg != EQUIP_ERR_OK)
    {
        m_bot->SendEquipError(msg, src_Item, nullptr);
        return;
    }

    uint16 src = src_Item->GetPos();
    if (dest == src)                                        // prevent equip in same slot, only at cheat
        return;

    Item *dest_Item = m_bot->GetItemByPos(dest);
    if (!dest_Item)                                          // empty slot, simple case
    {
        m_bot->RemoveItem(src_bagIndex, src_slot, true);
        m_bot->EquipItem(dest, src_Item, true);
        m_bot->AutoUnequipOffhandIfNeed();
    }
    else                                                    // have currently equipped item, not simple case
    {
        uint8 dest_bagIndex = dest_Item->GetBagSlot();
        uint8 dest_slot = dest_Item->GetSlot();

        msg = m_bot->CanUnequipItem(dest, false);
        if (msg != EQUIP_ERR_OK)
        {
            m_bot->SendEquipError(msg, dest_Item, nullptr);
            return;
        }

        // check dest->src move possibility
        ItemPosCountVec sSrc;
        if (m_bot->IsInventoryPos(src))
        {
            msg = m_bot->CanStoreItem(src_bagIndex, src_slot, sSrc, dest_Item, true);
            if (msg != EQUIP_ERR_OK)
                msg = m_bot->CanStoreItem(src_bagIndex, NULL_SLOT, sSrc, dest_Item, true);
            if (msg != EQUIP_ERR_OK)
                msg = m_bot->CanStoreItem(NULL_BAG, NULL_SLOT, sSrc, dest_Item, true);
        }

        if (msg != EQUIP_ERR_OK)
        {
            m_bot->SendEquipError(msg, dest_Item, src_Item);
            return;
        }

        // now do moves, remove...
        m_bot->RemoveItem(dest_bagIndex, dest_slot, false);
        m_bot->RemoveItem(src_bagIndex, src_slot, false);

        // add to dest
        m_bot->EquipItem(dest, src_Item, true);

        // add to src
        if (m_bot->IsInventoryPos(src))
            m_bot->StoreItem(sSrc, dest_Item, true);

        m_bot->AutoUnequipOffhandIfNeed();
    }
}

// submits packet to trade an item (trade window must already be open)
// default slot is -1 which means trade slots 0 to 5. if slot is set
// to TRADE_SLOT_NONTRADED (which is slot 6) item will be shown in the
// 'Will not be traded' slot.
bool PlayerbotAI::TradeItem(const Item& item, int8 slot)
{
    DEBUG_LOG ("[PlayerbotAI]: TradeItem - slot=%d, hasTrader=%d, itemInTrade=%d, itemTradeable=%d",
               slot,
               (m_bot->GetTrader() ? 1 : 0),
               (item.IsInTrade() ? 1 : 0),
               (item.CanBeTraded() ? 1 : 0)
               );

    if (!m_bot->GetTrader() || item.IsInTrade() || (!item.CanBeTraded() && slot != TRADE_SLOT_NONTRADED))
        return false;

    int8 tradeSlot = -1;

    TradeData* pTrade = m_bot->GetTradeData();
    if ((slot >= 0 && slot < TRADE_SLOT_COUNT) && pTrade->GetItem(TradeSlots(slot)) == nullptr)
        tradeSlot = slot;
    else
        for (uint8 i = 0; i < TRADE_SLOT_TRADED_COUNT && tradeSlot == -1; i++)
        {
            if (pTrade->GetItem(TradeSlots(i)) == nullptr)
            {
                tradeSlot = i;
                // reserve trade slot to allow multiple items to be traded
                pTrade->SetItem(TradeSlots(i), const_cast<Item*>(&item));
            }
        }

    if (tradeSlot == -1) return false;

    WorldPacket* const packet = new WorldPacket(CMSG_SET_TRADE_ITEM, 3);
    *packet << (uint8) tradeSlot << (uint8) item.GetBagSlot()
            << (uint8) item.GetSlot();
    m_bot->GetSession()->QueuePacket(packet);
    return true;
}

// submits packet to trade copper (trade window must be open)
bool PlayerbotAI::TradeCopper(uint32 copper)
{
    if (copper > 0)
    {
        WorldPacket* const packet = new WorldPacket(CMSG_SET_TRADE_GOLD, 4);
        *packet << copper;
        m_bot->GetSession()->QueuePacket(packet);
        return true;
    }
    return false;
}

bool PlayerbotAI::DoTeleport(WorldObject& /*obj*/)
{
    SetIgnoreUpdateTime(2);
    PlayerbotChatHandler ch(GetMaster());
    if (!ch.teleport(*m_bot))
    {
        ch.sysmessage(".. could not be teleported ..");
        // DEBUG_LOG ("[PlayerbotAI]: DoTeleport - %s failed to teleport", m_bot->GetName() );
        return false;
    }
    return true;
}

void PlayerbotAI::HandleTeleportAck()
{
    SetIgnoreUpdateTime(2);
    m_bot->GetMotionMaster()->Clear(true);
    if (m_bot->IsBeingTeleportedNear())
    {
        WorldPacket p = WorldPacket(MSG_MOVE_TELEPORT_ACK, 8+4+4);
        p << m_bot->GetObjectGuid();
        p << (uint32) 0; // supposed to be flags? not used currently
        p << (uint32) CurrentTime(); // time - not currently used
        m_bot->GetSession()->HandleMoveTeleportAckOpcode(p);
    }
    else if (m_bot->IsBeingTeleportedFar())
        m_bot->GetSession()->HandleMoveWorldportAckOpcode();
}

// Localization support
void PlayerbotAI::ItemLocalization(std::string& itemName, const uint32 itemID) const
{
    uint32 loc = GetMaster()->GetSession()->GetSessionDbLocaleIndex();
    std::wstring wnamepart;

    ItemLocale const *pItemInfo = sObjectMgr.GetItemLocale(itemID);
    if (pItemInfo)
        if (pItemInfo->Name.size() > loc && !pItemInfo->Name[loc].empty())
        {
            const std::string name = pItemInfo->Name[loc];
            if (Utf8FitTo(name, wnamepart))
                itemName = name.c_str();
        }
}

void PlayerbotAI::QuestLocalization(std::string& questTitle, const uint32 questID) const
{
    uint32 loc = GetMaster()->GetSession()->GetSessionDbLocaleIndex();
    std::wstring wnamepart;

    QuestLocale const *pQuestInfo = sObjectMgr.GetQuestLocale(questID);
    if (pQuestInfo)
        if (pQuestInfo->Title.size() > loc && !pQuestInfo->Title[loc].empty())
        {
            const std::string title = pQuestInfo->Title[loc];
            if (Utf8FitTo(title, wnamepart))
                questTitle = title.c_str();
        }
}

void PlayerbotAI::CreatureLocalization(std::string& creatureName, const uint32 entry) const
{
    uint32 loc = GetMaster()->GetSession()->GetSessionDbLocaleIndex();
    std::wstring wnamepart;

    CreatureLocale const *pCreatureInfo = sObjectMgr.GetCreatureLocale(entry);
    if (pCreatureInfo)
        if (pCreatureInfo->Name.size() > loc && !pCreatureInfo->Name[loc].empty())
        {
            const std::string title = pCreatureInfo->Name[loc];
            if (Utf8FitTo(title, wnamepart))
                creatureName = title.c_str();
        }
}

void PlayerbotAI::GameObjectLocalization(std::string& gameobjectName, const uint32 entry) const
{
    uint32 loc = GetMaster()->GetSession()->GetSessionDbLocaleIndex();
    std::wstring wnamepart;

    GameObjectLocale const *pGameObjectInfo = sObjectMgr.GetGameObjectLocale(entry);
    if (pGameObjectInfo)
        if (pGameObjectInfo->Name.size() > loc && !pGameObjectInfo->Name[loc].empty())
        {
            const std::string title = pGameObjectInfo->Name[loc];
            if (Utf8FitTo(title, wnamepart))
                gameobjectName = title.c_str();
        }
}

// Helper function for automatically selling poor quality items to the vendor
void PlayerbotAI::_doSellItem(Item* const item, std::ostringstream &report, std::ostringstream &canSell, uint32 &TotalCost, uint32 &TotalSold)
{
    if (!item)
        return;

    if (item->CanBeTraded() && item->GetProto()->Quality == ITEM_QUALITY_POOR)
    {
        uint32 cost = item->GetCount() * item->GetProto()->SellPrice;
        m_bot->ModifyMoney(cost);
        m_bot->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
        m_bot->AddItemToBuyBackSlot(item);

        ++TotalSold;
        TotalCost += cost;

        report << "Sold ";
        MakeItemLink(item, report, true);
        report << " for ";

        uint32 gold = uint32(cost / 10000);
        cost -= (gold * 10000);
        uint32 silver = uint32(cost / 100);
        cost -= (silver * 100);

        if (gold > 0)
            report << gold << "|r|cfffffc00g|r|cff00ff00";
        if (silver > 0)
            report << silver << "|r|cffc0c0c0s|r|cff00ff00";
        report << cost << "|r|cff95524Cc|r|cff00ff00\n";
    }
	// added to sell normal and uncommon items automatically if they are not useful to the bot or gear that is not an upgrade
	else if (item->CanBeTraded() && (item->GetProto()->Quality == ITEM_QUALITY_NORMAL || item->GetProto()->Quality == ITEM_QUALITY_UNCOMMON))
	{
		// If the items is not useful, sell it
		if (!IsItemUseful(item->GetProto()->ItemId)) {
			uint32 cost = item->GetCount() * item->GetProto()->SellPrice;
			m_bot->ModifyMoney(cost);
			m_bot->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
			m_bot->AddItemToBuyBackSlot(item);

			++TotalSold;
			TotalCost += cost;

			report << "Sold ";
			MakeItemLink(item, report, true);
			report << " for ";

			uint32 gold = uint32(cost / 10000);
			cost -= (gold * 10000);
			uint32 silver = uint32(cost / 100);
			cost -= (silver * 100);

			if (gold > 0)
				report << gold << "|r|cfffffc00g|r|cff00ff00";
			if (silver > 0)
				report << silver << "|r|cffc0c0c0s|r|cff00ff00";
			report << cost << "|r|cff95524Cc|r|cff00ff00\n";
		}
		else {
			// the item is useful, but if it's a weapon we can use and it's not an upgrade sell it. We don't keep greens lying around
			if (item->GetProto()->Class == ITEM_CLASS_WEAPON || item->GetProto()->Class == ITEM_CLASS_ARMOR) {
				if (!IsItemAnUpgrade(item)) {
					uint32 cost = item->GetCount() * item->GetProto()->SellPrice;
					m_bot->ModifyMoney(cost);
					m_bot->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
					m_bot->AddItemToBuyBackSlot(item);

					++TotalSold;
					TotalCost += cost;

					report << "Sold ";
					MakeItemLink(item, report, true);
					report << " for ";

					uint32 gold = uint32(cost / 10000);
					cost -= (gold * 10000);
					uint32 silver = uint32(cost / 100);
					cost -= (silver * 100);

					if (gold > 0)
						report << gold << "|r|cfffffc00g|r|cff00ff00";
					if (silver > 0)
						report << silver << "|r|cffc0c0c0s|r|cff00ff00";
					report << cost << "|r|cff95524Cc|r|cff00ff00\n";
				}
				else {
					// then equip it
					EquipItem(item);
				}
			}
			else if (item->GetProto()->SellPrice > 0) {
				MakeItemLink(item, canSell, true);
			}
		}
	}
    else if (item->GetProto()->SellPrice > 0)
        MakeItemLink(item, canSell, true);
}

bool PlayerbotAI::Withdraw(const uint32 itemid)
{
    Item* pItem = FindItemInBank(itemid);
    if (pItem)
    {
        std::ostringstream report;

        ItemPosCountVec dest;
        InventoryResult msg = m_bot->CanStoreItem(NULL_BAG, NULL_SLOT, dest, pItem, false);
        if (msg != EQUIP_ERR_OK)
        {
            m_bot->SendEquipError(msg, pItem, nullptr);
            return false;
        }

        m_bot->RemoveItem(pItem->GetBagSlot(), pItem->GetSlot(), true);
        m_bot->StoreItem(dest, pItem, true);

        report << "Withdrawn ";
        MakeItemLink(pItem, report, true);

        TellMaster(report.str());
    }
    return true;
}

bool PlayerbotAI::Deposit(const uint32 itemid)
{
    Item* pItem = FindItem(itemid);
    if (pItem)
    {
        std::ostringstream report;

        ItemPosCountVec dest;
        InventoryResult msg = m_bot->CanBankItem(NULL_BAG, NULL_SLOT, dest, pItem, false);
        if (msg != EQUIP_ERR_OK)
        {
            m_bot->SendEquipError(msg, pItem, nullptr);
            return false;
        }

        m_bot->RemoveItem(pItem->GetBagSlot(), pItem->GetSlot(), true);
        m_bot->BankItem(dest, pItem, true);

        report << "Deposited ";
        MakeItemLink(pItem, report, true);

        TellMaster(report.str());
    }
    return true;
}

void PlayerbotAI::BankBalance()
{
    std::ostringstream report;

    report << "In my bank\n ";
    report << "My item slots: ";

    for (uint8 slot = BANK_SLOT_ITEM_START; slot < BANK_SLOT_ITEM_END; ++slot)
    {
        Item* const item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
            MakeItemLink(item, report, true);
    }
    TellMaster(report.str());

    // and each of my bank bags
    for (uint8 bag = BANK_SLOT_BAG_START; bag < BANK_SLOT_BAG_END; ++bag)
    {
        std::ostringstream goods;
        const Bag* const pBag = static_cast<Bag *>(m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag));
        if (pBag)
        {
            goods << "\nMy ";
            const ItemPrototype* const pBagProto = pBag->GetProto();
            std::string bagName = pBagProto->Name1;
            ItemLocalization(bagName, pBagProto->ItemId);
            goods << bagName << " slot: ";

            for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                Item* const item = m_bot->GetItemByPos(bag, slot);
                if (item)
                    MakeItemLink(item, goods, true);
            }
            TellMaster(goods.str());
        }
    }
}

void PlayerbotAI::Repair(const uint32 itemid, Creature* rCreature)
{
    Item* rItem = FindItem(itemid); // if item equipped or in bags
    uint8 IsInGuild = (m_bot->GetGuildId() != 0) ? uint8(1) : uint8(0);
    ObjectGuid itemGuid = (rItem) ? rItem->GetObjectGuid() : ObjectGuid();

    WorldPacket* const packet = new WorldPacket(CMSG_REPAIR_ITEM, 8 + 8 + 1);
    *packet << rCreature->GetObjectGuid();  // repair npc guid
    *packet << itemGuid; // if item specified then repair this, else repair all
    *packet << IsInGuild;  // guildbank yes=1 no=0
    m_bot->GetSession()->QueuePacket(packet);  // queue the packet to get around race condition
}

bool PlayerbotAI::RemoveAuction(const uint32 auctionid)
{
    QueryResult *result = CharacterDatabase.PQuery(
        "SELECT houseid,itemguid,item_template,itemowner,buyoutprice,time,buyguid,lastbid,startbid,deposit FROM auction WHERE id = '%u'", auctionid);

    AuctionEntry *auction;

    if (result)
    {
        Field *fields = result->Fetch();

        auction = new AuctionEntry;
        auction->Id = auctionid;
        uint32 houseid  = fields[0].GetUInt32();
        auction->itemGuidLow = fields[1].GetUInt32();
        auction->itemTemplate = fields[2].GetUInt32();
        auction->owner = fields[3].GetUInt32();
        auction->buyout = fields[4].GetUInt32();
        auction->expireTime = fields[5].GetUInt32();
        auction->bidder = fields[6].GetUInt32();
        auction->bid = fields[7].GetUInt32();
        auction->startbid = fields[8].GetUInt32();
        auction->deposit = fields[9].GetUInt32();
        auction->auctionHouseEntry = nullptr;                  // init later

        // check if sold item exists for guid
        // and item_template in fact (GetAItem will fail if problematic in result check in AuctionHouseMgr::LoadAuctionItems)
        Item* pItem = sAuctionMgr.GetAItem(auction->itemGuidLow);
        if (!pItem)
        {
            auction->DeleteFromDB();
            sLog.outError("Auction %u has not a existing item : %u, deleted", auction->Id, auction->itemGuidLow);
            delete auction;
            delete result;
            return false;
        }

        auction->auctionHouseEntry = sAuctionHouseStore.LookupEntry(houseid);

        // Attempt send item back to owner
        std::ostringstream msgAuctionCanceledOwner;
        msgAuctionCanceledOwner << auction->itemTemplate << ":0:" << AUCTION_CANCELED << ":0:0";

        // item will deleted or added to received mail list
        MailDraft(msgAuctionCanceledOwner.str(), "")    // TODO: fix body
        .AddItem(pItem)
        .SendMailTo(MailReceiver(ObjectGuid(HIGHGUID_PLAYER, auction->owner)), auction, MAIL_CHECK_MASK_COPIED);

        if (sAuctionMgr.RemoveAItem(auction->itemGuidLow))
            m_bot->GetSession()->SendAuctionCommandResult(auction, AUCTION_REMOVED, AUCTION_OK);

        auction->DeleteFromDB();

        delete auction;
        delete result;
    }
    return true;
}

void PlayerbotAI::ListQuests(WorldObject * questgiver)
{
    if (!questgiver)
        return;

    // list all bot quests this NPC has
    m_bot->PrepareQuestMenu(questgiver->GetObjectGuid());
    QuestMenu& questMenu = m_bot->PlayerTalkClass->GetQuestMenu();
    std::ostringstream out;
    for (uint32 iI = 0; iI < questMenu.MenuItemCount(); ++iI)
    {
        QuestMenuItem const& qItem = questMenu.GetItem(iI);
        uint32 questID = qItem.m_qId;
        Quest const* pQuest = sObjectMgr.GetQuestTemplate(questID);

        std::string questTitle  = pQuest->GetTitle();
        QuestLocalization(questTitle, questID);

        if (m_bot->SatisfyQuestStatus(pQuest, false))
        {
            if (gQuestFetch != 1)
            {
            out << "|cff808080|Hquest:" << questID << ':' << pQuest->GetQuestLevel() << "|h[" << questTitle << "]|h|r";
    }
            else
            {
                if (!AddQuest(questID, questgiver))
                    continue;
            }
        }    
    }
    if (!out.str().empty())
        TellMaster(out.str());
}

bool PlayerbotAI::AddQuest(const uint32 entry, WorldObject * questgiver)
{
    std::ostringstream out;

    Quest const* qInfo = sObjectMgr.GetQuestTemplate(entry);
    if (!qInfo)
    {
        ChatHandler(GetMaster()).PSendSysMessage(LANG_COMMAND_QUEST_NOTFOUND, entry);
        return false;
    }

    if (m_bot->GetQuestStatus(entry) == QUEST_STATUS_COMPLETE)
    {
        TellMaster("I already completed that quest.");
        return false;
    }
    else if (!m_bot->CanTakeQuest(qInfo, false))
    {
        if (!m_bot->SatisfyQuestStatus(qInfo, false))
            TellMaster("I already have that quest.");
        else
            TellMaster("I can't take that quest.");
        return false;
    }
    else if (!m_bot->SatisfyQuestLog(false))
    {
        TellMaster("My quest log is full.");
        return false;
    }
    else if (m_bot->CanAddQuest(qInfo, false))
    {
        m_bot->AddQuest(qInfo, questgiver);

        std::string questTitle  = qInfo->GetTitle();
        QuestLocalization(questTitle, entry);

        out << "|cffffff00Quest taken " << "|cff808080|Hquest:" << entry << ':' << qInfo->GetQuestLevel() << "|h[" << questTitle << "]|h|r";

        if (m_bot->CanCompleteQuest(entry))
            m_bot->CompleteQuest(entry);

        // build needed items if quest contains any
        for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; i++)
            if (qInfo->ReqItemCount[i] > 0)
            {
                SetQuestNeedItems();
                break;
            }

        // build needed creatures if quest contains any
        for (int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
            if (qInfo->ReqCreatureOrGOCount[i] > 0)
            {
                SetQuestNeedCreatures();
                break;
            }

        TellMaster(out.str());
        return true;
    }
    return false;
}

void PlayerbotAI::ListAuctions()
{
    std::ostringstream report;

    QueryResult *result = CharacterDatabase.PQuery(
        "SELECT id,itemguid,item_template,time,buyguid,lastbid FROM auction WHERE itemowner = '%u'", m_bot->GetObjectGuid().GetCounter());
    if (result)
    {
        report << "My active auctions are: \n";
        do
        {
            Field *fields = result->Fetch();

            uint32 Id = fields[0].GetUInt32();
            uint32 itemGuidLow = fields[1].GetUInt32();
            uint32 itemTemplate = fields[2].GetUInt32();
            time_t expireTime = fields[3].GetUInt32();
            uint32 bidder = fields[4].GetUInt32();
            uint32 bid = fields[5].GetUInt32();

            time_t remtime = expireTime - CurrentTime();

            tm* aTm = gmtime(&remtime);

            if (expireTime > CurrentTime())
            {
                Item* aItem = sAuctionMgr.GetAItem(itemGuidLow);
                if (aItem)
                {
                    // Name
                    uint32 count = aItem->GetCount();
                    std::string name = aItem->GetProto()->Name1;
                    ItemLocalization(name, itemTemplate);
                    report << "\n|cffffffff|Htitle:" << Id << "|h[" << name;
                    if (count > 1)
                        report << "|cff00ff00x" << count << "|cffffffff" << "]|h|r";
                    else
                        report << "]|h|r";
                }

                if (bidder)
                {
                    ObjectGuid guid = ObjectGuid(HIGHGUID_PLAYER, bidder);
                    std::string bidder_name;
                    if (sObjectMgr.GetPlayerNameByGUID(guid, bidder_name))
                        report << " " << bidder_name << ": ";

                    uint32 gold = uint32(bid / 10000);
                    bid -= (gold * 10000);
                    uint32 silver = uint32(bid / 100);
                    bid -= (silver * 100);

                    if (gold > 0)
                        report << gold << "|r|cfffffc00g|r|cff00ff00";
                    if (silver > 0)
                        report << silver << "|r|cffc0c0c0s|r|cff00ff00";
                    report << bid << "|r|cff95524Cc|r|cff00ff00";
                }
                if(aItem)
                    report << " ends: " << aTm->tm_hour << "|cff0070dd|hH|h|r " << aTm->tm_min << "|cff0070dd|hmin|h|r";
            }
        } while (result->NextRow());

        delete result;
        TellMaster(report.str().c_str());
    }
}

void PlayerbotAI::AddAuction(const uint32 itemid, Creature* aCreature)
{
    Item* aItem = FindItem(itemid);
    if (aItem)
    {
        std::ostringstream out;
        srand(CurrentTime());
        uint32 duration[3] = { 720, 1440, 2880 };  // 720 = 12hrs, 1440 = 24hrs, 2880 = 48hrs
        uint32 etime = duration[rand() % 3];

        uint32 min = urand(aItem->GetProto()->SellPrice * aItem->GetCount(), aItem->GetProto()->BuyPrice * aItem->GetCount()) * (aItem->GetProto()->Quality + 1);
        uint32 max = urand(aItem->GetProto()->SellPrice * aItem->GetCount(), aItem->GetProto()->BuyPrice * aItem->GetCount()) * (aItem->GetProto()->Quality + 1);

        out << "Auctioning ";
        MakeItemLink(aItem, out, true);
        out << " with " << aCreature->GetCreatureInfo()->Name;
        TellMaster(out.str().c_str());

        WorldPacket* const packet = new WorldPacket(CMSG_AUCTION_SELL_ITEM, 8 + 4 + 8 + 4 + 4 + 4 + 4);
        *packet << aCreature->GetObjectGuid();     // auctioneer guid
        *packet << uint32(1);                      // const 1
        *packet << aItem->GetObjectGuid();         // item guid
        *packet << aItem->GetCount();      // stacksize
        *packet << uint32((min < max) ? min : max);  // starting bid
        *packet << uint32((max > min) ? max : min);  // buyout
        *packet << uint32(etime);  // auction duration

        m_bot->GetSession()->QueuePacket(packet);  // queue the packet to get around race condition
    }
}

void PlayerbotAI::Sell(const uint32 itemid)
{
    Item* pItem = FindItem(itemid);
    if (pItem)
    {
        std::ostringstream report;

        uint32 cost = pItem->GetCount() * pItem->GetProto()->SellPrice;
        m_bot->ModifyMoney(cost);
        m_bot->MoveItemFromInventory(pItem->GetBagSlot(), pItem->GetSlot(), true);
        m_bot->AddItemToBuyBackSlot(pItem);

        report << "Sold ";
        MakeItemLink(pItem, report, true);
        report << " for ";

        uint32 gold = uint32(cost / 10000);
        cost -= (gold * 10000);
        uint32 silver = uint32(cost / 100);
        cost -= (silver * 100);

        if (gold > 0)
            report << gold << "|r|cfffffc00g|r|cff00ff00";
        if (silver > 0)
            report << silver << "|r|cffc0c0c0s|r|cff00ff00";
        report << cost << "|r|cff95524Cc|r|cff00ff00";

        TellMaster(report.str());
    }
}

void PlayerbotAI::SellGarbage(bool bListNonTrash, bool bDetailTrashSold, bool bVerbose)
{
    uint32 SoldCost = 0;
    uint32 SoldQuantity = 0;
    std::ostringstream report, goods;

    // list out items in main backpack
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        Item* const item = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        if (item)
            _doSellItem(item, report, goods, SoldCost, SoldQuantity);
    }

    // and each of our other packs
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
    {
        std::ostringstream goods;
        const Bag* const pBag = static_cast<Bag *>(m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag));
        if (pBag)
        {
            // Very nice, but who cares what bag it's in?
            //const ItemPrototype* const pBagProto = pBag->GetProto();
            //std::string bagName = pBagProto->Name1;
            //ItemLocalization(bagName, pBagProto->ItemId);
            //goods << "\nIn my " << bagName << ":";

            for (uint8 slot = 0; slot < pBag->GetBagSize(); ++slot)
            {
                Item* const item = m_bot->GetItemByPos(bag, slot);
                if (item)
                    _doSellItem(item, report, goods, SoldCost, SoldQuantity);
            }
        }
    }

    if (!bDetailTrashSold)
        report.str(""); // clear ostringstream

    if (SoldCost > 0)
    {
        if (bDetailTrashSold)
            report << "Sold total " << SoldQuantity << " item(s) for ";
        else
            report << "Sold " << SoldQuantity << " trash item(s) for ";
        uint32 gold = uint32(SoldCost / 10000);
        SoldCost -= (gold * 10000);
        uint32 silver = uint32(SoldCost / 100);
        SoldCost -= (silver * 100);

        if (gold > 0)
            report << gold << "|r|cfffffc00g|r|cff00ff00";
        if (silver > 0)
            report << silver << "|r|cffc0c0c0s|r|cff00ff00";
        report << SoldCost << "|r|cff95524Cc|r|cff00ff00";

        if (bVerbose)
        TellMaster(report.str());
    }

    // For all bags, non-gray sellable items
    if (bVerbose)
    {
        if (bListNonTrash && goods.str().size() > 0)
        {
            if (SoldQuantity)
                TellMaster("I could also sell: %s", goods.str().c_str());
            else
                TellMaster("I could sell: %s", goods.str().c_str());
}
        else if (SoldQuantity == 0 && goods.str().size() == 0)
        {
            TellMaster("No items to sell, trash or otherwise.");
        }
    }
}

void PlayerbotAI::GetTaxi(ObjectGuid guid, BotTaxiNode& nodes)
{
    // DEBUG_LOG("[PlayerbotAI]: GetTaxi - %s node[0] %d node[1] %d", m_bot->GetName(), nodes[0], nodes[1]);

    Creature *unit = m_bot->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_FLIGHTMASTER);
    if (!unit)
    {
        DEBUG_LOG("[PlayerbotAI]: GetTaxi - %s not found or you can't interact with it.", guid.GetString().c_str());
        return;
    }

    if (m_bot->m_taxi.IsTaximaskNodeKnown(nodes[0]) ? 0 : 1)
        return;

    if (m_bot->m_taxi.IsTaximaskNodeKnown(nodes[nodes.size() - 1]) ? 0 : 1)
        return;

    if (m_bot->GetPlayerbotAI()->GetMovementOrder() != MOVEMENT_STAY)
    {
        m_taxiNodes = nodes;
        m_taxiMaster = guid;
        SetState(BOTSTATE_FLYING);
    }
}

// handle commands sent through chat channels
void PlayerbotAI::HandleCommand(const std::string& text, Player& fromPlayer)
{
    // prevent bot task spam
    m_tasks.unique();
    m_findNPC.unique();

    DEBUG_LOG("bot chat(%s)",text.c_str());

    // ignore any messages from Addons
    if (text.empty()                                   ||
        text.find("X-Perl")      != std::wstring::npos ||
        text.find("HealBot")     != std::wstring::npos ||
        text.find("HealComm")    != std::wstring::npos ||   // "HealComm    99990094"
        text.find("LOOT_OPENED") != std::wstring::npos ||
        text.find("CTRA") != std::wstring::npos ||
		text.find("KLHTM") != std::wstring::npos ||	// KTM Threat Manager
        text.find("GathX") == 0) // Gatherer
        return;

    // if message is not from a player in the masters account auto reply and ignore
    if (!canObeyCommandFrom(fromPlayer))
    {
        // only tell the player once instead of endlessly nagging them
        if (m_ignorePlayersChat.find(fromPlayer.GetObjectGuid()) == m_ignorePlayersChat.end())
        {
        std::string msg = "I can't talk to you. Please speak to my master ";
        msg += GetMaster()->GetName();
        SendWhisper(msg, fromPlayer);
        m_bot->HandleEmoteCommand(EMOTE_ONESHOT_NO);
            m_ignorePlayersChat.insert(fromPlayer.GetObjectGuid());
    }
        return;
    }

    // Passed along to ExtractCommand, if (sub)command is found "input" will only contain the rest of the string (or "")
    std::string input = text.c_str();

    // if in the middle of a trade, and player asks for an item/money
    // WARNING: This makes it so you can't use any other commands during a trade!
    if (m_bot->GetTrader() && m_bot->GetTrader()->GetObjectGuid() == fromPlayer.GetObjectGuid())
    {
        uint32 copper = extractMoney(text);
        if (copper > 0)
            TradeCopper(copper);

        std::list<uint32> itemIds;
        extractItemIds(text, itemIds);
        if (itemIds.size() == 0)
            SendWhisper("Show me what item you want by shift clicking the item in the chat window.", fromPlayer);
        else if (!strncmp(text.c_str(), "nt ", 3))
        {
            if (itemIds.size() > 1)
                SendWhisper("There is only one 'Will not be traded' slot. Shift-click just one item, please!", fromPlayer);
            else
            {
                std::list<Item*> itemList;
                findItemsInEquip(itemIds, itemList);
                findItemsInInv(itemIds, itemList);
                if (itemList.size() > 0)
                    TradeItem((**itemList.begin()), TRADE_SLOT_NONTRADED);
                else
                    SendWhisper("I do not have this item equipped or in my bags!", fromPlayer);
            }
        }
        else
        {
            std::list<Item*> itemList;
            findItemsInInv(itemIds, itemList);
            for (std::list<Item*>::iterator it = itemList.begin(); it != itemList.end(); ++it)
                TradeItem(**it);
        }
    }

    // Handle general commands
    else if (ExtractCommand("help", input))
        _HandleCommandHelp(input, fromPlayer);

    else if (ExtractCommand("reset", input))
        _HandleCommandReset(input, fromPlayer);
    else if (ExtractCommand("report", input))
        _HandleCommandReport(input, fromPlayer);
    else if (ExtractCommand("orders", input))
        _HandleCommandOrders(input, fromPlayer);
    else if (ExtractCommand("follow", input) || ExtractCommand("come", input))
        _HandleCommandFollow(input, fromPlayer);
    else if (ExtractCommand("stay", input) || ExtractCommand("stop", input))
        _HandleCommandStay(input, fromPlayer);
    else if (ExtractCommand("attack", input))
        _HandleCommandAttack(input, fromPlayer);
    else if (ExtractCommand("pull", input))
        _HandleCommandPull(input, fromPlayer);

    else if (ExtractCommand("neutralize", input) || ExtractCommand("neutral", input))
        _HandleCommandNeutralize(input, fromPlayer);

    else if (ExtractCommand("cast", input, true)) // true -> "cast" OR "c"
        _HandleCommandCast(input, fromPlayer);

    else if (ExtractCommand("sell", input))
        _HandleCommandSell(input, fromPlayer);

    else if (ExtractCommand("repair", input))
        _HandleCommandRepair(input, fromPlayer);

    else if (ExtractCommand("auction", input))
        _HandleCommandAuction(input, fromPlayer);

    else if (ExtractCommand("bank", input))
        _HandleCommandBank(input, fromPlayer);

	else if (ExtractCommand("talent", input))
		_HandleCommandTalent(input, fromPlayer);

    else if (ExtractCommand("use", input, true)) // true -> "use" OR "u"
        _HandleCommandUse(input, fromPlayer);

    else if (ExtractCommand("equip", input, true)) // true -> "equip" OR "e"
        _HandleCommandEquip(input, fromPlayer);

	else if (ExtractCommand("gear", input, true)) // true -> "equip" OR "e"
		_HandleCommandGear(input, fromPlayer);

    // find project: 20:50 02/12/10 rev.4 item in world and wait until ordered to follow
    else if (ExtractCommand("find", input, true)) // true -> "find" OR "f"
        _HandleCommandFind(input, fromPlayer);

    // get project: 20:50 02/12/10 rev.4 compact edition, handles multiple linked gameobject & improves visuals
    else if (ExtractCommand("get", input, true)) // true -> "get" OR "g"
        _HandleCommandGet(input, fromPlayer);

    // Handle all collection related commands here
    else if (ExtractCommand("collect", input))
        _HandleCommandCollect(input, fromPlayer);

    else if (ExtractCommand("quest", input))
        _HandleCommandQuest(input, fromPlayer);

    else if (ExtractCommand("pet", input))
        _HandleCommandPet(input, fromPlayer);

    else if (ExtractCommand("spells", input))
        _HandleCommandSpells(input, fromPlayer);

    // survey project: 20:50 02/12/10 rev.4 compact edition
    else if (ExtractCommand("survey", input))
        _HandleCommandSurvey(input, fromPlayer);

    // Handle class & professions training:
    else if (ExtractCommand("skill", input))
        _HandleCommandSkill(input, fromPlayer);

    // stats project: 11:30 15/12/10 rev.2 display bot statistics
    else if (ExtractCommand("stats", input))
        _HandleCommandStats(input, fromPlayer);

    else
    {
        // if this looks like an item link, reward item it completed quest and talking to NPC
        std::list<uint32> itemIds;
        extractItemIds(text, itemIds);
        if (!itemIds.empty()) {
            uint32 itemId = itemIds.front();
            bool wasRewarded = false;
            ObjectGuid questRewarderGUID = m_bot->GetSelectionGuid();
            Object* const pNpc = (WorldObject *) m_bot->GetObjectByTypeMask(questRewarderGUID, TYPEMASK_CREATURE_OR_GAMEOBJECT);
            if (!pNpc)
                return;

            QuestMenu& questMenu = m_bot->PlayerTalkClass->GetQuestMenu();
            for (uint32 iI = 0; !wasRewarded && iI < questMenu.MenuItemCount(); ++iI)
            {
                QuestMenuItem const& qItem = questMenu.GetItem(iI);

                uint32 questID = qItem.m_qId;
                Quest const* pQuest = sObjectMgr.GetQuestTemplate(questID);
                QuestStatus status = m_bot->GetQuestStatus(questID);

                // if quest is complete, turn it in
                if (status == QUEST_STATUS_COMPLETE &&
                    !m_bot->GetQuestRewardStatus(questID) &&
                    pQuest->GetRewChoiceItemsCount() > 1 &&
                    m_bot->CanRewardQuest(pQuest, false))
                    for (uint8 rewardIdx = 0; !wasRewarded && rewardIdx < pQuest->GetRewChoiceItemsCount(); ++rewardIdx)
                    {
                        ItemPrototype const * const pRewardItem = sObjectMgr.GetItemPrototype(pQuest->RewChoiceItemId[rewardIdx]);
                        if (itemId == pRewardItem->ItemId)
                        {
                            m_bot->RewardQuest(pQuest, rewardIdx, pNpc, false);

                            std::string questTitle  = pQuest->GetTitle();
                            m_bot->GetPlayerbotAI()->QuestLocalization(questTitle, questID);
                            std::string itemName = pRewardItem->Name1;
                            m_bot->GetPlayerbotAI()->ItemLocalization(itemName, pRewardItem->ItemId);

                            std::ostringstream out;
                            out << "|cffffffff|Hitem:" << pRewardItem->ItemId << ":0:0:0:0:0:0:0" << "|h[" << itemName << "]|h|r rewarded";
                            SendWhisper(out.str(), fromPlayer);
                            wasRewarded = true;
                        }
                    }
            }

        }
        else
        {
            // TODO: make this only in response to direct whispers (chatting in party chat can in fact be between humans)
            std::string msg = "What? For a list of commands, ask for 'help'.";
            SendWhisper(msg, fromPlayer);
            m_bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
        }
    }
}

/**
* ExtractCommand looks for a command in a text string
* sLookingFor       - string you're looking for (e.g. "help")
* text              - string which may or may not start with sLookingFor
* bUseShort         - does this command accept the shorthand command? If true, "help" would ALSO look for "h"
*
* returns true if the string has been found
* returns false if the string has not been found
*/
bool PlayerbotAI::ExtractCommand(const std::string sLookingFor, std::string &text, bool bUseShort)
{
    // ("help" + " ") < "help X"  AND  text's start (as big as sLookingFor) == sLookingFor
    // Recommend AGAINST adapting this for non-space situations (thinking MangosZero)
    // - unknown would risk being (short for "use") 'u' + "nknown"
    if (sLookingFor.size() + 1 < text.size() && text.at(sLookingFor.size()) == ' '
        && 0 == text.substr(0, sLookingFor.size()).compare(sLookingFor))
    {
        text = text.substr(sLookingFor.size()+1);
        return true;
    }

    if (0 == text.compare(sLookingFor))
    {
        text = "";
        return true;
    }

    if (bUseShort)
    {
        if (text.size() > 1 && sLookingFor.at(0) == text.at(0) && text.at(1) == ' ')
        {
            text = text.substr(2);
            return true;
        }
        else if(text.size() == 1 && sLookingFor.at(0) == text.at(0))
        {
            text = "";
            return true;
        }
    }

    return false;
}

void PlayerbotAI::_HandleCommandReset(std::string &text, Player &fromPlayer)
{
    if (text != "")
    {
        SendWhisper("reset does not have a subcommand.", fromPlayer);
        return;
    }
        SetState(BOTSTATE_NORMAL);
        MovementReset();
        SetQuestNeedItems();
        SetQuestNeedCreatures();
        UpdateAttackerInfo();
        m_lootTargets.clear();
        m_lootCurrent = ObjectGuid();
        m_targetCombat = 0;
    }

void PlayerbotAI::_HandleCommandReport(std::string &text, Player &fromPlayer)
{
    if (text != "")
    {
        SendWhisper("report cannot have a subcommand.", fromPlayer);
        return;
    }
        SendQuestNeedList();
}

void PlayerbotAI::_HandleCommandOrders(std::string &text, Player &fromPlayer)
{
    if (ExtractCommand("delay", text))
    {
        uint32 gdelay;
        sscanf(text.c_str(), "%d", &gdelay);
        if (gdelay <= 10)
        {
            m_DelayAttack = gdelay;
            TellMaster("Combat delay is now '%u' ", m_DelayAttack);
            CharacterDatabase.DirectPExecute("UPDATE playerbot_saved_data SET combat_delay = '%u' WHERE guid = '%u'", m_DelayAttack, m_bot->GetGUIDLow());
            return;
        }
        else
            SendWhisper("Invalid delay. choose a number between 0 and 10", fromPlayer);
        return;
    }
    else if (ExtractCommand("resume", text))
        CombatOrderRestore();
    else if (ExtractCommand("resume", text))
        CombatOrderRestore();
    else if (ExtractCommand("combat", text, true))
    {
        Unit *target = nullptr;

        QueryResult *resultlvl = CharacterDatabase.PQuery("SELECT guid FROM playerbot_saved_data WHERE guid = '%u'", m_bot->GetObjectGuid().GetCounter());
        if (!resultlvl)
            CharacterDatabase.DirectPExecute("INSERT INTO playerbot_saved_data (guid,combat_order,primary_target,secondary_target,pname,sname,combat_delay,active_spec) VALUES ('%u',0,0,0,'','',0,0)", m_bot->GetObjectGuid().GetCounter());
        else
            delete resultlvl;

        size_t protect = text.find("protect");
        size_t assist = text.find("assist");


        if (text == "")
        {
            SendWhisper("|cffff0000Syntax error:|cffffffff orders combat <botName> <reset | tank | heal | passive><assist | protect [targetPlayer]>", fromPlayer);
            return;
        }

        if (ExtractCommand("protect", text) || ExtractCommand("assist", text))
        {
            ObjectGuid targetGUID = fromPlayer.GetSelectionGuid();
            if (text == "" && !targetGUID)
            {
                SendWhisper("|cffff0000Combat orders protect and assist expect a target either by selection or by giving target player in command string!", fromPlayer);
                return;
            }

            if (text != "")
            {
                ObjectGuid targ_guid = sObjectMgr.GetPlayerGuidByName(text.c_str());
                targetGUID.Set(targ_guid.GetRawValue());
            }
            target = ObjectAccessor::GetUnit(fromPlayer, targetGUID);
            if (!target)
                return SendWhisper("|cffff0000Invalid target for combat order protect or assist!", fromPlayer);

            if (protect != std::string::npos)
                SetCombatOrder(ORDERS_PROTECT, target);
            else if (assist != std::string::npos)
                SetCombatOrder(ORDERS_ASSIST, target);
        }
        else
            SetCombatOrderByStr(text, target);
    }
    else if (text != "")
    {
        SendWhisper("See help for details on using 'orders'.", fromPlayer);
        return;
    }
        SendOrders(*GetMaster());
}

void PlayerbotAI::_HandleCommandFollow(std::string &text, Player &fromPlayer)
{
    if (text != "")
    {
        SendWhisper("follow cannot have a subcommand.", fromPlayer);
        return;
    }
        SetMovementOrder(MOVEMENT_FOLLOW, GetMaster());
}

void PlayerbotAI::_HandleCommandStay(std::string &text, Player &fromPlayer)
{
    if (text != "")
    {
        SendWhisper("stay cannot have a subcommand.", fromPlayer);
        return;
    }
        SetMovementOrder(MOVEMENT_STAY);
}

void PlayerbotAI::_HandleCommandAttack(std::string &text, Player &fromPlayer)
{
    if (text != "")
    {
        SendWhisper("attack cannot have a subcommand.", fromPlayer);
        return;
    }
        ObjectGuid attackOnGuid = fromPlayer.GetSelectionGuid();
        if (attackOnGuid)
        {
        if (Unit* thingToAttack = ObjectAccessor::GetUnit(*m_bot, attackOnGuid))
        {
            if (!m_bot->IsFriendlyTo(thingToAttack))
            {
				if (!m_bot->IsWithinLOSInMap(thingToAttack))
				{
					TellMaster("Trying to attack something, but I'm too far away!");
					DoTeleport(*m_followTarget);
				}
                if (m_bot->IsWithinLOSInMap(thingToAttack))
                    Attack(thingToAttack);
            }
        }
        }
        else
        {
        SendWhisper("No target is selected.", fromPlayer);
            m_bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
        }
}

void PlayerbotAI::_HandleCommandPull(std::string &text, Player &fromPlayer)
{
    bool bReadyCheck = false;

    if (!m_bot) return;

    if (ExtractCommand("test", text)) // switch to automatic follow distance
    {
        if (CanPull(fromPlayer))
            SendWhisper("Looks like I am capable of pulling. Ask me 'pull ready' with a target for a more precise check.", fromPlayer);
        return;
    }
    if (ExtractCommand("ready", text)) // switch to automatic follow distance
    {
        bReadyCheck = true;
    }
    else if (text != "")
    {
        SendWhisper("See 'help pull' for details on using the pull command.", fromPlayer);
        return;
    }

    // This function also takes care of error reporting
    if (!CanPull(fromPlayer))
        return;

    // Check for valid target
    m_bot->SetSelectionGuid(fromPlayer.GetSelectionGuid());
    ObjectGuid attackOnGuid = m_bot->GetSelectionGuid();
    if (!attackOnGuid)
    {
        SendWhisper("No target is selected.", fromPlayer);
        return;
    }

    Unit* thingToAttack = ObjectAccessor::GetUnit(*m_bot, attackOnGuid);
    if (!thingToAttack)
    {
        SendWhisper("No target is selected.", fromPlayer);
        return;
    }

    if (m_bot->IsFriendlyTo(thingToAttack))
    {
        SendWhisper("Where I come from we don't attack our friends.", fromPlayer);
        return;
    }
    // TODO: Okay, this one should actually be fixable. InMap should return, but LOS (Line of Sight) should result in moving, well, into LoS.
    if (!m_bot->IsWithinLOSInMap(thingToAttack))
    {
        SendWhisper("I can't see that target!", fromPlayer);
        return;
    }
    GetCombatTarget(thingToAttack);
    if (!GetCurrentTarget())
    {
        SendWhisper("Failed to set target, cause unknown.", fromPlayer);
        return;
    }
    if (bReadyCheck)
    {
        SendWhisper("All checks have been passed and I am ready to pull! ... Are you sure you wouldn't like a smaller target?", fromPlayer);
        return;
    }

    // All healers which have it available will cast any applicable HoT (Heal over Time) spell on the tank
    GroupHoTOnTank();

    /* Technically the tank should wait a bit if/until the HoT has been applied
       but the above function immediately casts it rather than wait for an UpdateAI tick

       There is no need to take into account that GroupHoTOnTank() may fail due to global cooldown. Either you're prepared for a difficult
       pull in which case it won't fail due to global cooldown, or you're chaining easy pulls in which case you don't care.
       */
    /* So have the group wait for the tank to take action (and aggro) - this way it will be easy to see if tank has aggro or not without having to
       worry about tank not being the first to have UpdateAI() called
       */

    // Need to have a group and a tank, both checked in "CanPull()" call above
    //if (!(GetGroupTank()->GetPlayerbotAI()->GetClassAI()->Pull()))
    // I've been told to pull and a check was done above whether I'm actually a tank, so *I* will try to pull:
    if (!CastPull())
    {
        SendWhisper("I did my best but I can't actually pull. How odd.", fromPlayer);
        return;
    }

    // Sets Combat Orders to PULL
    SetGroupCombatOrder(ORDERS_TEMP_WAIT_TANKAGGRO);

    SetGroupIgnoreUpdateTime(1);

    // Set all group members (save this tank) to wait 10 seconds. They will wait until the tank says so, until any non-tank gains aggro or 10 seconds - whichever is shortest
    if (m_bot->GetGroup()) // one last sanity check, should be unnecessary
    {
        Group::MemberSlotList const& groupSlot = m_bot->GetGroup()->GetMemberSlots();
        for (Group::member_citerator itr = groupSlot.begin(); itr != groupSlot.end(); itr++)
    {
            Player* groupMember = sObjectMgr.GetPlayer(itr->guid);
            if (!groupMember || !groupMember->GetPlayerbotAI() || groupMember == m_bot)
                continue;
            groupMember->GetPlayerbotAI()->GetClassAI()->SetWait(10);
        }
    }

    //(4a) if tank, deactivate any attack (such as 'shoot (bow/gun)' for warriors), wait until in melee range, attack
    //(4b) if dps, wait until the target is in melee range of the tank +2seconds or until tank no longer holds aggro
    //(4c) if healer, do healing checks
    //(5) when target is in melee range of tank, wait 2 seconds (healers continue to do group heal checks, all do self-heal checks), then return to normal functioning
}

void PlayerbotAI::_HandleCommandNeutralize(std::string &text, Player &fromPlayer)
{
    if (!m_bot) return;

    if (text != "")
    {
        SendWhisper("See 'help neutralize' for details on using the neutralize command.", fromPlayer);
        return;
    }

    // Check for valid target
    m_bot->SetSelectionGuid(fromPlayer.GetSelectionGuid());
    ObjectGuid selectOnGuid = m_bot->GetSelectionGuid();
    if (!selectOnGuid)
    {
        SendWhisper("No target is selected.", fromPlayer);
        return;
    }

    Unit* thingToNeutralize = ObjectAccessor::GetUnit(*m_bot, selectOnGuid);
    if (!thingToNeutralize)
    {
        SendWhisper("No valid target is selected.", fromPlayer);
        return;
    }

    if (m_bot->IsFriendlyTo(thingToNeutralize))
    {
        SendWhisper("I can't neutralize that target: this is a friend to me.", fromPlayer);
        return;
    }

    if (!m_bot->IsWithinLOSInMap(thingToNeutralize))
    {
        SendWhisper("I can't see that target!", fromPlayer);
        return;
    }
    
    if (IsNeutralized(thingToNeutralize))
    {
        SendWhisper("Target is already neutralized.", fromPlayer);
        return;
    }

    m_targetGuidCommand = selectOnGuid;

    // All checks passed: call the Neutralize function of each bot class
    // to define what spellid to use if available and if creature type is correct
    // m_spellIdCommand will be defined there and UpdateAI will then handle the cast
    if (!CastNeutralize())
    {
        SendWhisper("Something went wrong: I can't neutralize that target.", fromPlayer);
        return;
    }
}

void PlayerbotAI::_HandleCommandCast(std::string &text, Player &fromPlayer)
{
    if (text == "")
    {
        SendWhisper("cast must be used with a single spell link (shift + click the spell).", fromPlayer);
        return;
    }

    std::string spellStr = text;
        uint32 spellId = (uint32) atol(spellStr.c_str());

        // try and get spell ID by name
        if (spellId == 0)
        {
            spellId = getSpellId(spellStr.c_str(), true);

            // try link if text NOT (spellid OR spellname)
            if (spellId == 0)
            extractSpellId(text, spellId);
        }

        if (m_bot->HasAura(spellId))
        {
            m_bot->RemoveAurasByCasterSpell(spellId, m_bot->GetObjectGuid());
            return;
        }

        ObjectGuid castOnGuid = fromPlayer.GetSelectionGuid();
        if (spellId != 0 && castOnGuid && m_bot->HasSpell(spellId))
        {
            m_spellIdCommand = spellId;
            m_targetGuidCommand = castOnGuid;
        }
}

// _HandleCommandSell: Handle selling items
    // sell [Item Link][Item Link] .. -- Sells bot(s) items from inventory
void PlayerbotAI::_HandleCommandSell(std::string &text, Player &fromPlayer)
{

    if (text == "")
    {
        SendWhisper("sell must be used with one or more item links (shift + click the item).", fromPlayer);
        return;
    }
/*    enum NPCFlags VENDOR_MASK = (enum NPCFlags) (UNIT_NPC_FLAG_VENDOR
                                                    | UNIT_NPC_FLAG_VENDOR_AMMO
                                                    | UNIT_NPC_FLAG_VENDOR_FOOD
                                                    | UNIT_NPC_FLAG_VENDOR_POISON
                                                    | UNIT_NPC_FLAG_VENDOR_REAGENT);
*/
        std::list<uint32> itemIds;
        extractItemIds(text, itemIds);
    for (std::list<uint32>::iterator it = itemIds.begin(); it != itemIds.end(); ++it)
        m_tasks.push_back(std::pair<enum TaskFlags,uint32>(SELL, *it));
        m_findNPC.push_back(UNIT_NPC_FLAG_VENDOR);
    }


// _HandleCommandRepair: Handle repair items
    // repair  all                      -- repair all bot(s) items
    // repair [Item Link][Item Link] .. -- repair select bot(s) items
void PlayerbotAI::_HandleCommandRepair(std::string &text, Player &fromPlayer)
{
    if (ExtractCommand("all", text))
    {
        if (text != "")
    {
            SendWhisper("Invalid subcommand for 'repair all'", fromPlayer);
            return;
        }
        m_tasks.push_back(std::pair<enum TaskFlags,uint32>(REPAIR, 0));
        m_findNPC.push_back(UNIT_NPC_FLAG_REPAIR);
        return;
    }

        std::list<uint32> itemIds;
    extractItemIds(text, itemIds);

        for (std::list<uint32>::iterator it = itemIds.begin(); it != itemIds.end(); it++)
    {
        m_tasks.push_back(std::pair<enum TaskFlags,uint32>(REPAIR, *it));
        m_findNPC.push_back(UNIT_NPC_FLAG_REPAIR);
    }
}


// _HandleCommandAuction: Handle auctions:
    // auction                                        -- Lists bot(s) active auctions.
    // auction add [Item Link][Item Link] ..          -- Create bot(s) active auction.
    // auction remove [Auction Link][Auction Link] .. -- Cancel bot(s) active auction. ([Auction Link] from auction)
void PlayerbotAI::_HandleCommandAuction(std::string &text, Player &fromPlayer)
    {
    if (text == "")
        {
        m_findNPC.push_back(UNIT_NPC_FLAG_AUCTIONEER); // list all bot auctions
        }
    else if (ExtractCommand("add",text))
        {
            std::list<uint32> itemIds;
        extractItemIds(text, itemIds);
        for (std::list<uint32>::iterator it = itemIds.begin(); it != itemIds.end(); ++it)
            m_tasks.push_back(std::pair<enum TaskFlags,uint32>(ADD, *it));
            m_findNPC.push_back(UNIT_NPC_FLAG_AUCTIONEER);
        }
    else if (ExtractCommand("remove",text))
        {
            std::list<uint32> auctionIds;
        extractAuctionIds(text, auctionIds);
        for (std::list<uint32>::iterator it = auctionIds.begin(); it != auctionIds.end(); ++it)
            m_tasks.push_back(std::pair<enum TaskFlags,uint32>(REMOVE, *it));
            m_findNPC.push_back(UNIT_NPC_FLAG_AUCTIONEER);
        }
    else
    {
        SendWhisper("I don't understand what you're trying to do", fromPlayer);
    }
    }

// _HandleCommandBank: Handle bank:
    // bank                                        -- Lists bot(s) bank balance.
    // bank deposit [Item Link][Item Link] ..      -- Deposit item(s) in bank.
    // bank withdraw [Item Link][Item Link] ..     -- Withdraw item(s) from bank. ([Item Link] from bank)
void PlayerbotAI::_HandleCommandBank(std::string &text, Player &fromPlayer)
    {
    if (text == "")
        {
        m_findNPC.push_back(UNIT_NPC_FLAG_BANKER); // list all bot balance
        }
    else if (ExtractCommand("deposit", text))
        {
            std::list<uint32> itemIds;
        extractItemIds(text, itemIds);
        for (std::list<uint32>::iterator it = itemIds.begin(); it != itemIds.end(); ++it)
            m_tasks.push_back(std::pair<enum TaskFlags,uint32>(DEPOSIT, *it));
            m_findNPC.push_back(UNIT_NPC_FLAG_BANKER);
        }
    else if (ExtractCommand("withdraw", text))
        {
            std::list<uint32> itemIds;
        extractItemIds(text, itemIds);
        for (std::list<uint32>::iterator it = itemIds.begin(); it != itemIds.end(); ++it)
            m_tasks.push_back(std::pair<enum TaskFlags,uint32>(WITHDRAW, *it));
            m_findNPC.push_back(UNIT_NPC_FLAG_BANKER);
    }
    else
    {
        SendWhisper("I don't understand what you're trying to do", fromPlayer);
    }
}

void PlayerbotAI::_HandleCommandUse(std::string &text, Player &fromPlayer)
{
        std::list<uint32> itemIds;
        std::list<Item*> itemList;
    extractItemIds(text, itemIds);
        findItemsInInv(itemIds, itemList);
        // set target
        Unit* unit = ObjectAccessor::GetUnit(*m_bot, fromPlayer.GetSelectionGuid());

        for (std::list<Item*>::iterator it = itemList.begin(); it != itemList.end(); ++it)
        {
            if (unit)
                UseItem(*it, unit);
            else
                UseItem(*it);
        }
    }

void PlayerbotAI::_HandleCommandEquip(std::string &text, Player& /*fromPlayer*/)
    {
        std::list<uint32> itemIds;
        std::list<Item*> itemList;
    extractItemIds(text, itemIds);
        findItemsInInv(itemIds, itemList);

			for (std::list<Item*>::iterator it = itemList.begin(); it != itemList.end(); ++it)
				EquipItem(*it);
        
        SendNotEquipList(*m_bot);
    }

/// new command for gear checks. issued in group or raid and bots will link to you the items they would upgrade.
/// COMMAND: gear check [linked_item]
void PlayerbotAI::_HandleCommandGear(std::string &text, Player& /*fromPlayer*/)
{
	if (ExtractCommand("check", text))
	{
		std::list<uint32> itemIds;
		extractItemIds(text, itemIds);
		// If no items were linked return
		if (itemIds.size() == 0)
			return;
		// If more than one item was found, tell master to send only one so we know which one they are asking about
		if (itemIds.size() > 1) {
			TellMaster("Please link only one item for a gear check.");
			return;
		}

		bool created = false;
		for (std::list<uint32>::iterator it = itemIds.begin(); it != itemIds.end(); ++it) {	

			// Get the actual item from the masters inventory. We can't use a prototype because they don't have the
			// random properties that may be assigned.
			Item* pItem = GetMaster()->FindItem(*it);
			if (!pItem) {

				// Get the item from the loot list
				Loot* loot = sLootMgr.GetLoot(GetMaster(), ObjectGuid());
				if (!loot)
				{
					sLog.outError("PLAYERBOT Debug Error cannot get loot object info in Gear Check command!");
					return;
				}

				// Get the list of items first and iterate it
				LootItemList lootList;
				loot->GetLootItemsListFor(GetMaster(), lootList);

				for (LootItemList::const_iterator lootItr = lootList.begin(); lootItr != lootList.end(); ++lootItr)
				{
					LootItem* lootItem = *lootItr;

					// If this is the item then create it
					if (lootItem->itemProto->ItemId == *it) {
						// Create an item from the loot item
						pItem = Item::CreateItem(*it, 1, GetMaster(), lootItem->randomPropertyId);

						// set to created so we can clean it up
						created = true;

						break;
					}
				}

				// couldn't find the item
				if (!pItem)
				{
					TellMaster("Can't find item in inventory or loot list!");
					return;
				}
			}
				
			// Check if item is an upgrade
			if (IsItemAnUpgrade(pItem)){
				SendUpgradingItems(pItem->GetProto());
				if (created) delete pItem;
				return;
			}

			if (created) delete pItem;
		}
        
		// Not an upgrade - don't say anything
		TellMaster("Item is not an upgrade for me.");
	}
}

void PlayerbotAI::_HandleCommandFind(std::string &text, Player& /*fromPlayer*/)
{
    extractGOinfo(text, m_lootTargets);

    m_lootCurrent = m_lootTargets.front();
    m_lootTargets.pop_front();

    GameObject *go = m_bot->GetMap()->GetGameObject(m_lootCurrent);
    if (!go)
    {
        m_lootTargets.clear();
        m_lootCurrent = ObjectGuid();
        return;
    }

    SetMovementOrder(MOVEMENT_STAY);
    m_bot->GetMotionMaster()->MovePoint(go->GetMapId(), go->GetPositionX(), go->GetPositionY(), go->GetPositionZ());
    m_lootTargets.clear();
    m_lootCurrent = ObjectGuid();
}
 
void PlayerbotAI::_HandleCommandGet(std::string &text, Player &fromPlayer)
    {
    if (text != "")
        {
            extractGOinfo(text, m_lootTargets);
            SetState(BOTSTATE_LOOTING);
        return;
        }

    // get a selected lootable corpse
            ObjectGuid getOnGuid = fromPlayer.GetSelectionGuid();
            if (getOnGuid)
            {
                Creature *c = m_bot->GetMap()->GetCreature(getOnGuid);
                if (!c)
                    return;

                uint32 skillId = 0;
                if (c->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE))
                    skillId = c->GetCreatureInfo()->GetRequiredLootSkill();

                if (c->HasFlag(UNIT_DYNAMIC_FLAGS, UNIT_DYNFLAG_LOOTABLE) ||
                   (c->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SKINNABLE) && m_bot->HasSkill(skillId)))
                {
                    m_lootTargets.push_back(getOnGuid);
                    SetState(BOTSTATE_LOOTING);
                }
                else
            SendWhisper("Target is not lootable by me.", fromPlayer);
            }
            else
            {
        SendWhisper("No target is selected.", fromPlayer);
                m_bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
            }
    }

void PlayerbotAI::_HandleCommandCollect(std::string &text, Player &fromPlayer)
        {
    while (text.size() > 0)
            {
        if (ExtractCommand("all", text))
            {
                SetCollectFlag(COLLECT_FLAG_COMBAT);
                SetCollectFlag(COLLECT_FLAG_LOOT);
                SetCollectFlag(COLLECT_FLAG_QUEST);
                SetCollectFlag(COLLECT_FLAG_PROFESSION);
                SetCollectFlag(COLLECT_FLAG_NEAROBJECT);
                if (m_bot->HasSkill(SKILL_SKINNING))
                    SetCollectFlag(COLLECT_FLAG_SKIN);
            }
        if (ExtractCommand("combat", text))
                SetCollectFlag(COLLECT_FLAG_COMBAT);
        else if (ExtractCommand("loot", text))
                SetCollectFlag(COLLECT_FLAG_LOOT);
        else if (ExtractCommand("quest", text))
                SetCollectFlag(COLLECT_FLAG_QUEST);
        else if (ExtractCommand("profession", text) || ExtractCommand("skill", text))
                SetCollectFlag(COLLECT_FLAG_PROFESSION);
        else if (ExtractCommand("skin", text) && m_bot->HasSkill(SKILL_SKINNING)) // removes skin even if bot does not have skill
                SetCollectFlag(COLLECT_FLAG_SKIN);
        else if (ExtractCommand("objects", text) || ExtractCommand("nearby", text))
            {
                SetCollectFlag(COLLECT_FLAG_NEAROBJECT);
                if (!HasCollectFlag(COLLECT_FLAG_NEAROBJECT))
                    m_collectObjects.clear();
            }
        else if (ExtractCommand("none", text) || ExtractCommand("nothing", text))
            {
                m_collectionFlags = 0;
                m_collectObjects.clear();
            break;  // because none is an exclusive choice
            }
            else
            {
                std::string collout = "";
                if (m_bot->HasSkill(SKILL_SKINNING))
                    collout += ", skin";
                // TODO: perhaps change the command syntax, this way may be lacking in ease of use
            SendWhisper("Collect <collectable(s)>: none, combat, loot, quest, profession, objects" + collout, fromPlayer);
                break;
            }
        }

        std::string collset = "";
        if (HasCollectFlag(COLLECT_FLAG_LOOT))
            collset += ", all loot";
        if (HasCollectFlag(COLLECT_FLAG_PROFESSION))
            collset += ", profession";
        if (HasCollectFlag(COLLECT_FLAG_QUEST))
            collset += ", quest";
        if (HasCollectFlag(COLLECT_FLAG_SKIN))
            collset += ", skin";
        if (collset.length() > 1)
        {
            if (HasCollectFlag(COLLECT_FLAG_COMBAT))
                collset += " items after combat";
            else
                collset += " items";
        }

        if (HasCollectFlag(COLLECT_FLAG_NEAROBJECT))
        {
            if (collset.length() > 1)
                collset += " and ";
            else
            collset += " ";    // padding for substr
            collset += "nearby objects (";
            if (!m_collectObjects.empty())
            {
                std::string strobjects = "";
            for (BotEntryList::iterator itr = m_collectObjects.begin(); itr != m_collectObjects.end(); ++itr)
                {
                    uint32 objectentry = *(itr);
                    GameObjectInfo const * gInfo = ObjectMgr::GetGameObjectInfo(objectentry);
                    strobjects += ", ";
                    strobjects += gInfo->name;
                }
                collset += strobjects.substr(2);
            }
            else
                collset += "use survey and get to set";
            collset += ")";
        }

        if (collset.length() > 1)
        SendWhisper("I'm collecting " + collset.substr(2), fromPlayer);
        else
        SendWhisper("I'm collecting nothing.", fromPlayer);
    }

void PlayerbotAI::_HandleCommandQuest(std::string &text, Player &fromPlayer)
    {
        std::ostringstream msg;

    if (ExtractCommand("add", text, true)) // true -> "quest add" OR "quest a"
        {
            std::list<uint32> questIds;
        extractQuestIds(text, questIds);
            for (std::list<uint32>::iterator it = questIds.begin(); it != questIds.end(); it++)
            {
            m_tasks.push_back(std::pair<enum TaskFlags, uint32>(TAKE, *it));
                DEBUG_LOG(" questid (%u)",*it);
            }
            m_findNPC.push_back(UNIT_NPC_FLAG_QUESTGIVER);
        }
    else if (ExtractCommand("drop", text, true)) // true -> "quest drop" OR "quest d"
        {
            fromPlayer.SetSelectionGuid(m_bot->GetObjectGuid());
            PlayerbotChatHandler ch(GetMaster());
        int8 linkStart = text.find("|");
        if (text.find("|") != std::string::npos)
            if (!ch.dropQuest((char *) text.substr(linkStart).c_str()))
                    ch.sysmessage("ERROR: could not drop quest");
                else
                {
                    SetQuestNeedItems();
                    SetQuestNeedCreatures();
                }
            }
    else if (ExtractCommand("fetch", text, true)) // true -> "quest fetch"
    {
        gQuestFetch = 1;
        m_tasks.push_back(std::pair<enum TaskFlags, uint32>(LIST, 0));
        m_findNPC.push_back(UNIT_NPC_FLAG_QUESTGIVER);
        }
    else if (ExtractCommand("list", text, true)) // true -> "quest list" OR "quest l"
        {
            m_tasks.push_back(std::pair<enum TaskFlags, uint32>(LIST, 0));
            m_findNPC.push_back(UNIT_NPC_FLAG_QUESTGIVER);
        }
    else if (ExtractCommand("end", text, true)) // true -> "quest end" OR "quest e"
        {
            m_tasks.push_back(std::pair<enum TaskFlags, uint32>(END, 0));
            m_findNPC.push_back(UNIT_NPC_FLAG_QUESTGIVER);
        }
        else
        {
            bool hasIncompleteQuests = false;
            std::ostringstream incomout;
            incomout << "my incomplete quests are:";
            bool hasCompleteQuests = false;
            std::ostringstream comout;
            comout << "my complete quests are:";
            for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
            {
                if (uint32 questId = m_bot->GetQuestSlotQuestId(slot))
                {
                    Quest const* pQuest = sObjectMgr.GetQuestTemplate(questId);

                    std::string questTitle  = pQuest->GetTitle();
                    m_bot->GetPlayerbotAI()->QuestLocalization(questTitle, questId);

                    if (m_bot->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
                    {
                        hasCompleteQuests = true;
                        comout << " |cFFFFFF00|Hquest:" << questId << ':' << pQuest->GetQuestLevel() << "|h[" << questTitle << "]|h|r";
                    }
                    else
                    {
                        Item* qitem = FindItem(pQuest->GetSrcItemId());
                        if (qitem)
                            incomout << " use " << "|cffffffff|Hitem:" << qitem->GetProto()->ItemId << ":0:0:0:0:0:0:0" << "|h[" << qitem->GetProto()->Name1 << "]|h|r" << " on ";
                        hasIncompleteQuests = true;
                        incomout << " |cFFFFFF00|Hquest:" << questId << ':' << pQuest->GetQuestLevel() << "|h[" <<  questTitle << "]|h|r";
                    }
                }
            }
            if (hasCompleteQuests)
                SendWhisper(comout.str(), fromPlayer);
            if (hasIncompleteQuests)
                SendWhisper(incomout.str(), fromPlayer);
            if (!hasCompleteQuests && !hasIncompleteQuests)
            SendWhisper("I have no quests.", fromPlayer);
        }
    }

void PlayerbotAI::_HandleCommandPet(std::string &text, Player &fromPlayer)
    {
        Pet * pet = m_bot->GetPet();
        if (!pet)
        {
            SendWhisper("I have no pet.", fromPlayer);
            return;
        }

    if (ExtractCommand("react", text))
        {
        if (ExtractCommand("aggressive", text, true))
                pet->GetCharmInfo()->SetReactState(REACT_AGGRESSIVE);
        else if (ExtractCommand("defensive", text, true))
                pet->GetCharmInfo()->SetReactState(REACT_DEFENSIVE);
        else if (ExtractCommand("passive", text, true))
                pet->GetCharmInfo()->SetReactState(REACT_PASSIVE);
        else
            _HandleCommandHelp("pet react", fromPlayer);
        }
    else if (ExtractCommand("state", text))
    {
        if (text != "")
        {
            SendWhisper("'pet state' does not support subcommands.", fromPlayer);
            return;
        }

            std::string state;
            switch (pet->GetCharmInfo()->GetReactState())
            {
                case REACT_AGGRESSIVE:
                    SendWhisper("My pet is aggressive.", fromPlayer);
                    break;
                case REACT_DEFENSIVE:
                    SendWhisper("My pet is defensive.", fromPlayer);
                    break;
                case REACT_PASSIVE:
                    SendWhisper("My pet is passive.", fromPlayer);
            }
        }
    else if (ExtractCommand("cast", text))
    {
        if (text == "")
        {
            _HandleCommandHelp("pet cast", fromPlayer);
            return;
        }

        uint32 spellId = (uint32) atol(text.c_str());

            if (spellId == 0)
            {
            spellId = getPetSpellId(text.c_str());
                if (spellId == 0)
                extractSpellId(text, spellId);
            }

            if (spellId != 0 && pet->HasSpell(spellId))
            {
                if (pet->HasAura(spellId))
                {
                    pet->RemoveAurasByCasterSpell(spellId, pet->GetObjectGuid());
                    return;
                }

                ObjectGuid castOnGuid = fromPlayer.GetSelectionGuid();
                Unit* pTarget = ObjectAccessor::GetUnit(*m_bot, castOnGuid);
                CastPetSpell(spellId, pTarget);
            }
        }
    else if (ExtractCommand("toggle", text))
    {
        if (text == "")
        {
            _HandleCommandHelp("pet toggle", fromPlayer);
            return;
        }

        uint32 spellId = (uint32) atol(text.c_str());

            if (spellId == 0)
            {
            spellId = getPetSpellId(text.c_str());
                if (spellId == 0)
                extractSpellId(text, spellId);
            }

            if (spellId != 0 && pet->HasSpell(spellId))
            {
                PetSpellMap::iterator itr = pet->m_spells.find(spellId);
                if (itr != pet->m_spells.end())
                {
                    if (itr->second.active == ACT_ENABLED)
                    {
                        pet->ToggleAutocast(spellId, false);
                        if (pet->HasAura(spellId))
                            pet->RemoveAurasByCasterSpell(spellId, pet->GetObjectGuid());
                    }
                    else
                        pet->ToggleAutocast(spellId, true);
                }
            }
        }
    else if (ExtractCommand("spells", text))
    {
        if (text != "")
        {
            SendWhisper("'pet spells' does not support subcommands.", fromPlayer);
            return;
        }

            int loc = GetMaster()->GetSession()->GetSessionDbcLocale();

            std::ostringstream posOut;
            std::ostringstream negOut;

            for (PetSpellMap::iterator itr = pet->m_spells.begin(); itr != pet->m_spells.end(); ++itr)
            {
                const uint32 spellId = itr->first;

                if (itr->second.state == PETSPELL_REMOVED || IsPassiveSpell(spellId))
                    continue;

                const SpellEntry* const pSpellInfo = sSpellStore.LookupEntry(spellId);
                if (!pSpellInfo)
                    continue;

                std::string color;
                switch (itr->second.active)
                {
                    case ACT_ENABLED:
                        color = "cff35d22d"; // Some flavor of green
                        break;
                    default:
                        color = "cffffffff";
                }

                if (IsPositiveSpell(spellId))
                    posOut << " |" << color << "|Hspell:" << spellId << "|h["
                           << pSpellInfo->SpellName[loc] << "]|h|r";
                else
                    negOut << " |" << color << "|Hspell:" << spellId << "|h["
                           << pSpellInfo->SpellName[loc] << "]|h|r";
            }

            ChatHandler ch(&fromPlayer);
            SendWhisper("Here's my pet's non-attack spells:", fromPlayer);
            ch.SendSysMessage(posOut.str().c_str());
            SendWhisper("and here's my pet's attack spells:", fromPlayer);
            ch.SendSysMessage(negOut.str().c_str());
        }
    }

void PlayerbotAI::_HandleCommandSpells(std::string& /*text*/, Player &fromPlayer)
    {
        int loc = GetMaster()->GetSession()->GetSessionDbcLocale();

        std::ostringstream posOut;
        std::ostringstream negOut;

        typedef std::map<std::string, uint32> spellMap;
        spellMap posSpells, negSpells;
        std::string spellName;

        uint32 ignoredSpells[] = {1843, 5019, 2479, 6603, 3365, 8386, 21651, 21652, 6233, 6246, 6247,
                                  61437, 22810, 22027, 45927, 7266, 7267, 6477, 6478, 7355, 68398};
        uint32 ignoredSpellsCount = sizeof(ignoredSpells) / sizeof(uint32);

        for (PlayerSpellMap::iterator itr = m_bot->GetSpellMap().begin(); itr != m_bot->GetSpellMap().end(); ++itr)
        {
            const uint32 spellId = itr->first;

            if (itr->second.state == PLAYERSPELL_REMOVED || itr->second.disabled || IsPassiveSpell(spellId))
                continue;

            const SpellEntry* const pSpellInfo = sSpellStore.LookupEntry(spellId);
            if (!pSpellInfo)
                continue;

            spellName = pSpellInfo->SpellName[loc];

            SkillLineAbilityMapBounds const bounds = sSpellMgr.GetSkillLineAbilityMapBounds(spellId);

            bool isProfessionOrRidingSpell = false;
            for (SkillLineAbilityMap::const_iterator skillIter = bounds.first; skillIter != bounds.second; ++skillIter)
            {
                if (IsProfessionOrRidingSkill(skillIter->second->skillId) && skillIter->first == spellId) {
                    isProfessionOrRidingSpell = true;
                    break;
                }
            }
            if (isProfessionOrRidingSpell)
                continue;

            bool isIgnoredSpell = false;
            for (uint8 i = 0; i < ignoredSpellsCount; ++i)
            {
                if (spellId == ignoredSpells[i]) {
                    isIgnoredSpell = true;
                    break;
                }
            }
            if (isIgnoredSpell)
                continue;

            if (IsPositiveSpell(spellId)) {
                if (posSpells.find(spellName) == posSpells.end())
                    posSpells[spellName] = spellId;
            else if (posSpells[spellName] < spellId)
                    posSpells[spellName] = spellId;
            }
            else
            {
                if (negSpells.find(spellName) == negSpells.end())
                    negSpells[spellName] = spellId;
            else if (negSpells[spellName] < spellId)
                    negSpells[spellName] = spellId;
            }
        }

        for (spellMap::const_iterator iter = posSpells.begin(); iter != posSpells.end(); ++iter)
        {
        posOut << " |cffffffff|Hspell:" << iter->second << "|h[" << iter->first << "]|h|r";
        }

        for (spellMap::const_iterator iter = negSpells.begin(); iter != negSpells.end(); ++iter)
        {
        negOut << " |cffffffff|Hspell:" << iter->second << "|h[" << iter->first << "]|h|r";
        }

        ChatHandler ch(&fromPlayer);
        SendWhisper("here's my non-attack spells:", fromPlayer);
        ch.SendSysMessage(posOut.str().c_str());
        SendWhisper("and here's my attack spells:", fromPlayer);
        ch.SendSysMessage(negOut.str().c_str());
    }

void PlayerbotAI::_HandleCommandSurvey(std::string& /*text*/, Player &fromPlayer)
    {
        uint32 count = 0;
        std::ostringstream detectout;
        QueryResult *result;
        GameEventMgr::ActiveEvents const& activeEventsList = sGameEventMgr.GetActiveEventList();
        std::ostringstream eventFilter;
        eventFilter << " AND (event IS NULL ";
        bool initString = true;

        for (GameEventMgr::ActiveEvents::const_iterator itr = activeEventsList.begin(); itr != activeEventsList.end(); ++itr)
        {
            if (initString)
            {
                eventFilter <<  "OR event IN (" << *itr;
                initString = false;
            }
            else
                eventFilter << "," << *itr;
        }

        if (!initString)
            eventFilter << "))";
        else
            eventFilter << ")";

        result = WorldDatabase.PQuery("SELECT gameobject.guid, id, position_x, position_y, position_z, map, "
                                      "(POW(position_x - %f, 2) + POW(position_y - %f, 2) + POW(position_z - %f, 2)) AS order_ FROM gameobject "
                                      "LEFT OUTER JOIN game_event_gameobject on gameobject.guid=game_event_gameobject.guid WHERE map = '%i' %s ORDER BY order_ ASC LIMIT 10",
                                      m_bot->GetPositionX(), m_bot->GetPositionY(), m_bot->GetPositionZ(), m_bot->GetMapId(), eventFilter.str().c_str());

        if (result)
        {
            do
            {
                Field *fields = result->Fetch();
                uint32 guid = fields[0].GetUInt32();
                uint32 entry = fields[1].GetUInt32();

                GameObject *go = m_bot->GetMap()->GetGameObject(ObjectGuid(HIGHGUID_GAMEOBJECT, entry, guid));
                if (!go)
                    continue;

                if (!go->isSpawned())
                    continue;

                detectout << "|cFFFFFF00|Hfound:" << guid << ":" << entry  << ":" <<  "|h[" << go->GetGOInfo()->name << "]|h|r";
                ++count;
            } while (result->NextRow());

            delete result;
        }
        SendWhisper(detectout.str().c_str(), fromPlayer);
    }

// _HandleCommandSkill: Handle class & professions training:
    // skill                           -- Lists bot(s) Primary profession skills & weapon skills
// skill learn                     -- List available class or profession (Primary or Secondary) skills, spells & abilities from selected trainer.
// skill learn [HLINK][HLINK] ..   -- Learn selected skill and spells, from selected trainer ([HLINK] from skill learn).
    // skill unlearn [HLINK][HLINK] .. -- Unlearn selected primary profession skill(s) and all associated spells ([HLINK] from skill)
void PlayerbotAI::_HandleCommandSkill(std::string &text, Player &fromPlayer)
    {
        uint32 rank[8] = {0, 75, 150, 225, 300, 375, 450, 525};

        std::ostringstream msg;

    if (ExtractCommand("learn", text))
        {
            uint32 totalCost = 0;

            Unit* unit = ObjectAccessor::GetUnit(*m_bot, fromPlayer.GetSelectionGuid());
            if (!unit)
            {
                SendWhisper("Please select the trainer!", fromPlayer);
                return;
            }

            if (!unit->isTrainer())
            {
                SendWhisper("This is not a trainer!", fromPlayer);
                return;
            }

            Creature *creature =  m_bot->GetMap()->GetCreature(fromPlayer.GetSelectionGuid());
            if (!creature)
                return;

            if (!creature->IsTrainerOf(m_bot, false))
            {
                SendWhisper("This trainer can not teach me anything!", fromPlayer);
                return;
            }

            // check present spell in trainer spell list
            TrainerSpellData const* cSpells = creature->GetTrainerSpells();
            TrainerSpellData const* tSpells = creature->GetTrainerTemplateSpells();
            if (!cSpells && !tSpells)
            {
                SendWhisper("No spells can be learnt from this trainer", fromPlayer);
                return;
            }

            // reputation discount
            float fDiscountMod =  m_bot->GetReputationPriceDiscount(creature);

            // Handle: Learning class or profession (primary or secondary) skill & spell(s) for selected trainer, skill learn [HLINK][HLINK][HLINK].. ([HLINK] from skill train)
        if (text.size() > 0)
            {
            msg << "I have learned the following spells:\r";
                uint32 totalSpellLearnt = 0;
                bool visuals = true;
                m_spellsToLearn.clear();
            extractSpellIdList(text, m_spellsToLearn);
                for (std::list<uint32>::iterator it = m_spellsToLearn.begin(); it != m_spellsToLearn.end(); it++)
                {
                    uint32 spellId = *it;

                    if (!spellId)
                        break;

                    TrainerSpell const* trainer_spell = cSpells->Find(spellId);
                    if (!trainer_spell)
                        trainer_spell = tSpells->Find(spellId);

                if (!trainer_spell)
                        continue;

                TrainerSpellState state = m_bot->GetTrainerSpellState(trainer_spell, trainer_spell->reqLevel);
                if (state != TRAINER_SPELL_GREEN)
                        continue;

                    // apply reputation discount
                    uint32 cost = uint32(floor(trainer_spell->spellCost * fDiscountMod));
                    // check money requirement
                    if (m_bot->GetMoney() < cost)
                    {
                        Announce(CANT_AFFORD);
                        continue;
                    }

                    m_bot->ModifyMoney(-int32(cost));
                    // learn explicitly or cast explicitly
                    if (trainer_spell->IsCastable())
                        m_bot->CastSpell(m_bot, trainer_spell->spell, true);
                    else
                        m_bot->learnSpell(spellId, false);
                    ++totalSpellLearnt;
                    totalCost += cost;
                    const SpellEntry *const pSpellInfo =  sSpellStore.LookupEntry(spellId);
                    if (!pSpellInfo)
                        continue;

                    if (visuals)
                    {
                        visuals = false;
                        WorldPacket data(SMSG_PLAY_SPELL_VISUAL, 12);           // visual effect on trainer
                        data << ObjectGuid(fromPlayer.GetSelectionGuid());
                        data << uint32(0xB3);                                   // index from SpellVisualKit.dbc
                        GetMaster()->GetSession()->SendPacket(&data);
/*
                        data.Initialize(SMSG_PLAY_SPELL_IMPACT, 12);            // visual effect on player
                        data << m_bot->GetObjectGuid();
                        data << uint32(0x016A);                                 // index from SpellVisualKit.dbc
                        GetMaster()->GetSession()->SendPacket(&data);
*/                 }
/*
                    WorldPacket data(SMSG_TRAINER_BUY_SUCCEEDED, 12);
                    data << ObjectGuid(fromPlayer.GetSelectionGuid());
                    data << uint32(spellId);                                // should be same as in packet from client
                    GetMaster()->GetSession()->SendPacket(&data);
*/
                    MakeSpellLink(pSpellInfo, msg);
                    uint32 gold = uint32(cost / 10000);
                    cost -= (gold * 10000);
                    uint32 silver = uint32(cost / 100);
                    cost -= (silver * 100);
                    msg << " ";
                    if (gold > 0)
                        msg << gold <<  "|r|cfffffc00g|r|cff00ff00";
                    if (silver > 0)
                        msg << silver <<  "|r|cffc0c0c0s|r|cff00ff00";
                    msg << cost <<  "|r|cff95524Cc|r|cff00ff00\r";
                }
                ReloadAI();
                uint32 gold = uint32(totalCost / 10000);
                totalCost -= (gold * 10000);
                uint32 silver = uint32(totalCost / 100);
                totalCost -= (silver * 100);
                msg << "Total of " << totalSpellLearnt << " spell";
                if (totalSpellLearnt != 1) msg << "s";
                msg << " learnt, ";
                if (gold > 0)
                    msg << gold <<  "|r|cfffffc00g|r|cff00ff00";
                if (silver > 0)
                    msg << silver <<  "|r|cffc0c0c0s|r|cff00ff00";
                msg << totalCost <<  "|r|cff95524Cc|r|cff00ff00 spent.";
            }
        // Handle: List class or profession skills, spells & abilities for selected trainer
            else
            {
                msg << "The spells I can learn and their cost:\r";

                TrainerSpellData const* trainer_spells = cSpells;
                if (!trainer_spells)
                    trainer_spells = tSpells;

                for (TrainerSpellMap::const_iterator itr =  trainer_spells->spellList.begin(); itr !=  trainer_spells->spellList.end(); ++itr)
                {
                    TrainerSpell const* tSpell = &itr->second;

                    if (!tSpell)
                        break;

                    uint32 reqLevel = 0;
                    if (!tSpell->learnedSpell && !m_bot->IsSpellFitByClassAndRace(tSpell->learnedSpell, &reqLevel))
                        continue;

                if  (sSpellMgr.IsPrimaryProfessionFirstRankSpell(tSpell->learnedSpell) && m_bot->HasSpell(tSpell->learnedSpell))
                        continue;

                    reqLevel = tSpell->isProvidedReqLevel ? tSpell->reqLevel : std::max(reqLevel, tSpell->reqLevel);

                    TrainerSpellState state =  m_bot->GetTrainerSpellState(tSpell,reqLevel);
                    if (state != TRAINER_SPELL_GREEN)
                        continue;

                    uint32 spellId = tSpell->spell;
                    const SpellEntry *const pSpellInfo =  sSpellStore.LookupEntry(spellId);
                    if (!pSpellInfo)
                        continue;
                    uint32 cost = uint32(floor(tSpell->spellCost *  fDiscountMod));
                    totalCost += cost;

                    uint32 gold = uint32(cost / 10000);
                    cost -= (gold * 10000);
                    uint32 silver = uint32(cost / 100);
                    cost -= (silver * 100);
                    MakeSpellLink(pSpellInfo, msg);
                    msg << " ";
                    if (gold > 0)
                        msg << gold <<  "|r|cfffffc00g|r|cff00ff00";
                    if (silver > 0)
                        msg << silver <<  "|r|cffc0c0c0s|r|cff00ff00";
                    msg << cost <<  "|r|cff95524Cc|r|cff00ff00\r";
                }
                int32 moneyDiff = m_bot->GetMoney() - totalCost;
                if (moneyDiff >= 0)
                {
                    // calculate how much money bot has
                    uint32 gold = uint32(moneyDiff / 10000);
                    moneyDiff -= (gold * 10000);
                    uint32 silver = uint32(moneyDiff / 100);
                    moneyDiff -= (silver * 100);
                    msg << " ";
                    if (gold > 0)
                        msg << gold <<  "|r|cfffffc00g|r|cff00ff00";
                    if (silver > 0)
                        msg << silver <<  "|r|cffc0c0c0s|r|cff00ff00";
                    msg << moneyDiff <<  "|r|cff95524Cc|r|cff00ff00 left.";
                }
                else
                {
                    Announce(CANT_AFFORD);
                    moneyDiff *= -1;
                    uint32 gold = uint32(moneyDiff / 10000);
                    moneyDiff -= (gold * 10000);
                    uint32 silver = uint32(moneyDiff / 100);
                    moneyDiff -= (silver * 100);
                    msg << "I need ";
                    if (gold > 0)
                        msg << " " << gold <<  "|r|cfffffc00g|r|cff00ff00";
                    if (silver > 0)
                        msg << silver <<  "|r|cffc0c0c0s|r|cff00ff00";
                    msg << moneyDiff <<  "|r|cff95524Cc|r|cff00ff00 more to learn all the spells!";
                }
            }
        }
        // Handle: Unlearning selected primary profession skill(s) and all associated spells, skill unlearn [HLINK][HLINK].. ([HLINK] from skill)
    else if (ExtractCommand("unlearn", text))
        {
            m_spellsToLearn.clear();
        extractSpellIdList(text, m_spellsToLearn);
            for (std::list<uint32>::iterator it = m_spellsToLearn.begin(); it != m_spellsToLearn.end(); ++it)
            {
            if (sSpellMgr.IsPrimaryProfessionSpell(*it))
                {
                    SpellLearnSkillNode const* spellLearnSkill = sSpellMgr.GetSpellLearnSkill(*it);
                    uint32 prev_spell = sSpellMgr.GetPrevSpellInChain(*it);
                    if (!prev_spell)                                    // first rank, remove skill
                        GetPlayer()->SetSkill(spellLearnSkill->skill, 0, 0);
                    else
                    {
                        // search prev. skill setting by spell ranks chain
                        SpellLearnSkillNode const* prevSkill = sSpellMgr.GetSpellLearnSkill(prev_spell);
                        while (!prevSkill && prev_spell)
                        {
                            prev_spell = sSpellMgr.GetPrevSpellInChain(prev_spell);
                            prevSkill = sSpellMgr.GetSpellLearnSkill(sSpellMgr.GetFirstSpellInChain(prev_spell));
                        }
                        if (!prevSkill)                                 // not found prev skill setting, remove skill
                            GetPlayer()->SetSkill(spellLearnSkill->skill, 0, 0);
                    }
                }
            }
        }
        // Handle: Lists bot(s) primary profession skills & weapon skills.
        else
        {
            m_spellsToLearn.clear();
            m_bot->skill(m_spellsToLearn);
            msg << "My Primary Professions: ";
            for (std::list<uint32>::iterator it = m_spellsToLearn.begin(); it != m_spellsToLearn.end(); ++it)
            {
                if (IsPrimaryProfessionSkill(*it))
                    for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
                    {
                        SkillLineAbilityEntry const *skillLine = sSkillLineAbilityStore.LookupEntry(j);
                        if (!skillLine)
                            continue;

                        // has skill
                        if (skillLine->skillId == *it && skillLine->learnOnGetSkill == 0)
                        {
                            SpellEntry const* spellInfo = sSpellStore.LookupEntry(skillLine->spellId);
                            if (!spellInfo)
                                continue;

                            if (m_bot->GetSkillValue(*it) <= rank[sSpellMgr.GetSpellRank(skillLine->spellId)] && m_bot->HasSpell(skillLine->spellId))
                            {
                                // DEBUG_LOG ("[PlayerbotAI]: HandleCommand - skill (%u)(%u)(%u):",skillLine->spellId, rank[sSpellMgr.GetSpellRank(skillLine->spellId)], m_bot->GetSkillValue(*it));
                                MakeSpellLink(spellInfo, msg);
                                break;
                            }
                        }
                    }
            }

            msg << "\nMy Weapon skills: ";
            for (std::list<uint32>::iterator it = m_spellsToLearn.begin(); it != m_spellsToLearn.end(); ++it)
            {
                SkillLineEntry const *SkillLine = sSkillLineStore.LookupEntry(*it);
                // has weapon skill
                if (SkillLine->categoryId == SKILL_CATEGORY_WEAPON)
                {
                    for (uint32 j = 0; j < sSkillLineAbilityStore.GetNumRows(); ++j)
                    {
                        SkillLineAbilityEntry const *skillLine = sSkillLineAbilityStore.LookupEntry(j);
                        if (!skillLine)
                            continue;

                        SpellEntry const* spellInfo = sSpellStore.LookupEntry(skillLine->spellId);
                        if (!spellInfo)
                            continue;

                        if (skillLine->skillId == *it && spellInfo->Effect[0] == SPELL_EFFECT_WEAPON)
                            MakeWeaponSkillLink(spellInfo,msg,*it);
                    }
                }
            }
        }
        SendWhisper(msg.str(), fromPlayer);
        m_spellsToLearn.clear();
        m_bot->GetPlayerbotAI()->GetClassAI();
    }

void PlayerbotAI::_HandleCommandStats(std::string &text, Player &fromPlayer)
{
    if (text != "")
    {
        SendWhisper("'stats' does not have subcommands", fromPlayer);
        return;
    }

        std::ostringstream out;

        uint32 totalused = 0;
        // list out items in main backpack
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; slot++)
        {
            const Item* const pItem = m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (pItem)
                totalused++;
        }
        uint32 totalfree = 16 - totalused;
        // list out items in other removable backpacks
        for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        {
            const Bag* const pBag = (Bag *) m_bot->GetItemByPos(INVENTORY_SLOT_BAG_0, bag);
            if (pBag)
            {
                ItemPrototype const* pBagProto = pBag->GetProto();
                if (pBagProto->Class == ITEM_CLASS_CONTAINER && pBagProto->SubClass == ITEM_SUBCLASS_CONTAINER)
                    totalfree =  totalfree + pBag->GetFreeSlots();
            }

        }

        // estimate how much item damage the bot has
        uint32 copper = EstRepairAll();
        uint32 gold = uint32(copper / 10000);
        copper -= (gold * 10000);
        uint32 silver = uint32(copper / 100);
        copper -= (silver * 100);

        out << "|cffffffff[|h|cff00ffff" << m_bot->GetName() << "|h|cffffffff] has |cff00ff00";
        out << totalfree << " |h|cffffffff bag slots,|h" << " |cff00ff00";
        if (gold > 0)
            out << "|r|cff00ff00" << gold <<  "|r|cfffffc00g|r|cff00ff00";
        if (silver > 0)
            out << silver <<  "|r|cffc0c0c0s|r|cff00ff00";
        out << copper <<  "|r|cff95524Cc|r|cff00ff00";

        // calculate how much money bot has
        copper = m_bot->GetMoney();
        gold = uint32(copper / 10000);
        copper -= (gold * 10000);
        silver = uint32(copper / 100);
        copper -= (silver * 100);

        out << "|h|cffffffff item damage & has " << "|r|cff00ff00";
        if (gold > 0)
            out << gold <<  "|r|cfffffc00g|r|cff00ff00";
        if (silver > 0)
            out << silver <<  "|r|cffc0c0c0s|r|cff00ff00";
        out << copper <<  "|r|cff95524Cc|r|cff00ff00";
        ChatHandler ch(&fromPlayer);
        ch.SendSysMessage(out.str().c_str());
    }

/*    else
    {
        // if this looks like an item link, reward item it completed quest and talking to NPC
        std::list<uint32> itemIds;
        extractItemIds(text, itemIds);
        if (!itemIds.empty()) {
            uint32 itemId = itemIds.front();
            bool wasRewarded = false;
            ObjectGuid questRewarderGUID = m_bot->GetSelectionGuid();
            Object* const pNpc = (WorldObject *) m_bot->GetObjectByTypeMask(questRewarderGUID, TYPEMASK_CREATURE_OR_GAMEOBJECT);
            if (!pNpc)
                return;

            QuestMenu& questMenu = m_bot->PlayerTalkClass->GetQuestMenu();
            for (uint32 iI = 0; !wasRewarded && iI < questMenu.MenuItemCount(); ++iI)
            {
                QuestMenuItem const& qItem = questMenu.GetItem(iI);

                uint32 questID = qItem.m_qId;
                Quest const* pQuest = sObjectMgr.GetQuestTemplate(questID);
                QuestStatus status = m_bot->GetQuestStatus(questID);

                // if quest is complete, turn it in
                if (status == QUEST_STATUS_COMPLETE &&
                    !m_bot->GetQuestRewardStatus(questID) &&
                    pQuest->GetRewChoiceItemsCount() > 1 &&
                    m_bot->CanRewardQuest(pQuest, false))
                    for (uint8 rewardIdx = 0; !wasRewarded && rewardIdx < pQuest->GetRewChoiceItemsCount(); ++rewardIdx)
                    {
                        ItemPrototype const * const pRewardItem = sObjectMgr.GetItemPrototype(pQuest->RewChoiceItemId[rewardIdx]);
                        if (itemId == pRewardItem->ItemId)
                        {
                            m_bot->RewardQuest(pQuest, rewardIdx, pNpc, false);

                            std::string questTitle  = pQuest->GetTitle();
                            m_bot->GetPlayerbotAI()->QuestLocalization(questTitle, questID);
                            std::string itemName = pRewardItem->Name1;
                            m_bot->GetPlayerbotAI()->ItemLocalization(itemName, pRewardItem->ItemId);

                            std::ostringstream out;
                            out << "|cffffffff|Hitem:" << pRewardItem->ItemId << ":0:0:0:0:0:0:0" << "|h[" << itemName << "]|h|r rewarded";
                            SendWhisper(out.str(), fromPlayer);
                            wasRewarded = true;
                        }
                    }
            }
        }
        else
        {
            std::ostringstream msg;
            msg << "What? |cff339900attack auction |cffffffff| |cffFFB6C1(|cff1E90FFa|cffFFB6C1)|cff1E90FFdd |cffFFB6C1[Item link] |cffffffff| |cffFFB6C1(|cff1E90FFr|cffFFB6C1)|cff1E90FFemove |cffFFB6C1[Auction link]\r"
                << "|cff339900bank |cffffffff| |cffFFB6C1(|cff1E90FFd|cffFFB6C1)|cff1E90FFeposit |cffffffff| |cffFFB6C1(|cff1E90FFw|cffFFB6C1)|cff1E90FFithdraw |cffFFB6C1[Item link] |cffFFB6C1(|cffff4500c|cffFFB6C1)|cff339900ast |cffFFB6C1[Spell link] |cff339900collect |cffffffff<|cffffff00all none combat loot objects profession quest|cffffffff> "
                << "|cffFFB6C1(|cffff4500e|cffFFB6C1)|cff339900quip |cffFFB6C1[Item link] |cffFFB6C1(|cffff4500f|cffFFB6C1)|cff339900ind |cffFFB6C1[GO link] |cff339900follow |cffFFB6C1(|cffff4500g|cffFFB6C1)|cff339900et |cffffffff<|cffa52a2aTARGET |cffffffff| |cffFFB6C1[GO link] |cff339900orders "
                << "|cff339900pet |cff1E90FFspells |cffffffff| <|cff1E90FFcast |cffffffff| |cff1E90FFtoggle|cffffffff> |cffFFB6C1[Spell link] |cffffffff| |cff1E90FFstate |cffffffff| |cff1E90FFreact |cffffffff<|cffFFB6C1(|cffffff00a|cffFFB6C1)|cffffff00ggresive |cffffffff| |cffFFB6C1(|cffffff00d|cffFFB6C1)|cffffff00efensive |cffffffff| |cffFFB6C1(|cffffff00p|cffFFB6C1)|cffffff00assive|cffffffff> "
                << "|cff339900quest |cffFFB6C1(|cff1E90FFa|cffFFB6C1) |cff1E90FFdd |cffffffff| |cffFFB6C1(|cff1E90FFd|cffFFB6C1)|cff1E90FFrop |cffffffff| |cffFFB6C1(|cff1E90FFe|cffFFB6C1)|cff1E90FFnd |cffffffff| |cffFFB6C1(|cff1E90FFl|cffFFB6C1)|cff1E90FFist\r|cff339900repair |cffFFB6C1[Item Link] |cff339900report |cff339900reset |cffFFB6C1(|cffff4500s|cffFFB6C1)|cff339900ell |cffFFB6C1[Item link] "
                << "|cff339900skill |cffffffff| |cffFFB6C1(|cff1E90FFt|cffFFB6C1)|cff1E90FFrain |cffa52a2aTARGET |cffffffff| |cffFFB6C1(|cff1E90FFl|cffFFB6C1)|cff1E90FFearn |cffFFB6C1[Training link] |cffffffff| |cffFFB6C1(|cff1E90FFu|cffFFB6C1)|cff1E90FFnlearn |cffFFB6C1[Profession link] |cff339900spells stats stay survey use |cffffffff<|cffa52a2aTARGET|cffffffff> |cffFFB6C1[Item link]";
            TellMaster(msg.str());
            m_bot->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
        }
    }
}*/

void PlayerbotAI::_HandleCommandHelp(std::string &text, Player &fromPlayer)
{
    // "help help"? Seriously?
    if (ExtractCommand("help", text))
    {
        SendWhisper(_HandleCommandHelpHelper("help", "Lists all the things you can order me to do... But it's up to me whether to follow your orders... Or not."), fromPlayer);
        return;
    }

    bool bMainHelp = (text == "") ? true : false;
    const std::string sInvalidSubcommand = "That's not a valid subcommand.";
    std::string msg = "";
    // All of these must contain the 'bMainHelp' clause -> help lists all major commands
    // Further indented 'ExtractCommand("subcommand")' conditionals make sure these aren't printed for basic "help"
    if (bMainHelp || ExtractCommand("attack", text))
    {
        SendWhisper(_HandleCommandHelpHelper("attack", "Attack the selected target. Which would, of course, require a valid target.", HL_TARGET), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("pull", text))
    {
        SendWhisper(_HandleCommandHelpHelper("pull", "Pull the target in a coordinated party/raid manner.", HL_TARGET), fromPlayer);

        if (!bMainHelp)
        {
            SendWhisper(_HandleCommandHelpHelper("pull test", "I'll tell you if I could pull at all. Can be used anywhere."), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("pull ready", "I'll tell you if I'm ready to pull *right now*. To be used on location with valid target."), fromPlayer);
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("neutralize", text))
    {
        SendWhisper(_HandleCommandHelpHelper("neutralize|neutral", "The bot will try to put its master's target out of combat with crowd control abilities like polymorph, banish, hibernate, shackles and the like.", HL_TARGET), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("follow", text))
    {
        SendWhisper(_HandleCommandHelpHelper("follow", "I will follow you - this also revives me if dead and teleports me if I'm far away."), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("stay", text))
    {
        SendWhisper(_HandleCommandHelpHelper("stay", "I will stay put until told otherwise."), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("assist", text))
    {
        SendWhisper(_HandleCommandHelpHelper("assist", "I will assist the character listed, attacking as they attack.", HL_NAME), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("spells", text))
    {
        SendWhisper(_HandleCommandHelpHelper("spells", "I will list all the spells I know."), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("cast", text))
    {
        SendWhisper(_HandleCommandHelpHelper("cast", "I will cast the spell or ability listed.", HL_SPELL), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("use", text))
    {
        SendWhisper(_HandleCommandHelpHelper("use", "I will use the linked item.", HL_ITEM), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("equip", text))
    {
        SendWhisper(_HandleCommandHelpHelper("equip", "I will equip the linked item(s).", HL_ITEM, true), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("reset", text))
    {
        SendWhisper(_HandleCommandHelpHelper("reset", "I will reset all my states, orders, loot list, talent spec, ... Hey, that's kind of like memory loss."), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("report", text))
    {
        SendWhisper(_HandleCommandHelpHelper("report", "This will give you a full report of all the items, creatures or gameobjects needed to finish my quests."), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("stats", text))
    {
        SendWhisper(_HandleCommandHelpHelper("stats", "This will inform you of my wealth, free bag slots and estimated equipment repair costs."), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("survey", text))
    {
        SendWhisper(_HandleCommandHelpHelper("survey", "Lists all available game objects near me."), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("find", text))
    {
        SendWhisper(_HandleCommandHelpHelper("find", "I will find said game object, walk right up to it, and wait.", HL_GAMEOBJECT), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("get", text))
    {
        SendWhisper(_HandleCommandHelpHelper("get", "I will get said game object and return to your side.", HL_GAMEOBJECT), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("quest", text))
    {
        SendWhisper(_HandleCommandHelpHelper("quest", "Lists my current quests."), fromPlayer);

        if (!bMainHelp)
        {
            SendWhisper(_HandleCommandHelpHelper("quest add", "Adds this quest to my quest log.", HL_QUEST), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("quest drop", "Removes this quest from my quest log.", HL_QUEST), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("quest end", "Turns in my completed quests."), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("quest list", "Lists the quests offered to me by this target."), fromPlayer);

            // Catches all valid subcommands, also placeholders for potential future sub-subcommands
            if (ExtractCommand("add", text, true)) {}
            else if(ExtractCommand("drop", text, true)) {}
            else if(ExtractCommand("end", text, true)) {}
            else if (ExtractCommand("list", text, true)) {}

            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("orders", text))
    {
        SendWhisper(_HandleCommandHelpHelper("orders", "Shows you my orders. Free will is overrated, right?"), fromPlayer);

        if (!bMainHelp)
        {
            SendWhisper(_HandleCommandHelpHelper("orders combat <tank | heal | assist | protect | reset> [targetPlayer]", "Sets general orders I should follow. Assist and Protect require a target."), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("orders delay <0-10>", "Activates a delay before I start fighting."), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("orders resume", "Resume combat orders to what they were before logout."), fromPlayer);

            // Catches all valid subcommands, also placeholders for potential future sub-subcommands
            if (ExtractCommand("combat", text, true))
            {
                SendWhisper(_HandleCommandHelpHelper("orders combat tank", "Order me to tank. Best used on paladins, warriors or druids."), fromPlayer);
                SendWhisper(_HandleCommandHelpHelper("orders combat heal", "Order me to heal. Best used on shamans, priests, druids or paladins."), fromPlayer);
                SendWhisper(_HandleCommandHelpHelper("orders combat assist", "Assist the linked target focusing our killing power.", HL_TARGET), fromPlayer);
                SendWhisper(_HandleCommandHelpHelper("orders combat protect", "Protect the listed target, attempting to keep aggro away from the target.", HL_TARGET), fromPlayer);
                SendWhisper(_HandleCommandHelpHelper("orders combat reset", "Resets my combat orders as though you'd never given me any at all."), fromPlayer);

                if (ExtractCommand("tank", text, true)) {}
                else if (ExtractCommand("heal", text, true)) {}
                else if (ExtractCommand("assist", text, true)) {}
                else if (ExtractCommand("protect", text, true)) {}
                else if (ExtractCommand("reset", text, true)) {}

                else if (text != "") SendWhisper(sInvalidSubcommand.c_str(), fromPlayer);
            }
            else if (ExtractCommand("delay", text, true)) {}
            else if (ExtractCommand("resume", text, true)) {}

            else if (text != "") SendWhisper(sInvalidSubcommand.c_str(), fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("pet", text))
    {
        if (bMainHelp)
            SendWhisper(_HandleCommandHelpHelper("pet", "Helps command my pet. Must always be used with a subcommand."), fromPlayer);
        else if (text == "") // not "help" AND "help pet"
            SendWhisper(_HandleCommandHelpHelper("pet", "This by itself is not a valid command. Just so you know. To be used with a subcommand, such as..."), fromPlayer);

        if (!bMainHelp)
        {
            SendWhisper(_HandleCommandHelpHelper("pet spells", "Shows you the spells my pet knows."), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("pet cast", "Has my pet cast this spell. May require a treat. Or at least ask nicely.", HL_SPELL), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("pet toggle", "Toggles autocast for this spell.", HL_SPELL), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("pet state", "Shows my pet's aggro mode."), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("pet react", "Sets my pet's aggro mode.", HL_PETAGGRO), fromPlayer);

            // Catches all valid subcommands, also placeholders for potential future sub-subcommands
            if (ExtractCommand("spells", text)) {}
            else if(ExtractCommand("cast", text)) {}
            else if(ExtractCommand("toggle", text)) {}
            else if (ExtractCommand("state", text)) {}
            else if (ExtractCommand("react", text))
            {
                SendWhisper(_HandleCommandHelpHelper("pet react", "has three modes."), fromPlayer);
                SendWhisper(_HandleCommandHelpHelper("aggressive", "sets it so my precious attacks everything in sight.", HL_NONE, false, true), fromPlayer);
                SendWhisper(_HandleCommandHelpHelper("defensive", "sets it so it automatically attacks anything that attacks me, or anything I attack.", HL_NONE, false, true), fromPlayer);
                SendWhisper(_HandleCommandHelpHelper("passive", "makes it so my pet won't attack anything unless directly told to.", HL_NONE, false, true), fromPlayer);

                // Catches all valid subcommands, also placeholders for potential future sub-subcommands
                if (ExtractCommand("aggressive", text, true)) {}
                else if (ExtractCommand("defensive", text, true)) {}
                else if (ExtractCommand("passive", text, true)) {}
                if (text != "")
                    SendWhisper(sInvalidSubcommand, fromPlayer);
            }

            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("collect", text))
    {
        SendWhisper(_HandleCommandHelpHelper("collect", "Tells you what my current collect status is. Also lists possible options."), fromPlayer);
        SendWhisper(_HandleCommandHelpHelper("collect", "Sets what I collect. Obviously the 'none' option should be used alone, but all the others can be mixed.", HL_OPTION, true), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("sell", text))
    {
        SendWhisper(_HandleCommandHelpHelper("sell", "Adds this to my 'for sale' list.", HL_ITEM), fromPlayer);

        if (!bMainHelp)
        {
            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("auction", text))
    {
        SendWhisper(_HandleCommandHelpHelper("auction", "Lists all my active auctions. With pretty little links and such. Hi hi hi... I'm gonna be sooo rich!"), fromPlayer);

        if (!bMainHelp)
        {
            SendWhisper(_HandleCommandHelpHelper("auction add", "Adds the item to my 'auction off later' list. I have a lot of lists, you see...", HL_ITEM), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("auction remove", "Adds the item to my 'Don't auction after all' list. Hope it hasn't sold by then!", HL_AUCTION), fromPlayer);

            // Catches all valid subcommands, also placeholders for potential future sub-subcommands
            if (ExtractCommand("add", text, true)) {}
            else if(ExtractCommand("remove", text, true)) {}

            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("repair", text))
    {
        if (!bMainHelp && text == "")
            SendWhisper(_HandleCommandHelpHelper("repair", "This by itself is not a valid command. Just so you know. To be used with a subcommand, such as..."), fromPlayer);

        if (!bMainHelp)
        {
            SendWhisper(_HandleCommandHelpHelper("repair", "Has me find an armorer and repair the items you listed.", HL_ITEM), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("repair all", "Has me find an armorer and repair all my items, be they equipped or just taking up bagspace."), fromPlayer);

            // Catches all valid subcommands, also placeholders for potential future sub-subcommands
            if (ExtractCommand("all", text)) {}

            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("bank", text))
    {
        SendWhisper(_HandleCommandHelpHelper("bank", "Gives you my bank balance. I thought that was private."), fromPlayer);

        if (!bMainHelp)
        {
            SendWhisper(_HandleCommandHelpHelper("bank deposit", "Deposits the listed items in my bank.", HL_ITEM, true), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("bank withdraw", "Withdraw the listed items from my bank.", HL_ITEM, true), fromPlayer);

            // Catches all valid subcommands, also placeholders for potential future sub-subcommands
            if (ExtractCommand("deposit", text)) {}
            else if (ExtractCommand("withdraw", text)) {}

            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }
    if (bMainHelp || ExtractCommand("skill", text))
    {
        msg = _HandleCommandHelpHelper("skill", "Lists my primary professions.");
        SendWhisper(msg, fromPlayer);

        if (!bMainHelp)
        {
            SendWhisper(_HandleCommandHelpHelper("skill train", "Lists the things this trainer can teach me. If you've targeted a trainer, that is."), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("skill learn", "Have me learn this skill from the selected trainer.", HL_SKILL), fromPlayer);
            SendWhisper(_HandleCommandHelpHelper("skill unlearn", "Unlearn the linked (primary) profession and everything that goes with it.", HL_PROFESSION), fromPlayer);

            // Catches all valid subcommands, also placeholders for potential future sub-subcommands
            if (ExtractCommand("train", text)) {}
            else if (ExtractCommand("learn", text)) {}
            else if (ExtractCommand("unlearn", text)) {}

            if (text != "") SendWhisper(sInvalidSubcommand, fromPlayer);
            return;
        }
    }

    if (bMainHelp)
        SendWhisper(_HandleCommandHelpHelper("help", "Gives you this listing of main commands... But then, you know that already don't you."), fromPlayer);

    if(text != "")
        SendWhisper("Either that is not a valid command, or someone forgot to add it to my help journal. I mean seriously, they can't expect me to remember *all* this stuff, can they?", fromPlayer);
}

std::string PlayerbotAI::_HandleCommandHelpHelper(std::string sCommand, std::string sExplain, HELPERLINKABLES reqLink, bool bReqLinkMultiples, bool bCommandShort)
{
    if (sCommand == "")
    {
        DEBUG_LOG("[PlayerbotAI] _HandleCommandHelpHelper called with an empty sCommand. Ignoring call.");
        return "";
    }

    std::ostringstream oss;
    oss << "'|cffffffff";
    if (bCommandShort)
        oss << "(" << sCommand.at(0) << ")" << sCommand.substr(1);
    else
        oss << sCommand;

    if (reqLink != HL_NONE)
    {
        if (reqLink == HL_PROFESSION)
        {
            oss << " [PROFESSION]";
            if (bReqLinkMultiples)
                oss << " [PROFESSION] ..";
        }
        else if (reqLink == HL_ITEM)
        {
            oss << " [ITEM]";
            if (bReqLinkMultiples)
                oss << " [ITEM] ..";
        }
         else if (reqLink == HL_TALENT)
        {
            oss << " [TALENT]";
            if (bReqLinkMultiples)
                oss << " [TALENT] ..";
        }
        else if (reqLink == HL_SKILL)
        {
            oss << " [SKILL]";
            if (bReqLinkMultiples)
                oss << " [SKILL] ..";
        }
        else if (reqLink == HL_OPTION)
        {
            oss << " <OPTION>";
            if (bReqLinkMultiples)
                oss << " <OPTION> ..";
        }
        else if (reqLink == HL_PETAGGRO)
        {
            oss << " <(a)ggressive | (d)efensive | (p)assive>";
            if (bReqLinkMultiples)
                DEBUG_LOG("[PlayerbotAI] _HandleCommandHelpHelper: sCommand \"pet\" with bReqLinkMultiples \"true\". ... Why? Bug, surely.");
        }
        else if (reqLink == HL_QUEST)
        {
            oss << " [QUEST]";
            if (bReqLinkMultiples)
                oss << " [QUEST] ..";
        }
        else if (reqLink == HL_GAMEOBJECT)
        {
            oss << " [GAMEOBJECT]";
            if (bReqLinkMultiples)
                oss << " [GAMEOBJECT] ..";
        }
        else if (reqLink == HL_SPELL)
        {
            oss << " <Id# | (part of) name | [SPELL]>";
            if (bReqLinkMultiples)
                oss << " <Id# | (part of) name | [SPELL]> ..";
        }
        else if (reqLink == HL_TARGET)
        {
            oss << " (TARGET)";
            if (bReqLinkMultiples)
                oss << " (TARGET) ..";
        }
        else if (reqLink == HL_NAME)
        {
            oss << " <NAME>";
            if (bReqLinkMultiples)
                oss << " <NAME> ..";
        }
        else if (reqLink == HL_AUCTION)
        {
            oss << " [AUCTION]";
            if (bReqLinkMultiples)
                oss << " [AUCTION] ..";
        }
        else
        {
            oss << " {unknown}";
            if (bReqLinkMultiples)
                oss << " {unknown} ..";
            DEBUG_LOG("[PlayerbotAI]: _HandleCommandHelpHelper - Uncaught case");
        }
    }

    oss << "|r': " << sExplain;

    return oss.str();
}

/**
* Checks whether the TalentSpec database contains any obvious errors
*
* return 0  -> all ok
* return x  -> return the talentspec_id of the first talentspec that errors out
*/

// TODO: the way this is built is just begging for a memory leak (by adding a return case and forgetting to delete result)
uint32 PlayerbotAI::TalentSpecDBContainsError()
{
	QueryResult *result = CharacterDatabase.Query("SELECT * FROM playerbot_talentspec ORDER BY class ASC");

	if (!result)
	{
		// Do you really need a progress bar? No, but all the other kids jumped off the bridge too...
		BarGoLink bar(1);

		bar.step();

		sLog.outString();
		sLog.outString(">> Loaded `playerbot_talentspec`, table is empty.");

		return 0;   // Because, well, no specs means none contain errors...
	}

	BarGoLink bar(result->GetRowCount());

	do
	{
		bar.step();

		/* 0            talentspec_id
		1            name
		2            class
		3            purpose
		4 to 54        talent_10 to 51
		*/
		Field* fields = result->Fetch();

		uint32 ts_id = fields[0].GetUInt32();
		if (!ts_id)    // Nice bit of paranoia: ts_id is a non-zero NOT NULL AUTO_INCREMENT value
			continue;  // Of course, if the impossible ever does happen, we can't very well identify a TalentSpec without an ID...

		std::string ts_name = fields[1].GetCppString();
		/*    Commented out? Because it's only required if you assume only players (not the server) pick talentspecs
		if (0 == ts_name.size())
		{
		TellMaster("TalentSpec ID: %u does not have a name.", ts_id);

		delete result;
		return ts_id;
		}
		*/

		long ts_class = fields[2].GetInt32();
		if (ts_class != CLASS_DRUID && ts_class != CLASS_HUNTER && ts_class != CLASS_MAGE && ts_class != CLASS_PALADIN && ts_class != CLASS_PRIEST && ts_class != CLASS_ROGUE && ts_class != CLASS_SHAMAN && ts_class != CLASS_WARLOCK && ts_class != CLASS_WARRIOR)
		{
			TellMaster("TalentSpec: %u. \"%s\" contains an invalid class: %i.", ts_id, ts_name.c_str(), ts_class);

			delete result;
			return ts_id;    // invalid class
		}

		// Can't really be error checked, can it?
		// uint32 ts_purpose = fields[3].GetUInt32();

		// check all talents
		for (uint8 i = 0; i < 51; i++)
		{
			uint8 fieldLoc = i + 4;
			if (fields[fieldLoc].GetUInt16() == 0)
			{
				for (uint8 j = (i + 1); j < 51; j++)
				{
					fieldLoc = j + 4;
					if (fields[fieldLoc].GetUInt16() != 0)
					{
						TellMaster("TalentSpec: %u. \"%s\" contains an empty talent for level: %u while a talent for level: %u exists.", ts_id, ts_name.c_str(), (i + 10), (j + 10));

						delete result;
						return ts_id;
					}
				}
				break;
			}
			else if (!ValidateTalent(fields[fieldLoc].GetUInt16(), ts_class))
			{
				TellMaster("TalentSpec: %u. \"%s\" (class: %i) contains an invalid talent for level %u: %u", ts_id, ts_name.c_str(), ts_class, (i + 10), fields[fieldLoc].GetUInt16());

				delete result;
				return ts_id;    // invalid talent
			}
		}

		// No glyphs in class WOW
		//for (uint8 i = 75; i < 78; i++)  // as in, the 3 major glyphs
		//{
		//	if (fields[i].GetUInt16() != 0 && !ValidateMajorGlyph(fields[i].GetUInt16(), ts_class))
		//	{
		//		TellMaster("TalentSpec: %u. \"%s\" contains an invalid Major glyph %u: %u", ts_id, ts_name.c_str(), (i - 74), fields[i].GetUInt16());
		//		if (!ValidateGlyph(fields[i].GetUInt16(), ts_class))
		//			TellMaster("In fact, according to our records, it's no glyph at all");

		//		delete result;
		//		return ts_id;
		//	}
		//}
		//for (uint8 i = 78; i < 81; i++)  // as in, the 3 minor glyphs
		//{
		//	if (fields[i].GetUInt16() != 0 && !ValidateMinorGlyph(fields[i].GetUInt16(), ts_class))
		//	{
		//		TellMaster("TalentSpec: %u. \"%s\" contains an invalid Minor glyph %u: %u", ts_id, ts_name.c_str(), (i - 77), fields[i].GetUInt16());
		//		if (!ValidateGlyph(fields[i].GetUInt16(), ts_class))
		//			TellMaster("In fact, according to our records, it's no glyph at all");

		//		delete result;
		//		return ts_id;
		//	}
		//}
	} while (result->NextRow());

	delete result;
	return 0;
}

uint32 PlayerbotAI::GetTalentSpecsAmount()
{
	QueryResult *result = CharacterDatabase.Query("SELECT COUNT(*) FROM playerbot_talentspec");

	if (!result)
	{
		sLog.outString();
		sLog.outString(">> Loaded `playerbot_talentspec`, table is empty.");

		return 0;
	}

	Field* fields = result->Fetch();

	uint32 count = fields[0].GetUInt32();

	delete result;
	return count;
}

uint32 PlayerbotAI::GetTalentSpecsAmount(long specClass)
{
	QueryResult *result = CharacterDatabase.PQuery("SELECT COUNT(*) FROM playerbot_talentspec WHERE class = '%li'", specClass);

	if (!result)
	{
		sLog.outString();
		sLog.outString(">> Loaded `playerbot_talentspec`, found no talentspecs for class %li.", specClass);

		return 0;
	}

	Field* fields = result->Fetch();

	uint32 count = fields[0].GetUInt32();

	delete result;
	return count;
}

/**
* GetTalentSpecs queries DB for the talentspecs (for a class), returning them in a list of TS structures
*
* *** for the most part, GetTalentSpecs assumes ALL SPECS ARE VALID ***
*/
std::list<TalentSpec> PlayerbotAI::GetTalentSpecs(long specClass)
{
	TalentSpec ts;
	std::list<TalentSpec> tsList;

	QueryResult *result = CharacterDatabase.PQuery("SELECT * FROM playerbot_talentspec WHERE class = %li ORDER BY talentspec_id ASC", specClass);

	if (!result)
	{
		sLog.outString();
		sLog.outString(">> Loaded `playerbot_talentspec`, found no talentspecs for class %li.", specClass);

		return tsList; // empty
	}

	do
	{
		/* 0            talentspec_id
		1            name
		2            class
		3            purpose
		4 to 54        talent_10 to 51
		*/
		Field* fields = result->Fetch();

		/* ts_id = fields[0].GetUInt32(); // not used
		if (!ts_id)    // Nice bit of paranoia: ts_id is an AUTO_INCREMENT value
		continue;  // Of course, if the impossible ever does happen, we can't very well identify a TalentSpec without an ID...
		*/

		ts.specName = fields[1].GetCppString();
		ts.specClass = fields[2].GetInt16();
		if (ts.specClass != CLASS_DRUID && ts.specClass != CLASS_HUNTER && ts.specClass != CLASS_MAGE && ts.specClass != CLASS_PALADIN && ts.specClass != CLASS_PRIEST && ts.specClass != CLASS_ROGUE && ts.specClass != CLASS_SHAMAN && ts.specClass != CLASS_WARLOCK && ts.specClass != CLASS_WARRIOR)
		{
			TellMaster("TalentSpec: %u. \"%s\" contains an invalid class.", fields[0].GetUInt32(), ts.specName.c_str());

			continue;    // this spec is clearly broken, the next may or may not be
		}

		ts.specPurpose = (TalentSpecPurpose)fields[3].GetUInt32();

		// check all talents
		for (uint8 i = 0; i < 51; i++)
		{
			ts.talentId[i] = fields[i + 4].GetUInt16();
		}

		// No glyphs in classic WOW
		//for (uint8 i = 0; i < 3; i++)  // as in, the 3 major glyphs
		//{
		//	ts.glyphIdMajor[i] = fields[i + 75].GetUInt16();
		//}
		//for (uint8 i = 0; i < 3; i++)  // as in, the 3 minor glyphs
		//{
		//	ts.glyphIdMajor[i] = fields[i + 78].GetUInt16();
		//}

		tsList.push_back(ts);
	} while (result->NextRow());

	delete result;
	return tsList;
}

/**
* GetTalentSpec queries DB for a talentspec given a class and a choice.
* The choice applies to the results for that class only, and is volatile.
*
* *** for the most part, GetTalentSpec assumes ALL SPECS ARE VALID ***
*/
TalentSpec PlayerbotAI::GetTalentSpec(long specClass, long choice)
{
	TalentSpec ts;
	// Let's zero it out to be safe
	ts.specName = "";
	ts.specClass = 0;
	ts.specPurpose = TSP_NONE;
	for (int i = 0; i < 51; i++) ts.talentId[i] = 0;

	// Weed out invalid choice - ts has been zero'd out anyway
	if (0 >= choice || (long)GetTalentSpecsAmount(specClass) < choice) return ts;

	QueryResult *result = CharacterDatabase.PQuery("SELECT * FROM playerbot_talentspec WHERE class = %li ORDER BY talentspec_id ASC", specClass);

	if (!result)
	{
		sLog.outString();
		sLog.outString(">> Loaded `playerbot_talentspec`, found no talentspecs for class %li.", specClass);

		delete result;
		return ts; // empty
	}

	for (int i = 1; i <= (int)GetTalentSpecsAmount(specClass); i++)
	{

		if (i == choice)
		{
			/*
			0            talentspec_id
			1            name
			2            class
			3            purpose
			4 to 54    talent_10 to 51
			*/
			Field* fields = result->Fetch();

			/* ts_id = fields[0].GetUInt32(); // not used
			if (!ts_id)    // Nice bit of paranoia: ts_id is an AUTO_INCREMENT value
			continue;  // Of course, if the impossible ever does happen, we can't very well identify a TalentSpec without an ID...
			*/

			ts.specName = fields[1].GetCppString();
			ts.specClass = fields[2].GetInt16();
			if (ts.specClass != CLASS_DRUID && ts.specClass != CLASS_HUNTER && ts.specClass != CLASS_MAGE && ts.specClass != CLASS_PALADIN && ts.specClass != CLASS_PRIEST && ts.specClass != CLASS_ROGUE && ts.specClass != CLASS_SHAMAN && ts.specClass != CLASS_WARLOCK && ts.specClass != CLASS_WARRIOR)
			{
				TellMaster("TalentSpec: %u. \"%s\" contains an invalid class.", fields[0].GetUInt32(), ts.specName.c_str());

				ts.specName = "";
				ts.specClass = 0;
				delete result;
				return ts;
			}

			ts.specPurpose = (TalentSpecPurpose)fields[3].GetUInt32();

			// check all talents
			for (uint8 i = 0; i < 51; i++)
			{
				ts.talentId[i] = fields[i + 4].GetUInt16();
			}

			delete result;
			return ts;
		}

		// TODO: okay, this won't bog down the system, but it's still a waste. Figure out a better way.
		result->NextRow();
	}

	delete result;
	return ts;
}

/**
* ApplyActiveTalentSpec takes the active talent spec and attempts to apply it
*
* return true  -> ok, talentspec applied as fully as possible
* return false -> talentspec was not or only partially applied
*/
bool PlayerbotAI::ApplyActiveTalentSpec()
{
	// empty talent spec -> nothing to apply -> fully applied
	if (m_activeTalentSpec.specClass == 0 || m_activeTalentSpec.specPurpose == TSP_NONE)
		return true;

	// Some basic error checking just in case
	if (m_activeTalentSpec.specClass != m_bot->getClass())
		return false;

	std::vector<uint16> talentsToLearn;
	talentsToLearn.reserve(51);
	for (int i = 0; i < 51; i++)
	{
		if (m_activeTalentSpec.talentId[i] != 0)
			talentsToLearn.push_back(m_activeTalentSpec.talentId[i]);
	}

	// No dual spec in classic
	PlayerTalentMap ptm = m_bot->GetTalents(0);
	// First do a check as to whether all known talents are in the talent spec
	for (PlayerTalentMap::iterator iter = ptm.begin(); iter != ptm.end(); iter++)
	{
		PlayerTalent talent = (*iter).second;

		// WARNING: There may be more than 71 'talents' in the PTM - unlearned talents are simply set as disabled - not removed
		if (talent.state == PLAYERSPELL_REMOVED)
			continue;

		// currentRank = 0 to (MAX_RANK-1) not 1 to MAX_RANK
		for (int i = 0; i <= (int)talent.currentRank; i++)
		{
			int j = 0; // why 0 and not -1? Because if talentsToCheck (no TalentSpec) is empty and talents have been learned -> NOK
			for (std::vector<uint16>::iterator it = talentsToLearn.begin(); it != talentsToLearn.end(); it++)
			{
				if (talentsToLearn.at(j) == talent.talentEntry->TalentID)
				{
					talentsToLearn.erase(it);
					j = -1; // So j = -1 -> learned talent found in talentspec
					break;
				}
				j++;
			}

			// j == -1 signifies talent has been found in talent spec
			if (-1 != j)
			{
				TellMaster("I've learned talents that are not in my talent spec. If you want me to learn the talent spec anyway you should have me reset my talents.");
				return false;
			}
		}
	}

	int x = 0;
	for (std::vector<uint16>::iterator iter = talentsToLearn.begin(); iter != talentsToLearn.end(); iter++)
	{
		// find current talent rank
		uint32 learnTalentRank = 0;
		if (PlayerTalent const* talent = m_bot->GetKnownTalentById(talentsToLearn.at(x)))
			learnTalentRank = talent->currentRank + 1;
		// else -> not known -> to learn = 0

		// check if we have enough talent points
		uint32 freeTalentPointsBefore = m_bot->GetFreeTalentPoints();
		if (0 == freeTalentPointsBefore)
			return true;

		m_bot->LearnTalent(talentsToLearn.at(x), learnTalentRank);
		if (freeTalentPointsBefore == m_bot->GetFreeTalentPoints())
		{
			// Do not tell master - error is logged server side, master gets generic failure warning from calling function.
			//TellMaster("Failed to learn talent - Class: %i; TalentId: %i; TalentRank: %i. This error has been logged.", m_bot->getClass(), talentsToLearn.at(x), learnTalentRank);
			DEBUG_LOG("[PlayerbotAI]: ApplyActiveTalentSpec failure - Class: %i; TalentId: %i; TalentRank: %i.", m_bot->getClass(), talentsToLearn.at(x), learnTalentRank);
			return false;
		}

		x++;
	}

	return true;
}

/**
* ValidateTalent tests a talent against class to see if it belongs to that class
*
* uint16 talent:        talent ID
* long charClass:    member of the Classes enum or ClassesCombatPets enum
*
* return true  -> ok
* return false -> not a valid talent for that class
*/
bool PlayerbotAI::ValidateTalent(uint16 talent, long charClass)
{
	//if (charClass == CLASS_DRUID)
	//{
	//	if (DRUID_FEROCITY != talent && DRUID_FERAL_AGGRESSION != talent && DRUID_FERAL_INSTINCT != talent && DRUID_SAVAGE_FURY != talent && DRUID_THICK_HIDE != talent && DRUID_FERAL_SWIFTNESS != talent && DRUID_SURVIVAL_INSTINCTS != talent && DRUID_SHARPENED_CLAWS != talent && DRUID_SHREDDING_ATTACKS != talent && DRUID_PREDATORY_STRIKES != talent && DRUID_PRIMAL_FURY != talent && DRUID_PRIMAL_PRECISION != talent && DRUID_BRUTAL_IMPACT != talent && DRUID_FERAL_CHARGE != talent && DRUID_NURTURING_INSTINCT != talent && DRUID_NATURAL_REACTION != talent && DRUID_HEART_OF_THE_WILD != talent && DRUID_SURVIVAL_OF_THE_FITTEST != talent && DRUID_LEADER_OF_THE_PACK != talent && DRUID_IMPROVED_LEADER_OF_THE_PACK != talent && DRUID_PRIMAL_TENACITY != talent && DRUID_PROTECTOR_OF_THE_PACK != talent && DRUID_PREDATORY_INSTINCTS != talent && DRUID_INFECTED_WOUNDS != talent && DRUID_KING_OF_THE_JUNGLE != talent && DRUID_MANGLE != talent && DRUID_IMPROVED_MANGLE != talent && DRUID_REND_AND_TEAR != talent && DRUID_PRIMAL_GORE != talent && DRUID_BERSERK != talent && DRUID_IMPROVED_MARK_OF_THE_WILD != talent && DRUID_NATURES_FOCUS != talent && DRUID_FUROR != talent && DRUID_NATURALIST != talent && DRUID_SUBTLETY != talent && DRUID_NATURAL_SHAPESHIFTER != talent && DRUID_INTENSITY != talent && DRUID_OMEN_OF_CLARITY != talent && DRUID_MASTER_SHAPESHIFTER != talent && DRUID_TRANQUIL_SPIRIT != talent && DRUID_IMPROVED_REJUVENATION != talent && DRUID_NATURES_SWIFTNESS != talent && DRUID_GIFT_OF_NATURE != talent && DRUID_IMPROVED_TRANQUILITY != talent && DRUID_EMPOWERED_TOUCH != talent && DRUID_NATURES_BOUNTY != talent && DRUID_LIVING_SPIRIT != talent && DRUID_SWIFTMEND != talent && DRUID_NATURAL_PERFECTION != talent && DRUID_EMPOWERED_REJUVENATION != talent && DRUID_LIVING_SEED != talent && DRUID_REVITALIZE != talent && DRUID_TREE_OF_LIFE != talent && DRUID_IMPROVED_TREE_OF_LIFE != talent && DRUID_IMPROVED_BARKSKIN != talent && DRUID_GIFT_OF_THE_EARTHMOTHER != talent && DRUID_WILD_GROWTH != talent && DRUID_STARLIGHT_WRATH != talent && DRUID_GENESIS != talent && DRUID_MOONGLOW != talent && DRUID_NATURES_MAJESTY != talent && DRUID_IMPROVED_MOONFIRE != talent && DRUID_BRAMBLES != talent && DRUID_NATURES_GRACE != talent && DRUID_NATURES_SPLENDOR != talent && DRUID_NATURES_REACH != talent && DRUID_VENGEANCE != talent && DRUID_CELESTIAL_FOCUS != talent && DRUID_LUNAR_GUIDANCE != talent && DRUID_INSECT_SWARM != talent && DRUID_IMPROVED_INSECT_SWARM != talent && DRUID_DREAMSTATE != talent && DRUID_MOONFURY != talent && DRUID_BALANCE_OF_POWER != talent && DRUID_MOONKIN_FORM != talent && DRUID_IMPROVED_MOONKIN_FORM != talent && DRUID_IMPROVED_FAERIE_FIRE != talent && DRUID_OWLKIN_FRENZY != talent && DRUID_WRATH_OF_CENARIUS != talent && DRUID_ECLIPSE != talent && DRUID_TYPHOON != talent && DRUID_FORCE_OF_NATURE != talent && DRUID_GALE_WINDS != talent && DRUID_EARTH_AND_MOON != talent && DRUID_STARFALL != talent)
	//		return false;
	//}
	//else if (charClass == CLASS_HUNTER)
	//{
	//	if (HUNTER_IMPROVED_ASPECT_OF_THE_HAWK != talent && HUNTER_ENDURANCE_TRAINING != talent && HUNTER_FOCUSED_FIRE != talent && HUNTER_IMPROVED_ASPECT_OF_THE_MONKEY != talent && HUNTER_THICK_HIDE != talent && HUNTER_IMPROVED_REVIVE_PET != talent && HUNTER_PATHFINDING != talent && HUNTER_ASPECT_MASTERY != talent && HUNTER_UNLEASHED_FURY != talent && HUNTER_IMPROVED_MEND_PET != talent && HUNTER_FEROCITY != talent && HUNTER_SPIRIT_BOND != talent && HUNTER_INTIMIDATION != talent && HUNTER_BESTIAL_DISCIPLINE != talent && HUNTER_ANIMAL_HANDLER != talent && HUNTER_FRENZY != talent && HUNTER_FEROCIOUS_INSPIRATION != talent && HUNTER_BESTIAL_WRATH != talent && HUNTER_CATLIKE_REFLEXES != talent && HUNTER_INVIGORATION != talent && HUNTER_SERPENTS_SWIFTNESS != talent && HUNTER_LONGEVITY != talent && HUNTER_THE_BEAST_WITHIN != talent && HUNTER_COBRA_STRIKES != talent && HUNTER_KINDRED_SPIRITS != talent && HUNTER_BEAST_MASTERY != talent && HUNTER_IMPROVED_TRACKING != talent && HUNTER_HAWK_EYE != talent && HUNTER_SAVAGE_STRIKES != talent && HUNTER_SUREFOOTED != talent && HUNTER_ENTRAPMENT != talent && HUNTER_TRAP_MASTERY != talent && HUNTER_SURVIVAL_INSTINCTS != talent && HUNTER_SURVIVALIST != talent && HUNTER_SCATTER_SHOT != talent && HUNTER_DEFLECTION != talent && HUNTER_SURVIVAL_TACTICS != talent && HUNTER_TNT != talent && HUNTER_LOCK_AND_LOAD != talent && HUNTER_HUNTER_VS_WILD != talent && HUNTER_KILLER_INSTINCT != talent && HUNTER_COUNTERATTACK != talent && HUNTER_LIGHTNING_REFLEXES != talent && HUNTER_RESOURCEFULNESS != talent && HUNTER_EXPOSE_WEAKNESS != talent && HUNTER_WYVERN_STING != talent && HUNTER_THRILL_OF_THE_HUNT != talent && HUNTER_MASTER_TACTICIAN != talent && HUNTER_NOXIOUS_STINGS != talent && HUNTER_POINT_OF_NO_ESCAPE != talent && HUNTER_BLACK_ARROW != talent && HUNTER_SNIPER_TRAINING != talent && HUNTER_HUNTING_PARTY != talent && HUNTER_EXPLOSIVE_SHOT != talent && HUNTER_IMPROVED_CONCUSSIVE_SHOT != talent && HUNTER_FOCUSED_AIM != talent && HUNTER_LETHAL_SHOTS != talent && HUNTER_CAREFUL_AIM != talent && HUNTER_IMPROVED_HUNTERS_MARK != talent && HUNTER_MORTAL_SHOTS != talent && HUNTER_GO_FOR_THE_THROAT != talent && HUNTER_IMPROVED_ARCANE_SHOT != talent && HUNTER_AIMED_SHOT != talent && HUNTER_RAPID_KILLING != talent && HUNTER_IMPROVED_STINGS != talent && HUNTER_EFFICIENCY != talent && HUNTER_CONCUSSIVE_BARRAGE != talent && HUNTER_READINESS != talent && HUNTER_BARRAGE != talent && HUNTER_COMBAT_EXPERIENCE != talent && HUNTER_RANGED_WEAPON_SPECIALIZATION != talent && HUNTER_PIERCING_SHOTS != talent && HUNTER_TRUESHOT_AURA != talent && HUNTER_IMPROVED_BARRAGE != talent && HUNTER_MASTER_MARKSMAN != talent && HUNTER_RAPID_RECUPERATION != talent && HUNTER_WILD_QUIVER != talent && HUNTER_SILENCING_SHOT != talent && HUNTER_IMPROVED_STEADY_SHOT != talent && HUNTER_MARKED_FOR_DEATH != talent && HUNTER_CHIMERA_SHOT != talent)
	//		return false;
	//}
	//else if (charClass == CLASS_MAGE)
	//{
	//	if (MAGE_IMPROVED_FIRE_BLAST != talent && MAGE_INCINERATION != talent && MAGE_IMPROVED_FIREBALL != talent && MAGE_IGNITE != talent && MAGE_BURNING_DETERMINATION != talent && MAGE_WORLD_IN_FLAMES != talent && MAGE_FLAME_THROWING != talent && MAGE_IMPACT != talent && MAGE_PYROBLAST != talent && MAGE_BURNING_SOUL != talent && MAGE_IMPROVED_SCORCH != talent && MAGE_MOLTEN_SHIELDS != talent && MAGE_MASTER_OF_ELEMENTS != talent && MAGE_PLAYING_WITH_FIRE != talent && MAGE_CRITICAL_MASS != talent && MAGE_BLAST_WAVE != talent && MAGE_BLAZING_SPEED != talent && MAGE_FIRE_POWER != talent && MAGE_PYROMANIAC != talent && MAGE_COMBUSTION != talent && MAGE_MOLTEN_FURY != talent && MAGE_FIERY_PAYBACK != talent && MAGE_EMPOWERED_FIRE != talent && MAGE_FIRESTARTER != talent && MAGE_DRAGONS_BREATH != talent && MAGE_HOT_STREAK != talent && MAGE_BURNOUT != talent && MAGE_LIVING_BOMB != talent && MAGE_FROSTBITE != talent && MAGE_IMPROVED_FROSTBOLT != talent && MAGE_ICE_FLOES != talent && MAGE_ICE_SHARDS != talent && MAGE_FROST_WARDING != talent && MAGE_PRECISION != talent && MAGE_PERMAFROST != talent && MAGE_PIERCING_ICE != talent && MAGE_ICY_VEINS != talent && MAGE_IMPROVED_BLIZZARD != talent && MAGE_ARCTIC_REACH != talent && MAGE_FROST_CHANNELING != talent && MAGE_SHATTER != talent && MAGE_COLD_SNAP != talent && MAGE_IMPROVED_CONE_OF_COLD != talent && MAGE_FROZEN_CORE != talent && MAGE_COLD_AS_ICE != talent && MAGE_WINTERS_CHILL != talent && MAGE_SHATTERED_BARRIER != talent && MAGE_ICE_BARRIER != talent && MAGE_ARCTIC_WINDS != talent && MAGE_EMPOWERED_FROSTBOLT != talent && MAGE_FINGERS_OF_FROST != talent && MAGE_BRAIN_FREEZE != talent && MAGE_SUMMON_WATER_ELEMENTAL != talent && MAGE_ENDURING_WINTER != talent && MAGE_CHILLD_TO_THE_BONE != talent && MAGE_DEEP_FREEZE != talent && MAGE_ARCANE_SUBTLETY != talent && MAGE_ARCANE_FOCUS != talent && MAGE_ARCANE_STABILITY != talent && MAGE_ARCANE_FORTITUDE != talent && MAGE_MAGIC_ABSORPTION != talent && MAGE_ARCANE_CONCENTRATION != talent && MAGE_MAGIC_ATTUNEMENT != talent && MAGE_SPELL_IMPACT != talent && MAGE_STUDENT_OF_THE_MIND != talent && MAGE_FOCUS_MAGIC != talent && MAGE_ARCANE_SHIELDING != talent && MAGE_IMPROVED_COUNTERSPELL != talent && MAGE_ARCANE_MEDITATION != talent && MAGE_TORMENT_THE_WEAK != talent && MAGE_IMPROVED_BLINK != talent && MAGE_PRESENCE_OF_MIND != talent && MAGE_ARCANE_MIND != talent && MAGE_PRISMATIC_CLOAK != talent && MAGE_ARCANE_INSTABILITY != talent && MAGE_ARCANE_POTENCY != talent && MAGE_ARCANE_EMPOWERMENT != talent && MAGE_ARCANE_POWER != talent && MAGE_INCANTERS_ABSORPTION != talent && MAGE_ARCANE_FLOWS != talent && MAGE_MIND_MASTERY != talent && MAGE_SLOW != talent && MAGE_MISSILE_BARRAGE != talent && MAGE_NETHERWIND_PRESENCE != talent && MAGE_SPELL_POWER != talent && MAGE_ARCANE_BARRAGE != talent)
	//		return false;
	//}
	//else if (charClass == CLASS_PALADIN)
	//{
	//	if (PALADIN_DEFLECTION != talent && PALADIN_BENEDICTION != talent && PALADIN_IMPROVED_JUDGEMENTS != talent && PALADIN_HEART_OF_THE_CRUSADER != talent && PALADIN_IMPROVED_BLESSING_OF_MIGHT != talent && PALADIN_VINDICATION != talent && PALADIN_CONVICTION != talent && PALADIN_SEAL_OF_COMMAND != talent && PALADIN_PURSUIT_OF_JUSTICE != talent && PALADIN_EYE_FOR_AN_EYE != talent && PALADIN_SANCTITY_OF_BATTLE != talent && PALADIN_CRUSADE != talent && PALADIN_TWOHANDED_WEAPON_SPECIALIZATION != talent && PALADIN_SANCTIFIED_RETRIBUTION != talent && PALADIN_VENGEANCE != talent && PALADIN_DIVINE_PURPOSE != talent && PALADIN_THE_ART_OF_WAR != talent && PALADIN_REPENTANCE != talent && PALADIN_JUDGEMENTS_OF_THE_WISE != talent && PALADIN_FANATICISM != talent && PALADIN_SANCTIFIED_WRATH != talent && PALADIN_SWIFT_RETRIBUTION != talent && PALADIN_CRUSADER_STRIKE != talent && PALADIN_SHEATH_OF_LIGHT != talent && PALADIN_RIGHTEOUS_VENGEANCE != talent && PALADIN_DIVINE_STORM != talent && PALADIN_SPIRITUAL_FOCUS != talent && PALADIN_SEALS_OF_THE_PURE != talent && PALADIN_HEALING_LIGHT != talent && PALADIN_DIVINE_INTELLECT != talent && PALADIN_UNYIELDING_FAITH != talent && PALADIN_AURA_MASTERY != talent && PALADIN_ILLUMINATION != talent && PALADIN_IMPROVED_LAY_ON_HANDS != talent && PALADIN_IMPROVED_CONCENTRATION_AURA != talent && PALADIN_IMPROVED_BLESSING_OF_WISDOM != talent && PALADIN_BLESSED_HANDS != talent && PALADIN_PURE_OF_HEART != talent && PALADIN_DIVINE_FAVOR != talent && PALADIN_SANCTIFIED_LIGHT != talent && PALADIN_PURIFYING_POWER != talent && PALADIN_HOLY_POWER != talent && PALADIN_LIGHTS_GRACE != talent && PALADIN_HOLY_SHOCK != talent && PALADIN_BLESSED_LIFE != talent && PALADIN_SACRED_CLEANSING != talent && PALADIN_HOLY_GUIDANCE != talent && PALADIN_DIVINE_ILLUMINATION != talent && PALADIN_JUDGEMENTS_OF_THE_PURE != talent && PALADIN_INFUSION_OF_LIGHT != talent && PALADIN_ENLIGHTENED_JUDGEMENTS != talent && PALADIN_BEACON_OF_LIGHT != talent && PALADIN_DIVINITY != talent && PALADIN_DIVINE_STRENGTH != talent && PALADIN_STOICISM != talent && PALADIN_GUARDIANS_FAVOR != talent && PALADIN_ANTICIPATION != talent && PALADIN_DIVINE_SACRIFICE != talent && PALADIN_IMPROVED_RIGHTEOUS_FURY != talent && PALADIN_TOUGHNESS != talent && PALADIN_DIVINE_GUARDIAN != talent && PALADIN_IMPROVED_HAMMER_OF_JUSTICE != talent && PALADIN_IMPROVED_DEVOTION_AURA != talent && PALADIN_BLESSING_OF_SANCTUARY != talent && PALADIN_RECKONING != talent && PALADIN_SACRED_DUTY != talent && PALADIN_ONEHANDED_WEAPON_SPECIALIZATION != talent && PALADIN_SPIRITUAL_ATTUNEMENT != talent && PALADIN_HOLY_SHIELD != talent && PALADIN_ARDENT_DEFENDER != talent && PALADIN_REDOUBT != talent && PALADIN_COMBAT_EXPERTISE != talent && PALADIN_TOUCHER_BY_THE_LIGHT != talent && PALADIN_AVENGERS_SHIELD != talent && PALADIN_GUARDED_BY_THE_LIGHT != talent && PALADIN_SHIELD_OF_THE_TEMPLAR != talent && PALADIN_JUDGEMENT_OF_THE_JUST != talent && PALADIN_HAMMER_OF_THE_RIGHTEOUS != talent)
	//		return false;
	//}
	//else if (charClass == CLASS_PRIEST)
	//{
	//	if (PRIEST_UNBREAKABLE_WILL != talent && PRIEST_TWIN_DISCIPLINES != talent && PRIEST_SILENT_RESOLVE != talent && PRIEST_IMPROVED_INNER_FIRE != talent && PRIEST_IMPROVED_POWER_WORD_FORTITUDE != talent && PRIEST_MARTYRDOM != talent && PRIEST_MEDITATION != talent && PRIEST_INNER_FOCUS != talent && PRIEST_IMPROVED_POWER_WORD_SHIELD != talent && PRIEST_ABSOLUTION != talent && PRIEST_MENTAL_AGILITY != talent && PRIEST_IMPROVED_MANA_BURN != talent && PRIEST_REFLECTIVE_SHIELD != talent && PRIEST_MENTAL_STRENGTH != talent && PRIEST_SOUL_WARDING != talent && PRIEST_FOCUSED_POWER != talent && PRIEST_ENLIGHTENMENT != talent && PRIEST_FOCUSED_WILL != talent && PRIEST_POWER_INFUSION != talent && PRIEST_IMPROVED_FLASH_HEAL != talent && PRIEST_RENEWED_HOPE != talent && PRIEST_RAPTURE != talent && PRIEST_ASPIRATION != talent && PRIEST_DIVINE_AEGIS != talent && PRIEST_PAIN_SUPPRESSION != talent && PRIEST_GRACE != talent && PRIEST_BORROWED_TIME != talent && PRIEST_PENANCE != talent && PRIEST_HEALING_FOCUS != talent && PRIEST_IMPROVED_RENEW != talent && PRIEST_HOLY_SPECIALIZATION != talent && PRIEST_SPELL_WARDING != talent && PRIEST_DIVINE_FURY != talent && PRIEST_DESPERATE_PRAYER != talent && PRIEST_BLESSED_RECOVERY != talent && PRIEST_INSPIRATION != talent && PRIEST_HOLY_REACH != talent && PRIEST_IMPROVED_HEALIN != talent && PRIEST_SEARING_LIGHT != talent && PRIEST_HEALING_PRAYERS != talent && PRIEST_SPIRIT_OF_REDEMPTION != talent && PRIEST_SPIRITUAL_GUIDANCE != talent && PRIEST_SURGE_OF_LIGHT != talent && PRIEST_SPIRITUAL_HEALING != talent && PRIEST_HOLY_CONCENTRATION != talent && PRIEST_LIGHTWELL != talent && PRIEST_BLESSED_RESILIENCE != talent && PRIEST_BODY_AND_SOUL != talent && PRIEST_EMPOWERED_HEALING != talent && PRIEST_SERENDIPITY != talent && PRIEST_EMPOWERED_RENEW != talent && PRIEST_CIRCLE_OF_HEALING != talent && PRIEST_TEST_OF_FAITH != talent && PRIEST_DIVINE_PROVIDENCE != talent && PRIEST_GUARDIAN_SPIRIT != talent && PRIEST_SPIRIT_TAP != talent && PRIEST_IMPROVED_SPIRIT_TAP != talent && PRIEST_DARKNESS != talent && PRIEST_SHADOW_AFFINITY != talent && PRIEST_IMPROVED_SHADOW_WORD_PAIN != talent && PRIEST_SHADOW_FOCUS != talent && PRIEST_IMPROVED_PSYCHIC_SCREAM != talent && PRIEST_IMPROVED_MIND_BLAST != talent && PRIEST_MIND_FLAY != talent && PRIEST_VEILED_SHADOWS != talent && PRIEST_SHADOW_REACH != talent && PRIEST_SHADOW_WEAVING != talent && PRIEST_SILENCE != talent && PRIEST_VAMPIRIC_EMBRACE != talent && PRIEST_IMPROVED_VAMPIRIC_EMBRACE != talent && PRIEST_FOCUSED_MIND != talent && PRIEST_MIND_MELT != talent && PRIEST_IMPROVED_DEVOURING_PLAGUE != talent && PRIEST_SHADOWFORM != talent && PRIEST_SHADOW_POWER != talent && PRIEST_IMPROVED_SHADOWFORM != talent && PRIEST_MISERY != talent && PRIEST_PSYCHIC_HORROR != talent && PRIEST_VAMPIRIC_TOUCH != talent && PRIEST_PAIN_AND_SUFFERING != talent && PRIEST_TWISTED_FAITH != talent && PRIEST_DISPERSION != talent)
	//		return false;
	//}
	//else if (charClass == CLASS_ROGUE)
	//{
	//	if (ROGUE_IMPROVED_GOUGE != talent && ROGUE_IMPROVED_SINISTER_STRIKE != talent && ROGUE_DUAL_WIELD_SPECIALIZATION != talent && ROGUE_IMPROVED_SLICE_AND_DICE != talent && ROGUE_DEFLECTION != talent && ROGUE_PRECISION != talent && ROGUE_ENDURANCE != talent && ROGUE_RIPOSTE != talent && ROGUE_CLOSE_QUARTERS_COMBAT != talent && ROGUE_IMPROVED_KICK != talent && ROGUE_IMPROVED_SPRINT != talent && ROGUE_LIGHTNING_REFLEXES != talent && ROGUE_AGGRESSION != talent && ROGUE_MACE_SPECIALIZATION != talent && ROGUE_BLADE_FLURRY != talent && ROGUE_HACK_AND_SLASH != talent && ROGUE_WEAPON_EXPERTISE != talent && ROGUE_BLADE_TWISTING != talent && ROGUE_VITALITY != talent && ROGUE_ADRENALINE_RUSH != talent && ROGUE_NERVES_OF_STEEL != talent && ROGUE_THROWING_SPECIALIZATION != talent && ROGUE_COMBAT_POTENCY != talent && ROGUE_UNFAIR_ADVANTAGE != talent && ROGUE_SURPRISE_ATTACKS != talent && ROGUE_SAVAGE_COMBAT != talent && ROGUE_PREY_ON_THE_WEAK != talent && ROGUE_KILLING_SPREE != talent && ROGUE_IMPROVED_EVISCERATE != talent && ROGUE_REMORSELESS_ATTACKS != talent && ROGUE_MALICE != talent && ROGUE_RUTHLESSNESS != talent && ROGUE_BLOOD_SPATTER != talent && ROGUE_PUNCTURING_WOUNDS != talent && ROGUE_VIGOR != talent && ROGUE_IMPROVED_EXPOSE_ARMOR != talent && ROGUE_LETHALITY != talent && ROGUE_VILE_POISONS != talent && ROGUE_IMPROVED_POISONS != talent && ROGUE_FLEET_FOOTED != talent && ROGUE_COLD_BLOOD != talent && ROGUE_IMPROVED_KIDNEY_SHOT != talent && ROGUE_QUICK_RECOVERY != talent && ROGUE_SEAL_FATE != talent && ROGUE_MURDER != talent && ROGUE_DEADLY_BREW != talent && ROGUE_OVERKILL != talent && ROGUE_DEADENED_NERVES != talent && ROGUE_FOCUSED_ATTACKS != talent && ROGUE_FIND_WEAKNESS != talent && ROGUE_MASTER_POISONER != talent && ROGUE_MUTILATE != talent && ROGUE_TURN_THE_TABLES != talent && ROGUE_CUT_TO_THE_CHASE != talent && ROGUE_HUNGER_FOR_BLOOD != talent && ROGUE_RELENTLESS_STRIKES != talent && ROGUE_MASTER_OF_DECEPTION != talent && ROGUE_OPPORTUNITY != talent && ROGUE_SLEIGHT_OF_HAND != talent && ROGUE_DIRTY_TRICKS != talent && ROGUE_CAMOUFLAGE != talent && ROGUE_ELUSIVENESS != talent && ROGUE_GHOSTLY_STRIKE != talent && ROGUE_SERRATED_BLADES != talent && ROGUE_SETUP != talent && ROGUE_INITIATIVE != talent && ROGUE_IMPROVED_AMBUSH != talent && ROGUE_HEIGHTENED_SENSES != talent && ROGUE_PREPARATION != talent && ROGUE_DIRTY_DEEDS != talent && ROGUE_HEMORRHAGE != talent && ROGUE_MASTER_OF_SUBTLETY != talent && ROGUE_DEADLINESS != talent && ROGUE_ENVELOPING_SHADOWS != talent && ROGUE_PREMEDITATION != talent && ROGUE_CHEAT_DEATH != talent && ROGUE_SINISTER_CALLING != talent && ROGUE_WAYLAY != talent && ROGUE_HONOR_AMONG_THIEVES != talent && ROGUE_SHADOWSTEP != talent && ROGUE_FILTHY_TRICKS != talent && ROGUE_SLAUGHTER_FROM_THE_SHADOWS != talent && ROGUE_SHADOW_DANCE != talent)
	//		return false;
	//}
	//else if (charClass == CLASS_SHAMAN)
	//{
	//	if (SHAMAN_CONVECTION != talent && SHAMAN_CONCUSSION != talent && SHAMAN_CALL_OF_FLAME != talent && SHAMAN_ELEMENTAL_WARDING != talent && SHAMAN_ELEMENTAL_DEVASTATION != talent && SHAMAN_REVERBERATION != talent && SHAMAN_ELEMENTAL_FOCUS != talent && SHAMAN_ELEMENTAL_FURY != talent && SHAMAN_IMPROVED_FIRE_NOVA != talent && SHAMAN_EYE_OF_THE_STORM != talent && SHAMAN_ELEMENTAL_REACH != talent && SHAMAN_CALL_OF_THUNDER != talent && SHAMAN_UNRELENTING_STORM != talent && SHAMAN_ELEMENTAL_PRECISION != talent && SHAMAN_LIGHTNING_MASTERY != talent && SHAMAN_ELEMENTAL_MASTERY != talent && SHAMAN_STORM_EARTH_AND_FIRE != talent && SHAMAN_BOOMING_ECHOES != talent && SHAMAN_ELEMENTAL_OATH != talent && SHAMAN_LIGHTNING_OVERLOAD != talent && SHAMAN_ASTRAL_SHIFT != talent && SHAMAN_TOTEM_OF_WRATH != talent && SHAMAN_LAVA_FLOWS != talent && SHAMAN_SHAMANISM != talent && SHAMAN_THUNDERSTORM != talent && SHAMAN_IMPROVED_HEALING_WAVE != talent && SHAMAN_TOTEMIC_FOCUS != talent && SHAMAN_IMPROVED_REINCARNATION != talent && SHAMAN_HEALING_GRACE != talent && SHAMAN_TIDAL_FOCUS != talent && SHAMAN_IMPROVED_WATER_SHIELD != talent && SHAMAN_HEALING_FOCUS != talent && SHAMAN_TIDAL_FORCE != talent && SHAMAN_ANCESTRAL_HEALING != talent && SHAMAN_RESTORATIVE_TOTEMS != talent && SHAMAN_TIDAL_MASTERY != talent && SHAMAN_HEALING_WAY != talent && SHAMAN_NATURES_SWIFTNESS != talent && SHAMAN_FOCUSED_MIND != talent && SHAMAN_PURIFICATION != talent && SHAMAN_NATURES_GUARDIAN != talent && SHAMAN_MANA_TIDE_TOTEM != talent && SHAMAN_CLEANSE_SPIRIT != talent && SHAMAN_BLESSING_OF_THE_ETERNALS != talent && SHAMAN_IMPROVED_CHAIN_HEAL != talent && SHAMAN_NATURES_BLESSING != talent && SHAMAN_ANCESTRAL_AWAKENING != talent && SHAMAN_EARTH_SHIELD != talent && SHAMAN_IMPROVED_EARTH_SHIELD != talent && SHAMAN_TIDAL_WAVES != talent && SHAMAN_RIPTIDE != talent && SHAMAN_ENHANCING_TOTEMS != talent && SHAMAN_EARTHS_GRASP != talent && SHAMAN_ANCESTRAL_KNOWLEDGE != talent && SHAMAN_GUARDIAN_TOTEMS != talent && SHAMAN_THUNDERING_STRIKES != talent && SHAMAN_IMPROVED_GHOST_WOLF != talent && SHAMAN_IMPROVED_SHIELDS != talent && SHAMAN_ELEMENTAL_WEAPONS != talent && SHAMAN_SHAMANISTIC_FOCUS != talent && SHAMAN_ANTICIPATION != talent && SHAMAN_FLURRY != talent && SHAMAN_TOUGHNESS != talent && SHAMAN_IMPROVED_WINDFURY_TOTEM != talent && SHAMAN_SPIRIT_WEAPONS != talent && SHAMAN_MENTAL_DEXTERITY != talent && SHAMAN_UNLEASHED_RAGE != talent && SHAMAN_WEAPON_MASTERY != talent && SHAMAN_FROZEN_POWER != talent && SHAMAN_DUAL_WIELD_SPECIALIZATION != talent && SHAMAN_DUAL_WIELD != talent && SHAMAN_STORMSTRIKE != talent && SHAMAN_STATIC_SHOCK != talent && SHAMAN_LAVA_LASH != talent && SHAMAN_IMPROVED_STORMSTRIKE != talent && SHAMAN_MENTAL_QUICKNESS != talent && SHAMAN_SHAMANISTIC_RAGE != talent && SHAMAN_EARTHEN_POWER != talent && SHAMAN_MAELSTROM_WEAPON != talent && SHAMAN_FERAL_SPIRIT != talent)
	//		return false;
	//}
	//else if (charClass == CLASS_WARLOCK)
	//{
	//	if (WARLOCK_IMPROVED_SHADOW_BOLT != talent && WARLOCK_BANE != talent && WARLOCK_AFTERMATH != talent && WARLOCK_MOLTEN_SKIN != talent && WARLOCK_CATACLYSM != talent && WARLOCK_DEMONIC_POWER != talent && WARLOCK_SHADOWBURN != talent && WARLOCK_RUIN != talent && WARLOCK_INTENSITY != talent && WARLOCK_DESTRUCTIVE_REACH != talent && WARLOCK_IMPROVED_SEARING_PAIN != talent && WARLOCK_BACKLASH != talent && WARLOCK_IMPROVED_IMMOLATE != talent && WARLOCK_DEVASTATION != talent && WARLOCK_NETHER_PROTECTION != talent && WARLOCK_EMBERSTORM != talent && WARLOCK_CONFLAGRATE != talent && WARLOCK_SOUL_LEECH != talent && WARLOCK_PYROCLASM != talent && WARLOCK_SHADOW_AND_FLAME != talent && WARLOCK_IMPROVED_SOUL_LEECH != talent && WARLOCK_BACKDRAFT != talent && WARLOCK_SHADOWFURY != talent && WARLOCK_EMPOWERED_IMP != talent && WARLOCK_FIRE_AND_BRIMSTONE != talent && WARLOCK_CHAOS_BOLT != talent && WARLOCK_IMPROVED_CURSE_OF_AGONY != talent && WARLOCK_SUPPRESSION != talent && WARLOCK_IMPROVED_CORRUPTION != talent && WARLOCK_IMPROVED_CURSE_OF_WEAKNESS != talent && WARLOCK_IMPROVED_DRAIN_SOUL != talent && WARLOCK_IMPROVED_LIFE_TAP != talent && WARLOCK_SOUL_SIPHON != talent && WARLOCK_IMPROVED_FEAR != talent && WARLOCK_FEL_CONCENTRATION != talent && WARLOCK_AMPLIFY_CURSE != talent && WARLOCK_GRIM_REACH != talent && WARLOCK_NIGHTFALL != talent && WARLOCK_EMPOWERED_CORRUPTION != talent && WARLOCK_SHADOW_EMBRACE != talent && WARLOCK_SIPHON_LIFE != talent && WARLOCK_CURSE_OF_EXHAUSTION != talent && WARLOCK_IMPROVED_FELHUNTER != talent && WARLOCK_SHADOW_MASTERY != talent && WARLOCK_ERADICATION != talent && WARLOCK_CONTAGION != talent && WARLOCK_DARK_PACT != talent && WARLOCK_IMPROVED_HOWL_OF_TERROR != talent && WARLOCK_MALEDICTION != talent && WARLOCK_DEATHS_EMBRACE != talent && WARLOCK_UNSTABLE_AFFLICTION != talent && WARLOCK_PANDEMIC != talent && WARLOCK_EVERLASTING_AFFLICTION != talent && WARLOCK_HAUNT != talent && WARLOCK_IMPROVED_HEALTHSTONE != talent && WARLOCK_IMPROVED_IMP != talent && WARLOCK_DEMONIC_EMBRACE != talent && WARLOCK_FEL_SYNERGY != talent && WARLOCK_IMPROVED_HEALTH_FUNNEL != talent && WARLOCK_DEMONIC_BRUTALITY != talent && WARLOCK_FEL_VITALITY != talent && WARLOCK_IMPROVED_SUCCUBUS != talent && WARLOCK_SOUL_LINK != talent && WARLOCK_FEL_DOMINATION != talent && WARLOCK_DEMONIC_AEGIS != talent && WARLOCK_UNHOLY_POWER != talent && WARLOCK_MASTER_SUMMONER != talent && WARLOCK_MANA_FEED != talent && WARLOCK_MASTER_CONJURER != talent && WARLOCK_MASTER_DEMONOLOGIST != talent && WARLOCK_MOLTEN_CORE != talent && WARLOCK_DEMONIC_RESILIENCE != talent && WARLOCK_DEMONIC_EMPOWERMENT != talent && WARLOCK_DEMONIC_KNOWLEDGE != talent && WARLOCK_DEMONIC_TACTICS != talent && WARLOCK_DECIMATION != talent && WARLOCK_IMPROVED_DEMONIC_TACTICS != talent && WARLOCK_SUMMON_FELGUARD != talent && WARLOCK_NEMESIS != talent && WARLOCK_DEMONIC_PACT != talent && WARLOCK_METAMORPHOSIS != talent)
	//		return false;
	//}
	//else if (charClass == CLASS_WARRIOR)
	//{
	//	if (WARRIOR_IMPROVED_HEROIC_STRIKE != talent && WARRIOR_DEFLECTION != talent && WARRIOR_IMPROVED_REND != talent && WARRIOR_IMPROVED_CHARGE != talent && WARRIOR_IRON_WILL != talent && WARRIOR_TACTICAL_MASTERY != talent && WARRIOR_IMPROVED_OVERPOWER != talent && WARRIOR_ANGER_MANAGEMENT != talent && WARRIOR_IMPALE != talent && WARRIOR_DEEP_WOUNDS != talent && WARRIOR_TWOHANDED_WEAPON_SPECIALIZATION != talent && WARRIOR_TASTE_FOR_BLOOD != talent && WARRIOR_POLEAXE_SPECIALIZATION != talent && WARRIOR_SWEEPING_STRIKES != talent && WARRIOR_MACE_SPECIALIZATION != talent && WARRIOR_SWORD_SPECIALIZATION != talent && WARRIOR_WEAPON_MASTERY != talent && WARRIOR_IMPROVED_HAMSTRING != talent && WARRIOR_TRAUMA != talent && WARRIOR_SECOND_WIND != talent && WARRIOR_MORTAL_STRIKE != talent && WARRIOR_STRENGTH_OF_ARMS != talent && WARRIOR_IMPROVED_SLAM != talent && WARRIOR_JUGGERNAUT != talent && WARRIOR_IMPROVED_MORTAL_STRIKE != talent && WARRIOR_UNRELENTING_ASSAULT != talent && WARRIOR_SUDDEN_DEATH != talent && WARRIOR_ENDLESS_RAGE != talent && WARRIOR_BLOOD_FRENZY != talent && WARRIOR_WRECKING_CREW != talent && WARRIOR_BLADESTORM != talent && WARRIOR_IMPROVED_BLOODRAGE != talent && WARRIOR_SHIELD_SPECIALIZATION != talent && WARRIOR_IMPROVED_THUNDER_CLAP != talent && WARRIOR_INCITE != talent && WARRIOR_ANTICIPATION != talent && WARRIOR_LAST_STAND != talent && WARRIOR_IMPROVED_REVENGE != talent && WARRIOR_SHIELD_MASTERY != talent && WARRIOR_TOUGHNESS != talent && WARRIOR_IMPROVED_SPELL_REFLECTION != talent && WARRIOR_IMPROVED_DISARM != talent && WARRIOR_PUNCTURE != talent && WARRIOR_IMPROVED_DISCIPLINES != talent && WARRIOR_CONCUSSION_BLOW != talent && WARRIOR_GAG_ORDER != talent && WARRIOR_ONEHANDED_WEAPON_SPECIALIZATION != talent && WARRIOR_IMPROVED_DEFENSIVE_STANCE != talent && WARRIOR_VIGILANCE != talent && WARRIOR_FOCUSED_RAGE != talent && WARRIOR_VITALITY != talent && WARRIOR_SAFEGUARD != talent && WARRIOR_WARBRINGER != talent && WARRIOR_DEVASTATE != talent && WARRIOR_CRITICAL_BLOCK != talent && WARRIOR_SWORD_AND_BOARD != talent && WARRIOR_DAMAGE_SHIELD != talent && WARRIOR_SHOCKWAVE != talent && WARRIOR_ARMORED_TO_THE_TEETH != talent && WARRIOR_BOOMING_VOICE != talent && WARRIOR_CRUELTY != talent && WARRIOR_IMPROVED_DEMORALIZING_SHOUT != talent && WARRIOR_UNBRIDLED_WRATH != talent && WARRIOR_IMPROVED_CLEAVE != talent && WARRIOR_PIERCING_HOWL != talent && WARRIOR_BLOOD_CRAZE != talent && WARRIOR_COMMANDING_PRESENCE != talent && WARRIOR_DUAL_WIELD_SPECIALIZATION != talent && WARRIOR_IMPROVED_EXECUTE != talent && WARRIOR_ENRAGE != talent && WARRIOR_PRECISION != talent && WARRIOR_DEATH_WISH != talent && WARRIOR_IMPROVED_INTERCEPT != talent && WARRIOR_IMPROVED_BERSERKER_RAGE != talent && WARRIOR_FLURRY != talent && WARRIOR_INTENSIFY_RAGE != talent && WARRIOR_BLOODTHIRST != talent && WARRIOR_IMPROVED_WHIRLWIND != talent && WARRIOR_FURIOUS_ATTACKS != talent && WARRIOR_IMPROVED_BERSERKER_STANCE != talent && WARRIOR_HEROIC_FURY != talent && WARRIOR_RAMPAGE != talent && WARRIOR_BLOODSURGE != talent && WARRIOR_UNENDING_FURY != talent && WARRIOR_TITANS_GRIP != talent)
	//		return false;
	//}
	//else // charClass unknown
	//{
	//	DEBUG_LOG("[PlayerbotAI]: Someone was naughty and supplied an invalid class to ValidateTalent: %u", (uint32)charClass);
	//	return false;
	//}

	return true;
}

// _HandleCommandTalent: Handle talents & glyphs:
// talent                           -- Lists bot(s) active talents [TALENT LINK] & glyphs [GLYPH LINK], unspent points & cost to reset
// talent learn [TALENT LINK] ..    -- Learn selected talent from bot client 'inspect' dialog -> 'talent' tab or from talent command (shift click icon/link)
// talent reset                     -- Resets all talents
// talent spec                      -- Lists various talentspecs for this bot's class
// talent spec #                    -- Sets talent spec # as active talentspec
void PlayerbotAI::_HandleCommandTalent(std::string &text, Player &fromPlayer)
{
	std::ostringstream out;
	if (ExtractCommand("learn", text))
	{
		std::list<talentPair>talents;
		extractTalentIds(text, talents);

		for (std::list<talentPair>::iterator itr = talents.begin(); itr != talents.end(); itr++)
		{
			uint32 talentid;
			uint32 rank;

			talentid = itr->first;
			rank = itr->second;

			m_bot->LearnTalent(talentid, ++rank);
			// TOOD: Need this?
			//m_bot->SendTalentsInfoData(false);
			// Can't inspect talents in classic so we don't need to update it
			//InspectUpdate();
		}

		SendWhisper(out.str(), fromPlayer);
	}
	else if (ExtractCommand("reset", text))
	{
		FollowAutoReset();
		m_tasks.push_back(std::pair<enum TaskFlags, uint32>(RESET, 0));
		m_findNPC.push_back(UNIT_NPC_FLAG_TRAINER);
	}
	else if (ExtractCommand("spec", text))
	{
		// If no playerbot_saved_data record, add one now
		QueryResult *resultlvl = CharacterDatabase.PQuery("SELECT guid FROM playerbot_saved_data WHERE guid = '%u'", m_bot->GetObjectGuid().GetCounter());
		if (!resultlvl)
			CharacterDatabase.DirectPExecute("INSERT INTO playerbot_saved_data (guid,combat_order,primary_target,secondary_target,pname,sname,combat_delay,active_spec) VALUES ('%u',0,0,0,'','',0,0)", m_bot->GetObjectGuid().GetCounter());
		else
			delete resultlvl;


		if (0 == GetTalentSpecsAmount())
		{
			SendWhisper("Database does not contain any Talent Specs (for any classes).", fromPlayer);
			return;
		}
		if (text.size() == 0) // no spec chosen nor other subcommand
		{
			std::list<TalentSpec> classSpecs = GetTalentSpecs((long)m_bot->getClass());
			std::list<TalentSpec>::iterator it;
			int count = 0;

			SendWhisper("Please select a talent spec to activate (reply 'talent spec #'):", fromPlayer);
			for (it = classSpecs.begin(); it != classSpecs.end(); it++)
			{
				count++;

				std::ostringstream oss;
				oss << count << ". " << it->specName;
				SendWhisper(oss.str(), fromPlayer);
			}
			if (count == 0)
			{
				std::ostringstream oss;
				oss << "Error: No TalentSpecs listed. Specs retrieved from DB for this class: %u" << m_bot->getClass();
				SendWhisper(oss.str(), fromPlayer);
			}
		}
		else
		{
			uint32 chosenSpec = strtoul(text.c_str(), nullptr, 0); // non-int returns 0; too big returns UINT MAX (or somesuch)

			// Warning: also catches non-int sub2command's - e.g. 'talent spec foobar'
			if (0 == chosenSpec)
			{
				ClearActiveTalentSpec();
				SendWhisper("The talent spec has been cleared.", fromPlayer);
				CharacterDatabase.DirectPExecute("UPDATE playerbot_saved_data SET active_spec = 0 WHERE guid = '%u'", m_bot->GetGUIDLow());
			}
			else if (chosenSpec > GetTalentSpecsAmount((long)m_bot->getClass()))
				SendWhisper("The talent spec you have chosen is invalid. Please select one from the valid range (reply 'talent spec' for options).", fromPlayer);
			else
			{
				TalentSpec ts = GetTalentSpec((long)m_bot->getClass(), chosenSpec);

				// no use setting it to an invalid (and probably - hopefully - empty) TalentSpec
				if (0 != ts.specClass && TSP_NONE != ts.specPurpose)
				{
					out << "Activated talent spec: " << chosenSpec << ". " << ts.specName;
					SendWhisper(out.str(), fromPlayer);
					SetActiveTalentSpec(ts);
					if (!ApplyActiveTalentSpec())
					{
						SendWhisper("The talent spec has been set active but could not be applied. It appears something has gone awry.", fromPlayer);
						DEBUG_LOG("[PlayerbotAI]: Could set TalentSpec but could not apply it - 'talent spec #': Class: %u; chosenSpec: %u", m_bot->getClass(), chosenSpec);
					}
					// Can't inspect talents in classic so we don't need to update it
					//InspectUpdate();

					// Update saved spec for the bot
					CharacterDatabase.DirectPExecute("UPDATE playerbot_saved_data SET active_spec = '%u' WHERE guid = '%u'", chosenSpec, m_bot->GetGUIDLow());
				}
				else
				{
					SendWhisper("An error has occured. Please let a Game Master know. This error has been logged.", fromPlayer);
					DEBUG_LOG("[PlayerbotAI]: Could not GetTalentSpec to set & apply - 'talent spec #': Class: %u; chosenSpec: %u", m_bot->getClass(), chosenSpec);
				}
			}
		}
	}
	// no valid subcommand found for command 'talent'
	else
	{
		uint32 gold = uint32(m_bot->resetTalentsCost() / 10000);

		if (gold > 0)
			out << "Cost to reset all Talents is " << gold << " |TInterface\\Icons\\INV_Misc_Coin_01:8|t";

		SendWhisper(out.str(), fromPlayer);
	}
}

void PlayerbotAI::FollowAutoReset()
{
	if (m_FollowAutoGo != FOLLOWAUTOGO_OFF)
	{
		m_FollowAutoGo = FOLLOWAUTOGO_RESET;
		SetMovementOrder(MOVEMENT_FOLLOW, GetMaster());
		WorldObject* distTarget = m_followTarget;
		for (uint8 i = 0; i < 1; ++i)
		{
			if (m_bot->GetDistance(distTarget) < 3.0f)
				i = 1;
			else
				i = 0;
		}
	}
}