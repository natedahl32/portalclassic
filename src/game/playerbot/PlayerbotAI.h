#ifndef _PLAYERBOTAI_H
#define _PLAYERBOTAI_H

#include "Common.h"
#include "../QuestDef.h"
#include "../GameEventMgr.h"
#include "../ObjectGuid.h"
#include "../Unit.h"
#include "../ItemPrototype.h"

class WorldPacket;
class WorldObject;
class Player;
class Unit;
class Object;
class Item;
class PlayerbotClassAI;
class PlayerbotMgr;

#define BOTLOOT_DISTANCE 75.0f

enum RacialTraits
{
    BERSERKING_ALL                 = 26297,
    BLOOD_FURY_ALL                 = 20572,
    ESCAPE_ARTIST_ALL              = 20589,
    PERCEPTION_ALL                 = 20600,
    SHADOWMELD_ALL                 = 20580,
    STONEFORM_ALL                  = 20594,
    WAR_STOMP_ALL                  = 20549,
    WILL_OF_THE_FORSAKEN_ALL       = 7744
};

enum ProfessionSpells
{
    ALCHEMY_1                      = 2259,
	ALCHEMY_2					   = 3101,
	ALCHEMY_3					   = 3464,
	ALCHEMY_4					   = 11611,
    BLACKSMITHING_1                = 2018,
	BLACKSMITHING_2				   = 3100,
	BLACKSMITHING_3				   = 3538,
	BLACKSMITHING_4				   = 9785,
    COOKING_1                      = 2550,
    ENCHANTING_1                   = 7411,
	ENCHANTING_2				   = 7412,
	ENCHANTING_3				   = 7413,
	ENCHANTING_4				   = 13920,
    ENGINEERING_1                  = 4036,
	ENGINEERING_2				   = 4037,
	ENGINEERING_3				   = 4038,
	ENGINEERING_4				   = 12656,
    FIRST_AID_1                    = 3273,
    FISHING_1                      = 7620,
    HERB_GATHERING_1               = 2366,
	HERB_GATHERING_2			   = 2368,
	HERB_GATHERING_3			   = 3570,
	HERB_GATHERING_4			   = 11993,
	LEATHERWORKING_1			   = 2108,
	LEATHERWORKING_2			   = 3104,
	LEATHERWORKING_3			   = 3811,
	LEATHERWORKING_4			   = 10662,
    MINING_1                       = 2575,
	MINING_2					   = 2576,
	MINING_3					   = 3564,
	MINING_4					   = 10248,
    SKINNING_1                     = 8613,
	SKINNING_2					   = 8617,
	SKINNING_3					   = 8618,
	SKINNING_4					   = 10768,
    TAILORING_1                    = 3908,
	TAILORING_2					   = 3909,
	TAILORING_3					   = 3910,
	TAILORING_4					   = 12180
};

enum TalentsDruid  // 0x400
{
	DRUID_FEROCITY = 796, // TabId = 281 Tab = 1 Row = 0 Column = 1
	DRUID_FERAL_AGGRESSION = 795, // TabId = 281 Tab = 1 Row = 0 Column = 2
	DRUID_FERAL_INSTINCT = 799, // TabId = 281 Tab = 1 Row = 1 Column = 0
	DRUID_BRUTAL_IMPACT = 797, // TabId = 281 Tab = 1 Row = 1 Column = 1
	DRUID_THICK_HIDE = 794, // TabId = 281 Tab = 1 Row = 1 Column = 2
	DRUID_FELINE_SWIFTNESS = 807, // TabId = 281 Tab = 1 Row = 2 Column = 0
	DRUID_FERAL_CHARGE = 804, // TabId = 281 Tab = 1 Row = 2 Column = 1
	DRUID_SHARPENED_CLAWS = 798, // TabId = 281 Tab = 1 Row = 2 Column = 2
	DRUID_IMPROVED_SHRED = 802, // TabId = 281 Tab = 1 Row = 3 Column = 0
	DRUID_PREDATORY_STRIKES = 803, // TabId = 281 Tab = 1 Row = 3 Column = 1
	DRUID_BLOOD_FRENZY = 800, // TabId = 281 Tab = 1 Row = 3 Column = 2
	DRUID_PRIMAL_FURY = 801, // TabId = 281 Tab = 1 Row = 3 Column = 3
	DRUID_SAVAGE_FURY = 805, // TabId = 281 Tab = 1 Row = 4 Column = 0
	DRUID_FAERIE_FIRE_FERAL = 1162, // TabId = 281 Tab = 1 Row = 4 Column = 2
	DRUID_HEART_OF_THE_WILD = 808, // TabId = 281 Tab = 1 Row = 5 Column = 1
	DRUID_LEADER_OF_THE_PACK = 809, // TabId = 281 Tab = 1 Row = 6 Column = 1
	DRUID_IMPROVED_MARK_OF_THE_WILD = 821, // TabId = 282 Tab = 2 Row = 0 Column = 1
	DRUID_FUROR = 822, // TabId = 282 Tab = 2 Row = 0 Column = 2
	DRUID_IMPROVED_HEALING_TOUCH = 824, // TabId = 282 Tab = 2 Row = 1 Column = 0
	DRUID_NATURES_FOCUS = 823, // TabId = 282 Tab = 2 Row = 1 Column = 1
	DRUID_IMPROVED_ENRAGE = 826, // TabId = 282 Tab = 2 Row = 1 Column = 2
	DRUID_REFLECTION = 829, // TabId = 282 Tab = 2 Row = 2 Column = 1
	DRUID_INSECT_SWARM = 827, // TabId = 282 Tab = 2 Row = 2 Column = 2
	DRUID_SUBTLETY = 841, // TabId = 282 Tab = 2 Row = 2 Column = 3
	DRUID_TRANQUIL_SPIRIT = 843, // TabId = 282 Tab = 2 Row = 3 Column = 1
	DRUID_IMPROVED_REJUVENATION = 830, // TabId = 282 Tab = 2 Row = 3 Column = 3
	DRUID_NATURES_SWIFTNESS = 831, // TabId = 282 Tab = 2 Row = 4 Column = 0
	DRUID_GIFT_OF_NATURE = 828, // TabId = 282 Tab = 2 Row = 4 Column = 2
	DRUID_IMPROVED_TRANQUILITY = 842, // TabId = 282 Tab = 2 Row = 4 Column = 3
	DRUID_IMPROVED_REGROWTH = 825, // TabId = 282 Tab = 2 Row = 5 Column = 2
	DRUID_SWIFTMEND = 844, // TabId = 282 Tab = 2 Row = 6 Column = 1
	DRUID_IMPROVED_WRATH = 762, // TabId = 283 Tab = 0 Row = 0 Column = 0
	DRUID_NATURES_GRASP = 761, // TabId = 283 Tab = 0 Row = 0 Column = 1
	DRUID_IMPROVED_NATURES_GRASP = 921, // TabId = 283 Tab = 0 Row = 0 Column = 2
	DRUID_IMPROVED_ENTANGLING_ROOTS = 787, // TabId = 283 Tab = 0 Row = 1 Column = 0
	DRUID_IMPROVED_MOONFIRE = 763, // TabId = 283 Tab = 0 Row = 1 Column = 1
	DRUID_NATURAL_WEAPONS = 791, // TabId = 283 Tab = 0 Row = 1 Column = 2
	DRUID_NATURAL_SHAPESHIFTER = 781, // TabId = 283 Tab = 0 Row = 1 Column = 3
	DRUID_IMPROVED_THORNS = 782, // TabId = 283 Tab = 0 Row = 2 Column = 0
	DRUID_OMEN_OF_CLARITY = 788, // TabId = 283 Tab = 0 Row = 2 Column = 2
	DRUID_NATURES_REACH = 764, // TabId = 283 Tab = 0 Row = 2 Column = 3
	DRUID_VENGEANCE = 792, // TabId = 283 Tab = 0 Row = 3 Column = 1
	DRUID_IMPROVED_STARFIRE = 784, // TabId = 283 Tab = 0 Row = 3 Column = 2
	DRUID_NATURES_GRACE = 789, // TabId = 283 Tab = 0 Row = 4 Column = 1
	DRUID_MOONGLOW = 783, // TabId = 283 Tab = 0 Row = 4 Column = 2
	DRUID_MOONFURY = 790, // TabId = 283 Tab = 0 Row = 5 Column = 1
	DRUID_MOONKIN_FORM = 793 // TabId = 283 Tab = 0 Row = 6 Column = 1
};

enum TalentsHunter  // 0x004
{
	HUNTER_IMPROVED_ASPECT_OF_THE_HAWK = 1382, // TabId = 361 Tab = 0 Row = 0 Column = 1
	HUNTER_ENDURANCE_TRAINING = 1389, // TabId = 361 Tab = 0 Row = 0 Column = 2
	HUNTER_IMPROVED_EYES_OF_THE_BEAST = 1624, // TabId = 361 Tab = 0 Row = 1 Column = 0
	HUNTER_IMPROVED_ASPECT_OF_THE_MONKEY = 1381, // TabId = 361 Tab = 0 Row = 1 Column = 1
	HUNTER_THICK_HIDE = 1395, // TabId = 361 Tab = 0 Row = 1 Column = 2
	HUNTER_IMPROVED_REVIVE_PET = 1625, // TabId = 361 Tab = 0 Row = 1 Column = 3
	HUNTER_PATHFINDING = 1384, // TabId = 361 Tab = 0 Row = 2 Column = 0
	HUNTER_BESTIAL_SWIFTNESS = 1391, // TabId = 361 Tab = 0 Row = 2 Column = 1
	HUNTER_UNLEASHED_FURY = 1396, // TabId = 361 Tab = 0 Row = 2 Column = 2
	HUNTER_IMPROVED_MEND_PET = 1385, // TabId = 361 Tab = 0 Row = 3 Column = 1
	HUNTER_FEROCITY = 1393, // TabId = 361 Tab = 0 Row = 3 Column = 2
	HUNTER_SPIRIT_BOND = 1388, // TabId = 361 Tab = 0 Row = 4 Column = 0
	HUNTER_INTIMIDATION = 1387, // TabId = 361 Tab = 0 Row = 4 Column = 1
	HUNTER_BESTIAL_DISCIPLINE = 1390, // TabId = 361 Tab = 0 Row = 4 Column = 3
	HUNTER_FRENZY = 1397, // TabId = 361 Tab = 0 Row = 5 Column = 2
	HUNTER_BESTIAL_WRATH = 1386, // TabId = 361 Tab = 0 Row = 6 Column = 1
	HUNTER_MONSTER_SLAYING = 1623, // TabId = 362 Tab = 2 Row = 0 Column = 0
	HUNTER_HUMANOID_SLAYING = 1301, // TabId = 362 Tab = 2 Row = 0 Column = 1
	HUNTER_DEFLECTION = 1311, // TabId = 362 Tab = 2 Row = 0 Column = 2
	HUNTER_ENTRAPMENT = 1304, // TabId = 362 Tab = 2 Row = 1 Column = 0
	HUNTER_SAVAGE_STRIKES = 1621, // TabId = 362 Tab = 2 Row = 1 Column = 1
	HUNTER_IMPROVED_WING_CLIP = 1305, // TabId = 362 Tab = 2 Row = 1 Column = 2
	HUNTER_CLEVER_TRAPS = 1306, // TabId = 362 Tab = 2 Row = 2 Column = 0
	HUNTER_SURVIVALIST = 1622, // TabId = 362 Tab = 2 Row = 2 Column = 1
	HUNTER_DETERRENCE = 1308, // TabId = 362 Tab = 2 Row = 2 Column = 2
	HUNTER_TRAP_MASTERY = 1322, // TabId = 362 Tab = 2 Row = 3 Column = 0
	HUNTER_SUREFOOTED = 1310, // TabId = 362 Tab = 2 Row = 3 Column = 1
	HUNTER_IMPROVED_FEIGN_DEATH = 1309, // TabId = 362 Tab = 2 Row = 3 Column = 3
	HUNTER_KILLER_INSTINCT = 1321, // TabId = 362 Tab = 2 Row = 4 Column = 1
	HUNTER_COUNTERATTACK = 1312, // TabId = 362 Tab = 2 Row = 4 Column = 2
	HUNTER_LIGHTNING_REFLEXES = 1303, // TabId = 362 Tab = 2 Row = 5 Column = 2
	HUNTER_WYVERN_STING = 1325, // TabId = 362 Tab = 2 Row = 6 Column = 1
	HUNTER_IMPROVED_CONCUSSIVE_SHOT = 1341, // TabId = 363 Tab = 1 Row = 0 Column = 1
	HUNTER_EFFICIENCY = 1342, // TabId = 363 Tab = 1 Row = 0 Column = 2
	HUNTER_IMPROVED_HUNTERS_MARK = 1343, // TabId = 363 Tab = 1 Row = 1 Column = 1
	HUNTER_LETHAL_SHOTS = 1344, // TabId = 363 Tab = 1 Row = 1 Column = 2
	HUNTER_AIMED_SHOT = 1345, // TabId = 363 Tab = 1 Row = 2 Column = 0
	HUNTER_IMPROVED_ARCANE_SHOT = 1346, // TabId = 363 Tab = 1 Row = 2 Column = 1
	HUNTER_HAWK_EYE = 1352, // TabId = 363 Tab = 1 Row = 2 Column = 3
	HUNTER_IMPROVED_SERPENT_STING = 1348, // TabId = 363 Tab = 1 Row = 3 Column = 1
	HUNTER_MORTAL_SHOTS = 1349, // TabId = 363 Tab = 1 Row = 3 Column = 2
	HUNTER_SCATTER_SHOT = 1353, // TabId = 363 Tab = 1 Row = 4 Column = 0
	HUNTER_BARRAGE = 1347, // TabId = 363 Tab = 1 Row = 4 Column = 1
	HUNTER_IMPROVED_SCORPID_STING = 1351, // TabId = 363 Tab = 1 Row = 4 Column = 2
	HUNTER_RANGED_WEAPON_SPECIALIZATION = 1362, // TabId = 363 Tab = 1 Row = 5 Column = 2
	HUNTER_TRUESHOT_AURA = 1361 // TabId = 363 Tab = 1 Row = 6 Column = 1
};

enum TalentsMage  // 0x080
{
	MAGE_IMPROVED_FIREBALL = 26, // TabId = 41 Tab = 1 Row = 0 Column = 1
	MAGE_IMPACT = 30, // TabId = 41 Tab = 1 Row = 0 Column = 2
	MAGE_IGNITE = 34, // TabId = 41 Tab = 1 Row = 1 Column = 0
	MAGE_FLAME_THROWING = 28, // TabId = 41 Tab = 1 Row = 1 Column = 1
	MAGE_IMPROVED_FIRE_BLAST = 27, // TabId = 41 Tab = 1 Row = 1 Column = 2
	MAGE_INCINERATE = 1141, // TabId = 41 Tab = 1 Row = 2 Column = 0
	MAGE_IMPROVED_FLAMESTRIKE = 31, // TabId = 41 Tab = 1 Row = 2 Column = 1
	MAGE_PYROBLAST = 29, // TabId = 41 Tab = 1 Row = 2 Column = 2
	MAGE_BURNING_SOUL = 23, // TabId = 41 Tab = 1 Row = 2 Column = 3
	MAGE_IMPROVED_SCORCH = 25, // TabId = 41 Tab = 1 Row = 3 Column = 0
	MAGE_IMPROVED_FIRE_WARD = 24, // TabId = 41 Tab = 1 Row = 3 Column = 1
	MAGE_MASTER_OF_ELEMENTS = 1639, // TabId = 41 Tab = 1 Row = 3 Column = 3
	MAGE_CRITICAL_MASS = 33, // TabId = 41 Tab = 1 Row = 4 Column = 1
	MAGE_BLAST_WAVE = 32, // TabId = 41 Tab = 1 Row = 4 Column = 2
	MAGE_FIRE_POWER = 35, // TabId = 41 Tab = 1 Row = 5 Column = 2
	MAGE_COMBUSTION = 36, // TabId = 41 Tab = 1 Row = 6 Column = 1
	MAGE_FROST_WARDING = 70, // TabId = 61 Tab = 2 Row = 0 Column = 0
	MAGE_IMPROVED_FROSTBOLT = 37, // TabId = 61 Tab = 2 Row = 0 Column = 1
	MAGE_ELEMENTAL_PRECISION = 1649, // TabId = 61 Tab = 2 Row = 0 Column = 2
	MAGE_ICE_SHARDS = 73, // TabId = 61 Tab = 2 Row = 1 Column = 0
	MAGE_FROSTBITE = 38, // TabId = 61 Tab = 2 Row = 1 Column = 1
	MAGE_IMPROVED_FROST_NOVA = 62, // TabId = 61 Tab = 2 Row = 1 Column = 2
	MAGE_PERMAFROST = 65, // TabId = 61 Tab = 2 Row = 1 Column = 3
	MAGE_PIERCING_ICE = 61, // TabId = 61 Tab = 2 Row = 2 Column = 0
	MAGE_COLD_SNAP = 69, // TabId = 61 Tab = 2 Row = 2 Column = 1
	MAGE_IMPROVED_BLIZZARD = 63, // TabId = 61 Tab = 2 Row = 2 Column = 3
	MAGE_ARCTIC_REACH = 741, // TabId = 61 Tab = 2 Row = 3 Column = 0
	MAGE_FROST_CHANNELING = 66, // TabId = 61 Tab = 2 Row = 3 Column = 1
	MAGE_SHATTER = 67, // TabId = 61 Tab = 2 Row = 3 Column = 2
	MAGE_ICE_BLOCK = 72, // TabId = 61 Tab = 2 Row = 4 Column = 1
	MAGE_IMPROVED_CONE_OF_COLD = 64, // TabId = 61 Tab = 2 Row = 4 Column = 2
	MAGE_WINTERS_CHILL = 68, // TabId = 61 Tab = 2 Row = 5 Column = 2
	MAGE_ICE_BARRIER = 71, // TabId = 61 Tab = 2 Row = 6 Column = 1
	MAGE_ARCANE_SUBTLETY = 74, // TabId = 81 Tab = 0 Row = 0 Column = 0
	MAGE_ARCANE_FOCUS = 76, // TabId = 81 Tab = 0 Row = 0 Column = 1
	MAGE_IMPROVED_ARCANE_MISSILES = 80, // TabId = 81 Tab = 0 Row = 0 Column = 2
	MAGE_WAND_SPECIALIZATION = 78, // TabId = 81 Tab = 0 Row = 1 Column = 0
	MAGE_MAGIC_ABSORPTION = 1650, // TabId = 81 Tab = 0 Row = 1 Column = 1
	MAGE_ARCANE_CONCENTRATION = 75, // TabId = 81 Tab = 0 Row = 1 Column = 2
	MAGE_MAGIC_ATTUNEMENT = 82, // TabId = 81 Tab = 0 Row = 2 Column = 0
	MAGE_IMPROVED_ARCANE_EXPLOSION = 81, // TabId = 81 Tab = 0 Row = 2 Column = 1
	MAGE_ARCANE_RESILIENCE = 85, // TabId = 81 Tab = 0 Row = 2 Column = 2
	MAGE_IMPROVED_MANA_SHIELD = 83, // TabId = 81 Tab = 0 Row = 3 Column = 0
	MAGE_IMPROVED_COUNTERSPELL = 88, // TabId = 81 Tab = 0 Row = 3 Column = 1
	MAGE_ARCANE_MEDITATION = 1142, // TabId = 81 Tab = 0 Row = 3 Column = 3
	MAGE_PRESENCE_OF_MIND = 86, // TabId = 81 Tab = 0 Row = 4 Column = 1
	MAGE_ARCANE_MIND = 77, // TabId = 81 Tab = 0 Row = 4 Column = 2
	MAGE_ARCANE_INSTABILITY = 421, // TabId = 81 Tab = 0 Row = 5 Column = 1
	MAGE_ARCANE_POWER = 87 // TabId = 81 Tab = 0 Row = 6 Column = 1
};

enum TalentsPaladin  // 0x002
{
	PALADIN_IMPROVED_BLESSING_OF_MIGHT = 1401, // TabId = 381 Tab = 2 Row = 0 Column = 1
	PALADIN_BENEDICTION = 1407, // TabId = 381 Tab = 2 Row = 0 Column = 2
	PALADIN_IMPROVED_JUDGEMENT = 1631, // TabId = 381 Tab = 2 Row = 1 Column = 0
	PALADIN_IMPROVED_SEAL_OF_THE_CRUSADER = 1464, // TabId = 381 Tab = 2 Row = 1 Column = 1
	PALADIN_DEFLECTION = 1403, // TabId = 381 Tab = 2 Row = 1 Column = 2
	PALADIN_VINDICATION = 1633, // TabId = 381 Tab = 2 Row = 2 Column = 0
	PALADIN_CONVICTION = 1411, // TabId = 381 Tab = 2 Row = 2 Column = 1
	PALADIN_SEAL_OF_COMMAND = 1481, // TabId = 381 Tab = 2 Row = 2 Column = 2
	PALADIN_PURSUIT_OF_JUSTICE = 1634, // TabId = 381 Tab = 2 Row = 2 Column = 3
	PALADIN_EYE_FOR_AN_EYE = 1632, // TabId = 381 Tab = 2 Row = 3 Column = 0
	PALADIN_IMPROVED_RETRIBUTION_AURA = 1405, // TabId = 381 Tab = 2 Row = 3 Column = 2
	PALADIN_TWO_HANDED_WEAPON_SPECIALIZATION = 1410, // TabId = 381 Tab = 2 Row = 4 Column = 0
	PALADIN_SANCTITY_AURA = 1409, // TabId = 381 Tab = 2 Row = 4 Column = 2
	PALADIN_VENGEANCE = 1402, // TabId = 381 Tab = 2 Row = 5 Column = 1
	PALADIN_REPENTANCE = 1441, // TabId = 381 Tab = 2 Row = 6 Column = 1
	PALADIN_DIVINE_STRENGTH = 1450, // TabId = 382 Tab = 0 Row = 0 Column = 1
	PALADIN_DIVINE_INTELLECT = 1449, // TabId = 382 Tab = 0 Row = 0 Column = 2
	PALADIN_SPIRITUAL_FOCUS = 1432, // TabId = 382 Tab = 0 Row = 1 Column = 1
	PALADIN_IMPROVED_SEAL_OF_RIGHTEOUSNESS = 1463, // TabId = 382 Tab = 0 Row = 1 Column = 2
	PALADIN_HEALING_LIGHT = 1444, // TabId = 382 Tab = 0 Row = 2 Column = 0
	PALADIN_CONSECRATION = 1435, // TabId = 382 Tab = 0 Row = 2 Column = 1
	PALADIN_IMPROVED_LAY_ON_HANDS = 1443, // TabId = 382 Tab = 0 Row = 2 Column = 2
	PALADIN_UNYIELDING_FAITH = 1628, // TabId = 382 Tab = 0 Row = 2 Column = 3
	PALADIN_ILLUMINATION = 1461, // TabId = 382 Tab = 0 Row = 3 Column = 1
	PALADIN_IMPROVED_BLESSING_OF_WISDOM = 1446, // TabId = 382 Tab = 0 Row = 3 Column = 2
	PALADIN_DIVINE_FAVOR = 1433, // TabId = 382 Tab = 0 Row = 4 Column = 1
	PALADIN_LASTING_JUDGEMENT = 1465, // TabId = 382 Tab = 0 Row = 4 Column = 2
	PALADIN_HOLY_POWER = 1627, // TabId = 382 Tab = 0 Row = 5 Column = 2
	PALADIN_HOLY_SHOCK = 1502, // TabId = 382 Tab = 0 Row = 6 Column = 1
	PALADIN_IMPROVED_DEVOTION_AURA = 1422, // TabId = 383 Tab = 1 Row = 0 Column = 1
	PALADIN_REDOUBT = 1421, // TabId = 383 Tab = 1 Row = 0 Column = 2
	PALADIN_PRECISION = 1630, // TabId = 383 Tab = 1 Row = 1 Column = 0
	PALADIN_GUARDIANS_FAVOR = 1425, // TabId = 383 Tab = 1 Row = 1 Column = 1
	PALADIN_TOUGHNESS = 1423, // TabId = 383 Tab = 1 Row = 1 Column = 3
	PALADIN_BLESSING_OF_KINGS = 1442, // TabId = 383 Tab = 1 Row = 2 Column = 0
	PALADIN_IMPROVED_RIGHTEOUS_FURY = 1501, // TabId = 383 Tab = 1 Row = 2 Column = 1
	PALADIN_SHIELD_SPECIALIZATION = 1424, // TabId = 383 Tab = 1 Row = 2 Column = 2
	PALADIN_ANTICIPATION = 1629, // TabId = 383 Tab = 1 Row = 2 Column = 3
	PALADIN_IMPROVED_HAMMER_OF_JUSTICE = 1521, // TabId = 383 Tab = 1 Row = 3 Column = 1
	PALADIN_IMPROVED_CONCENTRATION_AURA = 1626, // TabId = 383 Tab = 1 Row = 3 Column = 2
	PALADIN_BLESSING_OF_SANCTUARY = 1431, // TabId = 383 Tab = 1 Row = 4 Column = 1
	PALADIN_RECKONING = 1426, // TabId = 383 Tab = 1 Row = 4 Column = 2
	PALADIN_ONE_HANDED_WEAPON_SPECIALIZATION = 1429, // TabId = 383 Tab = 1 Row = 5 Column = 2
	PALADIN_HOLY_SHIELD = 1430 // TabId = 383 Tab = 1 Row = 6 Column = 1
};

enum TalentsPriest  // 0x010
{
	PRIEST_UNBREAKABLE_WILL = 342, // TabId = 201 Tab = 0 Row = 0 Column = 1
	PRIEST_WAND_SPECIALIZATION = 345, // TabId = 201 Tab = 0 Row = 0 Column = 2
	PRIEST_SILENT_RESOLVE = 352, // TabId = 201 Tab = 0 Row = 1 Column = 0
	PRIEST_IMPROVED_POWER_WORD_FORTITUDE = 344, // TabId = 201 Tab = 0 Row = 1 Column = 1
	PRIEST_IMPROVED_POWER_WORD_SHIELD = 343, // TabId = 201 Tab = 0 Row = 1 Column = 2
	PRIEST_MARTYRDOM = 321, // TabId = 201 Tab = 0 Row = 1 Column = 3
	PRIEST_INNER_FOCUS = 348, // TabId = 201 Tab = 0 Row = 2 Column = 1
	PRIEST_MEDITATION = 347, // TabId = 201 Tab = 0 Row = 2 Column = 2
	PRIEST_IMPROVED_INNER_FIRE = 346, // TabId = 201 Tab = 0 Row = 3 Column = 0
	PRIEST_MENTAL_AGILITY = 341, // TabId = 201 Tab = 0 Row = 3 Column = 1
	PRIEST_IMPROVED_MANA_BURN = 350, // TabId = 201 Tab = 0 Row = 3 Column = 3
	PRIEST_MENTAL_STRENGTH = 1201, // TabId = 201 Tab = 0 Row = 4 Column = 1
	PRIEST_DIVINE_SPIRIT = 351, // TabId = 201 Tab = 0 Row = 4 Column = 2
	PRIEST_FORCE_OF_WILL = 1202, // TabId = 201 Tab = 0 Row = 5 Column = 2
	PRIEST_POWER_INFUSION = 322, // TabId = 201 Tab = 0 Row = 6 Column = 1
	PRIEST_HEALING_FOCUS = 410, // TabId = 202 Tab = 1 Row = 0 Column = 0
	PRIEST_IMPROVED_RENEW = 406, // TabId = 202 Tab = 1 Row = 0 Column = 1
	PRIEST_HOLY_SPECIALIZATION = 401, // TabId = 202 Tab = 1 Row = 0 Column = 2
	PRIEST_SPELL_WARDING = 411, // TabId = 202 Tab = 1 Row = 1 Column = 1
	PRIEST_DIVINE_FURY = 1181, // TabId = 202 Tab = 1 Row = 1 Column = 2
	PRIEST_HOLY_NOVA = 442, // TabId = 202 Tab = 1 Row = 2 Column = 0
	PRIEST_BLESSED_RECOVERY = 1636, // TabId = 202 Tab = 1 Row = 2 Column = 1
	PRIEST_INSPIRATION = 361, // TabId = 202 Tab = 1 Row = 2 Column = 3
	PRIEST_HOLY_REACH = 1635, // TabId = 202 Tab = 1 Row = 3 Column = 0
	PRIEST_IMPROVED_HEALING = 408, // TabId = 202 Tab = 1 Row = 3 Column = 1
	PRIEST_SEARING_LIGHT = 403, // TabId = 202 Tab = 1 Row = 3 Column = 2
	PRIEST_IMPROVED_PRAYER_OF_HEALING = 413, // TabId = 202 Tab = 1 Row = 4 Column = 0
	PRIEST_SPIRIT_OF_REDEMPTION = 1561, // TabId = 202 Tab = 1 Row = 4 Column = 1
	PRIEST_SPIRITUAL_GUIDANCE = 402, // TabId = 202 Tab = 1 Row = 4 Column = 2
	PRIEST_SPIRITUAL_HEALING = 404, // TabId = 202 Tab = 1 Row = 5 Column = 2
	PRIEST_LIGHTWELL = 1637, // TabId = 202 Tab = 1 Row = 6 Column = 1
	PRIEST_SPIRIT_TAP = 465, // TabId = 203 Tab = 2 Row = 0 Column = 1
	PRIEST_BLACKOUT = 464, // TabId = 203 Tab = 2 Row = 0 Column = 2
	PRIEST_SHADOW_AFFINITY = 466, // TabId = 203 Tab = 2 Row = 1 Column = 0
	PRIEST_IMPROVED_SHADOW_WORD_PAIN = 482, // TabId = 203 Tab = 2 Row = 1 Column = 1
	PRIEST_SHADOW_FOCUS = 463, // TabId = 203 Tab = 2 Row = 1 Column = 2
	PRIEST_IMPROVED_PSYCHIC_SCREAM = 542, // TabId = 203 Tab = 2 Row = 2 Column = 0
	PRIEST_IMPROVED_MIND_BLAST = 481, // TabId = 203 Tab = 2 Row = 2 Column = 1
	PRIEST_MIND_FLAY = 501, // TabId = 203 Tab = 2 Row = 2 Column = 2
	PRIEST_IMPROVED_FADE = 483, // TabId = 203 Tab = 2 Row = 3 Column = 1
	PRIEST_SHADOW_REACH = 881, // TabId = 203 Tab = 2 Row = 3 Column = 2
	PRIEST_SHADOW_WEAVING = 461, // TabId = 203 Tab = 2 Row = 3 Column = 3
	PRIEST_SILENCE = 541, // TabId = 203 Tab = 2 Row = 4 Column = 0
	PRIEST_VAMPIRIC_EMBRACE = 484, // TabId = 203 Tab = 2 Row = 4 Column = 1
	PRIEST_IMPROVED_VAMPIRIC_EMBRACE = 1638, // TabId = 203 Tab = 2 Row = 4 Column = 2
	PRIEST_DARKNESS = 462, // TabId = 203 Tab = 2 Row = 5 Column = 2
	PRIEST_SHADOWFORM = 521 // TabId = 203 Tab = 2 Row = 6 Column = 1
};

enum TalentsRogue  // 0x008
{
	ROGUE_IMPROVED_GOUGE = 203, // TabId = 181 Tab = 1 Row = 0 Column = 0
	ROGUE_IMPROVED_SINISTER_STRIKE = 201, // TabId = 181 Tab = 1 Row = 0 Column = 1
	ROGUE_LIGHTNING_REFLEXES = 186, // TabId = 181 Tab = 1 Row = 0 Column = 2
	ROGUE_IMPROVED_BACKSTAB = 202, // TabId = 181 Tab = 1 Row = 1 Column = 0
	ROGUE_DEFLECTION = 187, // TabId = 181 Tab = 1 Row = 1 Column = 1
	ROGUE_PRECISION = 181, // TabId = 181 Tab = 1 Row = 1 Column = 2
	ROGUE_ENDURANCE = 204, // TabId = 181 Tab = 1 Row = 2 Column = 0
	ROGUE_RIPOSTE = 301, // TabId = 181 Tab = 1 Row = 2 Column = 1
	ROGUE_IMPROVED_SPRINT = 222, // TabId = 181 Tab = 1 Row = 2 Column = 3
	ROGUE_IMPROVED_KICK = 206, // TabId = 181 Tab = 1 Row = 3 Column = 0
	ROGUE_DAGGER_SPECIALIZATION = 182, // TabId = 181 Tab = 1 Row = 3 Column = 1
	ROGUE_DUAL_WIELD_SPECIALIZATION = 221, // TabId = 181 Tab = 1 Row = 3 Column = 2
	ROGUE_MACE_SPECIALIZATION = 184, // TabId = 181 Tab = 1 Row = 4 Column = 0
	ROGUE_BLADE_FLURRY = 223, // TabId = 181 Tab = 1 Row = 4 Column = 1
	ROGUE_SWORD_SPECIALIZATION = 242, // TabId = 181 Tab = 1 Row = 4 Column = 2
	ROGUE_FIST_WEAPON_SPECIALIZATION = 183, // TabId = 181 Tab = 1 Row = 4 Column = 3
	ROGUE_WEAPON_EXPERTISE = 1703, // TabId = 181 Tab = 1 Row = 5 Column = 1
	ROGUE_AGGRESSION = 1122, // TabId = 181 Tab = 1 Row = 5 Column = 2
	ROGUE_ADRENALINE_RUSH = 205, // TabId = 181 Tab = 1 Row = 6 Column = 1
	ROGUE_IMPROVED_EVISCERATE = 276, // TabId = 182 Tab = 0 Row = 0 Column = 0
	ROGUE_REMORSELESS_ATTACKS = 272, // TabId = 182 Tab = 0 Row = 0 Column = 1
	ROGUE_MALICE = 270, // TabId = 182 Tab = 0 Row = 0 Column = 2
	ROGUE_RUTHLESSNESS = 273, // TabId = 182 Tab = 0 Row = 1 Column = 0
	ROGUE_MURDER = 274, // TabId = 182 Tab = 0 Row = 1 Column = 1
	ROGUE_IMPROVED_SLICE_AND_DICE = 277, // TabId = 182 Tab = 0 Row = 1 Column = 3
	ROGUE_RELENTLESS_STRIKES = 281, // TabId = 182 Tab = 0 Row = 2 Column = 0
	ROGUE_IMPROVED_EXPOSE_ARMOR = 278, // TabId = 182 Tab = 0 Row = 2 Column = 1
	ROGUE_LETHALITY = 269, // TabId = 182 Tab = 0 Row = 2 Column = 2
	ROGUE_VILE_POISONS = 682, // TabId = 182 Tab = 0 Row = 3 Column = 1
	ROGUE_IMPROVED_POISONS = 268, // TabId = 182 Tab = 0 Row = 3 Column = 2
	ROGUE_COLD_BLOOD = 280, // TabId = 182 Tab = 0 Row = 4 Column = 1
	ROGUE_IMPROVED_KIDNEY_SHOT = 279, // TabId = 182 Tab = 0 Row = 4 Column = 2
	ROGUE_SEAL_FATE = 283, // TabId = 182 Tab = 0 Row = 5 Column = 1
	ROGUE_VIGOR = 382, // TabId = 182 Tab = 0 Row = 6 Column = 1
	ROGUE_MASTER_OF_DECEPTION = 241, // TabId = 183 Tab = 2 Row = 0 Column = 1
	ROGUE_OPPORTUNITY = 261, // TabId = 183 Tab = 2 Row = 0 Column = 2
	ROGUE_SLEIGHT_OF_HAND = 1700, // TabId = 183 Tab = 2 Row = 1 Column = 0
	ROGUE_ELUSIVENESS = 247, // TabId = 183 Tab = 2 Row = 1 Column = 1
	ROGUE_CAMOUFLAGE = 244, // TabId = 183 Tab = 2 Row = 1 Column = 2
	ROGUE_INITIATIVE = 245, // TabId = 183 Tab = 2 Row = 2 Column = 0
	ROGUE_GHOSTLY_STRIKE = 303, // TabId = 183 Tab = 2 Row = 2 Column = 1
	ROGUE_IMPROVED_AMBUSH = 263, // TabId = 183 Tab = 2 Row = 2 Column = 2
	ROGUE_SETUP = 246, // TabId = 183 Tab = 2 Row = 3 Column = 0
	ROGUE_IMPROVED_SAP = 262, // TabId = 183 Tab = 2 Row = 3 Column = 1
	ROGUE_SERRATED_BLADES = 1123, // TabId = 183 Tab = 2 Row = 3 Column = 2
	ROGUE_HEIGHTENED_SENSES = 1701, // TabId = 183 Tab = 2 Row = 4 Column = 0
	ROGUE_PREPARATION = 284, // TabId = 183 Tab = 2 Row = 4 Column = 1
	ROGUE_DIRTY_DEEDS = 265, // TabId = 183 Tab = 2 Row = 4 Column = 2
	ROGUE_HEMORRHAGE = 681, // TabId = 183 Tab = 2 Row = 4 Column = 3
	ROGUE_DEADLINESS = 1702, // TabId = 183 Tab = 2 Row = 5 Column = 2
	ROGUE_PREMEDITATION = 381 // TabId = 183 Tab = 2 Row = 6 Column = 1
};

enum TalentsShaman  // 0x040
{
	SHAMAN_CONVECTION = 564, // TabId = 261 Tab = 0 Row = 0 Column = 1
	SHAMAN_CONCUSSION = 563, // TabId = 261 Tab = 0 Row = 0 Column = 2
	SHAMAN_EARTHS_GRASP = 572, // TabId = 261 Tab = 0 Row = 1 Column = 0
	SHAMAN_ELEMENTAL_WARDING = 1640, // TabId = 261 Tab = 0 Row = 1 Column = 1
	SHAMAN_CALL_OF_FLAME = 561, // TabId = 261 Tab = 0 Row = 1 Column = 2
	SHAMAN_ELEMENTAL_FOCUS = 574, // TabId = 261 Tab = 0 Row = 2 Column = 0
	SHAMAN_REVERBERATION = 575, // TabId = 261 Tab = 0 Row = 2 Column = 1
	SHAMAN_CALL_OF_THUNDER = 562, // TabId = 261 Tab = 0 Row = 2 Column = 2
	SHAMAN_IMPROVED_FIRE_TOTEMS = 567, // TabId = 261 Tab = 0 Row = 3 Column = 0
	SHAMAN_EYE_OF_THE_STORM = 1642, // TabId = 261 Tab = 0 Row = 3 Column = 1
	SHAMAN_ELEMENTAL_DEVASTATION = 1645, // TabId = 261 Tab = 0 Row = 3 Column = 3
	SHAMAN_STORM_REACH = 1641, // TabId = 261 Tab = 0 Row = 4 Column = 0
	SHAMAN_ELEMENTAL_FURY = 565, // TabId = 261 Tab = 0 Row = 4 Column = 1
	SHAMAN_LIGHTNING_MASTERY = 721, // TabId = 261 Tab = 0 Row = 5 Column = 2
	SHAMAN_ELEMENTAL_MASTERY = 573, // TabId = 261 Tab = 0 Row = 6 Column = 1
	SHAMAN_IMPROVED_HEALING_WAVE = 586, // TabId = 262 Tab = 2 Row = 0 Column = 1
	SHAMAN_TIDAL_FOCUS = 593, // TabId = 262 Tab = 2 Row = 0 Column = 2
	SHAMAN_IMPROVED_REINCARNATION = 589, // TabId = 262 Tab = 2 Row = 1 Column = 0
	SHAMAN_ANCESTRAL_HEALING = 581, // TabId = 262 Tab = 2 Row = 1 Column = 1
	SHAMAN_TOTEMIC_FOCUS = 595, // TabId = 262 Tab = 2 Row = 1 Column = 2
	SHAMAN_NATURES_GUIDANCE = 583, // TabId = 262 Tab = 2 Row = 2 Column = 0
	SHAMAN_HEALING_FOCUS = 587, // TabId = 262 Tab = 2 Row = 2 Column = 1
	SHAMAN_TOTEMIC_MASTERY = 582, // TabId = 262 Tab = 2 Row = 2 Column = 2
	SHAMAN_HEALING_GRACE = 1646, // TabId = 262 Tab = 2 Row = 2 Column = 3
	SHAMAN_RESTORATIVE_TOTEMS = 588, // TabId = 262 Tab = 2 Row = 3 Column = 1
	SHAMAN_TIDAL_MASTERY = 594, // TabId = 262 Tab = 2 Row = 3 Column = 2
	SHAMAN_HEALING_WAY = 1648, // TabId = 262 Tab = 2 Row = 4 Column = 0
	SHAMAN_NATURES_SWIFTNESS = 591, // TabId = 262 Tab = 2 Row = 4 Column = 2
	SHAMAN_PURIFICATION = 592, // TabId = 262 Tab = 2 Row = 5 Column = 2
	SHAMAN_MANA_TIDE_TOTEM = 590, // TabId = 262 Tab = 2 Row = 6 Column = 1
	SHAMAN_ANCESTRAL_KNOWLEDGE = 614, // TabId = 263 Tab = 1 Row = 0 Column = 1
	SHAMAN_SHIELD_SPECIALIZATION = 612, // TabId = 263 Tab = 1 Row = 0 Column = 2
	SHAMAN_GUARDIAN_TOTEMS = 609, // TabId = 263 Tab = 1 Row = 1 Column = 0
	SHAMAN_THUNDERING_STRIKES = 613, // TabId = 263 Tab = 1 Row = 1 Column = 1
	SHAMAN_IMPROVED_GHOST_WOLF = 605, // TabId = 263 Tab = 1 Row = 1 Column = 2
	SHAMAN_IMPROVED_LIGHTNING_SHIELD = 607, // TabId = 263 Tab = 1 Row = 1 Column = 3
	SHAMAN_ENHANCING_TOTEMS = 610, // TabId = 263 Tab = 1 Row = 2 Column = 0
	SHAMAN_TWO_HANDED_AXES_AND_MACES = 617, // TabId = 263 Tab = 1 Row = 2 Column = 2
	SHAMAN_ANTICIPATION = 601, // TabId = 263 Tab = 1 Row = 2 Column = 3
	SHAMAN_FLURRY = 602, // TabId = 263 Tab = 1 Row = 3 Column = 1
	SHAMAN_TOUGHNESS = 615, // TabId = 263 Tab = 1 Row = 3 Column = 2
	SHAMAN_IMPROVED_WEAPON_TOTEMS = 1647, // TabId = 263 Tab = 1 Row = 4 Column = 0
	SHAMAN_ELEMENTAL_WEAPONS = 611, // TabId = 263 Tab = 1 Row = 4 Column = 1
	SHAMAN_PARRY = 616, // TabId = 263 Tab = 1 Row = 4 Column = 2
	SHAMAN_WEAPON_MASTERY = 1643, // TabId = 263 Tab = 1 Row = 5 Column = 2
	SHAMAN_STORMSTRIKE = 901 // TabId = 263 Tab = 1 Row = 6 Column = 1
};

enum TalentsWarlock  // 0x100
{
	WARLOCK_IMPROVED_SHADOW_BOLT = 944, // TabId = 301 Tab = 2 Row = 0 Column = 1
	WARLOCK_CATACLYSM = 941, // TabId = 301 Tab = 2 Row = 0 Column = 2
	WARLOCK_BANE = 943, // TabId = 301 Tab = 2 Row = 1 Column = 1
	WARLOCK_AFTERMATH = 982, // TabId = 301 Tab = 2 Row = 1 Column = 2
	WARLOCK_IMPROVED_FIREBOLT = 983, // TabId = 301 Tab = 2 Row = 2 Column = 0
	WARLOCK_IMPROVED_LASH_OF_PAIN = 984, // TabId = 301 Tab = 2 Row = 2 Column = 1
	WARLOCK_DEVASTATION = 981, // TabId = 301 Tab = 2 Row = 2 Column = 2
	WARLOCK_SHADOWBURN = 963, // TabId = 301 Tab = 2 Row = 2 Column = 3
	WARLOCK_INTENSITY = 985, // TabId = 301 Tab = 2 Row = 3 Column = 0
	WARLOCK_DESTRUCTIVE_REACH = 964, // TabId = 301 Tab = 2 Row = 3 Column = 1
	WARLOCK_IMPROVED_SEARING_PAIN = 965, // TabId = 301 Tab = 2 Row = 3 Column = 3
	WARLOCK_PYROCLASM = 986, // TabId = 301 Tab = 2 Row = 4 Column = 0
	WARLOCK_IMPROVED_IMMOLATE = 961, // TabId = 301 Tab = 2 Row = 4 Column = 1
	WARLOCK_RUIN = 967, // TabId = 301 Tab = 2 Row = 4 Column = 2
	WARLOCK_EMBERSTORM = 966, // TabId = 301 Tab = 2 Row = 5 Column = 2
	WARLOCK_CONFLAGRATE = 968, // TabId = 301 Tab = 2 Row = 6 Column = 1
	WARLOCK_SUPPRESSION = 1005, // TabId = 302 Tab = 0 Row = 0 Column = 1
	WARLOCK_IMPROVED_CORRUPTION = 1003, // TabId = 302 Tab = 0 Row = 0 Column = 2
	WARLOCK_IMPROVED_CURSE_OF_WEAKNESS = 1006, // TabId = 302 Tab = 0 Row = 1 Column = 0
	WARLOCK_IMPROVED_DRAIN_SOUL = 1101, // TabId = 302 Tab = 0 Row = 1 Column = 1
	WARLOCK_IMPROVED_LIFE_TAP = 1007, // TabId = 302 Tab = 0 Row = 1 Column = 2
	WARLOCK_IMPROVED_DRAIN_LIFE = 1004, // TabId = 302 Tab = 0 Row = 1 Column = 3
	WARLOCK_IMPROVED_CURSE_OF_AGONY = 1284, // TabId = 302 Tab = 0 Row = 2 Column = 0
	WARLOCK_FEL_CONCENTRATION = 1001, // TabId = 302 Tab = 0 Row = 2 Column = 1
	WARLOCK_AMPLIFY_CURSE = 1061, // TabId = 302 Tab = 0 Row = 2 Column = 2
	WARLOCK_GRIM_REACH = 1021, // TabId = 302 Tab = 0 Row = 3 Column = 0
	WARLOCK_NIGHTFALL = 1002, // TabId = 302 Tab = 0 Row = 3 Column = 1
	WARLOCK_IMPROVED_DRAIN_MANA = 1121, // TabId = 302 Tab = 0 Row = 3 Column = 3
	WARLOCK_SIPHON_LIFE = 1041, // TabId = 302 Tab = 0 Row = 4 Column = 1
	WARLOCK_CURSE_OF_EXHAUSTION = 1081, // TabId = 302 Tab = 0 Row = 4 Column = 2
	WARLOCK_IMPROVED_CURSE_OF_EXHAUSTION = 1082, // TabId = 302 Tab = 0 Row = 4 Column = 3
	WARLOCK_SHADOW_MASTERY = 1042, // TabId = 302 Tab = 0 Row = 5 Column = 1
	WARLOCK_DARK_PACT = 1022, // TabId = 302 Tab = 0 Row = 6 Column = 1
	WARLOCK_IMPROVED_HEALTHSTONE = 1221, // TabId = 303 Tab = 1 Row = 0 Column = 0
	WARLOCK_IMPROVED_IMP = 1222, // TabId = 303 Tab = 1 Row = 0 Column = 1
	WARLOCK_DEMONIC_EMBRACE = 1223, // TabId = 303 Tab = 1 Row = 0 Column = 2
	WARLOCK_IMPROVED_HEALTH_FUNNEL = 1224, // TabId = 303 Tab = 1 Row = 1 Column = 0
	WARLOCK_IMPROVED_VOIDWALKER = 1225, // TabId = 303 Tab = 1 Row = 1 Column = 1
	WARLOCK_FEL_INTELLECT = 1242, // TabId = 303 Tab = 1 Row = 1 Column = 2
	WARLOCK_IMPROVED_SUCCUBUS = 1243, // TabId = 303 Tab = 1 Row = 2 Column = 0
	WARLOCK_FEL_DOMINATION = 1226, // TabId = 303 Tab = 1 Row = 2 Column = 1
	WARLOCK_FEL_STAMINA = 1241, // TabId = 303 Tab = 1 Row = 2 Column = 2
	WARLOCK_MASTER_SUMMONER = 1227, // TabId = 303 Tab = 1 Row = 3 Column = 1
	WARLOCK_UNHOLY_POWER = 1262, // TabId = 303 Tab = 1 Row = 3 Column = 2
	WARLOCK_IMPROVED_ENSLAVE_DEMON = 1283, // TabId = 303 Tab = 1 Row = 4 Column = 0
	WARLOCK_DEMONIC_SACRIFICE = 1281, // TabId = 303 Tab = 1 Row = 4 Column = 1
	WARLOCK_IMPROVED_FIRESTONE = 1261, // TabId = 303 Tab = 1 Row = 4 Column = 3
	WARLOCK_MASTER_DEMONOLOGIST = 1244, // TabId = 303 Tab = 1 Row = 5 Column = 2
	WARLOCK_SOUL_LINK = 1282, // TabId = 303 Tab = 1 Row = 6 Column = 1
	WARLOCK_IMPROVED_SPELLSTONE = 1263 // TabId = 303 Tab = 1 Row = 6 Column = 2
};

enum TalentsWarrior  // 0x001
{
	WARRIOR_IMPROVED_HEROIC_STRIKE = 124, // TabId = 161 Tab = 0 Row = 0 Column = 0
	WARRIOR_DEFLECTION = 130, // TabId = 161 Tab = 0 Row = 0 Column = 1
	WARRIOR_IMPROVED_REND = 127, // TabId = 161 Tab = 0 Row = 0 Column = 2
	WARRIOR_IMPROVED_CHARGE = 126, // TabId = 161 Tab = 0 Row = 1 Column = 0
	WARRIOR_TACTICAL_MASTERY = 641, // TabId = 161 Tab = 0 Row = 1 Column = 1
	WARRIOR_IMPROVED_THUNDER_CLAP = 128, // TabId = 161 Tab = 0 Row = 1 Column = 3
	WARRIOR_IMPROVED_OVERPOWER = 131, // TabId = 161 Tab = 0 Row = 2 Column = 0
	WARRIOR_ANGER_MANAGEMENT = 137, // TabId = 161 Tab = 0 Row = 2 Column = 1
	WARRIOR_DEEP_WOUNDS = 121, // TabId = 161 Tab = 0 Row = 2 Column = 2
	WARRIOR_TWO_HANDED_WEAPON_SPECIALIZATION = 136, // TabId = 161 Tab = 0 Row = 3 Column = 1
	WARRIOR_IMPALE = 662, // TabId = 161 Tab = 0 Row = 3 Column = 2
	WARRIOR_AXE_SPECIALIZATION = 132, // TabId = 161 Tab = 0 Row = 4 Column = 0
	WARRIOR_SWEEPING_STRIKES = 133, // TabId = 161 Tab = 0 Row = 4 Column = 1
	WARRIOR_MACE_SPECIALIZATION = 125, // TabId = 161 Tab = 0 Row = 4 Column = 2
	WARRIOR_SWORD_SPECIALIZATION = 123, // TabId = 161 Tab = 0 Row = 4 Column = 3
	WARRIOR_POLEARM_SPECIALIZATION = 134, // TabId = 161 Tab = 0 Row = 5 Column = 0
	WARRIOR_IMPROVED_HAMSTRING = 129, // TabId = 161 Tab = 0 Row = 5 Column = 2
	WARRIOR_MORTAL_STRIKE = 135, // TabId = 161 Tab = 0 Row = 6 Column = 1
	WARRIOR_SHIELD_SPECIALIZATION = 1601, // TabId = 163 Tab = 2 Row = 0 Column = 1
	WARRIOR_ANTICIPATION = 138, // TabId = 163 Tab = 2 Row = 0 Column = 2
	WARRIOR_IMPROVED_BLOODRAGE = 142, // TabId = 163 Tab = 2 Row = 1 Column = 0
	WARRIOR_TOUGHNESS = 140, // TabId = 163 Tab = 2 Row = 1 Column = 2
	WARRIOR_IRON_WILL = 141, // TabId = 163 Tab = 2 Row = 1 Column = 3
	WARRIOR_LAST_STAND = 153, // TabId = 163 Tab = 2 Row = 2 Column = 0
	WARRIOR_IMPROVED_SHIELD_BLOCK = 145, // TabId = 163 Tab = 2 Row = 2 Column = 1
	WARRIOR_IMPROVED_REVENGE = 147, // TabId = 163 Tab = 2 Row = 2 Column = 2
	WARRIOR_DEFIANCE = 144, // TabId = 163 Tab = 2 Row = 2 Column = 3
	WARRIOR_IMPROVED_SUNDER_ARMOR = 146, // TabId = 163 Tab = 2 Row = 3 Column = 0
	WARRIOR_IMPROVED_DISARM = 151, // TabId = 163 Tab = 2 Row = 3 Column = 1
	WARRIOR_IMPROVED_TAUNT = 143, // TabId = 163 Tab = 2 Row = 3 Column = 2
	WARRIOR_IMPROVED_SHIELD_WALL = 150, // TabId = 163 Tab = 2 Row = 4 Column = 0
	WARRIOR_CONCUSSION_BLOW = 152, // TabId = 163 Tab = 2 Row = 4 Column = 1
	WARRIOR_IMPROVED_SHIELD_BASH = 149, // TabId = 163 Tab = 2 Row = 4 Column = 2
	WARRIOR_ONE_HANDED_WEAPON_SPECIALIZATION = 702, // TabId = 163 Tab = 2 Row = 5 Column = 2
	WARRIOR_SHIELD_SLAM = 148, // TabId = 163 Tab = 2 Row = 6 Column = 1
	WARRIOR_BOOMING_VOICE = 158, // TabId = 164 Tab = 1 Row = 0 Column = 1
	WARRIOR_CRUELTY = 157, // TabId = 164 Tab = 1 Row = 0 Column = 2
	WARRIOR_IMPROVED_DEMORALIZING_SHOUT = 161, // TabId = 164 Tab = 1 Row = 1 Column = 1
	WARRIOR_UNBRIDLED_WRATH = 159, // TabId = 164 Tab = 1 Row = 1 Column = 2
	WARRIOR_IMPROVED_CLEAVE = 166, // TabId = 164 Tab = 1 Row = 2 Column = 0
	WARRIOR_PIERCING_HOWL = 160, // TabId = 164 Tab = 1 Row = 2 Column = 1
	WARRIOR_BLOOD_CRAZE = 661, // TabId = 164 Tab = 1 Row = 2 Column = 2
	WARRIOR_IMPROVED_BATTLE_SHOUT = 154, // TabId = 164 Tab = 1 Row = 2 Column = 3
	WARRIOR_DUAL_WIELD_SPECIALIZATION = 1581, // TabId = 164 Tab = 1 Row = 3 Column = 0
	WARRIOR_IMPROVED_EXECUTE = 1542, // TabId = 164 Tab = 1 Row = 3 Column = 1
	WARRIOR_ENRAGE = 155, // TabId = 164 Tab = 1 Row = 3 Column = 2
	WARRIOR_IMPROVED_SLAM = 168, // TabId = 164 Tab = 1 Row = 4 Column = 0
	WARRIOR_DEATH_WISH = 165, // TabId = 164 Tab = 1 Row = 4 Column = 1
	WARRIOR_IMPROVED_INTERCEPT = 1543, // TabId = 164 Tab = 1 Row = 4 Column = 3
	WARRIOR_IMPROVED_BERSERKER_RAGE = 1541, // TabId = 164 Tab = 1 Row = 5 Column = 0
	WARRIOR_FLURRY = 156, // TabId = 164 Tab = 1 Row = 5 Column = 2
	WARRIOR_BLOODTHIRST = 167 // TabId = 164 Tab = 1 Row = 6 Column = 1
};

enum TalentSpecPurpose
{
	TSP_NONE = 0x00000000,  // should probably error out?
	TSP_PVE_TANK = 0x00000001,
	TSP_PVE_DPS = 0x00000002,
	TSP_PVE_LEVELING = 0x00000004,
	TSP_PVE_HEALING = 0x00000008,
	TSP_PVE_INSTANCING = 0x00000010,
	TSP_PVE_RAID = 0x00000020,
	TSP_PVE_ALL = 0x0000FFFF,  // Highly recommend AGAINST using this
	TSP_PVP_TANK = 0x00010000,
	TSP_PVP_DPS = 0x00020000,
	TSP_PVP_LEVELING = 0x00040000,
	TSP_PVP_TWINK = 0x00080000,
	TSP_PVP_HEALING = 0x00100000,
	TSP_PVP_ARENA = 0x00200000,
	TSP_PVP_ALL = 0xFFFF0000   // Highly recommend AGAINST using this
};

enum MainSpec
{
	MAGE_SPEC_FIRE = 41,
	MAGE_SPEC_FROST = 61,
	MAGE_SPEC_ARCANE = 81,
	WARRIOR_SPEC_ARMS = 161,
	WARRIOR_SPEC_PROTECTION = 163,
	WARRIOR_SPEC_FURY = 164,
	ROGUE_SPEC_COMBAT = 181,
	ROGUE_SPEC_ASSASSINATION = 182,
	ROGUE_SPEC_SUBTELTY = 183,
	PRIEST_SPEC_DISCIPLINE = 201,
	PRIEST_SPEC_HOLY = 202,
	PRIEST_SPEC_SHADOW = 203,
	SHAMAN_SPEC_ELEMENTAL = 261,
	SHAMAN_SPEC_RESTORATION = 262,
	SHAMAN_SPEC_ENHANCEMENT = 263,
	DRUID_SPEC_FERAL = 281,
	DRUID_SPEC_RESTORATION = 282,
	DRUID_SPEC_BALANCE = 283,
	WARLOCK_SPEC_DESTRUCTION = 301,
	WARLOCK_SPEC_AFFLICTION = 302,
	WARLOCK_SPEC_DEMONOLOGY = 303,
	HUNTER_SPEC_BEASTMASTERY = 361,
	HUNTER_SPEC_SURVIVAL = 362,
	HUNTER_SPEC_MARKSMANSHIP = 363,
	PALADIN_SPEC_RETRIBUTION = 381,
	PALADIN_SPEC_HOLY = 382,
	PALADIN_SPEC_PROTECTION = 383
};

struct TalentSpec
{
	std::string specName;
	short specClass;
	TalentSpecPurpose specPurpose;
	uint16 talentId[71];
};

enum NotableItems
{
    // Skeleton Keys
    SILVER_SKELETON_KEY            = 15869,
    GOLDEN_SKELETON_KEY            = 15870,
    TRUESILVER_SKELETON_KEY        = 15871,
    ARCANITE_SKELETON_KEY          = 15872,
    // Lock Charges
    SMALL_SEAFORIUM_CHARGE         = 4367,
    LARGE_SEAFORIUM_CHARGE         = 4398,
    POWERFUL_SEAFORIUM_CHARGE      = 18594
};

//enum MainSpec
//{
//    MAGE_SPEC_FIRE              = 41,
//    MAGE_SPEC_FROST             = 61,
//    MAGE_SPEC_ARCANE            = 81,
//    WARRIOR_SPEC_ARMS           = 161,
//    WARRIOR_SPEC_PROTECTION     = 163,
//    WARRIOR_SPEC_FURY           = 164,
//    ROGUE_SPEC_COMBAT           = 181,
//    ROGUE_SPEC_ASSASSINATION    = 182,
//    ROGUE_SPEC_SUBTELTY         = 183,
//    PRIEST_SPEC_DISCIPLINE      = 201,
//    PRIEST_SPEC_HOLY            = 202,
//    PRIEST_SPEC_SHADOW          = 203,
//    SHAMAN_SPEC_ELEMENTAL       = 261,
//    SHAMAN_SPEC_RESTORATION     = 262,
//    SHAMAN_SPEC_ENHANCEMENT     = 263,
//    DRUID_SPEC_FERAL            = 281,
//    DRUID_SPEC_RESTORATION      = 282,
//    DRUID_SPEC_BALANCE          = 283,
//    WARLOCK_SPEC_DESTRUCTION    = 301,
//    WARLOCK_SPEC_AFFLICTION     = 302,
//    WARLOCK_SPEC_DEMONOLOGY     = 303,
//    HUNTER_SPEC_BEASTMASTERY    = 361,
//    HUNTER_SPEC_SURVIVAL        = 362,
//    HUNTER_SPEC_MARKSMANSHIP    = 363,
//    PALADIN_SPEC_RETRIBUTION    = 381,
//    PALADIN_SPEC_HOLY           = 382,
//    PALADIN_SPEC_PROTECTION     = 383
//};

enum CombatManeuverReturns
{
    // TODO: RETURN_NO_ACTION_UNKNOWN is not part of ANY_OK or ANY_ERROR. It's also bad form and should be eliminated ASAP.
    RETURN_NO_ACTION_OK                 = 0x01, // No action taken during this combat maneuver, as intended (just wait, etc...)
    RETURN_NO_ACTION_UNKNOWN            = 0x02, // No action taken during this combat maneuver, unknown reason
    RETURN_NO_ACTION_ERROR              = 0x04, // No action taken due to error
    RETURN_NO_ACTION_INVALIDTARGET      = 0x08, // No action taken - invalid target
    RETURN_FINISHED_FIRST_MOVES         = 0x10, // Last action of first-combat-maneuver finished, continue onto next-combat-maneuver
    RETURN_CONTINUE                     = 0x20, // Continue first moves; normal return value for next-combat-maneuver
    RETURN_NO_ACTION_INSUFFICIENT_POWER = 0x40, // No action taken due to insufficient power (rage, focus, mana, runes)
    RETURN_ANY_OK                       = 0x31, // All the OK values bitwise OR'ed
    RETURN_ANY_ACTION                   = 0x30, // All returns that result in action (which should also be 'OK')
    RETURN_ANY_ERROR                    = 0x4C  // All the ERROR values bitwise OR'ed
};

enum m_FollowAutoGo
{
	FOLLOWAUTOGO_OFF = 0,
	FOLLOWAUTOGO_INIT = 1,
	FOLLOWAUTOGO_SET = 2,
	FOLLOWAUTOGO_RESET = 3,
	FOLLOWAUTOGO_RUN = 4
};

class MANGOS_DLL_SPEC PlayerbotAI
{
public:
    enum ScenarioType
    {
        SCENARIO_PVE,
        SCENARIO_PVE_ELITE, // group (5 members max) when an elite is near - most likely instance (can happen in open world)
        SCENARIO_PVE_RAID,
        SCENARIO_PVP_DUEL,
        SCENARIO_PVP_BG,    // You'll probably want to expand this to suit goal? (capture the flag, assault, domination, ...)
        SCENARIO_PVP_ARENA,
        SCENARIO_PVP_OPENWORLD
    };

    enum CombatStyle
    {
        COMBAT_MELEE                = 0x01,             // class melee attacker
        COMBAT_RANGED               = 0x02              // class is ranged attacker
    };

    // masters orders that should be obeyed by the AI during the updteAI routine
    // the master will auto set the target of the bot
    enum CombatOrderType
    {
        ORDERS_NONE                 = 0x0000,   // no special orders given
        ORDERS_TANK                 = 0x0001,   // bind attackers by gaining threat
        ORDERS_ASSIST               = 0x0002,   // assist someone (dps type)
        ORDERS_HEAL                 = 0x0004,   // concentrate on healing (no attacks, only self defense)
        ORDERS_NODISPEL             = 0x0008,   // Dont dispel anything
        ORDERS_PROTECT              = 0x0010,   // combinable state: check if protectee is attacked
        ORDERS_PASSIVE              = 0x0020,   // bots do nothing
        ORDERS_TEMP_WAIT_TANKAGGRO  = 0x0040,   // Wait on tank to build aggro - expect healing to continue, disable setting when tank loses focus
        ORDERS_TEMP_WAIT_OOC        = 0x0080,   // Wait but only while OOC - wait only - combat will resume healing, dps, tanking, ...
        ORDERS_RESIST_FIRE          = 0x0100,   // resist fire
        ORDERS_RESIST_NATURE        = 0x0200,   // resist nature
        ORDERS_RESIST_FROST         = 0x0400,   // resist frost
        ORDERS_RESIST_SHADOW        = 0x0800,   // resist shadow

        // Cumulative orders
        ORDERS_PRIMARY              = 0x0007,
        ORDERS_SECONDARY            = 0x0F78,
        ORDERS_RESIST               = 0x0F00,
        ORDERS_TEMP                 = 0x00C0,   // All orders NOT to be saved, turned off by bots (or logoff, reset, ...)
        ORDERS_RESET                = 0xFFFF
    };

    enum CombatTargetType
    {
        TARGET_NORMAL               = 0x00,
        TARGET_THREATEN             = 0x01
    };

    enum BotState
    {
        BOTSTATE_NORMAL,            // normal AI routines are processed
        BOTSTATE_COMBAT,            // bot is in combat
        BOTSTATE_DEAD,              // we are dead and wait for becoming ghost
        BOTSTATE_DEADRELEASED,      // we released as ghost and wait to revive
        BOTSTATE_LOOTING,           // looting mode, used just after combat
        BOTSTATE_FLYING,             // bot is flying
        BOTSTATE_DELAYED            // bot delay action
    };

    enum CollectionFlags
    {
        COLLECT_FLAG_NOTHING    = 0x00,     // skip looting of anything
        COLLECT_FLAG_COMBAT     = 0x01,     // loot after combat
        COLLECT_FLAG_QUEST      = 0x02,     // quest and needed items
        COLLECT_FLAG_PROFESSION = 0x04,     // items related to skills
        COLLECT_FLAG_LOOT       = 0x08,     // all loot on corpses
        COLLECT_FLAG_SKIN       = 0x10,     // skin creatures if available
        COLLECT_FLAG_NEAROBJECT = 0x20      // collect specified nearby object
    };

    enum MovementOrderType
    {
        MOVEMENT_NONE               = 0x00,
        MOVEMENT_FOLLOW             = 0x01,
        MOVEMENT_STAY               = 0x02
    };

    enum TaskFlags
    {
        NONE                        = 0x00,  // do nothing
        SELL                        = 0x01,  // sell items
        REPAIR                      = 0x02,  // repair items
        ADD                         = 0x03,  // add auction
        REMOVE                      = 0x04,  // remove auction
        RESET                       = 0x05,  // reset all talents
        WITHDRAW                    = 0x06,  // withdraw item from bank
        DEPOSIT                     = 0x07,  // deposit item in bank
        LIST                        = 0x08,  // list quests
        END                         = 0x09,  // turn in quests
        TAKE                        = 0x0A   // take quest
    };

    enum AnnounceFlags
    {
        NOTHING                     = 0x00,
        INVENTORY_FULL              = 0x01,
        CANT_AFFORD                 = 0x02
    };

    typedef std::pair<enum TaskFlags, uint32> taskPair;
    typedef std::list<taskPair> BotTaskList;
    typedef std::list<enum NPCFlags> BotNPCList;
    typedef std::map<uint32, uint32> BotNeedItem;
    typedef std::pair<uint32,uint32> talentPair;
    typedef std::list<ObjectGuid> BotObjectList;
    typedef std::list<uint32> BotEntryList;
    typedef std::vector<uint32> BotTaxiNode;
    typedef std::set<ObjectGuid> BotObjectSet;

    // attacker query used in PlayerbotAI::FindAttacker()
    enum ATTACKERINFOTYPE
    {
        AIT_NONE                    = 0x00,
        AIT_LOWESTTHREAT            = 0x01,
        AIT_HIGHESTTHREAT           = 0x02,
        AIT_VICTIMSELF              = 0x04,
        AIT_VICTIMNOTSELF           = 0x08      // could/should use victim param in FindAttackers
    };
    struct AttackerInfo
    {
        Unit*    attacker;            // reference to the attacker
        Unit*    victim;              // combatant's current victim
        float threat;                 // own threat on this combatant
        float threat2;                // highest threat not caused by bot
        uint32 count;                 // number of units attacking
        uint32 source;                // 1=bot, 2=master, 3=group
    };
    typedef std::map<ObjectGuid, AttackerInfo> AttackerInfoList;
    typedef std::map<uint32, float> SpellRanges;

    enum HELPERLINKABLES
    {
        HL_NONE,
        HL_PROFESSION,
        HL_ITEM,
        HL_TALENT,
        HL_SKILL,
        HL_OPTION,
        HL_PETAGGRO,
        HL_QUEST,
        HL_GAMEOBJECT,
        HL_SPELL,
        HL_TARGET,
        HL_NAME,
        HL_AUCTION
    };  
    
public:
    PlayerbotAI(PlayerbotMgr * const mgr, Player * const bot);
    virtual ~PlayerbotAI();

    // This is called from Unit.cpp and is called every second (I think)
    void UpdateAI(const uint32 p_time);

    // This is called from ChatHandler.cpp when there is an incoming message to the bot
    // from a whisper or from the party channel
    void HandleCommand(const std::string& text, Player& fromPlayer);

    // This is called by WorldSession.cpp
    // It provides a view of packets normally sent to the client.
    // Since there is no client at the other end, the packets are dropped of course.
    // For a list of opcodes that can be caught see Opcodes.cpp (SMSG_* opcodes only)
    void HandleBotOutgoingPacket(const WorldPacket& packet);

    // This is called by WorldSession.cpp
    // when it detects that a bot is being teleported. It acknowledges to the server to complete the
    // teleportation
    void HandleTeleportAck();

    // Returns what kind of situation we are in so the ai can react accordingly
    ScenarioType GetScenarioType() { return m_ScenarioType; }
    CombatStyle GetCombatStyle() { return m_combatStyle; }
    void SetCombatStyle(CombatStyle cs) { m_combatStyle = cs; }

    PlayerbotClassAI* GetClassAI() { return m_classAI; }
    PlayerbotMgr* GetManager() { return m_mgr; }
    void ReloadAI();

    // finds spell ID for matching substring args
    // in priority of full text match, spells not taking reagents, and highest rank
    uint32 getSpellId(const char* args, bool master = false) const;
    uint32 getPetSpellId(const char* args) const;
    // Initialize spell using rank 1 spell id
    uint32 initSpell(uint32 spellId);
    uint32 initPetSpell(uint32 spellIconId);

    // extract quest ids from links
    void extractQuestIds(const std::string& text, std::list<uint32>& questIds) const;

    // extract auction ids from links
    void extractAuctionIds(const std::string& text, std::list<uint32>& auctionIds) const;

    // extracts talent ids to list
    void extractTalentIds(const std::string& text, std::list<talentPair>& talentIds) const;

    // extracts item ids from links
    void extractItemIds(const std::string& text, std::list<uint32>& itemIds) const;

    // extract spellid from links
    void extractSpellId(const std::string& text, uint32 &spellId) const;

    // extract spellids from links to list
    void extractSpellIdList(const std::string& text, BotEntryList& m_spellsToLearn) const;

    // extracts currency from a string as #g#s#c and returns the total in copper
    uint32 extractMoney(const std::string& text) const;

    // extracts gameobject info from link
    void extractGOinfo(const std::string& text, BotObjectList& m_lootTargets) const;

    // finds items in bots equipment and adds them to foundItemList, removes found items from itemIdSearchList
    void findItemsInEquip(std::list<uint32>& itemIdSearchList, std::list<Item*>& foundItemList) const;
    // finds items in bots inventory and adds them to foundItemList, removes found items from itemIdSearchList
    void findItemsInInv(std::list<uint32>& itemIdSearchList, std::list<Item*>& foundItemList) const;
    // finds nearby game objects that are specified in m_collectObjects then adds them to the m_lootTargets list
    void findNearbyGO();
    // finds nearby creatures, whose UNIT_NPC_FLAGS match the flags specified in item list m_itemIds
    void findNearbyCreature();
    bool IsElite(Unit* pTarget) const;
    // Used by bots to check if their target is neutralized (polymorph, shackle or the like). Useful to avoid breaking crowd control
    bool IsNeutralized(Unit* pTarget);

    void MakeSpellLink(const SpellEntry *sInfo, std::ostringstream &out);
    void MakeWeaponSkillLink(const SpellEntry *sInfo, std::ostringstream &out, uint32 skillid);

    // currently bots only obey commands from the master
    bool canObeyCommandFrom(const Player& player) const;

    // get current casting spell (will return NULL if no spell!)
    Spell* GetCurrentSpell() const;
    uint32 GetCurrentSpellId() { return m_CurrentlyCastingSpellId; }

    bool HasAura(uint32 spellId, const Unit& player) const;
    bool HasAura(const char* spellName, const Unit& player) const;
    bool HasAura(const char* spellName) const;

    bool CanReceiveSpecificSpell(uint8 spec, Unit* target) const;

    bool PickPocket(Unit* pTarget);
    bool HasTool(uint32 TC);
    bool HasSpellReagents(uint32 spellId);

    uint8 GetHealthPercent(const Unit& target) const;
    uint8 GetHealthPercent() const;
    uint8 GetBaseManaPercent(const Unit& target) const;
    uint8 GetBaseManaPercent() const;
    uint8 GetManaPercent(const Unit& target) const;
    uint8 GetManaPercent() const;
    uint8 GetRageAmount(const Unit& target) const;
    uint8 GetRageAmount() const;
    uint8 GetEnergyAmount(const Unit& target) const;
    uint8 GetEnergyAmount() const;

    Item* FindFood() const;
    Item* FindDrink() const;
    Item* FindBandage() const;
    Item* FindPoison() const;
    Item* FindMount(uint32 matchingRidingSkill) const;
    Item* FindItem(uint32 ItemId);
    Item* FindItemInBank(uint32 ItemId);
    Item* FindKeyForLockValue(uint32 reqSkillValue);
    Item* FindBombForLockValue(uint32 reqSkillValue);
    Item* FindConsumable(uint32 displayId) const;
    bool  FindAmmo() const;
    uint8 _findItemSlot(Item* target);
    bool CanStore();

    // ******* Actions ****************************************
    // Your handlers can call these actions to make the bot do things.
    void TellMaster(const std::string& text) const;
    void TellMaster(const char *fmt, ...) const;
    void SendWhisper(const std::string& text, Player& player) const;
    bool CastSpell(const char* args);
    bool CastSpell(uint32 spellId);
    bool CastSpell(uint32 spellId, Unit& target);
    bool CheckBotCast(const SpellEntry *sInfo );
    bool CastPetSpell(uint32 spellId, Unit* target = nullptr);
    bool Buff(uint32 spellId, Unit * target, void (*beforeCast)(Player *) = nullptr);
    bool SelfBuff(uint32 spellId);
    bool In_Range(Unit* Target, uint32 spellId);
    bool In_Reach(Unit* Target, uint32 spellId);
    bool CanReachWithSpellAttack(Unit* target);

    void UseItem(Item *item, uint16 targetFlag, ObjectGuid targetGUID);
    void UseItem(Item *item, uint8 targetInventorySlot);
    void UseItem(Item *item, Unit *target);
    void UseItem(Item *item);

    void PlaySound(uint32 soundid);
    void Announce(AnnounceFlags msg);

    void EquipItem(Item* src_Item);
    //void Stay();
    //bool Follow(Player& player);
    void SendNotEquipList(Player& player);

    uint8  m_DelayAttack;
    time_t m_DelayAttackInit;
    Unit *gPrimtarget;
    Unit *gSectarget;
    uint32 gQuestFetch;
    void BotDataRestore();
    void CombatOrderRestore();
    void InterruptCurrentCastingSpell();
    void Attack(Unit* forcedTarget = nullptr);
    void GetCombatTarget(Unit* forcedTarget = 0);
    void GetDuelTarget(Unit* forcedTarget);
    Unit* GetCurrentTarget() { return m_targetCombat; };
    void DoNextCombatManeuver();
    void DoCombatMovement();
    void SetIgnoreUpdateTime(uint8 t = 0) { m_ignoreAIUpdatesUntilTime = time(nullptr) + t; };
    time_t CurrentTime() { return time(nullptr); };

    Player* GetPlayerBot() const { return m_bot; }
    Player* GetPlayer() const { return m_bot; }
    Player* GetMaster() const;

    BotState GetState() { return m_botState; };
    void SetState(BotState state);
    void SetQuestNeedItems();
    void SetQuestNeedCreatures();
    void SendQuestNeedList();
    bool IsInQuestItemList(uint32 itemid) { return m_needItemList.find(itemid) != m_needItemList.end(); };
    bool IsInQuestCreatureList(uint32 id) { return m_needCreatureOrGOList.find(id) != m_needCreatureOrGOList.end(); };
    bool IsItemUseful(uint32 itemid);
	bool IsItemAnUpgrade(Item* pItem);
	std::list<const ItemPrototype*> GetExistingItemsInSlot(ItemPrototype const *pNewItem);
	void SendUpgradingItems(ItemPrototype const *newItem);

    void SendOrders(Player& player);
    bool DoTeleport(WorldObject &obj);
    void DoLoot();
    void DoFlight();
    void GetTaxi(ObjectGuid guid, BotTaxiNode& nodes);

    bool HasCollectFlag(uint8 flag) { return m_collectionFlags & flag; }
    void SetCollectFlag(uint8 flag)
    {
        if (HasCollectFlag(flag)) m_collectionFlags &= ~flag;
        else m_collectionFlags |= flag;
    }

    uint32 EstRepairAll();
    uint32 EstRepair(uint16 pos);

    void AcceptQuest(Quest const *qInfo, Player *pGiver);
    void TurnInQuests(WorldObject *questgiver);
    void ListQuests(WorldObject* questgiver);
    bool AddQuest(const uint32 entry, WorldObject* questgiver);

    bool IsInCombat();
    bool IsRegenerating();
    bool IsGroupInCombat();
    Player* GetGroupTank(); // TODO: didn't want to pollute non-playerbot code but this should really go in group.cpp
    void SetGroupCombatOrder(CombatOrderType co);
    void ClearGroupCombatOrder(CombatOrderType co);
    void SetGroupIgnoreUpdateTime(uint8 t);
    bool GroupHoTOnTank();
    bool CanPull(Player &fromPlayer);
    bool CastPull();
    bool GroupTankHoldsAggro();
    bool CastNeutralize();
    void UpdateAttackerInfo();
    Unit* FindAttacker(ATTACKERINFOTYPE ait = AIT_NONE, Unit *victim = 0);
    uint32 GetAttackerCount() { return m_attackerInfo.size(); };
    void SetCombatOrderByStr(std::string str, Unit *target = 0);
    void SetCombatOrder(CombatOrderType co, Unit *target = 0);
    void ClearCombatOrder(CombatOrderType co);
    CombatOrderType GetCombatOrder() { return this->m_combatOrder; }
    bool IsTank() { return (m_combatOrder & ORDERS_TANK) ? true : false; }
    bool IsHealer() { return (m_combatOrder & ORDERS_HEAL) ? true : false; }
    bool IsDPS() { return (m_combatOrder & ORDERS_ASSIST) ? true : false; }
    bool Impulse() { srand ( time(nullptr) ); return(((rand() % 100) > 50) ? true : false); }
    void SetMovementOrder(MovementOrderType mo, Unit *followTarget = 0);
    MovementOrderType GetMovementOrder() { return this->m_movementOrder; }
    void MovementReset();
    void MovementClear();
    bool IsMoving();

    void SetInFront(const Unit* obj);

    void ItemLocalization(std::string& itemName, const uint32 itemID) const;
    void QuestLocalization(std::string& questTitle, const uint32 questID) const;
    void CreatureLocalization(std::string& creatureName, const uint32 entry) const;
    void GameObjectLocalization(std::string& gameobjectName, const uint32 entry) const;

    uint8 GetFreeBagSpace() const;
    void SellGarbage(bool listNonTrash = true, bool bDetailTrashSold = false, bool verbose = true);
    void Sell(const uint32 itemid);
    void AddAuction(const uint32 itemid, Creature* aCreature);
    void ListAuctions();
    bool RemoveAuction(const uint32 auctionid);
    void Repair(const uint32 itemid, Creature* rCreature);
    bool Talent(Creature* tCreature);
    void InspectUpdate();
    bool Withdraw(const uint32 itemid);
    bool Deposit(const uint32 itemid);
    void BankBalance();

	void Levelup();
	
	// Error check the TS DB. Should only be used when admins want to verify their new TS input
	uint32 TalentSpecDBContainsError();

	// Get talent specs or counts thereof
	uint32 GetTalentSpecsAmount();
	uint32 GetTalentSpecsAmount(long specClass);
	std::list<TalentSpec> GetTalentSpecs(long specClass);
	TalentSpec GetTalentSpec(long specClass, long choice);
	TalentSpec GetActiveTalentSpec() { return m_activeTalentSpec; }
	void ClearActiveTalentSpec() { m_activeTalentSpec.specName = ""; m_activeTalentSpec.specClass = 0; m_activeTalentSpec.specPurpose = TSP_NONE; for (int i = 0; i < 51; i++) m_activeTalentSpec.talentId[i] = 0; }
	void SetActiveTalentSpec(TalentSpec ts) { m_activeTalentSpec = ts; }
	bool ApplyActiveTalentSpec();

	// added from 3.3.5 version
	void FollowAutoReset();

protected:
	bool ValidateTalent(uint16 talent, long charClass);

private:
    bool ExtractCommand(const std::string sLookingFor, std::string &text, bool bUseShort = false);
    // outsource commands for code clarity
    void _HandleCommandReset(std::string &text, Player &fromPlayer);
    void _HandleCommandReport(std::string &text, Player &fromPlayer);
    void _HandleCommandOrders(std::string &text, Player &fromPlayer);
    void _HandleCommandFollow(std::string &text, Player &fromPlayer);
    void _HandleCommandStay(std::string &text, Player &fromPlayer);
    void _HandleCommandAttack(std::string &text, Player &fromPlayer);
    void _HandleCommandPull(std::string &text, Player &fromPlayer);
    void _HandleCommandNeutralize(std::string &text, Player &fromPlayer);
    void _HandleCommandCast(std::string &text, Player &fromPlayer);
    void _HandleCommandSell(std::string &text, Player &fromPlayer);
    void _HandleCommandRepair(std::string &text, Player &fromPlayer);
    void _HandleCommandAuction(std::string &text, Player &fromPlayer);
    void _HandleCommandBank(std::string &text, Player &fromPlayer);
    void _HandleCommandUse(std::string &text, Player &fromPlayer);
    void _HandleCommandEquip(std::string &text, Player &fromPlayer);
	void _HandleCommandGear(std::string &text, Player &fromPlayer);
    void _HandleCommandFind(std::string &text, Player &fromPlayer);
    void _HandleCommandGet(std::string &text, Player &fromPlayer);
    void _HandleCommandCollect(std::string &text, Player &fromPlayer);
    void _HandleCommandQuest(std::string &text, Player &fromPlayer);
    void _HandleCommandPet(std::string &text, Player &fromPlayer);
    void _HandleCommandSpells(std::string &text, Player &fromPlayer);
    void _HandleCommandSurvey(std::string &text, Player &fromPlayer);
    void _HandleCommandSkill(std::string &text, Player &fromPlayer);
    void _HandleCommandStats(std::string &text, Player &fromPlayer);
    void _HandleCommandHelp(std::string &text, Player &fromPlayer);
	void _HandleCommandTalent(std::string &text, Player &fromPlayer);
    void _HandleCommandHelp(const char* szText, Player &fromPlayer) { std::string text = szText; _HandleCommandHelp(text, fromPlayer); }
    std::string _HandleCommandHelpHelper(std::string sCommand, std::string sExplain, HELPERLINKABLES reqLink = HL_NONE, bool bReqLinkMultiples = false, bool bCommandShort = false);

    // ****** Closed Actions ********************************
    // These actions may only be called at special times.
    // Trade methods are only applicable when the trade window is open
    // and are only called from within HandleCommand.
    bool TradeItem(const Item& item, int8 slot = -1);
    bool TradeCopper(uint32 copper);

    // Helper routines not needed by class AIs.
    void UpdateAttackersForTarget(Unit *victim);

    void _doSellItem(Item* const item, std::ostringstream &report, std::ostringstream &canSell, uint32 &TotalCost, uint32 &TotalSold);
    void MakeItemLink(const Item *item, std::ostringstream &out, bool IncludeQuantity = true);
    void MakeItemLink(const ItemPrototype *item, std::ostringstream &out);

	bool IsItemAnUpgrade(ItemPrototype const *pProto);
	void UpgradeProfession(uint32 profId, uint32 upgrades[3]);

    // it is safe to keep these back reference pointers because m_bot
    // owns the "this" object and m_master owns m_bot. The owner always cleans up.
    PlayerbotMgr* const m_mgr;
    Player* const m_bot;
    PlayerbotClassAI* m_classAI;

    // ignores AI updates until time specified
    // no need to waste CPU cycles during casting etc
    time_t m_ignoreAIUpdatesUntilTime;

    CombatStyle m_combatStyle;
    CombatOrderType m_combatOrder;
    MovementOrderType m_movementOrder;

	TalentSpec m_activeTalentSpec;

    ScenarioType m_ScenarioType;

    // defines the state of behaviour of the bot
    BotState m_botState;

    // list of items, creatures or gameobjects needed to fullfill quests
    BotNeedItem m_needItemList;
    BotNeedItem m_needCreatureOrGOList;

    // list of creatures we recently attacked and want to loot
    BotNPCList m_findNPC;               // list of NPCs
    BotTaskList m_tasks;                // list of tasks
    BotObjectList m_lootTargets;        // list of targets
    BotEntryList m_spellsToLearn;       // list of spells
    ObjectGuid m_lootCurrent;           // current remains of interest
    ObjectGuid m_lootPrev;              // previous loot
    BotEntryList m_collectObjects;      // object entries searched for in findNearbyGO
    BotTaxiNode m_taxiNodes;            // flight node chain;

    uint8 m_collectionFlags;            // what the bot should look for to loot
    bool m_inventory_full;

    time_t m_TimeDoneEating;
    time_t m_TimeDoneDrinking;
    uint32 m_CurrentlyCastingSpellId;
    //bool m_IsFollowingMaster;

    // if master commands bot to do something, store here until updateAI
    // can do it
    uint32 m_spellIdCommand;
    ObjectGuid m_targetGuidCommand;
    ObjectGuid m_taxiMaster;

    BotObjectSet m_ignorePlayersChat;  // list of players that the bot will not respond to

    AttackerInfoList m_attackerInfo;

    bool m_targetChanged;
    CombatTargetType m_targetType;

    Unit* m_targetCombat;       // current combat target
    Unit* m_targetAssist;       // get new target by checking attacker list of assisted player
    Unit* m_targetProtect;      // check

    Unit *m_followTarget;       // whom to follow in non combat situation?

    uint32 FISHING,
           HERB_GATHERING,
           MINING,
           SKINNING;

	uint32 ALCHEMY,
		BLACKSMITHING,
		ENCHANTING,
		ENGINEERING,
		LEATHERWORKING,
		TAILORING;

	uint32 m_primaryProfession1,
		   m_primaryProfession2;

    SpellRanges m_spellRangeMap;

    float m_destX, m_destY, m_destZ; // latest coordinates for chase and point movement types

	uint8 m_FollowAutoGo;
};

#endif
