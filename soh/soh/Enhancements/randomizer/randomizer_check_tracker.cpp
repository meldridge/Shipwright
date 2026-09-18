#include "randomizer_check_tracker.h"
#include "randomizer_entrance_tracker.h"
#include "randomizer_item_tracker.h"
#include "randomizerTypes.h"
#include "soh/OTRGlobals.h"
#include "soh/SaveManager.h"
#include "soh/ResourceManagerHelpers.h"
#include "soh/SohGui/UIWidgets.hpp"
#include "soh/SohGui/SohGui.hpp"
#include "soh/SohGui/SohMenu.h"
#include "dungeon.h"
#include "entrance.h"
#include "location_access.h"
#include "3drando/fill.hpp"
#include "soh/Enhancements/debugger/performanceTimer.h"
#include "soh/Enhancements/randomizer/randomizer.h"
#include "soh/ObjectExtension/ObjectExtension.h"
#include "overlays/actors/ovl_En_GirlA/z_en_girla.h"
#include "overlays/actors/ovl_En_Box/z_en_box.h"
#include "overlays/actors/ovl_En_Ex_Item/z_en_ex_item.h"
#include "overlays/actors/ovl_En_Si/z_en_si.h"
#include "overlays/actors/ovl_Item_B_Heart/z_item_b_heart.h"
#include "overlays/actors/ovl_Item_Etcetera/z_item_etcetera.h"
#include "soh/Enhancements/randomizer/item_category_adj.h"

#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <libultraship/controller/controldeck/ControlDeck.h>
#include "location.h"
#include "item_location.h"
#include "randomizer_check_objects.h"
#include "soh/Enhancements/game-interactor/GameInteractor.h"
#include "z64item.h"

extern "C" {
#include "variables.h"
#include "functions.h"
#include "macros.h"
extern PlayState* gPlayState;
}
extern "C" GetItemEntry ItemTable_RetrieveEntry(s16 modIndex, s16 getItemID);

extern std::vector<ItemTrackerItem> dungeonRewardStones;
extern std::vector<ItemTrackerItem> dungeonRewardMedallions;
extern std::vector<ItemTrackerItem> songItems;
extern std::vector<ItemTrackerItem> equipmentItems;

using json = nlohmann::json;
using namespace UIWidgets;

namespace CheckTracker {
static WidgetInfo backgroundColorWidget;
static WidgetInfo windowTypeWidget;
static WidgetInfo dungeonSpoilerWidget;
static WidgetInfo hideUnshuffledShopWidget;
static WidgetInfo showGSWidget;
static WidgetInfo showLogicWidget;
static WidgetInfo checkAvailabilityWidget;

// settings
bool showShops;
bool showOverworldTokens;
bool showDungeonTokens;
bool showBeans;
bool showScrubs;
bool showMajorScrubs;
bool showMerchants;
bool showSongs;
bool showBeehives;
bool showCows;
bool showOverworldFreestanding;
bool showDungeonFreestanding;
bool showAdultTrade;
bool showKokiriSword;
bool showMasterSword;
bool showHyruleLoach;
bool showWeirdEgg;
bool showZeldasLetter;
bool showGerudoCard;
bool showOverworldPots;
bool showDungeonPots;
bool showOverworldGrass;
bool showDungeonGrass;
bool showOverworldCrates;
bool showDungeonCrates;
bool showRocks;
bool showOverworldBoulders;
bool showDungeonBoulders;
bool showTrees;
bool showBushes;
bool showOverworldSigns;
bool showDungeonSigns;
bool showOverworldWonderItems;
bool showDungeonWonderItems;
bool showBeggar;
bool showIcicles;
bool showRedIce;
bool showFrogSongRupees;
bool showFountainFairies;
bool showStoneFairies;
bool showBeanFairies;
bool showSongFairies;
bool showButterflyFairies;
bool showStartingMapsCompasses;
bool showKeysanity;
bool showGerudoFortressKeys;
bool showBossKeysanity;
bool showGanonBossKey;
bool showOcarinas;
bool show100SkullReward;
bool showLinksPocket;
bool fortressFast;
bool fortressNormal;

u8 fishsanityMode;
u8 fishsanityPondCount;
bool fishsanityAgeSplit;

// persistent during gameplay
bool initialized;
bool doAreaScroll;
bool previousShowHidden = false;
bool hideShopUnshuffledChecks = false;
bool alwaysShowGS = false;

static bool presetLoaded = false;
static ImVec2 presetPos;
static ImVec2 presetSize;

std::map<uint32_t, RandomizerCheck> startingShopItem = {
    { SCENE_KOKIRI_SHOP, RC_KF_SHOP_ITEM_1 },
    { SCENE_BAZAAR, RC_MARKET_BAZAAR_ITEM_1 },
    { SCENE_POTION_SHOP_MARKET, RC_MARKET_POTION_SHOP_ITEM_1 },
    { SCENE_BOMBCHU_SHOP, RC_MARKET_BOMBCHU_SHOP_ITEM_1 },
    { SCENE_POTION_SHOP_KAKARIKO, RC_KAK_POTION_SHOP_ITEM_1 },
    { SCENE_ZORA_SHOP, RC_ZD_SHOP_ITEM_1 },
    { SCENE_GORON_SHOP, RC_GC_SHOP_ITEM_1 },
};

std::map<SceneID, RandomizerCheckArea> DungeonRCAreasBySceneID = {
    { SCENE_DEKU_TREE, RCAREA_DEKU_TREE },
    { SCENE_DODONGOS_CAVERN, RCAREA_DODONGOS_CAVERN },
    { SCENE_JABU_JABU, RCAREA_JABU_JABUS_BELLY },
    { SCENE_FOREST_TEMPLE, RCAREA_FOREST_TEMPLE },
    { SCENE_FIRE_TEMPLE, RCAREA_FIRE_TEMPLE },
    { SCENE_WATER_TEMPLE, RCAREA_WATER_TEMPLE },
    { SCENE_SHADOW_TEMPLE, RCAREA_SHADOW_TEMPLE },
    { SCENE_SPIRIT_TEMPLE, RCAREA_SPIRIT_TEMPLE },
    { SCENE_BOTTOM_OF_THE_WELL, RCAREA_BOTTOM_OF_THE_WELL },
    { SCENE_ICE_CAVERN, RCAREA_ICE_CAVERN },
    { SCENE_GERUDO_TRAINING_GROUND, RCAREA_GERUDO_TRAINING_GROUND },
    { SCENE_INSIDE_GANONS_CASTLE, RCAREA_GANONS_CASTLE },
};

// Dungeon entrances with obvious visual differences between MQ and vanilla qualifying as spoiling on sight
std::vector<uint32_t> spoilingEntrances = {
    ENTR_DEKU_TREE_ENTRANCE,
    ENTR_DODONGOS_CAVERN_BOSS_DOOR,
    ENTR_JABU_JABU_ENTRANCE,
    ENTR_JABU_JABU_BOSS_DOOR,
    ENTR_FOREST_TEMPLE_ENTRANCE,
    ENTR_FIRE_TEMPLE_ENTRANCE,
    ENTR_FIRE_TEMPLE_BOSS_DOOR,
    ENTR_WATER_TEMPLE_BOSS_DOOR,
    ENTR_SPIRIT_TEMPLE_ENTRANCE,
    ENTR_SHADOW_TEMPLE_BOSS_DOOR,
    ENTR_ICE_CAVERN_ENTRANCE,
    ENTR_GERUDO_TRAINING_GROUND_ENTRANCE,
    ENTR_INSIDE_GANONS_CASTLE_ENTRANCE,
};

std::map<RandomizerCheckArea, std::vector<RandomizerCheck>> checksByArea;
bool areasFullyChecked[RCAREA_INVALID];
u32 areasSpoiled = 0;
bool showVOrMQ;
s16 areaChecksGotten[RCAREA_INVALID]; //|     "Kokiri Forest (4/9)"
s16 areaChecksAvailable[RCAREA_INVALID];
s16 areaCheckTotals[RCAREA_INVALID];
uint16_t totalChecks = 0;
uint16_t totalChecksAvailable = 0;
uint16_t totalChecksGotten = 0;
bool optCollapseAll; // A bool that will collapse all checks once
bool optExpandAll;   // A bool that will expand all checks once
RandomizerCheck lastLocationChecked = RC_UNKNOWN_CHECK;
RandomizerCheckArea previousArea = RCAREA_INVALID;
RandomizerCheckArea currentArea = RCAREA_INVALID;
OSContPad* trackerButtonsPressed;
std::unordered_map<RandomizerCheck, std::string> checkNameOverrides;

bool ShouldShowCheck(RandomizerCheck rc);
bool UpdateFilters();
bool CompareChecks(RandomizerCheck, RandomizerCheck);
bool CheckByArea(RandomizerCheckArea);
void DrawLocation(RandomizerCheck);
void LoadSettings();
void RainbowTick();
void UpdateAreas(RandomizerCheckArea area);
void UpdateInventoryChecks();
void UpdateOrdering(RandomizerCheckArea);
int sectionId;

bool hideUnchecked = false;
bool hideCategory = false;
bool hideScummed = false;
bool hideSeen = false;
bool hideSkipped = false;
bool hideSaved = false;
bool hideCollected = false;
bool showHidden = true;
bool mystery = false;
bool showLogicTooltip = false;
bool enableAvailableChecks = false;
bool onlyShowAvailable = false;

SceneID DungeonSceneLookupByArea(RandomizerCheckArea area) {
    switch (area) {
        case RCAREA_DEKU_TREE:
            return SCENE_DEKU_TREE;
        case RCAREA_DODONGOS_CAVERN:
            return SCENE_DODONGOS_CAVERN;
        case RCAREA_JABU_JABUS_BELLY:
            return SCENE_JABU_JABU;
        case RCAREA_FOREST_TEMPLE:
            return SCENE_FOREST_TEMPLE;
        case RCAREA_FIRE_TEMPLE:
            return SCENE_FIRE_TEMPLE;
        case RCAREA_WATER_TEMPLE:
            return SCENE_WATER_TEMPLE;
        case RCAREA_SPIRIT_TEMPLE:
            return SCENE_SPIRIT_TEMPLE;
        case RCAREA_SHADOW_TEMPLE:
            return SCENE_SHADOW_TEMPLE;
        case RCAREA_BOTTOM_OF_THE_WELL:
            return SCENE_BOTTOM_OF_THE_WELL;
        case RCAREA_ICE_CAVERN:
            return SCENE_ICE_CAVERN;
        case RCAREA_GERUDO_TRAINING_GROUND:
            return SCENE_GERUDO_TRAINING_GROUND;
        case RCAREA_GANONS_CASTLE:
            return SCENE_INSIDE_GANONS_CASTLE;
        default:
            return SCENE_ID_MAX;
    }
}

const Color_RGBA8 Color_Main_Default = { 255, 255, 255, 255 };                  // White
const Color_RGBA8 Color_Area_Incomplete_Extra_Default = { 255, 255, 255, 255 }; // White
const Color_RGBA8 Color_Area_Complete_Extra_Default = { 255, 255, 255, 255 };   // White
const Color_RGBA8 Color_Unchecked_Extra_Default = { 255, 255, 255, 255 };       // White
const Color_RGBA8 Color_Skipped_Main_Default = { 160, 160, 160, 255 };          // Grey
const Color_RGBA8 Color_Skipped_Extra_Default = { 160, 160, 160, 255 };         // Grey
const Color_RGBA8 Color_Seen_Extra_Default = { 255, 255, 255, 255 };            // TODO
const Color_RGBA8 Color_Hinted_Extra_Default = { 193, 84, 216, 255 };           // Purple
const Color_RGBA8 Color_Collected_Extra_Default = { 242, 101, 34, 255 };        // Orange
const Color_RGBA8 Color_Scummed_Extra_Default = { 0, 174, 239, 255 };           // Blue
const Color_RGBA8 Color_Saved_Extra_Default = { 0, 185, 0, 255 };               // Green

Color_RGBA8 Color_Background = { 0, 0, 0, 255 };

Color_RGBA8 Color_Area_Incomplete_Main = { 255, 255, 255, 255 };  // White
Color_RGBA8 Color_Area_Incomplete_Extra = { 255, 255, 255, 255 }; // White
Color_RGBA8 Color_Area_Complete_Main = { 255, 255, 255, 255 };    // White
Color_RGBA8 Color_Area_Complete_Extra = { 255, 255, 255, 255 };   // White
Color_RGBA8 Color_Unchecked_Main = { 255, 255, 255, 255 };        // White
Color_RGBA8 Color_Unchecked_Extra = { 255, 255, 255, 255 };       // Useless
Color_RGBA8 Color_Skipped_Main = { 160, 160, 160, 255 };          // Grey
Color_RGBA8 Color_Skipped_Extra = { 160, 160, 160, 255 };         // Grey
Color_RGBA8 Color_Seen_Main = { 255, 255, 255, 255 };             // TODO
Color_RGBA8 Color_Seen_Extra = { 160, 160, 160, 255 };            // TODO
Color_RGBA8 Color_Hinted_Main = { 255, 255, 255, 255 };           // White
Color_RGBA8 Color_Hinted_Extra = { 193, 84, 216, 255 };           // Purple
Color_RGBA8 Color_Collected_Main = { 255, 255, 255, 255 };        // White
Color_RGBA8 Color_Collected_Extra = { 242, 101, 34, 255 };        // Orange
Color_RGBA8 Color_Scummed_Main = { 255, 255, 255, 255 };          // White
Color_RGBA8 Color_Scummed_Extra = { 0, 174, 239, 255 };           // Blue
Color_RGBA8 Color_Saved_Main = { 255, 255, 255, 255 };            // White
Color_RGBA8 Color_Saved_Extra = { 0, 185, 0, 255 };               // Green

static ImGuiTextFilter checkSearch;
static bool recalculateAvailable = false;
static RandomizerRegion availableChecksStartingRegion = RR_ROOT;
static RandoAgeTime availableChecksStartingAgeTime = RAT_NONE;
static int16_t previousEntrance = 0;
std::array<bool, RCAREA_INVALID> filterAreasHidden = { 0 };
std::array<bool, RC_MAX> filterChecksHidden = { 0 };

void TrySetAreas() {
    if (checksByArea.empty()) {
        for (int i = RCAREA_KOKIRI_FOREST; i < RCAREA_INVALID; i++) {
            checksByArea.emplace(static_cast<RandomizerCheckArea>(i), std::vector<RandomizerCheck>());
        }
    }
}

void CalculateTotals() {
    totalChecks = 0;
    totalChecksAvailable = 0;
    totalChecksGotten = 0;

    for (uint8_t i = 0; i < RCAREA_INVALID; i++) {
        totalChecks += areaCheckTotals[i];
        totalChecksAvailable += areaChecksAvailable[i];
        totalChecksGotten += areaChecksGotten[i];
    }
}

uint16_t GetTotalChecks() {
    return totalChecks;
}

uint16_t GetTotalChecksGotten() {
    return totalChecksGotten;
}

bool IsCheckHidden(RandomizerCheck rc) {
    Rando::ItemLocation* itemLocation = OTRGlobals::Instance->gRandoContext->GetItemLocation(rc);
    RandomizerCheckStatus status = itemLocation->GetCheckStatus();
    bool available = itemLocation->IsAvailable();
    bool skipped = itemLocation->GetIsSkipped();
    bool obtained = itemLocation->HasObtained();
    bool seen = status == RCSHOW_SEEN_OR_HINTED || status == RCSHOW_IDENTIFIED;
    bool category = status == RCSHOW_CATEGORY;
    bool scummed = status == RCSHOW_SCUMMED;
    bool unchecked = status == RCSHOW_UNCHECKED;

    return !showHidden && ((skipped && hideSkipped) || (seen && hideSeen) || (category && hideCategory) ||
                           (scummed && hideScummed) || (unchecked && hideUnchecked));
}

void RecalculateAreaTotals(RandomizerCheckArea rcArea) {
    areaChecksGotten[rcArea] = 0;
    areaChecksAvailable[rcArea] = 0;
    areaCheckTotals[rcArea] = 0;
    for (auto rc : checksByArea.at(rcArea)) {
        if (!IsVisibleInCheckTracker(rc)) {
            continue;
        }
        areaCheckTotals[rcArea]++;

        Rando::ItemLocation* itemLoc = OTRGlobals::Instance->gRandoContext->GetItemLocation(rc);

        if (itemLoc->GetIsSkipped() || itemLoc->HasObtained()) {
            areaChecksGotten[rcArea]++;
        }

        if (itemLoc->IsAvailable() && !IsCheckHidden(rc)) {
            areaChecksAvailable[rcArea]++;
        }
    }
    CalculateTotals();
}

std::map<RandomizerGet, RandomizerCheckArea> MapRGtoRandomizerCheckArea = {
    { RG_DEKU_TREE_MAP, RCAREA_DEKU_TREE },
    { RG_DODONGOS_CAVERN_MAP, RCAREA_DODONGOS_CAVERN },
    { RG_JABU_JABUS_BELLY_MAP, RCAREA_JABU_JABUS_BELLY },
    { RG_FOREST_TEMPLE_MAP, RCAREA_FOREST_TEMPLE },
    { RG_FIRE_TEMPLE_MAP, RCAREA_FIRE_TEMPLE },
    { RG_WATER_TEMPLE_MAP, RCAREA_WATER_TEMPLE },
    { RG_SPIRIT_TEMPLE_MAP, RCAREA_SPIRIT_TEMPLE },
    { RG_SHADOW_TEMPLE_MAP, RCAREA_SHADOW_TEMPLE },
    { RG_BOTTOM_OF_THE_WELL_MAP, RCAREA_BOTTOM_OF_THE_WELL },
    { RG_ICE_CAVERN_MAP, RCAREA_ICE_CAVERN }
};

void SpoilAreaFromCheck(RandomizerCheck rc) {
    Rando::Location* loc = Rando::StaticData::GetLocation(rc);
    Rando::ItemLocation* itemLoc = Rando::Context::GetInstance()->GetItemLocation(rc);
    if (itemLoc->GetPlacedItem().GetItemType() == ItemType::ITEMTYPE_MAP) {
        RandomizerCheckArea area = MapRGtoRandomizerCheckArea[itemLoc->GetPlacedRandomizerGet()];
        if (!IsAreaSpoiled(area)) {
            SetAreaSpoiled(area);
        }
    }
    if (!IsAreaSpoiled(loc->GetArea())) {
        SetAreaSpoiled(loc->GetArea());
    }
}

void RecalculateAllAreaTotals() {
    for (auto& [rcArea, checks] : checksByArea) {
        if (rcArea == RCAREA_INVALID) {
            return;
        }
        RecalculateAreaTotals(rcArea);
    }
}

void SetCheckCollected(RandomizerCheck rc) {
    OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->SetCheckStatus(RCSHOW_COLLECTED);
    Rando::Location* loc = Rando::StaticData::GetLocation(rc);
    if (IsVisibleInCheckTracker(rc)) {
        if (!OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->GetIsSkipped()) {
            areaChecksGotten[loc->GetArea()]++;
            areaChecksAvailable[loc->GetArea()]--;
        } else {
            OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->SetIsSkipped(false);
        }
    }
    SaveManager::Instance->SaveSection(gSaveContext.fileNum, sectionId, true);

    if (!IsAreaSpoiled(loc->GetArea())) {
        SetAreaSpoiled(loc->GetArea());
    }

    doAreaScroll = true;
    UpdateOrdering(loc->GetArea());
    UpdateInventoryChecks();
}

bool IsAreaScene(SceneID sceneNum) {
    switch (sceneNum) {
        case SCENE_HYRULE_FIELD:
        case SCENE_KAKARIKO_VILLAGE:
        case SCENE_GRAVEYARD:
        case SCENE_ZORAS_RIVER:
        case SCENE_KOKIRI_FOREST:
        case SCENE_SACRED_FOREST_MEADOW:
        case SCENE_LAKE_HYLIA:
        case SCENE_ZORAS_DOMAIN:
        case SCENE_ZORAS_FOUNTAIN:
        case SCENE_GERUDO_VALLEY:
        case SCENE_LOST_WOODS:
        case SCENE_DESERT_COLOSSUS:
        case SCENE_GERUDOS_FORTRESS:
        case SCENE_HAUNTED_WASTELAND:
        case SCENE_HYRULE_CASTLE:
        case SCENE_DEATH_MOUNTAIN_TRAIL:
        case SCENE_DEATH_MOUNTAIN_CRATER:
        case SCENE_GORON_CITY:
        case SCENE_LON_LON_RANCH:
        case SCENE_DEKU_TREE:
        case SCENE_DODONGOS_CAVERN:
        case SCENE_JABU_JABU:
        case SCENE_FOREST_TEMPLE:
        case SCENE_FIRE_TEMPLE:
        case SCENE_WATER_TEMPLE:
        case SCENE_SPIRIT_TEMPLE:
        case SCENE_SHADOW_TEMPLE:
        case SCENE_BOTTOM_OF_THE_WELL:
        case SCENE_ICE_CAVERN:
        case SCENE_GERUDO_TRAINING_GROUND:
        case SCENE_GANONS_TOWER:
        case SCENE_INSIDE_GANONS_CASTLE:
        case SCENE_BACK_ALLEY_DAY:
        case SCENE_BACK_ALLEY_NIGHT:
        case SCENE_MARKET_DAY:
        case SCENE_MARKET_NIGHT:
        case SCENE_MARKET_RUINS:
            return true;
        default:
            return false;
    }
}

RandomizerCheckArea AreaFromEntranceGroup[] = {
    RCAREA_INVALID,          RCAREA_KOKIRI_FOREST, RCAREA_LOST_WOODS,           RCAREA_SACRED_FOREST_MEADOW,
    RCAREA_KAKARIKO_VILLAGE, RCAREA_GRAVEYARD,     RCAREA_DEATH_MOUNTAIN_TRAIL, RCAREA_DEATH_MOUNTAIN_CRATER,
    RCAREA_GORON_CITY,       RCAREA_ZORAS_RIVER,   RCAREA_ZORAS_DOMAIN,         RCAREA_ZORAS_FOUNTAIN,
    RCAREA_HYRULE_FIELD,     RCAREA_LON_LON_RANCH, RCAREA_LAKE_HYLIA,           RCAREA_GERUDO_VALLEY,
    RCAREA_GERUDO_FORTRESS,  RCAREA_WASTELAND,     RCAREA_DESERT_COLOSSUS,      RCAREA_MARKET,
    RCAREA_HYRULE_CASTLE,
};

RandomizerCheckArea GetCheckArea() {
    auto scene = static_cast<SceneID>(gPlayState->sceneNum);
    bool grottoScene = (scene == SCENE_GROTTOS || scene == SCENE_FAIRYS_FOUNTAIN);
    const EntranceData* ent = EntranceTracker::GetEntranceData(
        grottoScene ? ENTRANCE_GROTTO_EXIT_START + EntranceTracker::GetCurrentGrottoId() : gSaveContext.entranceIndex);
    RandomizerCheckArea area = RCAREA_INVALID;
    if (ent != nullptr && !IsAreaScene(scene) && ent->type != ENTRANCE_TYPE_DUNGEON) {
        if (ent->source == "Desert Colossus" || ent->destination == "Desert Colossus") {
            area = RCAREA_DESERT_COLOSSUS;
        } else {
            area = AreaFromEntranceGroup[ent->dstGroup];
        }
    }
    if (area == RCAREA_INVALID) {
        if (grottoScene && (EntranceTracker::GetCurrentGrottoId() == -1) &&
            (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_GROTTO_ENTRANCES) == RO_GENERIC_OFF)) {
            area = previousArea;
        } else {
            area = RandomizerCheckObjects::GetRCAreaBySceneID(scene);
        }
    }
    return area;
}

std::array<SceneID, 4> skipScenes = {
    SCENE_GANON_BOSS,
    SCENE_GANONS_TOWER_COLLAPSE_EXTERIOR,
    SCENE_INSIDE_GANONS_CASTLE_COLLAPSE,
    SCENE_GANONS_TOWER_COLLAPSE_INTERIOR,
};

void ClearAreaChecksAndTotals() {
    for (auto& [rcArea, vec] : checksByArea) {
        vec.clear();
        areaChecksGotten[rcArea] = 0;
        areaChecksAvailable[rcArea] = 0;
        areaCheckTotals[rcArea] = 0;
    }
    totalChecks = 0;
    totalChecksGotten = 0;
    totalChecksAvailable = 0;
}

void SetShopSeen(uint32_t sceneNum, bool prices) {
    RandomizerCheck start = startingShopItem.find(sceneNum)->second;
    if (sceneNum == SCENE_POTION_SHOP_KAKARIKO && !LINK_IS_ADULT) {
        return;
    }
    if (GetCheckArea() == RCAREA_KAKARIKO_VILLAGE && sceneNum == SCENE_BAZAAR) {
        start = RC_KAK_BAZAAR_ITEM_1;
    }
    bool statusChanged = false;
    for (int i = start; i < start + 8; i++) {
        if (OTRGlobals::Instance->gRandoContext->GetItemLocation(i)->GetCheckStatus() == RCSHOW_UNCHECKED) {
            OTRGlobals::Instance->gRandoContext->GetItemLocation(i)->SetCheckStatus(RCSHOW_SEEN_OR_HINTED);
            statusChanged = true;
        }
    }
    if (statusChanged) {
        SaveManager::Instance->SaveSection(gSaveContext.fileNum, sectionId, true);
    }
}

// Items share hint text keys: all six jabber nuts are "the ability to speak".
// Counted once on first use.
static bool HintNamesItemUniquely(RandomizerGet rg) {
    static const auto keyUses = [] {
        std::array<uint16_t, RHT_MAX> uses{};
        for (const auto& item : Rando::StaticData::GetItemTable()) {
            uses[item.GetHintKey()]++;
        }
        return uses;
    }();
    return keyUses[Rando::StaticData::RetrieveItem(rg).GetHintKey()] == 1;
}

// Only HINT_TYPE_ITEM hints name a check's item outright; other types stay
// ambiguous. Marks Seen, not Identified, since hints never state a price.
static bool ApplyItemHintToChecks(RandomizerHint hintKey) {
    // Ambiguous/obscure hints reuse the same phrase across items (all four swords are
    // just "a sword"), so only clear hints are safe to mark - skip anything else
    if (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_HINT_CLARITY) != RO_HINT_CLARITY_CLEAR) {
        return false;
    }

    if (hintKey == RH_NONE) {
        return false;
    }

    // The hint-revealed hook can fire for hints the seed has disabled.
    auto hint = OTRGlobals::Instance->gRandoContext->GetHint(hintKey);
    if (!hint->IsEnabled() || hint->GetHintType() != HINT_TYPE_ITEM) {
        return false;
    }

    // Loop over hinted locations, apply the ones which are unambiguous
    bool changed = false;
    for (RandomizerCheck rc : hint->GetHintedLocations()) {
        if (rc == RC_UNKNOWN_CHECK) {
            continue;
        }
        auto loc = OTRGlobals::Instance->gRandoContext->GetItemLocation(rc);
        // Ice traps hint, and display, as their disguise.
        RandomizerGet named = loc->GetPlacedRandomizerGet();
        auto& overrides = OTRGlobals::Instance->gRandoContext->overrides;
        if (named == RG_ICE_TRAP && overrides.contains(rc)) {
            named = overrides[rc].LooksLike();
        }
        if (!HintNamesItemUniquely(named)) {
            // The hint could mean several items, no spoilers!
            continue;
        }
        // A hint names the item, so it outranks a chest's category-only sighting.
        RandomizerCheckStatus status = loc->GetCheckStatus();
        if (status == RCSHOW_UNCHECKED || status == RCSHOW_CATEGORY) {
            loc->SetCheckStatus(RCSHOW_SEEN_OR_HINTED);
            changed = true;
        }
    }
    return changed;
}

void CheckTrackerHintRevealed(RandomizerHint hintKey) {
    if (!GameInteractor::IsSaveLoaded() || !IS_RANDO) {
        return;
    }
    if (ApplyItemHintToChecks(hintKey)) {
        SaveManager::Instance->SaveSection(gSaveContext.fileNum, sectionId, true);
    }
}

// Whether the player has actually seen a check is answered by the depth buffer
// of the finished frame, not by gameplay collision: isDrawn only means the draw
// function ran, and BgCheck line tests are blind to ordinary actors, so a chest
// behind a boulder read as visible. Every candidate is sampled at a few points
// and counts only where the frontmost surface at that pixel is the candidate's
// own - anything nearer is an occluder, anything further means the sample
// missed the model and saw past it.

static constexpr int kSeenFramesRequired = 8; // consecutive gameplay frames looked at
static constexpr int kSeenProbesPerCandidate = 5;
static constexpr int kSeenProbeMatchesRequired = 3;
// Metal's shader takes at most 1024 coordinates and OpenGL blits one per point,
// so the per-frame budget is capped well below either.
static constexpr int kSeenMaxProbeCoordinates = 48;
// Below this projected radius (in N64 screen units) a candidate is too small to
// recognise, whatever the depth buffer says.
static constexpr float kSeenMinScreenRadius = 3.0f;

// The live budget. Debug-only override, so scheduling behaviour above the budget
// can be reproduced without needing ten candidates on screen at once.
static int SeenProbeBudget() {
    if (CVarGetInteger(CVAR_TRACKER_CHECK("ProbeOverlay"), 0) != 0) {
        int budget = CVarGetInteger(CVAR_TRACKER_CHECK("ProbeBudget"), kSeenMaxProbeCoordinates);
        if (budget >= kSeenProbesPerCandidate && budget <= kSeenMaxProbeCoordinates) {
            return budget;
        }
    }
    return kSeenMaxProbeCoordinates;
}

// Items have no front boundary to measure a band against, so they keep a flat
// depth tolerance - generous, because the default profile is generic. It carries
// the same distance problem the chest slop below solves, and is left alone only
// because world items are not part of this pass.
static constexpr int32_t kSeenItemDepthTolerance = 40;

// What the chest tolerance actually covers is the disagreement between a point
// projected on the CPU and the pixel the GPU rasterised, which is a fixed
// displacement in *screen* space. Depth units are not, so the equivalent has to
// be measured at each probe's own distance: a constant is sub-unit slop up close
// and a window hundreds of world units deep at range. Calibrated against Mido,
// where the old constant 24 worked out at 1.3 px near and 3.3 px far.
static constexpr float kSeenProbeSlopPixels = 3.0f;
// The sampler quantises, so never ask for a band finer than it can resolve.
static constexpr int32_t kSeenProbeMinTolerance = 2;

// Half the range a projected depth is encoded into. Lights_GlowCheck uses the
// N64-native 16352, but the depth returned here has been through the host's
// buffer: gfx_opengl.cpp reads GL_UNSIGNED_INT_24_8 and returns
// (value >> 18) << 2, which SoH then halves, landing on a full-scale 16384.
// Encoding against 16352 and comparing against that is a 0.2% mismatch - only
// ~60 depth units, but the band closes to single digits at range, so it read as
// every probe seeing past the model.
static constexpr int32_t kSeenDepthScale = 16384;

// How far a chest may be and still disclose. Not a limit of the depth buffer,
// which reads correctly well past this - it is where a mark stops being
// something the player could reasonably have read off the chest themselves. At
// this range the accepted window is around 13 world units, and a grotto chest
// seen from the entrance sits at roughly 650.
static constexpr float kSeenMaxProbeDistance = 900.0f;

// Keyed by check, not actor, so respawns/room reloads can't dangle it.
static std::unordered_map<RandomizerCheck, int> seenItemFrames;

// One depth sample: where to look, and the depth the candidate's own surface
// should be found at.
struct SeenProbePoint {
    float x;
    float y;
    // x squeezed into the native field the depth sampler assumes. See
    // SeenProjectPoint: sample with this, display and bound with x.
    float sampleX;
    int32_t expectedZ;
    // Depth of the candidate's own front surface on the line to the eye. Chests
    // accept the band between the two; items still use expectedZ alone.
    int32_t nearZ;
    // Slop either side of the band, measured at this probe's own distance.
    int32_t tolerance;
};

// Evidence captured while a frame was being built, held until that frame's
// depth buffer can be read. Deliberately carries no Actor*: results are
// consumed a frame later, by which point the actor may be gone.
struct SeenProbeRecord {
    RandomizerCheck rc;
    bool isChest;
    GetItemCategory category;
    GetItemEntry drawnEntry;
    int32_t slabHalfWidth;
    int16_t sceneNum;
    float distance; // camera to candidate centre, diagnostics only
    int pointCount;
    SeenProbePoint points[kSeenProbesPerCandidate];
};

// Built during the gameplay update; promoted to inFlight once that frame's
// display list is complete, and read back on the following callback.
static std::vector<SeenProbeRecord> seenProbesPending;
static std::vector<SeenProbeRecord> seenProbesInFlight;
static uint32_t seenProbesInFlightFrame = 0;
static uint32_t seenProbesPendingFrame = 0;
static uint32_t seenProbesLastEvaluated = UINT32_MAX;
// Every gameplay frame produces a batch, empty ones included: an empty batch is
// what makes "consecutive" mean consecutive, since it resets every counter.
static bool seenProbesPendingValid = false;
static bool seenProbesInFlightValid = false;

// Diagnostics, surfaced in the tracker's settings window.
static int seenProbeCoordinatesLastFrame = 0;
static int seenProbeCandidatesLastFrame = 0;
static int seenProbeDeferredLastFrame = 0; // eligible, but no budget left this frame
static int seenProbeRejectedLastFrame = 0; // eligible, but would not project usefully
static int seenProbeMatchesLastFrame = 0;
// Fairness is keyed by check, not by position: the eligible list is rebuilt from
// the actor lists every frame and its order is not stable.
static RandomizerCheck seenProbeScheduleAfter = RC_UNKNOWN_CHECK;

// Snapshot of the last evaluated batch, kept only while the overlay is open.
// The probe geometry and tolerances are guesses until they are calibrated
// against this, so it shows every number the decision was made from.
struct SeenProbeDebugPoint {
    float x;
    float y;
    int32_t expectedZ;
    int32_t nearZ;
    int32_t sampledZ;
    bool matched;
};
struct SeenProbeDebugRecord {
    RandomizerCheck rc;
    bool isChest;
    bool wasSampled; // false means deferred for want of budget
    int matches;
    int32_t tolerance;
    int pointCount;
    int framesBanked;
    float distance;
    SeenProbeDebugPoint points[kSeenProbesPerCandidate];
};
static std::vector<SeenProbeDebugRecord> seenProbeDebug;
static uint32_t seenProbeDebugFrame = 0;

// Instrumentation for the probe-geometry rework: what the collision header
// reports, and the two candidate placement transforms. Read-only - the
// predicate still runs on the old constants.
struct SeenChestGeometryDebug {
    RandomizerCheck rc;
    int32_t bgId;
    const char* colReason;   // null once the collision lookup is usable
    const char* probeReason; // why probe building gave up, null if it did not
    int projected;           // probes that landed on screen
    float spanX;             // achieved screen span, N64 units
    float spanY;
    float distance; // camera to AABB centre, euclidean
    float axial;    // the same, projected onto the view axis: what depth is a function of
    Vec3s minBounds;
    Vec3s maxBounds;
    Vec3f centre;     // model units
    Vec3f halfExtent; // model units
    ScaleRotPos dyna; // BgActor.curTransform
    ScaleRotPos root; // placement as Actor_Draw builds it
};
static std::vector<SeenChestGeometryDebug> seenChestGeometryDebug;

// What a depth tolerance actually admits, in world units. Depth is non-linear,
// so a constant number of depth units is a window that grows with the square of
// the viewing distance: sub-unit slop up close, hundreds of units far away.
// Derived rather than stored, so it cannot disagree with the tolerance it came
// from.
static float SeenDepthWindowWorld(float distance, int32_t tolerance) {
    if (gPlayState == nullptr || !(distance > 0.0f)) {
        return 0.0f;
    }
    return tolerance * distance * distance / (kSeenDepthScale * 2.0f * gPlayState->view.zNear);
}

// What kSeenProbeSlopPixels is worth in world units at a given axial depth -
// the accepted window either side of the band, now growing linearly with
// distance rather than with its square.
static float SeenProbeSlopWorld(float axial) {
    if (gPlayState == nullptr) {
        return 0.0f;
    }
    float tangent = tanf(gPlayState->view.fovy * 0.5f * 0.017453293f);
    return tangent > 0.0001f ? kSeenProbeSlopPixels * axial * tangent / 120.0f : 0.0f;
}

// Inverse of the depth encoding in SeenProjectPoint: the axial depth a stored
// value corresponds to. Only the projection's own near/far are involved, so a
// disagreement with the measured geometry means the two are not describing the
// same point.
static float SeenDepthToAxial(int32_t depth) {
    if (gPlayState == nullptr) {
        return 0.0f;
    }
    float zn = gPlayState->view.zNear;
    float zf = gPlayState->view.zFar;
    float ndc = (depth - kSeenDepthScale) / static_cast<float>(kSeenDepthScale);
    float denom = (zn + zf) / (zf - zn) - ndc;
    return denom > 1e-6f ? (2.0f * zn * zf / (zf - zn)) / denom : 0.0f;
}

// The check types a world sighting may mark, as an allowlist: most shuffle
// features route their items through EnItem00, so a new one has to opt in here
// rather than start marking on its own. Freestanding rupees and hearts are left
// out deliberately - they sit inside bushes and grass, whose actor collision the
// occlusion ray can't see, so they would mark while hidden.
static bool SeenItemTypeAllowed(RandomizerCheck rc) {
    switch (Rando::StaticData::GetLocation(rc)->GetRCType()) {
        case RCTYPE_STANDARD:
        case RCTYPE_SKULL_TOKEN:
        case RCTYPE_BOSS_HEART_OR_OTHER_REWARD:
            return true;
        default:
            return false;
    }
}

// Resolves an actor's check + the item entry its draw function renders.
// False when the actor carries no check identity.
static bool SeenItemIdentity(Actor* actor, RandomizerCheck& rc, GetItemEntry& drawnEntry) {
    switch (actor->id) {
        case ACTOR_EN_ITEM00: {
            EnItem00* item00 = reinterpret_cast<EnItem00*>(actor);
            if (item00->actor.params != ITEM00_SOH_DUMMY) {
                return false;
            }
            // Heart piece / small key drops only stamp the entry, not the check.
            rc = item00->randoCheck != RC_UNKNOWN_CHECK ? item00->randoCheck
                                                        : OTRGlobals::Instance->gRandomizer->GetCheckFromActor(
                                                              actor->id, gPlayState->sceneNum, item00->ogParams);
            drawnEntry = item00->itemEntry;
            break;
        }
        case ACTOR_EN_SI: {
            // Token drops encode their GS flag in params; unshuffled ones miss.
            rc = OTRGlobals::Instance->gRandomizer->GetCheckFromActor(actor->id, gPlayState->sceneNum, actor->params);
            drawnEntry = reinterpret_cast<EnSi*>(actor)->sohGetItemEntry;
            break;
        }
        case ACTOR_ITEM_B_HEART: {
            rc = OTRGlobals::Instance->gRandomizer->GetCheckFromActor(actor->id, gPlayState->sceneNum, actor->params);
            drawnEntry = reinterpret_cast<ItemBHeart*>(actor)->sohItemEntry;
            break;
        }
        case ACTOR_ITEM_ETCETERA: {
            // Excludes the chest-game rupees: they draw through the Lens of Truth.
            int32_t type = actor->params & 0xFF;
            if (type != ITEM_ETC_LETTER && type != ITEM_ETC_ARROW_FIRE) {
                return false;
            }
            rc = OTRGlobals::Instance->gRandomizer->GetCheckFromActor(actor->id, gPlayState->sceneNum, actor->params);
            drawnEntry = reinterpret_cast<ItemEtcetera*>(actor)->sohItemEntry;
            break;
        }
        case ACTOR_EN_EX_ITEM: {
            // Only the prizes on display at the Bombchu Bowling counter: the awarded
            // variants and the Lost Woods target's fly straight to Link, so seeing
            // one tells the player nothing they aren't about to be told anyway.
            EnExItem* exItem = reinterpret_cast<EnExItem*>(actor);
            switch (exItem->type) {
                case EXITEM_BOMB_BAG_COUNTER:
                    rc = RC_MARKET_BOMBCHU_BOWLING_FIRST_PRIZE;
                    break;
                case EXITEM_HEART_PIECE_COUNTER:
                    rc = RC_MARKET_BOMBCHU_BOWLING_SECOND_PRIZE;
                    break;
                default:
                    return false;
            }
            drawnEntry = exItem->sohItemEntry;
            break;
        }
        default:
            return false;
    }
    return rc != RC_UNKNOWN_CHECK && SeenItemTypeAllowed(rc);
}

// Screen position and depth of a world point, in the N64 320x240 space that
// OTRGetPixelDepth expects. Follows Lights_GlowCheck's projection, but encodes
// depth against the sampler's scale rather than the N64's - see kSeenDepthScale.
static bool SeenProjectPoint(const Vec3f& world, SeenProbePoint& out) {
    Vec3f projected;
    float w;
    Actor_ProjectPos(gPlayState, const_cast<Vec3f*>(&world), &projected, &w);
    if (projected.z <= 1.0f) { // behind the camera
        return false;
    }
    out.x = projected.x * w * 160.0f + 160.0f;
    out.y = projected.y * w * 120.0f + 120.0f;
    out.expectedZ = static_cast<int32_t>((projected.z * w) * kSeenDepthScale) + kSeenDepthScale;
    out.nearZ = out.expectedZ; // callers with a real boundary overwrite this
    // The depth sampler stretches the native SCREEN_WIDTH field across the whole
    // framebuffer, while a widescreen projection puts the visible edges outside
    // that field. Squeeze x back before sampling or every off-centre probe reads
    // the wrong column, by more the further it sits from screen centre.
    float left = OTRGetDimensionFromLeftEdge(0.0f);
    float right = OTRGetDimensionFromRightEdge(SCREEN_WIDTH);
    out.sampleX = right > left ? (out.x - left) / (right - left) * SCREEN_WIDTH : out.x;
    return true;
}

// Widescreen shows more than the 320-wide native field, so the horizontal
// bounds have to come from the active aspect rather than 0..SCREEN_WIDTH.
static bool SeenPointOnScreen(const SeenProbePoint& point) {
    return point.x >= OTRGetDimensionFromLeftEdge(0.0f) && point.x <= OTRGetDimensionFromRightEdge(SCREEN_WIDTH) &&
           point.y >= 0.0f && point.y <= SCREEN_HEIGHT;
}

static float SeenVecLength(const Vec3f& v) {
    return sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);
}

// Builds probes from model-space points transformed by whatever matrix the draw
// is currently using, so scale, rotation and animation come from the render
// rather than being reconstructed. Tolerance is deliberately narrow: a point
// that does not land on the surface it was meant for fails, which is the safe
// direction.
// A shuffled chest currently showing its item category (CSMC), whose category
// the player can therefore read off the chest. Mirrors EnBox_UpdateTexture's
// gate so we only mark when the chest actually looks the part.
static bool SeenChestCategory(Actor* actor, RandomizerCheck& rc) {
    if (actor->id != ACTOR_EN_BOX || !CVarGetInteger(CVAR_ENHANCEMENT("ChestSizeAndTextureMatchContents"), 0) ||
        (CVarGetInteger(CVAR_ENHANCEMENT("ChestSizeDependsStoneOfAgony"), 0) &&
         !CHECK_QUEST_ITEM(QUEST_STONE_OF_AGONY)) ||
        (gPlayState->sceneNum == SCENE_TREASURE_BOX_SHOP && actor->room != 6)) {
        return false;
    }
    // Lens chests stay excluded until the surface probes are proven; their
    // aperture is already in the depth buffer, so this is temporary.
    EnBox* box = reinterpret_cast<EnBox*>(actor);
    if (box->type == ENBOX_TYPE_4 || box->type == ENBOX_TYPE_6) {
        return false;
    }
    rc = OTRGlobals::Instance->gRandomizer->GetCheckFromActor(actor->id, gPlayState->sceneNum, actor->params);
    return rc != RC_UNKNOWN_CHECK && OTRGlobals::Instance->gRandoContext->IsLocationShuffled(rc);
}

static bool SeenBuildProbesFromMatrix(const Vec3f* modelPoints, int count, int32_t tolerance, SeenProbeRecord& record) {
    record.pointCount = 0;
    record.slabHalfWidth = tolerance;
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (int i = 0; i < count && record.pointCount < kSeenProbesPerCandidate; i++) {
        Vec3f world;
        Matrix_MultVec3f(const_cast<Vec3f*>(&modelPoints[i]), &world);
        SeenProbePoint point;
        if (!SeenProjectPoint(world, point) || !SeenPointOnScreen(point)) {
            continue;
        }
        if (record.pointCount == 0) {
            const Vec3f& eye = gPlayState->view.eye;
            record.distance = SeenVecLength({ world.x - eye.x, world.y - eye.y, world.z - eye.z });
        }
        // Items have no boundary to measure against, so they keep the flat value.
        point.tolerance = tolerance;
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
        record.points[record.pointCount++] = point;
    }
    if (record.pointCount < kSeenProbeMatchesRequired) {
        return false;
    }
    // Minimum on-screen extent, in N64 screen units, so a distant speck whose
    // probes all land in one pixel cannot qualify.
    return (maxX - minX) >= kSeenMinScreenRadius || (maxY - minY) >= kSeenMinScreenRadius;
}

// Shared by both registration paths: eligibility, de-duplication and budget.
static bool SeenRegisterCommon(RandomizerCheck rc, SeenProbeRecord& record) {
    if (!seenProbesPendingValid || rc == RC_UNKNOWN_CHECK) {
        return false;
    }
    if (OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->GetCheckStatus() != RCSHOW_UNCHECKED &&
        // Debug only: keep sampling checks that have already been marked, so the
        // geometry can be re-tested from other angles without a fresh seed. The
        // consume phase still refuses to re-mark them.
        !(CVarGetInteger(CVAR_TRACKER_CHECK("ProbeOverlay"), 0) != 0 &&
          CVarGetInteger(CVAR_TRACKER_CHECK("ProbeRetestMarked"), 0) != 0)) {
        return false;
    }
    for (const auto& queued : seenProbesPending) {
        if (queued.rc == rc) {
            return false; // one record per check per frame
        }
    }
    record.rc = rc;
    record.sceneNum = gPlayState->sceneNum;
    return true;
}

// The chest's collision AABB, placed in world space. Collision supplies the
// model-space bounds only; placement is rebuilt the way Actor_Draw does it,
// because dynamic collision is set up before ACTORCAT_CHEST updates
// (z_actor.c:2703) and so lags a moving chest by a tick.
struct SeenChestObb {
    Vec3f centre;  // world
    Vec3f axis[3]; // unit, world
    float half[3]; // world
};

static bool SeenBuildChestObb(EnBox* box, SeenChestObb& obb) {
    Actor* actor = &box->dyna.actor;
    int32_t bgId = box->dyna.bgId;
    DynaCollisionContext& dyna = gPlayState->colCtx.dyna;
    // A stale or half-torn-down slot rejects the candidate; never fall back to
    // permissive geometry.
    if (bgId < 0 || bgId >= BG_ACTOR_MAX || (dyna.bgActorFlags[bgId] & 1) == 0 || (dyna.bgActorFlags[bgId] & 2) != 0 ||
        dyna.bgActors[bgId].actor != actor || dyna.bgActors[bgId].colHeader == nullptr) {
        return false;
    }
    const CollisionHeader* header = dyna.bgActors[bgId].colHeader;
    Vec3f centre = { (header->minBounds.x + header->maxBounds.x) * 0.5f,
                     (header->minBounds.y + header->maxBounds.y) * 0.5f,
                     (header->minBounds.z + header->maxBounds.z) * 0.5f };
    Vec3f halfExtent = { (header->maxBounds.x - header->minBounds.x) * 0.5f,
                         (header->maxBounds.y - header->minBounds.y) * 0.5f,
                         (header->maxBounds.z - header->minBounds.z) * 0.5f };
    if (halfExtent.x <= 0.0f || halfExtent.y <= 0.0f || halfExtent.z <= 0.0f) {
        return false;
    }

    Vec3s rot = actor->shape.rot;
    Matrix_Push();
    Matrix_SetTranslateRotateYXZ(actor->world.pos.x, actor->world.pos.y + actor->shape.yOffset * actor->scale.y,
                                 actor->world.pos.z, &rot);
    Matrix_Scale(actor->scale.x, actor->scale.y, actor->scale.z, MTXMODE_APPLY);
    Matrix_MultVec3f(&centre, &obb.centre);
    bool ok = true;
    for (int i = 0; i < 3; i++) {
        Vec3f corner = centre;
        (&corner.x)[i] += (&halfExtent.x)[i];
        Vec3f world;
        Matrix_MultVec3f(&corner, &world);
        Vec3f edge = { world.x - obb.centre.x, world.y - obb.centre.y, world.z - obb.centre.z };
        float length = SeenVecLength(edge);
        if (!(length > 0.001f)) {
            ok = false;
            break;
        }
        obb.half[i] = length;
        obb.axis[i] = { edge.x / length, edge.y / length, edge.z / length };
    }
    Matrix_Pop();
    return ok;
}

// Distance from an interior point to where the ray leaves the box. A slab test
// in the box's own axes, so this is the actual exit; the support extent would
// overshoot it and put the accepted boundary outside the chest.
static bool SeenObbExitDistance(const SeenChestObb& obb, const Vec3f& from, const Vec3f& dir, float& exit) {
    Vec3f rel = { from.x - obb.centre.x, from.y - obb.centre.y, from.z - obb.centre.z };
    float best = std::numeric_limits<float>::max();
    for (int i = 0; i < 3; i++) {
        float p = rel.x * obb.axis[i].x + rel.y * obb.axis[i].y + rel.z * obb.axis[i].z;
        float d = dir.x * obb.axis[i].x + dir.y * obb.axis[i].y + dir.z * obb.axis[i].z;
        if (fabsf(p) > obb.half[i] + 0.01f) {
            return false; // the point is not inside the box
        }
        if (fabsf(d) < 0.0001f) {
            continue; // parallel to this slab
        }
        float t = ((d > 0.0f ? obb.half[i] : -obb.half[i]) - p) / d;
        if (t < best) {
            best = t;
        }
    }
    if (!(best > 0.0f) || !std::isfinite(best) || best == std::numeric_limits<float>::max()) {
        return false;
    }
    exit = best;
    return true;
}

// Probes on a view-aligned cross-section through the box centre, so the pattern
// keeps its screen spread from any angle instead of collapsing when a local axis
// happens to point at the camera. Each probe carries the depth of the chest's own
// front surface along its line to the eye, which is what makes the accepted band
// track the non-linear depth curve at that probe's distance.
static bool SeenBuildChestProbes(EnBox* box, SeenProbeRecord& record, SeenChestGeometryDebug* diag) {
    // "Rejected" bundles several unrelated causes, and telling them apart is the
    // difference between "too far away" and "the geometry is wrong".
    auto give_up = [diag](const char* why) {
        if (diag != nullptr) {
            diag->probeReason = why;
        }
        return false;
    };
    SeenChestObb obb;
    if (!SeenBuildChestObb(box, obb)) {
        return give_up("collision unusable");
    }
    // Same view the projection uses, so cutscene and quake camera offsets agree.
    const View& view = gPlayState->view;
    Vec3f eye = view.eye;
    // Recorded before the basis checks so a rejected candidate still reports how
    // far away it was.
    record.distance = SeenVecLength({ obb.centre.x - eye.x, obb.centre.y - eye.y, obb.centre.z - eye.z });
    if (diag != nullptr) {
        diag->distance = record.distance;
    }
    Vec3f forward = { view.lookAt.x - eye.x, view.lookAt.y - eye.y, view.lookAt.z - eye.z };
    float length = SeenVecLength(forward);
    if (!(length > 0.001f)) {
        return give_up("camera basis degenerate");
    }
    forward = { forward.x / length, forward.y / length, forward.z / length };
    Vec3f right = { forward.y * view.up.z - forward.z * view.up.y, forward.z * view.up.x - forward.x * view.up.z,
                    forward.x * view.up.y - forward.y * view.up.x };
    length = SeenVecLength(right);
    if (!(length > 0.001f)) {
        return give_up("camera basis degenerate");
    }
    right = { right.x / length, right.y / length, right.z / length };
    Vec3f up = { right.y * forward.z - right.z * forward.y, right.z * forward.x - right.x * forward.z,
                 right.x * forward.y - right.y * forward.x };
    if (diag != nullptr) {
        // expectedZ is a function of this, not of the euclidean distance; the two
        // differ by the off-axis angle and nothing else.
        diag->axial = (obb.centre.x - eye.x) * forward.x + (obb.centre.y - eye.y) * forward.y +
                      (obb.centre.z - eye.z) * forward.z;
    }
    // Checked before any probe is projected, but after the diagnostics above so a
    // chest rejected on range still reports how far away it was.
    if (record.distance > kSeenMaxProbeDistance) {
        return give_up("too far");
    }

    // Centre, plus four points half way out to where the cross-section meets the
    // box. Half keeps them inside the silhouette at oblique angles, since the box
    // bounds the chest including its rounded lid.
    const float kSpreadFraction = 0.5f;
    Vec3f offsets[kSeenProbesPerCandidate];
    int count = 0;
    offsets[count++] = { 0.0f, 0.0f, 0.0f };
    const Vec3f spread[4] = { right, { -right.x, -right.y, -right.z }, up, { -up.x, -up.y, -up.z } };
    for (const Vec3f& dir : spread) {
        float reach;
        if (count >= kSeenProbesPerCandidate || !SeenObbExitDistance(obb, obb.centre, dir, reach)) {
            return give_up("cross-section missed the box");
        }
        reach *= kSpreadFraction;
        offsets[count++] = { dir.x * reach, dir.y * reach, dir.z * reach };
    }

    // Screen units per world unit at unit depth, so a pixel of slop can be turned
    // into a world offset at whatever distance each probe sits.
    float halfFov = view.fovy * 0.5f * 0.017453293f;
    float tangent = tanf(halfFov);
    if (!(tangent > 0.0001f) || !std::isfinite(tangent)) {
        return give_up("camera fov degenerate");
    }
    float screenScale = 120.0f / tangent;

    record.pointCount = 0;
    float minX = 1e9f, maxX = -1e9f, minY = 1e9f, maxY = -1e9f;
    for (int i = 0; i < count; i++) {
        Vec3f world = { obb.centre.x + offsets[i].x, obb.centre.y + offsets[i].y, obb.centre.z + offsets[i].z };
        Vec3f toEye = { eye.x - world.x, eye.y - world.y, eye.z - world.z };
        float distance = SeenVecLength(toEye);
        if (!(distance > 0.001f)) {
            continue;
        }
        toEye = { toEye.x / distance, toEye.y / distance, toEye.z / distance };
        float exit;
        // exit >= distance means the eye is inside the box; there is no front
        // surface between the two and nothing meaningful to compare against.
        if (!SeenObbExitDistance(obb, world, toEye, exit) || exit >= distance) {
            continue;
        }
        SeenProbePoint point;
        if (!SeenProjectPoint(world, point) || !SeenPointOnScreen(point)) {
            continue;
        }
        Vec3f front = { world.x + toEye.x * exit, world.y + toEye.y * exit, world.z + toEye.z * exit };
        SeenProbePoint boundary;
        if (!SeenProjectPoint(front, boundary)) {
            continue;
        }
        point.nearZ = boundary.expectedZ;
        // Slide the boundary a few pixels' worth toward the eye and project again:
        // the depth that move costs is what the slop is worth here.
        float slop = kSeenProbeSlopPixels * distance / screenScale;
        Vec3f slid = { front.x + toEye.x * slop, front.y + toEye.y * slop, front.z + toEye.z * slop };
        SeenProbePoint slidPoint;
        if (!SeenProjectPoint(slid, slidPoint)) {
            continue;
        }
        point.tolerance = std::max(boundary.expectedZ - slidPoint.expectedZ, kSeenProbeMinTolerance);
        minX = std::min(minX, point.x);
        maxX = std::max(maxX, point.x);
        minY = std::min(minY, point.y);
        maxY = std::max(maxY, point.y);
        record.points[record.pointCount++] = point;
    }
    float spanX = record.pointCount > 0 ? maxX - minX : 0.0f;
    float spanY = record.pointCount > 0 ? maxY - minY : 0.0f;
    if (diag != nullptr) {
        diag->projected = record.pointCount;
        diag->spanX = spanX;
        diag->spanY = spanY;
    }
    if (record.pointCount < kSeenProbeMatchesRequired) {
        return give_up("too few probes on screen");
    }
    if (spanX < kSeenMinScreenRadius && spanY < kSeenMinScreenRadius) {
        return give_up("screen span below minimum");
    }
    return true;
}

// Default world-item profile, in the item's own model space so the draw's scale
// applies. Deliberately tight around the origin: most randomized item models are
// solid near their centre, and a point that misses fails rather than guessing.
static const Vec3f kItemProbes[] = {
    { 0.0f, 0.0f, 0.0f }, { -6.0f, 4.0f, 0.0f }, { 6.0f, 4.0f, 0.0f }, { -6.0f, -4.0f, 0.0f }, { 6.0f, -4.0f, 0.0f },
};

// Collision supplies model-space bounds; placement comes from the actor root,
// which updates after DynaPoly_Setup has already run for the tick (z_actor.c:2703
// fires on ACTORCAT_BG, while En_Box is ACTORCAT_CHEST). Both are captured so any
// lag on a moving chest is measurable rather than assumed.
static void SeenCaptureChestGeometry(EnBox* box, RandomizerCheck rc) {
    Actor* actor = &box->dyna.actor;
    SeenChestGeometryDebug entry{};
    entry.rc = rc;
    entry.bgId = box->dyna.bgId;
    entry.root.pos = actor->world.pos;
    entry.root.pos.y += actor->shape.yOffset * actor->scale.y;
    entry.root.rot = actor->shape.rot;
    entry.root.scale = actor->scale;

    DynaCollisionContext& dyna = gPlayState->colCtx.dyna;
    int32_t bgId = entry.bgId;
    if (bgId < 0 || bgId >= BG_ACTOR_MAX) {
        entry.colReason = "bgId out of range";
    } else if ((dyna.bgActorFlags[bgId] & 1) == 0) {
        entry.colReason = "slot not allocated";
    } else if ((dyna.bgActorFlags[bgId] & 2) != 0) {
        entry.colReason = "slot pending deletion";
    } else if (dyna.bgActors[bgId].actor != actor) {
        entry.colReason = "slot owned by another actor";
    } else if (dyna.bgActors[bgId].colHeader == nullptr) {
        entry.colReason = "no collision header";
    } else {
        const BgActor& bgActor = dyna.bgActors[bgId];
        entry.minBounds = bgActor.colHeader->minBounds;
        entry.maxBounds = bgActor.colHeader->maxBounds;
        entry.centre = { (entry.minBounds.x + entry.maxBounds.x) * 0.5f, (entry.minBounds.y + entry.maxBounds.y) * 0.5f,
                         (entry.minBounds.z + entry.maxBounds.z) * 0.5f };
        entry.halfExtent = { (entry.maxBounds.x - entry.minBounds.x) * 0.5f,
                             (entry.maxBounds.y - entry.minBounds.y) * 0.5f,
                             (entry.maxBounds.z - entry.minBounds.z) * 0.5f };
        entry.dyna = bgActor.curTransform;
    }
    seenChestGeometryDebug.push_back(entry);
}

extern "C" void CheckTracker_RegisterChestProbes(EnBox* box, int32_t limbIndex) {
    if (gPlayState == nullptr || !GameInteractor::IsSaveLoaded() || !IS_RANDO || box == nullptr) {
        return;
    }
    // The geometry is actor-wide, so one registration is enough; limb 1's body
    // display list is the submission this gates on.
    if (limbIndex != 1) {
        return;
    }
    RandomizerCheck rc;
    if (!SeenChestCategory(&box->dyna.actor, rc)) {
        return;
    }
    // Ahead of the eligibility checks, so a chest that dedups out is still measurable.
    int diagIndex = -1;
    if (CVarGetInteger(CVAR_TRACKER_CHECK("ProbeOverlay"), 0)) {
        SeenCaptureChestGeometry(box, rc);
        diagIndex = static_cast<int>(seenChestGeometryDebug.size()) - 1;
    }
    SeenProbeRecord record{};
    if (!SeenRegisterCommon(rc, record)) {
        return;
    }
    record.isChest = true;
    // What the chest is texturing from, captured as displayed.
    record.category = Randomizer_AdjustItemCategory(box->getItemEntry);
    if (!SeenBuildChestProbes(box, record, diagIndex >= 0 ? &seenChestGeometryDebug[diagIndex] : nullptr)) {
        seenProbeRejectedLastFrame++;
        return;
    }
    seenProbesPending.push_back(record);
}

extern "C" void CheckTracker_RegisterItemProbes(Actor* actor, GetItemEntry entry) {
    if (gPlayState == nullptr || !GameInteractor::IsSaveLoaded() || !IS_RANDO || actor == nullptr) {
        return;
    }
    // A "?" model says nothing about the item, so it must not disclose.
    if (CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("MysteriousShuffle"), 0)) {
        return;
    }
    RandomizerCheck rc;
    GetItemEntry identified;
    if (!SeenItemIdentity(actor, rc, identified)) {
        return;
    }
    SeenProbeRecord record{};
    if (!SeenRegisterCommon(rc, record)) {
        return;
    }
    record.isChest = false;
    record.drawnEntry = entry; // the entry actually handed to GetItemEntry_Draw
    if (!SeenBuildProbesFromMatrix(kItemProbes, ARRAY_COUNT(kItemProbes), kSeenItemDepthTolerance, record)) {
        seenProbeRejectedLastFrame++;
        return;
    }
    seenProbesPending.push_back(record);
}

// Opens the batch the draw paths fill during Play_Draw. Must run at draw-begin:
// opening it later in the frame discards what those draws registered.
void CheckTrackerSeenItemsFrame() {
    if (gPlayState == nullptr || !GameInteractor::IsSaveLoaded() || !IS_RANDO) {
        return;
    }
    seenProbesPending.clear();
    seenChestGeometryDebug.clear();
    seenProbesPendingFrame = gPlayState->gameplayFrames;
    seenProbesPendingValid = true;
    seenProbeRejectedLastFrame = 0;
    // Nothing is redrawn behind the pause menu, so leave the batch empty: that
    // ends any run of consecutive frames rather than freezing it.
    if (gPlayState->pauseCtx.state != 0) {
        seenProbesPendingValid = true;
        return;
    }
    // Don't let a glimpse in one scene carry over into the next.
    static int16_t lastSceneNum = -1;
    if (gPlayState->sceneNum != lastSceneNum) {
        lastSceneNum = gPlayState->sceneNum;
        seenItemFrames.clear();
        seenProbesInFlight.clear();
        seenProbesInFlightValid = false;
    }
}

// Which records the prepare phase queued coordinates for, held until consume.
static std::vector<size_t> seenProbesSampled;
// Registered by a draw but left unsampled for want of budget. Distinct from
// absence: these must not lose banked progress, or nothing can ever converge once
// more candidates are eligible than the budget covers.
static std::unordered_set<RandomizerCheck> seenProbesDeferred;
static bool seenProbesAwaitingConsume = false;
// Highest banked count reached per check, so a counter that churns faster than the
// eye can follow is still legible in the overlay. Diagnostics only.
static std::unordered_map<RandomizerCheck, int> seenProbePeakBanked;

// Prepare phase: queue coordinates only, never read. The depth buffer here holds
// the frame built during the previous gameplay update, which is why the records
// used are that update's rather than the ones just registered.
void CheckTrackerDepthProbePrepare() {
    seenProbesSampled.clear();
    seenProbesDeferred.clear();
    seenProbesAwaitingConsume = false;
    if (gPlayState == nullptr || !GameInteractor::IsSaveLoaded() || !IS_RANDO) {
        return;
    }
    // Interpolation can run this more than once per gameplay frame; disclosure
    // must not be able to happen faster because the renderer is faster. An empty
    // batch is still evaluated - that is what resets progress when the player
    // looks away, pauses, or the actor goes.
    if (seenProbesInFlightValid && seenProbesInFlightFrame != seenProbesLastEvaluated) {
        seenProbesAwaitingConsume = true;
        // Budget and fairness are applied here, where the whole frame's records
        // are known. Resuming after the check that led last time stops the draw
        // order deciding who never gets sampled.
        seenProbeCandidatesLastFrame = static_cast<int>(seenProbesInFlight.size());
        size_t startAt = 0;
        for (size_t i = 0; i < seenProbesInFlight.size(); i++) {
            if (seenProbesInFlight[i].rc == seenProbeScheduleAfter) {
                startAt = (i + 1) % seenProbesInFlight.size();
                break;
            }
        }
        int coordinateBudget = SeenProbeBudget();
        for (size_t n = 0; n < seenProbesInFlight.size(); n++) {
            size_t index = (startAt + n) % seenProbesInFlight.size();
            if (seenProbesInFlight[index].pointCount > coordinateBudget) {
                continue; // deferred; counted below and treated as a non-match
            }
            coordinateBudget -= seenProbesInFlight[index].pointCount;
            seenProbesSampled.push_back(index);
        }
        seenProbeDeferredLastFrame = static_cast<int>(seenProbesInFlight.size() - seenProbesSampled.size());
        {
            std::unordered_set<size_t> sampledSet(seenProbesSampled.begin(), seenProbesSampled.end());
            for (size_t i = 0; i < seenProbesInFlight.size(); i++) {
                if (sampledSet.count(i) == 0) {
                    seenProbesDeferred.insert(seenProbesInFlight[i].rc);
                }
            }
        }
        if (!seenProbesInFlight.empty()) {
            seenProbeScheduleAfter = seenProbesInFlight[startAt].rc;
        }

        seenProbeCoordinatesLastFrame = 0;
        for (size_t index : seenProbesSampled) {
            const auto& record = seenProbesInFlight[index];
            for (int i = 0; i < record.pointCount; i++) {
                OTRGetPixelDepthPrepare(record.points[i].sampleX, record.points[i].y);
                seenProbeCoordinatesLastFrame++;
            }
        }
    }
}

// Consume phase: runs after the environment's read has flushed the whole pending
// set, so every value here comes from that one round-trip.
void CheckTrackerDepthProbeConsume() {
    if (gPlayState == nullptr || !GameInteractor::IsSaveLoaded() || !IS_RANDO) {
        return;
    }
    if (seenProbesAwaitingConsume) {
        seenProbesAwaitingConsume = false;
        seenProbesLastEvaluated = seenProbesInFlightFrame;
        const std::vector<size_t>& sampled = seenProbesSampled;

        bool anySeen = false;
        seenProbeMatchesLastFrame = 0;
        const bool debugging = CVarGetInteger(CVAR_TRACKER_CHECK("ProbeOverlay"), 0) != 0;
        if (debugging) {
            seenProbeDebug.clear();
            seenProbeDebugFrame = seenProbesInFlightFrame;
            // Deferred records are recorded too: "why did nothing happen" is
            // usually answered by budget, not by the depth comparison.
            std::unordered_set<size_t> sampledSet(sampled.begin(), sampled.end());
            for (size_t i = 0; i < seenProbesInFlight.size(); i++) {
                if (sampledSet.count(i) == 0) {
                    SeenProbeDebugRecord entry{};
                    entry.rc = seenProbesInFlight[i].rc;
                    entry.isChest = seenProbesInFlight[i].isChest;
                    entry.wasSampled = false;
                    entry.tolerance = seenProbesInFlight[i].pointCount > 0 ? seenProbesInFlight[i].points[0].tolerance
                                                                           : seenProbesInFlight[i].slabHalfWidth;
                    entry.pointCount = seenProbesInFlight[i].pointCount;
                    entry.distance = seenProbesInFlight[i].distance;
                    seenProbeDebug.push_back(entry);
                }
            }
        }
        std::unordered_set<RandomizerCheck> matchedThisFrame;
        for (size_t index : sampled) {
            const auto& record = seenProbesInFlight[index];
            if (record.sceneNum != gPlayState->sceneNum) {
                continue; // evidence belongs to a scene we have left
            }
            SeenProbeDebugRecord entry{};
            int matches = 0;
            for (int i = 0; i < record.pointCount; i++) {
                int32_t sampled = OTRGetPixelDepth(record.points[i].sampleX, record.points[i].y) * 4 >> 3;
                // A chest accepts the band from its own front surface back to the
                // interior probe: nearer than that is an occluder, further means the
                // probe saw past the model. The tolerance covers projection and
                // rasterisation only, never geometry error.
                bool matched;
                int32_t tolerance = record.points[i].tolerance;
                if (record.isChest) {
                    matched = sampled >= record.points[i].nearZ - tolerance &&
                              sampled <= record.points[i].expectedZ + tolerance;
                } else {
                    matched = std::abs(sampled - record.points[i].expectedZ) <= tolerance;
                }
                if (matched) {
                    matches++;
                }
                if (debugging) {
                    entry.points[i] = { record.points[i].x,
                                        record.points[i].y,
                                        record.points[i].expectedZ,
                                        record.points[i].nearZ,
                                        sampled,
                                        matched };
                }
            }
            if (debugging) {
                entry.rc = record.rc;
                entry.isChest = record.isChest;
                entry.wasSampled = true;
                entry.matches = matches;
                entry.tolerance = record.pointCount > 0 ? record.points[0].tolerance : record.slabHalfWidth;
                entry.pointCount = record.pointCount;
                entry.distance = record.distance;
                auto banked = seenItemFrames.find(record.rc);
                entry.framesBanked = banked == seenItemFrames.end() ? 0 : banked->second;
                seenProbeDebug.push_back(entry);
            }
            if (matches < kSeenProbeMatchesRequired) {
                continue;
            }
            matchedThisFrame.insert(record.rc);
            seenProbeMatchesLastFrame++;
            int banked = ++seenItemFrames[record.rc];
            if (debugging) {
                int& peak = seenProbePeakBanked[record.rc];
                peak = std::max(peak, banked);
            }
            if (banked < kSeenFramesRequired) {
                continue;
            }
            seenItemFrames.erase(record.rc);
            auto loc = OTRGlobals::Instance->gRandoContext->GetItemLocation(record.rc);
            if (loc->GetCheckStatus() != RCSHOW_UNCHECKED) {
                continue; // marked by another path since the evidence was taken
            }
            if (record.isChest) {
                loc->SetSeenCategory(record.category);
                loc->SetCheckStatus(RCSHOW_CATEGORY);
                anySeen = true;
                continue;
            }
            // Unobtainable items draw as a blue rupee; only mark when the drawn
            // model matches the truth (disguises match both sides).
            GetItemEntry truthful = OTRGlobals::Instance->gRandoContext->GetFinalGIEntry(
                record.rc, false, (GetItemID)Rando::StaticData::GetLocation(record.rc)->GetVanillaItem());
            if (record.drawnEntry.modIndex == truthful.modIndex && record.drawnEntry.getItemId == truthful.getItemId) {
                loc->SetCheckStatus(RCSHOW_SEEN_OR_HINTED);
                anySeen = true;
            }
        }
        // The frames have to be consecutive, so a miss starts over. Budget deferral
        // is not a miss: the candidate was registered by a draw and simply wasn't
        // looked at, so it holds its progress without advancing. Erasing it here
        // would stop anything converging whenever more candidates are eligible than
        // the coordinate budget covers.
        for (auto it = seenItemFrames.begin(); it != seenItemFrames.end();) {
            bool keep = matchedThisFrame.count(it->first) != 0 || seenProbesDeferred.count(it->first) != 0;
            it = keep ? std::next(it) : seenItemFrames.erase(it);
        }
        if (anySeen) {
            SaveManager::Instance->SaveSection(gSaveContext.fileNum, sectionId, true);
        }
        seenProbesInFlight.clear();
        seenProbesInFlightValid = false;
    }

    // Hand this frame's evidence over to be read once its frame has rendered.
    if (seenProbesPendingValid) {
        seenProbesInFlight = std::move(seenProbesPending);
        seenProbesInFlightFrame = seenProbesPendingFrame;
        seenProbesInFlightValid = true;
        seenProbesPending.clear();
        seenProbesPendingValid = false;
    }
}

void CheckTrackerLoadGame(int32_t fileNum) {
    if (IS_BOSS_RUSH) {
        return;
    }
    LoadSettings();
    TrySetAreas();
    for (auto& entry : Rando::StaticData::GetLocationTable()) {
        RandomizerCheck rc = entry.GetRandomizerCheck();
        if (rc == RC_UNKNOWN_CHECK || rc == RC_MAX || rc == RC_LINKS_POCKET ||
            !Rando::StaticData::GetLocation(rc) != RC_UNKNOWN_CHECK) {
            continue;
        }

        Rando::Location* entry2 = Rando::StaticData::GetLocation(rc);
        Rando::ItemLocation* loc = OTRGlobals::Instance->gRandoContext->GetItemLocation(rc);

        checksByArea.find(entry2->GetArea())->second.push_back(entry2->GetRandomizerCheck());
        if (IsVisibleInCheckTracker(entry2->GetRandomizerCheck())) {
            areaCheckTotals[entry2->GetArea()]++;
            if (loc->GetCheckStatus() == RCSHOW_SAVED || loc->GetIsSkipped()) {
                areaChecksGotten[entry2->GetArea()]++;
            }
            if (loc->IsAvailable()) {
                areaChecksAvailable[entry2->GetArea()]++;
            }
        }

        if (areaChecksGotten[entry2->GetArea()] != 0 || RandomizerCheckObjects::AreaIsOverworld(entry2->GetArea()) ||
            loc->GetCheckStatus() == RCSHOW_SCUMMED) {
            areasSpoiled |= (1 << entry2->GetArea());
        }

        // Create check name overrides for child pond fish if age split is disabled
        if (fishsanityMode != RO_FISHSANITY_OFF && fishsanityMode != RO_FISHSANITY_OVERWORLD &&
            entry.GetRCType() == RCTYPE_FISH && entry.GetScene() == SCENE_FISHING_POND &&
            entry.GetActorParams() != 116 && !fishsanityAgeSplit) {
            if (entry.GetShortName().starts_with("Child")) {
                checkNameOverrides[rc] = entry.GetShortName().substr(6);
            }
        }
    }
    for (int i = RCAREA_KOKIRI_FOREST; i < RCAREA_INVALID; i++) {
        if (!IsAreaSpoiled(static_cast<RandomizerCheckArea>(i)) &&
            (RandomizerCheckObjects::AreaIsOverworld(static_cast<RandomizerCheckArea>(i)) || !IS_RANDO ||
             OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MQ_DUNGEON_RANDOM) == RO_MQ_DUNGEONS_NONE ||
             (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MQ_DUNGEON_RANDOM) ==
                  RO_MQ_DUNGEONS_SELECTION &&
              OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(
                  static_cast<RandomizerSettingKey>(RSK_MQ_DEKU_TREE + (i - RCAREA_DEKU_TREE))) != RO_MQ_SET_RANDOM) ||
             (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MQ_DUNGEON_SET) == RO_GENERIC_ON &&
              OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(
                  static_cast<RandomizerSettingKey>(RSK_MQ_DEKU_TREE + (i - RCAREA_DEKU_TREE))) != RO_MQ_SET_RANDOM) ||
             (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MQ_DUNGEON_RANDOM) ==
                  RO_MQ_DUNGEONS_SET_NUMBER &&
              (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MQ_DUNGEON_COUNT) == MAX_MQ_DUNGEON_COUNT ||
               OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MQ_DUNGEON_COUNT) == 0)))) {
            SetAreaSpoiled(static_cast<RandomizerCheckArea>(i));
        }
    }
    if (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_LINKS_POCKET) != RO_LINKS_POCKET_NOTHING &&
        IS_RANDO) {
        uint8_t startingAge = OTRGlobals::Instance->gRandoContext->GetOption(RSK_SELECTED_STARTING_AGE).Get();
        RandomizerCheckArea startingArea;
        switch (startingAge) {
            case RO_AGE_CHILD:
                startingArea = RCAREA_KOKIRI_FOREST;
                break;
            case RO_AGE_ADULT:
                startingArea = RCAREA_MARKET;
                break;
            default:
                startingArea = RCAREA_KOKIRI_FOREST;
                break;
        }

        checksByArea.find(startingArea)->second.push_back(RC_LINKS_POCKET);
        areaChecksGotten[startingArea]++;
        areaCheckTotals[startingArea]++;
    }

    showVOrMQ =
        (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MQ_DUNGEON_RANDOM) ==
             RO_MQ_DUNGEONS_RANDOM_NUMBER ||
         (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MQ_DUNGEON_RANDOM) == RO_MQ_DUNGEONS_SET_NUMBER &&
          OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MQ_DUNGEON_COUNT) < MAX_MQ_DUNGEON_COUNT));
    initialized = true;
    UpdateAllOrdering();
    UpdateInventoryChecks();
    UpdateFilters();

    RegionTable_Init();

    Rando::Context::GetInstance()->GetEntranceShuffler()->ApplyEntranceOverrides();

    recalculateAvailable = true;
}

void CheckTrackerShopSlotChange(uint8_t cursorSlot, int16_t basePrice) {
    if (gPlayState->sceneNum == SCENE_HAPPY_MASK_SHOP) { // Happy Mask Shop is not used in rando, so is not tracked
        return;
    }

    auto slot = startingShopItem.find(gPlayState->sceneNum)->second + cursorSlot;
    if (GetCheckArea() == RCAREA_KAKARIKO_VILLAGE && gPlayState->sceneNum == SCENE_BAZAAR) {
        slot = RC_KAK_BAZAAR_ITEM_1 + cursorSlot;
    }
    auto status = OTRGlobals::Instance->gRandoContext->GetItemLocation(slot)->GetCheckStatus();
    if (status == RCSHOW_SEEN_OR_HINTED) {
        OTRGlobals::Instance->gRandoContext->GetItemLocation(slot)->SetCheckStatus(RCSHOW_IDENTIFIED);
        SaveManager::Instance->SaveSection(gSaveContext.fileNum, sectionId, true);
        RecalculateAvailableChecks();
    }
}

void CheckTrackerTransition(uint32_t sceneNum) {
    if (!GameInteractor::IsSaveLoaded()) {
        return;
    }
    doAreaScroll = true;
    previousArea = currentArea;
    currentArea = GetCheckArea();
    switch (sceneNum) {
        case SCENE_KOKIRI_SHOP:
        case SCENE_BAZAAR:
        case SCENE_POTION_SHOP_MARKET:
        case SCENE_BOMBCHU_SHOP:
        case SCENE_POTION_SHOP_KAKARIKO:
        case SCENE_GORON_SHOP:
        case SCENE_ZORA_SHOP:
            SetShopSeen(sceneNum, false);
            break;
    }
    if (!IsAreaSpoiled(currentArea) && (RandomizerCheckObjects::AreaIsOverworld(currentArea) ||
                                        std::find(spoilingEntrances.begin(), spoilingEntrances.end(),
                                                  gPlayState->nextEntranceIndex) != spoilingEntrances.end())) {
        SetAreaSpoiled(currentArea);
    }
}

void CheckTrackerItemReceive(GetItemEntry giEntry) {
    if (!GameInteractor::IsSaveLoaded() || std::find(std::begin(skipScenes), std::end(skipScenes),
                                                     (SceneID)gPlayState->sceneNum) != std::end(skipScenes)) {
        return;
    }
    auto scene = static_cast<SceneID>(gPlayState->sceneNum);
    // Vanilla special item checks
    if (!IS_RANDO) {
        if (giEntry.itemId == ITEM_SHIELD_DEKU) {
            SetCheckCollected(RC_KF_SHOP_ITEM_1);
            return;
        } else if (giEntry.itemId == ITEM_KOKIRI_EMERALD) {
            SetCheckCollected(RC_QUEEN_GOHMA);
            return;
        } else if (giEntry.itemId == ITEM_GORON_RUBY) {
            SetCheckCollected(RC_KING_DODONGO);
            return;
        } else if (giEntry.itemId == ITEM_ZORA_SAPPHIRE) {
            SetCheckCollected(RC_BARINADE);
            return;
        } else if (giEntry.itemId == ITEM_MEDALLION_FOREST) {
            SetCheckCollected(RC_PHANTOM_GANON);
            return;
        } else if (giEntry.itemId == ITEM_MEDALLION_FIRE) {
            SetCheckCollected(RC_VOLVAGIA);
            return;
        } else if (giEntry.itemId == ITEM_MEDALLION_WATER) {
            SetCheckCollected(RC_MORPHA);
            return;
        } else if (giEntry.itemId == ITEM_MEDALLION_SHADOW) {
            SetCheckCollected(RC_BONGO_BONGO);
            return;
        } else if (giEntry.itemId == ITEM_MEDALLION_SPIRIT) {
            SetCheckCollected(RC_TWINROVA);
            return;
        } else if (giEntry.itemId == ITEM_MEDALLION_LIGHT) {
            SetCheckCollected(RC_GIFT_FROM_RAURU);
            return;
        } else if (giEntry.itemId == ITEM_SONG_EPONA) {
            SetCheckCollected(RC_SONG_FROM_MALON);
            return;
        } else if (giEntry.itemId == ITEM_SONG_SARIA) {
            SetCheckCollected(RC_SONG_FROM_SARIA);
            return;
        } else if (giEntry.itemId == ITEM_BEAN) {
            SetCheckCollected(RC_ZR_MAGIC_BEAN_SALESMAN);
            return;
        } else if (giEntry.itemId == ITEM_BRACELET) {
            SetCheckCollected(RC_GC_DARUNIAS_JOY);
            return;
        } /* else if (giEntry.itemId == ITEM_SONG_SUN) {
             SetCheckCollected(RC_SONG_FROM_ROYAL_FAMILYS_TOMB);
             return;
         } else if (giEntry.itemId == ITEM_SONG_TIME) {
             SetCheckCollected(RC_SONG_FROM_OCARINA_OF_TIME);
             return;
         } else if (giEntry.itemId == ITEM_SONG_STORMS) {
             SetCheckCollected(RC_SONG_FROM_WINDMILL);
             return;
         } else if (giEntry.itemId == ITEM_SONG_MINUET) {
             SetCheckCollected(RC_SHEIK_IN_FOREST);
             return;
         } else if (giEntry.itemId == ITEM_SONG_BOLERO) {
             SetCheckCollected(RC_SHEIK_IN_CRATER);
             return;
         } else if (giEntry.itemId == ITEM_SONG_SERENADE) {
             SetCheckCollected(RC_SHEIK_IN_ICE_CAVERN);
             return;
         } else if (giEntry.itemId == ITEM_SONG_NOCTURNE) {
             SetCheckCollected(RC_SHEIK_IN_KAKARIKO);
             return;
         } else if (giEntry.itemId == ITEM_SONG_REQUIEM) {
             SetCheckCollected(RC_SHEIK_AT_COLOSSUS);
             return;
         } else if (giEntry.itemId == ITEM_SONG_PRELUDE) {
             SetCheckCollected(RC_SHEIK_AT_TEMPLE);
             return;
         }*/
    }
}

void CheckTrackerSceneFlagSet(int16_t sceneNum, int16_t flagType, int32_t flag) {
    if (IS_RANDO) {
        return;
    }

    if (flagType != FLAG_SCENE_TREASURE && flagType != FLAG_SCENE_COLLECTIBLE) {
        return;
    }
    if (sceneNum == SCENE_GRAVEYARD && flag == 0x19 &&
        flagType == FLAG_SCENE_COLLECTIBLE) { // Gravedigging tour special case
        SetCheckCollected(RC_GRAVEYARD_DAMPE_GRAVEDIGGING_TOUR);
        return;
    }
    for (auto& loc : Rando::StaticData::GetLocationTable()) {
        if (!IsVisibleInCheckTracker(loc.GetRandomizerCheck())) {
            continue;
        }
        SpoilerCollectionCheckType checkMatchType = flagType == FLAG_SCENE_TREASURE
                                                        ? SpoilerCollectionCheckType::SPOILER_CHK_CHEST
                                                        : SpoilerCollectionCheckType::SPOILER_CHK_COLLECTABLE;
        Rando::SpoilerCollectionCheck scCheck = loc.GetCollectionCheck();
        if (scCheck.scene == sceneNum && scCheck.flag == flag && scCheck.type == checkMatchType) {
            SetCheckCollected(loc.GetRandomizerCheck());
            return;
        }
    }
}

void CheckTrackerFlagSet(int16_t flagType, int32_t flag) {
    if (IS_RANDO) {
        return;
    }

    SpoilerCollectionCheckType checkMatchType = SpoilerCollectionCheckType::SPOILER_CHK_NONE;
    switch (flagType) {
        case FLAG_GS_TOKEN:
            checkMatchType = SpoilerCollectionCheckType::SPOILER_CHK_GOLD_SKULLTULA;
            break;
        case FLAG_EVENT_CHECK_INF:
            if ((flag == EVENTCHKINF_CARPENTERS_FREE(0) || flag == EVENTCHKINF_CARPENTERS_FREE(1) ||
                 flag == EVENTCHKINF_CARPENTERS_FREE(2) || flag == EVENTCHKINF_CARPENTERS_FREE(3)) &&
                GET_EVENTCHKINF_CARPENTERS_FREE_ALL()) {
                SetCheckCollected(RC_TH_FREED_CARPENTERS);
                return;
            }
            checkMatchType = SpoilerCollectionCheckType::SPOILER_CHK_EVENT_CHK_INF;
            break;
        case FLAG_INF_TABLE:
            if (flag == INFTABLE_190) {
                SetCheckCollected(RC_GF_HBA_1000_POINTS);
                return;
            } else if (flag == INFTABLE_11E) {
                SetCheckCollected(RC_GC_ROLLING_GORON_AS_CHILD);
                return;
            } else if (flag == INFTABLE_GORON_CITY_DOORS_UNLOCKED) {
                SetCheckCollected(RC_GC_ROLLING_GORON_AS_ADULT);
                return;
            } else if (flag == INFTABLE_139) {
                SetCheckCollected(RC_ZD_KING_ZORA_THAWED);
                return;
            } else if (flag == INFTABLE_191) {
                SetCheckCollected(RC_MARKET_LOST_DOG);
                return;
            }
            if (!IS_RANDO) {
                if (flag == INFTABLE_BOUGHT_STICK_UPGRADE) {
                    SetCheckCollected(RC_LW_DEKU_SCRUB_NEAR_BRIDGE);
                    return;
                } else if (flag == INFTABLE_BOUGHT_NUT_UPGRADE) {
                    SetCheckCollected(RC_LW_DEKU_SCRUB_GROTTO_FRONT);
                    return;
                }
            }
            break;
        case FLAG_ITEM_GET_INF:
            if (!IS_RANDO) {
                if (flag == ITEMGETINF_OBTAINED_STICK_UPGRADE_FROM_STAGE) {
                    SetCheckCollected(RC_DEKU_THEATER_SKULL_MASK);
                    return;
                } else if (flag == ITEMGETINF_OBTAINED_NUT_UPGRADE_FROM_STAGE) {
                    SetCheckCollected(RC_DEKU_THEATER_MASK_OF_TRUTH);
                    return;
                } else if (flag == ITEMGETINF_DEKU_SCRUB_HEART_PIECE) {
                    SetCheckCollected(RC_HF_DEKU_SCRUB_GROTTO);
                    return;
                }
            }
            checkMatchType = SpoilerCollectionCheckType::SPOILER_CHK_ITEM_GET_INF;
            break;
        case FLAG_RANDOMIZER_INF:
            checkMatchType = SpoilerCollectionCheckType::SPOILER_CHK_RANDOMIZER_INF;
            break;
    }
    if (checkMatchType == SpoilerCollectionCheckType::SPOILER_CHK_NONE) {
        return;
    }
    for (auto& loc : Rando::StaticData::GetLocationTable()) {
        if ((!IS_RANDO && ((loc.GetQuest() == RCQUEST_MQ && !IS_MASTER_QUEST) ||
                           (loc.GetQuest() == RCQUEST_VANILLA && IS_MASTER_QUEST))) ||
            (IS_RANDO &&
             !(OTRGlobals::Instance->gRandoContext->GetDungeons()->GetDungeonFromScene(loc.GetScene()) == nullptr) &&
             ((OTRGlobals::Instance->gRandoContext->GetDungeons()->GetDungeonFromScene(loc.GetScene())->IsMQ() &&
               loc.GetQuest() == RCQUEST_VANILLA) ||
              OTRGlobals::Instance->gRandoContext->GetDungeons()->GetDungeonFromScene(loc.GetScene())->IsVanilla() &&
                  loc.GetQuest() == RCQUEST_MQ))) {
            continue;
        }
        Rando::SpoilerCollectionCheck scCheck = loc.GetCollectionCheck();
        SpoilerCollectionCheckType scCheckType = scCheck.type;
        if (checkMatchType == SpoilerCollectionCheckType::SPOILER_CHK_RANDOMIZER_INF &&
            scCheckType == SpoilerCollectionCheckType::SPOILER_CHK_RANDOMIZER_INF) {
            if (flag == OTRGlobals::Instance->gRandomizer->GetRandomizerInfFromCheck(loc.GetRandomizerCheck())) {
                SetCheckCollected(loc.GetRandomizerCheck());
                return;
            }
            continue;
        }
        int16_t checkFlag = scCheck.flag;
        if (checkMatchType == SpoilerCollectionCheckType::SPOILER_CHK_GOLD_SKULLTULA) {
            checkFlag = loc.GetActorParams();
        }
        if (checkFlag == flag && scCheck.type == checkMatchType) {
            SetCheckCollected(loc.GetRandomizerCheck());
            return;
        }
    }
}

void CheckTrackerDialogMessage() {
    // These dialogues state the price, so a Seen check upgrades to Identified.
    auto identifyCheck = [](RandomizerCheck rc) {
        auto loc = OTRGlobals::Instance->gRandoContext->GetItemLocation(rc);
        RandomizerCheckStatus status = loc->GetCheckStatus();
        if (status == RCSHOW_UNCHECKED || status == RCSHOW_SEEN_OR_HINTED) {
            loc->SetCheckStatus(RCSHOW_IDENTIFIED);
            RecalculateAvailableChecks();
        }
    };

    if (gPlayState->msgCtx.textId == TEXT_BEAN_SALESMAN_BUY_FOR_10) {
        identifyCheck(RC_ZR_MAGIC_BEAN_SALESMAN);
    } else if (gPlayState->msgCtx.textId == TEXT_MEDIGORON) {
        identifyCheck(RC_GC_MEDIGORON);
    } else if (gPlayState->msgCtx.textId == TEXT_GRANNYS_SHOP) {
        identifyCheck(RC_KAK_GRANNYS_SHOP);
    } else if (gPlayState->msgCtx.textId == TEXT_CARPET_SALESMAN_1) {
        identifyCheck(RC_WASTELAND_BOMBCHU_SALESMAN);
    } else if (gPlayState->msgCtx.textId == TEXT_SCRUB_RANDOM) {
        if (auto* actor = gPlayState->msgCtx.talkActor) {
            if (auto* checkIdentity = ObjectExtension::GetInstance().Get<ScrubIdentity>(actor)) {
                identifyCheck(checkIdentity->identity.randomizerCheck);
            }
        }
    }
}

void InitTrackerData(bool isDebug) {
    TrySetAreas();
    areasSpoiled = 0;
}

void SaveTrackerData(SaveContext* saveContext, int sectionID, bool fullSave) {
    bool updateOrdering = false;
    std::vector<RandomizerCheck> checkCount;
    for (int i = RC_UNKNOWN_CHECK; i < RC_MAX; i++) {
        if (OTRGlobals::Instance->gRandoContext->GetItemLocation(i)->GetCheckStatus() != RCSHOW_UNCHECKED ||
            OTRGlobals::Instance->gRandoContext->GetItemLocation(i)->GetIsSkipped())
            checkCount.push_back(static_cast<RandomizerCheck>(i));
    }
    SaveManager::Instance->SaveArray("checkStatus", checkCount.size(), [&](size_t i) {
        RandomizerCheck check = checkCount.at(i);
        RandomizerCheckStatus savedStatus =
            OTRGlobals::Instance->gRandoContext->GetItemLocation(check)->GetCheckStatus();
        bool isSkipped = OTRGlobals::Instance->gRandoContext->GetItemLocation(check)->GetIsSkipped();
        if (savedStatus == RCSHOW_COLLECTED) {
            if (fullSave) {
                OTRGlobals::Instance->gRandoContext->GetItemLocation(check)->SetCheckStatus(RCSHOW_SAVED);
                savedStatus = RCSHOW_SAVED;
                updateOrdering = true;
            } else {
                savedStatus = RCSHOW_SCUMMED;
            }
        }
        if (savedStatus != RCSHOW_UNCHECKED || isSkipped) {
            SaveManager::Instance->SaveStruct("", [&]() {
                SaveManager::Instance->SaveData("randomizerCheck", check);
                SaveManager::Instance->SaveData("status", savedStatus);
                SaveManager::Instance->SaveData("skipped", isSkipped);
                int16_t seenCategory = OTRGlobals::Instance->gRandoContext->GetItemLocation(check)->GetSeenCategory();
                if (seenCategory >= 0) {
                    SaveManager::Instance->SaveData("seenCategory", seenCategory);
                }
            });
        }
    });
    SaveManager::Instance->SaveData("areasSpoiled", areasSpoiled);
    if (updateOrdering) {
        UpdateAllOrdering();
        UpdateAllAreas();
    }
}

void SaveFile(SaveContext* saveContext, int sectionID, bool fullSave) {
    SaveTrackerData(saveContext, sectionID, fullSave);
    if (fullSave) {
        recalculateAvailable = true;
    }
}

void LoadFile() {
    SaveManager::Instance->LoadArray("checkStatus", RC_MAX, [](size_t i) {
        SaveManager::Instance->LoadStruct("", [&]() {
            RandomizerCheckStatus status;
            bool skipped;
            RandomizerCheck rc;
            int32_t seenCategory;
            SaveManager::Instance->LoadData("randomizerCheck", rc, RC_UNKNOWN_CHECK);
            SaveManager::Instance->LoadData("status", status, RCSHOW_UNCHECKED);
            SaveManager::Instance->LoadData("skipped", skipped, false);
            SaveManager::Instance->LoadData("seenCategory", seenCategory, (int32_t)-1);
            if (seenCategory >= 0) {
                OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->SetSeenCategory(
                    static_cast<GetItemCategory>(seenCategory));
            }
            OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->SetCheckStatus(status);
            OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->SetIsSkipped(skipped);
        });
    });
    SaveManager::Instance->LoadData("areasSpoiled", areasSpoiled, (uint32_t)0);
    UpdateAllOrdering();
    UpdateAllAreas();
}

void Teardown() {
    initialized = false;
    ClearAreaChecksAndTotals();
    checksByArea.clear();
    areasSpoiled = 0;
    filterAreasHidden = { 0 };
    filterChecksHidden = { 0 };
    seenItemFrames.clear();
    seenProbesPending.clear();
    seenProbesInFlight.clear();
    seenProbesPendingValid = false;
    seenProbesInFlightValid = false;
    seenProbesPendingFrame = 0;
    seenProbesInFlightFrame = 0;
    seenProbesLastEvaluated = UINT32_MAX;
    seenProbeScheduleAfter = RC_UNKNOWN_CHECK;
    seenProbeCoordinatesLastFrame = 0;
    seenProbeCandidatesLastFrame = 0;
    seenProbeDeferredLastFrame = 0;
    seenProbeRejectedLastFrame = 0;
    seenProbeMatchesLastFrame = 0;

    lastLocationChecked = RC_UNKNOWN_CHECK;
}

bool IsAreaSpoiled(RandomizerCheckArea rcArea) {
    return areasSpoiled & (1 << rcArea);
}

void SetAreaSpoiled(RandomizerCheckArea rcArea) {
    areasSpoiled |= (1 << rcArea);
    SaveManager::Instance->SaveSection(gSaveContext.fileNum, sectionId, true);
}

void InternalRecalculateAvailableChecks(RandomizerRegion startingRegion, RandoAgeTime startingAgeTime);

void CheckTrackerWindow::DrawElement() {
    Color_Background = CVarGetColor(CVAR_TRACKER_CHECK("BgColor.Value"), Color_Bg_Default);
    Color_Area_Incomplete_Main = CVarGetColor(CVAR_TRACKER_CHECK("AreaIncomplete.MainColor.Value"), Color_Main_Default);
    Color_Area_Incomplete_Extra =
        CVarGetColor(CVAR_TRACKER_CHECK("AreaIncomplete.ExtraColor.Value"), Color_Area_Incomplete_Extra_Default);
    Color_Area_Complete_Main = CVarGetColor(CVAR_TRACKER_CHECK("AreaComplete.MainColor.Value"), Color_Main_Default);
    Color_Area_Complete_Extra =
        CVarGetColor(CVAR_TRACKER_CHECK("AreaComplete.ExtraColor.Value"), Color_Area_Complete_Extra_Default);
    Color_Unchecked_Main = CVarGetColor(CVAR_TRACKER_CHECK("Unchecked.MainColor.Value"), Color_Main_Default);
    Color_Unchecked_Extra =
        CVarGetColor(CVAR_TRACKER_CHECK("Unchecked.ExtraColor.Value"), Color_Unchecked_Extra_Default);
    Color_Skipped_Main = CVarGetColor(CVAR_TRACKER_CHECK("Skipped.MainColor.Value"), Color_Main_Default);
    Color_Skipped_Extra = CVarGetColor(CVAR_TRACKER_CHECK("Skipped.ExtraColor.Value"), Color_Skipped_Extra_Default);
    Color_Seen_Main = CVarGetColor(CVAR_TRACKER_CHECK("Seen.MainColor.Value"), Color_Main_Default);
    Color_Seen_Extra = CVarGetColor(CVAR_TRACKER_CHECK("Seen.ExtraColor.Value"), Color_Seen_Extra_Default);
    Color_Hinted_Main = CVarGetColor(CVAR_TRACKER_CHECK("Hinted.MainColor.Value"), Color_Main_Default);
    Color_Hinted_Extra = CVarGetColor(CVAR_TRACKER_CHECK("Hinted.ExtraColor.Value"), Color_Hinted_Extra_Default);
    Color_Collected_Main = CVarGetColor(CVAR_TRACKER_CHECK("Collected.MainColor.Value"), Color_Main_Default);
    Color_Collected_Extra =
        CVarGetColor(CVAR_TRACKER_CHECK("Collected.ExtraColor.Value"), Color_Collected_Extra_Default);
    Color_Scummed_Main = CVarGetColor(CVAR_TRACKER_CHECK("Scummed.MainColor.Value"), Color_Main_Default);
    Color_Scummed_Extra = CVarGetColor(CVAR_TRACKER_CHECK("Scummed.ExtraColor.Value"), Color_Scummed_Extra_Default);
    Color_Saved_Main = CVarGetColor(CVAR_TRACKER_CHECK("Saved.MainColor.Value"), Color_Main_Default);
    Color_Saved_Extra = CVarGetColor(CVAR_TRACKER_CHECK("Saved.ExtraColor.Value"), Color_Saved_Extra_Default);
    hideUnchecked = CVarGetInteger(CVAR_TRACKER_CHECK("Unchecked.Hide"), 0);
    hideCategory = CVarGetInteger(CVAR_TRACKER_CHECK("Hinted.Hide"), 0);
    hideScummed = CVarGetInteger(CVAR_TRACKER_CHECK("Scummed.Hide"), 0);
    hideSeen = CVarGetInteger(CVAR_TRACKER_CHECK("Seen.Hide"), 0);
    hideSkipped = CVarGetInteger(CVAR_TRACKER_CHECK("Skipped.Hide"), 0);
    hideSaved = CVarGetInteger(CVAR_TRACKER_CHECK("Saved.Hide"), 0);
    hideCollected = CVarGetInteger(CVAR_TRACKER_CHECK("Collected.Hide"), 0);
    showHidden = CVarGetInteger(CVAR_TRACKER_CHECK("ShowHidden"), 0);
    mystery = CVarGetInteger(CVAR_RANDOMIZER_ENHANCEMENT("MysteriousShuffle"), 0);
    showLogicTooltip = CVarGetInteger(CVAR_TRACKER_CHECK("ShowLogic"), 0);
    enableAvailableChecks = CVarGetInteger(CVAR_TRACKER_CHECK("EnableAvailableChecks"), 0);
    onlyShowAvailable = CVarGetInteger(CVAR_TRACKER_CHECK("OnlyShowAvailable"), 0);

    hideShopUnshuffledChecks = CVarGetInteger(CVAR_TRACKER_CHECK("HideUnshuffledShopChecks"), 0);
    alwaysShowGS = CVarGetInteger(CVAR_TRACKER_CHECK("AlwaysShowGSLocs"), 0);
    if (CVarGetInteger(CVAR_TRACKER_CHECK("WindowType"), TRACKER_WINDOW_WINDOW) == TRACKER_WINDOW_FLOATING) {
        if (CVarGetInteger(CVAR_TRACKER_CHECK("ShowOnlyPaused"), 0) &&
            (gPlayState == nullptr || gPlayState->pauseCtx.state == 0)) {
            return;
        }

        if (CVarGetInteger(CVAR_TRACKER_CHECK("DisplayType"), TRACKER_DISPLAY_ALWAYS) == TRACKER_DISPLAY_COMBO_BUTTON) {
            int comboButton1Mask = buttons[CVarGetInteger(CVAR_TRACKER_CHECK("ComboButton1"), TRACKER_COMBO_BUTTON_L)];
            int comboButton2Mask = buttons[CVarGetInteger(CVAR_TRACKER_CHECK("ComboButton2"), TRACKER_COMBO_BUTTON_R)];
            OSContPad* trackerButtonsPressed =
                std::dynamic_pointer_cast<LUS::ControlDeck>(Ship::Context::GetRawInstance()->GetControlDeck())
                    ->GetPads();
            bool comboButtonsHeld = trackerButtonsPressed != nullptr &&
                                    trackerButtonsPressed[0].button & comboButton1Mask &&
                                    trackerButtonsPressed[0].button & comboButton2Mask;
            if (!comboButtonsHeld) {
                return;
            }
        }
    }

    if (presetLoaded) {
        ImGui::SetNextWindowSize(presetSize);
        ImGui::SetNextWindowPos(presetPos);
        presetLoaded = false;
    } else {
        ImGui::SetNextWindowSize(ImVec2(400, 540), ImGuiCond_FirstUseEver);
    }
    if (Trackers::BeginFloatWindows(
            "Check Tracker", mIsVisible, Color_Background,
            static_cast<TrackerWindowType>(CVarGetInteger(CVAR_TRACKER_CHECK("WindowType"), TRACKER_WINDOW_WINDOW)),
            CVarGetInteger(CVAR_TRACKER_CHECK("Draggable"), 1), ImGuiWindowFlags_NoScrollbar)) {
        if (!GameInteractor::IsSaveLoaded() || !initialized) {
            ImGui::Text("Waiting for file load..."); // TODO Language
            Trackers::EndFloatWindows();
            return;
        }

        if (recalculateAvailable) {
            recalculateAvailable = false;
            InternalRecalculateAvailableChecks(availableChecksStartingRegion, availableChecksStartingAgeTime);
            availableChecksStartingRegion = RR_ROOT;
            availableChecksStartingAgeTime = RAT_NONE;
        }

        // Quick Options
#ifdef __WIIU__
        float headerHeight = 40.0f;
#else
        float headerHeight = 20.0f;
#endif
        if (!ImGui::BeginTable("Check Tracker", 1, 0)) {
            Trackers::EndFloatWindows();
            return;
        }

        ImGui::SetWindowFontScale(CVarGetFloat(CVAR_TRACKER_CHECK("FontSize"), 1.0f));

        ImGui::TableNextRow(0, 0);
        ImGui::TableNextColumn();
        if (CVarGetInteger(CVAR_TRACKER_CHECK("HiddenItemsToggleVisible"), 1) &&
            UIWidgets::CVarCheckbox(
                "Show Hidden Items", CVAR_TRACKER_CHECK("ShowHidden"),
                UIWidgets::CheckboxOptions(
                    { { .tooltip =
                            "When active, items will show hidden checks by default when updated to this state." } })
                    .Color(THEME_COLOR))) {
            doAreaScroll = true;
            showHidden = CVarGetInteger(CVAR_TRACKER_CHECK("ShowHidden"), 0);
            RecalculateAllAreaTotals();
        }
        if (enableAvailableChecks && CVarGetInteger(CVAR_TRACKER_CHECK("AvailableChecksToggleVisible"), 1)) {
            if (UIWidgets::CVarCheckbox(
                    "Only Show Available Checks", CVAR_TRACKER_CHECK("OnlyShowAvailable"),
                    UIWidgets::CheckboxOptions({ { .tooltip = "When active, unavailable checks will be hidden." } })
                        .Color(THEME_COLOR))) {
                doAreaScroll = true;
                RecalculateAllAreaTotals();
            }
        }
        if (CVarGetInteger(CVAR_TRACKER_CHECK("ExpandCollapseButtonsVisible"), 0)) {
            if (UIWidgets::Button("Expand All", UIWidgets::ButtonOptions()
                                                    .Color(THEME_COLOR)
                                                    .Size({ ImGui::GetContentRegionAvail().x / 2 - 6, 0 }))) {
                optCollapseAll = false;
                optExpandAll = true;
                doAreaScroll = true;
            }
            ImGui::SameLine();
            if (UIWidgets::Button(
                    "Collapse All",
                    UIWidgets::ButtonOptions().Color(THEME_COLOR).Size({ ImGui::GetContentRegionAvail().x - 6, 0 }))) {
                optExpandAll = false;
                optCollapseAll = true;
            }
        }
        UIWidgets::PushStyleCombobox(THEME_COLOR);
        if (CVarGetInteger(CVAR_TRACKER_CHECK("SearchInputVisible"), 1)) {
            if (checkSearch.Draw("", ImGui::GetContentRegionAvail().x - 42)) {
                UpdateFilters();
            }
            std::string checkSearchText = checkSearch.InputBuf;
            checkSearchText.erase(std::remove(checkSearchText.begin(), checkSearchText.end(), ' '),
                                  checkSearchText.end());
            ImGui::SameLine();
            if (UIWidgets::Button(ICON_FA_ERASER, UIWidgets::ButtonOptions()
                                                      .Size(UIWidgets::Sizes::Inline)
                                                      .Color(THEME_COLOR)
                                                      .Padding(ImVec2(10.f, 6.f)))) {
                checkSearch.Clear();
                UpdateFilters();
                doAreaScroll = true;
            }
            if (checkSearchText.length() < 1) {
                ImGui::SameLine(20.0f);
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 0.4f), "Search...");
            }
        }
        UIWidgets::PopStyleCombobox();

        if (CVarGetInteger(CVAR_TRACKER_CHECK("CheckTotalsVisible"), 1)) {
            std::ostringstream totalChecksSS;
            totalChecksSS << "";
            if (enableAvailableChecks) {
                totalChecksSS << totalChecksAvailable << " Available / ";
            }
            totalChecksSS << totalChecksGotten << " Checked / " << totalChecks << " Total";
            ImGui::Text("%s", totalChecksSS.str().c_str());
        }

        bool headerPresent =
            CVarGetInteger(CVAR_TRACKER_CHECK("HiddenItemsToggleVisible"), 1) ||
            (enableAvailableChecks && CVarGetInteger(CVAR_TRACKER_CHECK("AvailableChecksToggleVisible"), 1)) ||
            CVarGetInteger(CVAR_TRACKER_CHECK("ExpandCollapseButtonsVisible"), 0) ||
            CVarGetInteger(CVAR_TRACKER_CHECK("SearchInputVisible"), 1) ||
            CVarGetInteger(CVAR_TRACKER_CHECK("CheckTotalsVisible"), 1);
        if (headerPresent) {
            ImGui::Separator();
        }

        // Checks Section Lead-in
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        if (!ImGui::BeginTable("CheckTracker##Checks", 1, ImGuiTableFlags_ScrollY)) {
            ImGui::EndTable();
            Trackers::EndFloatWindows();
            return;
        }
        ImGui::TableNextRow();
        ImGui::TableNextColumn();

        // Prep for loop
        RainbowTick();
        bool doDraw = false;
        bool thisAreaFullyChecked = false;
        bool mqSpoilers = CVarGetInteger(CVAR_TRACKER_CHECK("MQSpoilers"), 0);
        bool hideIncomplete = CVarGetInteger(CVAR_TRACKER_CHECK("AreaIncomplete.Hide"), 0);
        bool hideComplete = CVarGetInteger(CVAR_TRACKER_CHECK("AreaComplete.Hide"), 0);
        bool collapseLogic;
        bool doingCollapseOrExpand = optExpandAll || optCollapseAll;
        bool isThisAreaSpoiled;
        RandomizerCheckArea lastArea = RCAREA_INVALID;
        Color_RGBA8 mainColor;
        Color_RGBA8 extraColor;
        std::string stemp;

        bool shouldHideFilteredAreas = CVarGetInteger(CVAR_TRACKER_CHECK("HideFilteredAreas"), 1);

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4.0f, 3.0f));
        for (auto& [rcArea, checks] : checksByArea) {
            RandomizerCheckArea thisArea = currentArea;

            thisAreaFullyChecked = (areaChecksGotten[rcArea] == areaCheckTotals[rcArea]);
            // Last Area needs to be cleaned up
            if (lastArea != RCAREA_INVALID && doDraw) {
                UIWidgets::PaddedSeparator();
            }
            lastArea = rcArea;
            if (previousShowHidden != showHidden) {
                previousShowHidden = showHidden;
                doAreaScroll = true;
            }
            if ((shouldHideFilteredAreas && filterAreasHidden[rcArea]) ||
                (!showHidden &&
                 ((hideComplete && thisAreaFullyChecked) || (hideIncomplete && !thisAreaFullyChecked))) ||
                (enableAvailableChecks && onlyShowAvailable && areaChecksAvailable[rcArea] == 0)) {
                doDraw = false;
            } else {
                // Get the colour for the area
                if (thisAreaFullyChecked) {
                    mainColor = Color_Area_Complete_Main;
                    extraColor = Color_Area_Complete_Extra;
                } else {
                    mainColor = Color_Area_Incomplete_Main;
                    extraColor = Color_Area_Incomplete_Extra;
                }

                // Draw the area
                collapseLogic = !thisAreaFullyChecked;
                if (doingCollapseOrExpand) {
                    if (optExpandAll) {
                        collapseLogic = true;
                    } else if (optCollapseAll) {
                        collapseLogic = false;
                    }
                }
                stemp = RandomizerCheckObjects::GetRCAreaName(rcArea) + "##TreeNode";
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(mainColor.r / 255.0f, mainColor.g / 255.0f,
                                                            mainColor.b / 255.0f, mainColor.a / 255.0f));
                if (doingCollapseOrExpand) {
                    ImGui::SetNextItemOpen(collapseLogic, ImGuiCond_Always);
                } else {
                    ImGui::SetNextItemOpen(!thisAreaFullyChecked, ImGuiCond_Once);
                }
                doDraw = ImGui::TreeNodeEx(stemp.c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen);
                ImGui::PopStyleColor();
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(extraColor.r / 255.0f, extraColor.g / 255.0f,
                                                            extraColor.b / 255.0f, extraColor.a / 255.0f));

                isThisAreaSpoiled = IsAreaSpoiled(rcArea) || mqSpoilers;

                if (isThisAreaSpoiled) {
                    std::ostringstream areaTotalsSS;
                    std::ostringstream areaTotalsTooltipSS;

                    areaTotalsSS << "(";
                    if (enableAvailableChecks) {
                        areaTotalsSS << static_cast<uint16_t>(areaChecksAvailable[rcArea]) << " / ";
                        areaTotalsTooltipSS << "Available / ";
                    }
                    areaTotalsSS << static_cast<uint16_t>(areaChecksGotten[rcArea]) << " / "
                                 << static_cast<uint16_t>(areaCheckTotals[rcArea]) << ")";
                    areaTotalsTooltipSS << "Checked / Total";

                    if (showVOrMQ && RandomizerCheckObjects::AreaIsDungeon(rcArea)) {
                        if (OTRGlobals::Instance->gRandoContext->GetDungeons()
                                ->GetDungeonFromScene(DungeonSceneLookupByArea(rcArea))
                                ->IsMQ()) {
                            areaTotalsSS << " - MQ";
                        } else {
                            areaTotalsSS << " - Vanilla";
                        }
                    }

                    ImGui::Text("%s", areaTotalsSS.str().c_str());
                    UIWidgets::Tooltip(areaTotalsTooltipSS.str().c_str());
                } else {
                    ImGui::Text("???");
                }

                ImGui::PopStyleColor();

                // Keep areas loaded between transitions
                if (thisArea == rcArea && doAreaScroll) {
                    ImGui::SetScrollHereY(0.0f);
                    doAreaScroll = false;
                }
                for (auto rc : checks) {
                    if (doDraw && isThisAreaSpoiled && !filterChecksHidden[rc]) {
                        DrawLocation(rc);
                    }
                }
            }
        }
        ImGui::PopStyleVar();

        ImGui::EndTable(); // Checks Lead-out
        ImGui::EndTable(); // Quick Options Lead-out
        if (doingCollapseOrExpand) {
            optCollapseAll = false;
            optExpandAll = false;
        }
    }
    Trackers::EndFloatWindows();
}

bool UpdateFilters() {
    for (auto& [rcArea, checks] : checksByArea) {
        filterAreasHidden[rcArea] = !checkSearch.PassFilter(RandomizerCheckObjects::GetRCAreaName(rcArea).c_str());
        for (auto check : checks) {
            if (ShouldShowCheck(check)) {
                filterAreasHidden[rcArea] = false;
                filterChecksHidden[check] = false;
            } else {
                filterChecksHidden[check] = true;
            }
        }
    }

    return true;
}

bool ShouldShowCheck(RandomizerCheck check) {
    auto itemLoc = Rando::Context::GetInstance()->GetItemLocation(check);
    std::string search = (Rando::StaticData::GetLocation(check)->GetShortName() + " " +
                          Rando::StaticData::GetLocation(check)->GetName() + " " +
                          RandomizerCheckObjects::GetRCAreaName(Rando::StaticData::GetLocation(check)->GetArea()));
    if (itemLoc->HasObtained() || itemLoc->GetCheckStatus() == RCSHOW_SCUMMED ||
        (!mystery &&
         (itemLoc->GetCheckStatus() == RCSHOW_IDENTIFIED || itemLoc->GetCheckStatus() == RCSHOW_SEEN_OR_HINTED) &&
         itemLoc->GetPlacedRandomizerGet() != RG_ICE_TRAP)) {
        search += " " + itemLoc->GetPlacedItemName().GetForLanguage(gSaveContext.language);
    } else if (itemLoc->GetCheckStatus() == RCSHOW_IDENTIFIED && !mystery) {
        search +=
            OTRGlobals::Instance->gRandoContext->overrides[check].GetTrickName().GetForLanguage(gSaveContext.language);
    } else if (itemLoc->GetCheckStatus() == RCSHOW_SEEN_OR_HINTED && !mystery) {
        search += Rando::StaticData::RetrieveItem(OTRGlobals::Instance->gRandoContext->overrides[check].LooksLike())
                      .GetName()
                      .GetForLanguage(gSaveContext.language);
    } else if (itemLoc->GetCheckStatus() == RCSHOW_CATEGORY) {
        // Category-seen: searchable by the category observed, never the item name.
        int16_t seenCategory = itemLoc->GetSeenCategory();
        if (seenCategory >= 0) {
            search += " " + ItemCategoryName(static_cast<GetItemCategory>(seenCategory))
                                .GetForLanguage(gSaveContext.language, MF_CLEAN);
        }
    }
    return (IsVisibleInCheckTracker(check) &&
            (checkSearch.Filters.Size == 0 || checkSearch.PassFilter(search.c_str())));
}

void LoadSettings() {
    // If in randomzer, then get the setting and check if in general we should be showing the settings
    // If in vanilla, _try_ to show items that at least are needed for 100%

    showShops =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHOPSANITY) != RO_SHOPSANITY_OFF : false;
    showBeans = IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_MERCHANTS) ==
                                   RO_SHUFFLE_MERCHANTS_BEANS_ONLY ||
                               OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_MERCHANTS) ==
                                   RO_SHUFFLE_MERCHANTS_ALL
                         : true;
    showScrubs =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_SCRUBS) == RO_SCRUBS_ALL : false;
    showMajorScrubs =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_SCRUBS) != RO_SCRUBS_OFF : false;
    showMerchants = IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_MERCHANTS) ==
                                       RO_SHUFFLE_MERCHANTS_ALL_BUT_BEANS ||
                                   OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_MERCHANTS) ==
                                       RO_SHUFFLE_MERCHANTS_ALL
                             : true;
    showSongs = IS_RANDO
                    ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_SONGS) != RO_SONG_SHUFFLE_OFF
                    : false;
    showBeehives = IS_RANDO
                       ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_BEEHIVES) == RO_GENERIC_YES
                       : false;
    showCows =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_COWS) == RO_GENERIC_YES : false;
    showAdultTrade =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ADULT_TRADE) == RO_GENERIC_YES
                 : true;
    showKokiriSword =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_KOKIRI_SWORD) == RO_GENERIC_YES
                 : true;
    showMasterSword =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_MASTER_SWORD) == RO_GENERIC_YES
                 : true;
    showHyruleLoach =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_FISHSANITY) == RO_FISHSANITY_HYRULE_LOACH
                 : false;
    showWeirdEgg = IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_WEIRD_EGG) ==
                                  RO_WEIRD_EGG_SHUFFLED
                            : true;
    showZeldasLetter =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ZELDAS_LETTER) == RO_GENERIC_YES
                 : true;
    showGerudoCard = IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(
                                    RSK_SHUFFLE_GERUDO_MEMBERSHIP_CARD) == RO_GENERIC_YES
                              : true;
    showFrogSongRupees =
        IS_RANDO
            ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_FROG_SONG_RUPEES) == RO_GENERIC_YES
            : false;
    showFountainFairies =
        IS_RANDO
            ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_FOUNTAIN_FAIRIES) == RO_GENERIC_YES
            : false;
    showStoneFairies =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_STONE_FAIRIES) == RO_GENERIC_YES
                 : false;
    showBeanFairies =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_BEAN_FAIRIES) == RO_GENERIC_YES
                 : false;
    showSongFairies =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_SONG_FAIRIES) == RO_GENERIC_YES
                 : false;
    showButterflyFairies =
        IS_RANDO
            ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_BUTTERFLY_FAIRIES) == RO_GENERIC_YES
            : false;
    showStartingMapsCompasses = IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(
                                               RSK_SHUFFLE_MAPANDCOMPASS) != RO_DUNGEON_ITEM_LOC_VANILLA
                                         : false;
    showKeysanity =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_KEYSANITY) != RO_DUNGEON_ITEM_LOC_VANILLA
                 : false;
    showBossKeysanity = IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_BOSS_KEYSANITY) !=
                                       RO_DUNGEON_ITEM_LOC_VANILLA
                                 : false;
    showGerudoFortressKeys =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_GERUDO_KEYS) != RO_GERUDO_KEYS_VANILLA
                 : false;
    showGanonBossKey = IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_GANONS_BOSS_KEY) !=
                                      RO_GANON_BOSS_KEY_VANILLA
                                : false;
    showOcarinas = IS_RANDO
                       ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_OCARINA) == RO_GENERIC_YES
                       : false;
    show100SkullReward =
        IS_RANDO ? OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_100_GS_REWARD) == RO_GENERIC_YES
                 : false;
    showLinksPocket =
        IS_RANDO ? // don't show Link's Pocket if not randomizer, or if rando and pocket is disabled
            OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_LINKS_POCKET) != RO_LINKS_POCKET_NOTHING
                 : false;

    if (IS_RANDO) {
        switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_TOKENS)) {
            case RO_TOKENSANITY_ALL:
                showOverworldTokens = true;
                showDungeonTokens = true;
                break;
            case RO_TOKENSANITY_OVERWORLD:
                showOverworldTokens = true;
                showDungeonTokens = false;
                break;
            case RO_TOKENSANITY_DUNGEONS:
                showOverworldTokens = false;
                showDungeonTokens = true;
                break;
            default:
                showOverworldTokens = false;
                showDungeonTokens = false;
                break;
        }

        switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_POTS)) {
            case RO_SHUFFLE_POTS_ALL:
                showOverworldPots = true;
                showDungeonPots = true;
                break;
            case RO_SHUFFLE_POTS_OVERWORLD:
                showOverworldPots = true;
                showDungeonPots = false;
                break;
            case RO_SHUFFLE_POTS_DUNGEONS:
                showOverworldPots = false;
                showDungeonPots = true;
                break;
            default:
                showOverworldPots = false;
                showDungeonPots = false;
                break;
        }

        switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_GRASS)) {
            case RO_SHUFFLE_GRASS_ALL:
                showOverworldGrass = true;
                showDungeonGrass = true;
                break;
            case RO_SHUFFLE_GRASS_OVERWORLD:
                showOverworldGrass = true;
                showDungeonGrass = false;
                break;
            case RO_SHUFFLE_GRASS_DUNGEONS:
                showOverworldGrass = false;
                showDungeonGrass = true;
                break;
            default:
                showOverworldGrass = false;
                showDungeonGrass = false;
                break;
        }

        switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_CRATES)) {
            case RO_SHUFFLE_CRATES_ALL:
                showOverworldCrates = true;
                showDungeonCrates = true;
                break;
            case RO_SHUFFLE_CRATES_OVERWORLD:
                showOverworldCrates = true;
                showDungeonCrates = false;
                break;
            case RO_SHUFFLE_CRATES_DUNGEONS:
                showOverworldCrates = false;
                showDungeonCrates = true;
                break;
            default:
                showOverworldCrates = false;
                showDungeonCrates = false;
                break;
        }

        showRocks = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ROCKS);
        switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_BOULDERS)) {
            case RO_SHUFFLE_BOULDERS_ALL:
                showOverworldBoulders = true;
                showDungeonBoulders = true;
                break;
            case RO_SHUFFLE_BOULDERS_OVERWORLD:
                showOverworldBoulders = true;
                showDungeonBoulders = false;
                break;
            case RO_SHUFFLE_BOULDERS_DUNGEONS:
                showOverworldBoulders = false;
                showDungeonBoulders = true;
                break;
            default:
                showOverworldBoulders = false;
                showDungeonBoulders = false;
                break;
        }

        switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_SIGNS)) {
            case RO_SHUFFLE_SIGNS_ALL:
                showOverworldSigns = true;
                showDungeonSigns = true;
                break;
            case RO_SHUFFLE_SIGNS_OVERWORLD:
                showOverworldSigns = true;
                showDungeonSigns = false;
                break;
            case RO_SHUFFLE_SIGNS_DUNGEONS:
                showOverworldSigns = false;
                showDungeonSigns = true;
                break;
            default:
                showOverworldSigns = false;
                showDungeonSigns = false;
                break;
        }

        showTrees = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_TREES);
        showBushes = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_BUSHES);

        switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_WONDER_ITEMS)) {
            case RO_SHUFFLE_WONDER_ITEMS_ALL:
                showOverworldWonderItems = true;
                showDungeonWonderItems = true;
                break;
            case RO_SHUFFLE_WONDER_ITEMS_OVERWORLD:
                showOverworldWonderItems = true;
                showDungeonWonderItems = false;
                break;
            case RO_SHUFFLE_WONDER_ITEMS_DUNGEONS:
                showOverworldWonderItems = false;
                showDungeonWonderItems = true;
                break;
            default:
                showOverworldWonderItems = false;
                showDungeonWonderItems = false;
                break;
        }
        showBeggar = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_BEGGAR);
        showIcicles = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_ICICLES);
        showRedIce = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_RED_ICE);
    } else { // Vanilla
        showOverworldTokens = true;
        showDungeonTokens = true;
        showOverworldPots = false;
        showDungeonPots = false;
        showOverworldGrass = false;
        showDungeonGrass = false;
        showOverworldCrates = false;
        showDungeonCrates = false;
        showRocks = false;
        showOverworldBoulders = false;
        showDungeonBoulders = false;
        showTrees = false;
        showBushes = false;
        showOverworldWonderItems = false;
        showDungeonWonderItems = false;
        showOverworldSigns = false;
        showDungeonSigns = false;
        showBeggar = false;
        showIcicles = false;
        showRedIce = false;
    }

    fortressFast = false;
    fortressNormal = false;
    switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_GERUDO_FORTRESS)) {
        case RO_GF_CARPENTERS_FREE:
            showGerudoFortressKeys = false;
            showGerudoCard = false;
            break;
        case RO_GF_CARPENTERS_FAST:
            fortressFast = true;
            break;
        case RO_GF_CARPENTERS_NORMAL:
            fortressNormal = true;
            break;
    }

    fishsanityMode = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_FISHSANITY);
    fishsanityPondCount = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_FISHSANITY_POND_COUNT);
    fishsanityAgeSplit = OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_FISHSANITY_AGE_SPLIT);

    if (IS_RANDO) {
        switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SHUFFLE_FREESTANDING)) {
            case RO_SHUFFLE_FREESTANDING_ALL:
                showOverworldFreestanding = true;
                showDungeonFreestanding = true;
                break;
            case RO_SHUFFLE_FREESTANDING_OVERWORLD:
                showOverworldFreestanding = true;
                showDungeonFreestanding = false;
                break;
            case RO_SHUFFLE_FREESTANDING_DUNGEONS:
                showOverworldFreestanding = false;
                showDungeonFreestanding = true;
                break;
            default:
                showOverworldFreestanding = false;
                showDungeonFreestanding = false;
                break;
        }
    } else { // Vanilla
        showOverworldFreestanding = false;
        showDungeonFreestanding = true;
    }

    switch (OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_GANONS_BOSS_KEY)) {
        case RO_GANON_BOSS_KEY_STONES:
            Rando::Context::GetInstance()->GBKCondition(RO_CHECK_TRIGGER_STONES);
            break;
        case RO_GANON_BOSS_KEY_MEDALLIONS:
            Rando::Context::GetInstance()->GBKCondition(RO_CHECK_TRIGGER_MEDALLIONS);
            break;
        case RO_GANON_BOSS_KEY_REWARDS:
            Rando::Context::GetInstance()->GBKCondition(RO_CHECK_TRIGGER_REWARDS);
            break;
        case RO_GANON_BOSS_KEY_DUNGEONS:
            Rando::Context::GetInstance()->GBKCondition(RO_CHECK_TRIGGER_DUNGEONS);
            break;
        case RO_GANON_BOSS_KEY_TOKENS:
            Rando::Context::GetInstance()->GBKCondition(RO_CHECK_TRIGGER_TOKENS);
            break;
        default:
            Rando::Context::GetInstance()->GBKCondition(RO_CHECK_TRIGGER_NONE);
            break;
    }
}

bool IsCheckShuffled(RandomizerCheck rc) {
    Rando::Location* loc = Rando::StaticData::GetLocation(rc);
    if (loc->GetRCType() == RCTYPE_SHOP) {
        auto identity = OTRGlobals::Instance->gRandomizer->IdentifyShopItem(loc->GetScene(), loc->GetActorParams() + 1);
    }
    if (IS_RANDO) {
        return (loc->GetArea() != RCAREA_INVALID) &&        // don't show Invalid locations
               (loc->GetRCType() != RCTYPE_GOSSIP_STONE) && // TODO: Don't show hints until tracker supports them
               (loc->GetRCType() != RCTYPE_STATIC_HINT) &&  // TODO: Don't show hints until tracker supports them
               (loc->GetRCType() != RCTYPE_CHEST_GAME) &&   // don't show non final reward chest game checks until we
                                                            // support shuffling them
               (rc != RC_HC_ZELDAS_LETTER || showZeldasLetter) && (rc != RC_LINKS_POCKET || showLinksPocket) &&
               OTRGlobals::Instance->gRandoContext->IsQuestOfLocationActive(rc) &&
               (loc->GetRCType() != RCTYPE_SHOP ||
                (showShops &&
                 OTRGlobals::Instance->gRandomizer->IdentifyShopItem(loc->GetScene(), loc->GetActorParams() + 1)
                         .enGirlAShopItem == 50)) &&
               (rc != RC_WINCON) && (rc != RC_GANON) &&
               (loc->GetRCType() != RCTYPE_SCRUB || showScrubs ||
                (showMajorScrubs && (rc == RC_LW_DEKU_SCRUB_NEAR_BRIDGE || // The 3 scrubs that are always randomized
                                     rc == RC_HF_DEKU_SCRUB_GROTTO || rc == RC_LW_DEKU_SCRUB_GROTTO_FRONT))) &&
               ((loc->GetRCType() != RCTYPE_MERCHANT || (showMerchants && rc != RC_ZR_MAGIC_BEAN_SALESMAN)) ||
                (rc == RC_ZR_MAGIC_BEAN_SALESMAN && showBeans)) &&
               (loc->GetRCType() != RCTYPE_BEGGAR || showBeggar) &&
               (loc->GetRCType() != RCTYPE_SONG_LOCATION || showSongs) &&
               (loc->GetRCType() != RCTYPE_BEEHIVE || showBeehives) &&
               (loc->GetRCType() != RCTYPE_OCARINA || showOcarinas) &&
               (loc->GetRCType() != RCTYPE_SKULL_TOKEN || alwaysShowGS ||
                (showOverworldTokens && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea())) ||
                (showDungeonTokens && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_POT ||
                (showOverworldPots && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea())) ||
                (showDungeonPots && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_GRASS ||
                (showOverworldGrass && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea())) ||
                (showDungeonGrass && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_CRATE ||
                (showOverworldCrates && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea())) ||
                (showDungeonCrates && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_NLCRATE ||
                (showOverworldCrates && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea()) &&
                 OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_LOGIC_RULES) == RO_LOGIC_NO_LOGIC) ||
                (showDungeonCrates && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_SMALL_CRATE ||
                (showOverworldCrates && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea())) ||
                (showDungeonCrates && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_ROCK || showRocks) &&
               (loc->GetRCType() != RCTYPE_BOULDER ||
                (showOverworldBoulders && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea())) ||
                (showDungeonBoulders && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_TREE || showTrees) &&
               (loc->GetRCType() != RCTYPE_NLTREE ||
                (showTrees &&
                 OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_LOGIC_RULES) == RO_LOGIC_NO_LOGIC)) &&
               (loc->GetRCType() != RCTYPE_BUSH || showBushes) && (loc->GetRCType() != RCTYPE_COW || showCows) &&
               (loc->GetRCType() != RCTYPE_SIGN ||
                (showOverworldSigns && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea())) ||
                (showDungeonSigns && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_WONDER_ITEM ||
                (showOverworldWonderItems && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea())) ||
                (showDungeonWonderItems && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_ICICLE || showIcicles) &&
               (loc->GetRCType() != RCTYPE_RED_ICE || showRedIce) &&
               (loc->GetRCType() != RCTYPE_FISH ||
                OTRGlobals::Instance->gRandoContext->GetFishsanity()->GetFishLocationIncluded(loc)) &&
               (loc->GetRCType() != RCTYPE_FREESTANDING ||
                (showOverworldFreestanding && RandomizerCheckObjects::AreaIsOverworld(loc->GetArea())) ||
                (showDungeonFreestanding && RandomizerCheckObjects::AreaIsDungeon(loc->GetArea()))) &&
               (loc->GetRCType() != RCTYPE_ADULT_TRADE || showAdultTrade ||
                rc == RC_KAK_ANJU_AS_ADULT ||  // adult trade checks that are always shuffled
                rc == RC_DMT_TRADE_CLAIM_CHECK // even when shuffle adult trade is off
                ) &&
               (rc != RC_KF_KOKIRI_SWORD_CHEST || showKokiriSword) && (rc != RC_TOT_MASTER_SWORD || showMasterSword) &&
               (rc != RC_LH_HYRULE_LOACH || showHyruleLoach) && (rc != RC_HC_MALON_EGG || showWeirdEgg) &&
               (loc->GetRCType() != RCTYPE_FROG_SONG || showFrogSongRupees) &&
               ((loc->GetRCType() != RCTYPE_MAP && loc->GetRCType() != RCTYPE_COMPASS) || showStartingMapsCompasses) &&
               (loc->GetRCType() != RCTYPE_FOUNTAIN_FAIRY || showFountainFairies) &&
               (loc->GetRCType() != RCTYPE_STONE_FAIRY || showStoneFairies) &&
               (loc->GetRCType() != RCTYPE_BEAN_FAIRY || showBeanFairies) &&
               (loc->GetRCType() != RCTYPE_SONG_FAIRY || showSongFairies) &&
               (loc->GetRCType() != RCTYPE_BUTTERFLY_FAIRY || showButterflyFairies) &&
               (loc->GetRCType() != RCTYPE_SMALL_KEY || showKeysanity) &&
               (loc->GetRCType() != RCTYPE_BOSS_KEY || showBossKeysanity) &&
               (loc->GetRCType() != RCTYPE_GANON_BOSS_KEY || showGanonBossKey) &&
               (rc != RC_KAK_100_GOLD_SKULLTULA_REWARD || show100SkullReward) &&
               (loc->GetRCType() != RCTYPE_GF_KEY && rc != RC_TH_FREED_CARPENTERS ||
                (showGerudoCard && rc == RC_TH_FREED_CARPENTERS) ||
                (fortressNormal && showGerudoFortressKeys && loc->GetRCType() == RCTYPE_GF_KEY) ||
                (fortressFast && showGerudoFortressKeys && rc == RC_TH_1_TORCH_CARPENTER));
    } else if (loc->IsVanillaCompletion()) {
        return (OTRGlobals::Instance->gRandoContext->IsQuestOfLocationActive(rc) || rc == RC_GIFT_FROM_RAURU) &&
               rc != RC_LINKS_POCKET;
    }
    return false;
}

bool IsVisibleInCheckTracker(RandomizerCheck rc) {
    auto loc = Rando::StaticData::GetLocation(rc);
    if (IS_RANDO) {
        return !Rando::Context::GetInstance()->GetItemLocation(rc)->IsExcluded() &&
               (IsCheckShuffled(rc) ||
                (alwaysShowGS && loc->GetRCType() == RCTYPE_SKULL_TOKEN &&
                 OTRGlobals::Instance->gRandoContext->IsQuestOfLocationActive(rc)) ||
                (loc->GetRCType() == RCTYPE_SHOP && showShops && !hideShopUnshuffledChecks));
    } else {
        return loc->IsVanillaCompletion() &&
               (!loc->IsDungeon() || (loc->IsDungeon() && loc->GetQuest() == gSaveContext.ship.quest.id));
    }
}

void UpdateInventoryChecks() {
    // For all the areas with maps, if you have one, spoil the area
    for (auto [scene, area] : DungeonRCAreasBySceneID) {
        if (CHECK_DUNGEON_ITEM(DUNGEON_MAP, scene)) {
            SetAreaSpoiled(area);
        }
    }
}

void UpdateAreaFullyChecked(RandomizerCheckArea area) {
}

void UpdateAllAreas() {
    // Sort the entire thing
    for (int i = 0; i < RCAREA_INVALID; i++) {
        UpdateAreas(static_cast<RandomizerCheckArea>(i));
    }
}

void UpdateAreas(RandomizerCheckArea area) {
    if (checksByArea.contains(area)) {
        areasFullyChecked[area] = areaChecksGotten[area] == checksByArea.find(area)->second.size();
    }
}

void UpdateAllOrdering() {
    // Sort the entire thing
    for (int i = 0; i < RCAREA_INVALID; i++) {
        UpdateOrdering(static_cast<RandomizerCheckArea>(i));
    }
}

void UpdateOrdering(RandomizerCheckArea rcArea) {
    // Sort a single area
    if (checksByArea.contains(rcArea)) {
        std::sort(checksByArea.find(rcArea)->second.begin(), checksByArea.find(rcArea)->second.end(), CompareChecks);
    }
    RecalculateAllAreaTotals();
    CalculateTotals();
}

bool IsEoDCheck(RandomizerCheckType type) {
    return type == RCTYPE_BOSS_HEART_OR_OTHER_REWARD || type == RCTYPE_DUNGEON_REWARD;
}

bool CompareChecks(RandomizerCheck i, RandomizerCheck j) {
    Rando::Location* x = Rando::StaticData::GetLocation(i);
    Rando::Location* y = Rando::StaticData::GetLocation(j);
    auto itemI = OTRGlobals::Instance->gRandoContext->GetItemLocation(i);
    auto itemJ = OTRGlobals::Instance->gRandoContext->GetItemLocation(j);
    bool iCollected = itemI->HasObtained();
    bool iSaved = itemI->GetCheckStatus() == RCSHOW_SAVED;
    bool jCollected = itemJ->HasObtained();
    bool jSaved = itemJ->GetCheckStatus() == RCSHOW_SAVED;

    if (!iCollected && jCollected) {
        return true;
    } else if (iCollected && !jCollected) {
        return false;
    }

    if (!iSaved && jSaved) {
        return true;
    } else if (iSaved && !jSaved) {
        return false;
    }

    if (!itemI->GetIsSkipped() && itemJ->GetIsSkipped()) {
        return true;
    } else if (itemI->GetIsSkipped() && !itemJ->GetIsSkipped()) {
        return false;
    }

    if (!IsEoDCheck(x->GetRCType()) && IsEoDCheck(y->GetRCType())) {
        return true;
    } else if (IsEoDCheck(x->GetRCType()) && !IsEoDCheck(y->GetRCType())) {
        return false;
    }

    if (i < j) {
        return true;
    } else if (i > j) {
        return false;
    }

    return false;
}

bool IsHeartPiece(GetItemID giid) {
    return giid == GI_HEART_PIECE || giid == GI_HEART_PIECE_WIN;
}

bool IsMysteryShopItem(RandomizerCheck rc) {
    s32 sceneNum = 0;
    u8 slotIndex = 0;

    if (rc >= RC_KF_SHOP_ITEM_1 && rc <= RC_KF_SHOP_ITEM_8) {
        sceneNum = SCENE_KOKIRI_SHOP;
        slotIndex = rc - RC_KF_SHOP_ITEM_1;
    } else if (rc >= RC_MARKET_BAZAAR_ITEM_1 && rc <= RC_MARKET_BAZAAR_ITEM_8) {
        sceneNum = SCENE_BAZAAR;
        slotIndex = rc - RC_MARKET_BAZAAR_ITEM_1;
    } else if (rc >= RC_MARKET_POTION_SHOP_ITEM_1 && rc <= RC_MARKET_POTION_SHOP_ITEM_8) {
        sceneNum = SCENE_POTION_SHOP_MARKET;
        slotIndex = rc - RC_MARKET_POTION_SHOP_ITEM_1;
    } else if (rc >= RC_MARKET_BOMBCHU_SHOP_ITEM_1 && rc <= RC_MARKET_BOMBCHU_SHOP_ITEM_8) {
        sceneNum = SCENE_BOMBCHU_SHOP;
        slotIndex = rc - RC_MARKET_BOMBCHU_SHOP_ITEM_1;
    } else if (rc >= RC_KAK_BAZAAR_ITEM_1 && rc <= RC_KAK_BAZAAR_ITEM_8) {
        sceneNum = SCENE_TEST01;
        slotIndex = rc - RC_KAK_BAZAAR_ITEM_1;
    } else if (rc >= RC_KAK_POTION_SHOP_ITEM_1 && rc <= RC_KAK_POTION_SHOP_ITEM_8) {
        sceneNum = SCENE_POTION_SHOP_KAKARIKO;
        slotIndex = rc - RC_KAK_POTION_SHOP_ITEM_1;
    } else if (rc >= RC_GC_SHOP_ITEM_1 && rc <= RC_GC_SHOP_ITEM_8) {
        sceneNum = SCENE_GORON_SHOP;
        slotIndex = rc - RC_GC_SHOP_ITEM_1;
    } else if (rc >= RC_ZD_SHOP_ITEM_1 && rc <= RC_ZD_SHOP_ITEM_8) {
        sceneNum = SCENE_ZORA_SHOP;
        slotIndex = rc - RC_ZD_SHOP_ITEM_1;
    } else {
        return false;
    }

    ShopItemIdentity shopItemIdentity = OTRGlobals::Instance->gRandomizer->IdentifyShopItem(sceneNum, slotIndex + 1);
    return shopItemIdentity.enGirlAShopItem == SI_RANDOMIZED_ITEM;
}

void DrawLocation(RandomizerCheck rc) {
    Color_RGBA8 mainColor;
    Color_RGBA8 extraColor;
    std::string txt;
    Rando::Location* loc = Rando::StaticData::GetLocation(rc);
    Rando::ItemLocation* itemLoc = OTRGlobals::Instance->gRandoContext->GetItemLocation(rc);
    RandomizerCheckStatus status = itemLoc->GetCheckStatus();
    bool skipped = itemLoc->GetIsSkipped();
    bool available = itemLoc->IsAvailable();

    if (enableAvailableChecks && onlyShowAvailable && !available) {
        return;
    }

    if (status == RCSHOW_COLLECTED) {
        if (!showHidden && hideCollected) {
            return;
        }
        mainColor =
            !IsHeartPiece((GetItemID)Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetItemID()) && !IS_RANDO
                ? Color_Collected_Extra
                : Color_Collected_Main;
        extraColor = Color_Collected_Extra;
    } else if (status == RCSHOW_SAVED) {
        if (!showHidden && hideSaved) {
            return;
        }
        mainColor =
            !IsHeartPiece((GetItemID)Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetItemID()) && !IS_RANDO
                ? Color_Saved_Extra
                : Color_Saved_Main;
        extraColor = Color_Saved_Extra;
    } else if (skipped) {
        if (!showHidden && hideSkipped) {
            return;
        }
        mainColor =
            !IsHeartPiece((GetItemID)Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetItemID()) && !IS_RANDO
                ? Color_Skipped_Extra
                : Color_Skipped_Main;
        extraColor = Color_Skipped_Extra;
    } else if (status == RCSHOW_SEEN_OR_HINTED || status == RCSHOW_IDENTIFIED) {
        if (!showHidden && hideSeen) {
            return;
        }
        mainColor =
            !IsHeartPiece((GetItemID)Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetItemID()) && !IS_RANDO
                ? Color_Seen_Extra
                : Color_Seen_Main;
        extraColor = Color_Seen_Extra;
    } else if (status == RCSHOW_CATEGORY) {
        if (!showHidden && hideCategory) {
            return;
        }
        mainColor = Color_Hinted_Main;
        extraColor = Color_Hinted_Extra;
    } else if (status == RCSHOW_SCUMMED) {
        if (!showHidden && hideScummed) {
            return;
        }
        mainColor =
            !IsHeartPiece((GetItemID)Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetItemID()) && !IS_RANDO
                ? Color_Scummed_Extra
                : Color_Scummed_Main;
        extraColor = Color_Scummed_Extra;
    } else if (status == RCSHOW_UNCHECKED) {
        if (!showHidden && hideUnchecked) {
            return;
        }
        mainColor =
            !IsHeartPiece((GetItemID)Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetItemID()) && !IS_RANDO
                ? Color_Unchecked_Extra
                : Color_Unchecked_Main;
        extraColor = Color_Unchecked_Extra;
    }

    // Main Text
    if (checkNameOverrides.contains(loc->GetRandomizerCheck())) {
        txt = checkNameOverrides[loc->GetRandomizerCheck()];
    } else {
        txt = loc->GetShortName();
    }

    if (lastLocationChecked == loc->GetRandomizerCheck()) {
        txt = "* " + txt;
    }

    // Draw button - for Category/Skipped/Seen/Scummed/Unchecked only
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 4.0f, 3.0f });
    float sz = ImGui::GetFrameHeight();
    if (status == RCSHOW_UNCHECKED || status == RCSHOW_CATEGORY || status == RCSHOW_SEEN_OR_HINTED ||
        status == RCSHOW_IDENTIFIED || status == RCSHOW_SCUMMED || skipped) {
        if (UIWidgets::StateButton(std::to_string(rc).c_str(), skipped ? ICON_FA_PLUS : ICON_FA_TIMES, ImVec2(sz, sz),
                                   UIWidgets::ButtonOptions().Color(THEME_COLOR))) {
            if (skipped) {
                OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->SetIsSkipped(false);
                areaChecksGotten[loc->GetArea()]--;
                totalChecksGotten--;
                if (available) {
                    areaChecksAvailable[loc->GetArea()]++;
                    totalChecksAvailable++;
                }
            } else {
                OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->SetIsSkipped(true);
                areaChecksGotten[loc->GetArea()]++;
                totalChecksGotten++;
                if (available) {
                    areaChecksAvailable[loc->GetArea()]--;
                    totalChecksAvailable--;
                }
            }
            UpdateOrdering(loc->GetArea());
            UpdateInventoryChecks();
            SaveManager::Instance->SaveSection(gSaveContext.fileNum, sectionId, true);
        }
    } else {
        ImGui::Dummy(ImVec2(sz, sz));
    }
    ImGui::PopStyleVar();

    ImGui::SameLine();

    // Draw
    ImVec4 styleColor(mainColor.r / 255.0f, mainColor.g / 255.0f, mainColor.b / 255.0f, mainColor.a / 255.0f);
    if (enableAvailableChecks) {
        if (itemLoc->HasObtained()) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0, 0, 0, 0));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, styleColor);
        }
        ImGui::Text("%s", available ? ICON_FA_UNLOCK : ICON_FA_LOCK);
        ImGui::PopStyleColor();
        ImGui::SameLine();
    }

    ImGui::PushStyleColor(ImGuiCol_Text, styleColor);
    ImGui::Text("%s", txt.c_str());
    ImGui::PopStyleColor();

    // Draw the extra info
    txt = "";

    if (status != RCSHOW_UNCHECKED) {
        switch (status) {
            case RCSHOW_CATEGORY: {
                // A seen chest reveals only its category (via CSMC), so show the
                // category the chest drew at the time, not the item. Not
                // mystery-gated: the chest shows its category under mystery too.
                // A check recorded before categories were stored has none, and
                // shows nothing rather than a guess.
                int16_t seenCategory = itemLoc->GetSeenCategory();
                if (seenCategory >= 0) {
                    txt = ItemCategoryName(static_cast<GetItemCategory>(seenCategory))
                              .GetForLanguage(gSaveContext.language, MF_CLEAN);
                }
                break;
            }
            case RCSHOW_SAVED:
            case RCSHOW_COLLECTED:
            case RCSHOW_SCUMMED:
                if (IS_RANDO) {
                    txt = itemLoc->GetPlacedItem().GetName().GetForLanguage(gSaveContext.language);
                } else {
                    if (IsHeartPiece((GetItemID)Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetItemID())) {
                        if (gSaveContext.language == LANGUAGE_ENG || gSaveContext.language == LANGUAGE_GER ||
                            gSaveContext.language == LANGUAGE_JPN) {
                            txt = Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetName().english;
                        } else if (gSaveContext.language == LANGUAGE_FRA) {
                            txt = Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetName().french;
                        }
                    }
                }
                break;
            case RCSHOW_IDENTIFIED:
            case RCSHOW_SEEN_OR_HINTED:
                if (IS_RANDO) {
                    const auto checkType = loc->GetRCType();
                    const bool hideMerchantName =
                        checkType == RCTYPE_MERCHANT &&
                        (!OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_MERCHANT_TEXT_HINT) || mystery);
                    const bool hideScrubName =
                        checkType == RCTYPE_SCRUB &&
                        (!OTRGlobals::Instance->gRandomizer->GetRandoSettingValue(RSK_SCRUB_TEXT_HINT) || mystery);
                    const bool hideShopName = checkType == RCTYPE_SHOP && mystery && IsMysteryShopItem(rc);
                    const bool revealItemName = !(hideMerchantName || hideScrubName || hideShopName);

                    if (itemLoc->GetPlacedRandomizerGet() == RG_ICE_TRAP && revealItemName) {
                        if (status == RCSHOW_IDENTIFIED) {
                            txt = OTRGlobals::Instance->gRandoContext->overrides[rc].GetTrickName().GetForLanguage(
                                gSaveContext.language);
                        } else {
                            txt = Rando::StaticData::RetrieveItem(
                                      OTRGlobals::Instance->gRandoContext->overrides[rc].LooksLike())
                                      .GetName()
                                      .GetForLanguage(gSaveContext.language);
                        }
                    } else if (revealItemName) {
                        txt = itemLoc->GetPlacedItem().GetName().GetForLanguage(gSaveContext.language);
                    }
                    if (itemLoc->CanBePurchased() && IsVisibleInCheckTracker(rc) && status == RCSHOW_IDENTIFIED) {
                        auto price = OTRGlobals::Instance->gRandoContext->GetItemLocation(rc)->GetPrice();
                        txt = !txt.empty() ? fmt::format("{} - {}", txt, price) : fmt::format("{}", price);
                    }
                } else {
                    if (IsHeartPiece((GetItemID)Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetItemID())) {
                        if (gSaveContext.language == LANGUAGE_ENG || gSaveContext.language == LANGUAGE_GER ||
                            gSaveContext.language == LANGUAGE_JPN) {
                            txt = Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetName().english;
                        } else if (gSaveContext.language == LANGUAGE_FRA) {
                            txt = Rando::StaticData::RetrieveItem(loc->GetVanillaItem()).GetName().french;
                        }
                    }
                }
                break;
        }
    }
    if (txt == "" && skipped) {
        txt = "Skipped"; // TODO language
    }

    if (txt != "") {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(extraColor.r / 255.0f, extraColor.g / 255.0f, extraColor.b / 255.0f,
                                                    extraColor.a / 255.0f));
        ImGui::SameLine();
        ImGui::Text(" (%s)", txt.c_str());
        ImGui::PopStyleColor();
    }

    if (showLogicTooltip) {
        for (auto& locationInRegion : areaTable[itemLoc->GetParentRegionKey()].locations) {
            if (locationInRegion.GetLocation() == rc) {
                std::string conditionStr = locationInRegion.GetConditionStr();
                if (conditionStr != "true") {
                    UIWidgets::Tooltip(conditionStr.c_str());
                }
                break;
            }
        }
    }
}

static std::set<std::string> rainbowCVars = {
    CVAR_TRACKER_CHECK("AreaIncomplete.MainColor"), CVAR_TRACKER_CHECK("AreaIncomplete.ExtraColor"),
    CVAR_TRACKER_CHECK("AreaComplete.MainColor"),   CVAR_TRACKER_CHECK("AreaComplete.ExtraColor"),
    CVAR_TRACKER_CHECK("Unchecked.MainColor"),      CVAR_TRACKER_CHECK("Unchecked.ExtraColor"),
    CVAR_TRACKER_CHECK("Skipped.MainColor"),        CVAR_TRACKER_CHECK("Skipped.ExtraColor"),
    CVAR_TRACKER_CHECK("Seen.MainColor"),           CVAR_TRACKER_CHECK("Seen.ExtraColor"),
    CVAR_TRACKER_CHECK("Hinted.MainColor"),         CVAR_TRACKER_CHECK("Hinted.ExtraColor"),
    CVAR_TRACKER_CHECK("Collected.MainColor"),      CVAR_TRACKER_CHECK("Collected.ExtraColor"),
    CVAR_TRACKER_CHECK("Scummed.MainColor"),        CVAR_TRACKER_CHECK("Scummed.ExtraColor"),
    CVAR_TRACKER_CHECK("Saved.MainColor"),          CVAR_TRACKER_CHECK("Saved.ExtraColor"),
};

int hue = 0;
void RainbowTick() {
    float freqHue = hue * 2 * M_PIf / (360 * CVarGetFloat(CVAR_COSMETIC("RainbowSpeed"), 0.6f));
    for (auto& cvar : rainbowCVars) {
        if (CVarGetInteger((cvar + ".Rainbow").c_str(), 0) == 0) {
            continue;
        }

        Color_RGBA8 newColor;
        newColor.r = static_cast<uint8_t>(sin(freqHue + 0) * 127) + 128;
        newColor.g = static_cast<uint8_t>(sin(freqHue + (2 * M_PI / 3)) * 127) + 128;
        newColor.b = static_cast<uint8_t>(sin(freqHue + (4 * M_PI / 3)) * 127) + 128;
        newColor.a = 255;

        CVarSetColor((cvar + ".Value").c_str(), newColor);
    }

    hue++;
    hue %= 360;
}

void ImGuiDrawTwoColorPickerSection(const char* text, const char* cvarMainName, const char* cvarExtraName,
                                    Color_RGBA8& main_color, Color_RGBA8& extra_color,
                                    const Color_RGBA8& main_default_color, const Color_RGBA8& extra_default_color,
                                    const char* cvarHideName, const char* tooltip, UIWidgets::Colors theme) {
    Color_RGBA8 cvarMainColor = CVarGetColor(cvarMainName, main_default_color);
    Color_RGBA8 cvarExtraColor = CVarGetColor(cvarExtraName, extra_default_color);
    main_color = cvarMainColor;
    extra_color = cvarExtraColor;

    UIWidgets::PushStyleCombobox(theme);
    if (ImGui::CollapsingHeader(text)) {
        if (*cvarHideName != '\0') {
            std::string label = cvarHideName;
            label += "##Hidden";
            ImGui::PushID(label.c_str());
            UIWidgets::CVarCheckbox(
                "Hidden", cvarHideName,
                UIWidgets::CheckboxOptions(
                    { { .tooltip = "When active, checks will hide by default when updated to this state. Can "
                                   "be overridden with the \"Show Hidden Items\" option." } })
                    .Color(theme));
            ImGui::PopID();
        }
        std::string mainLabel = "Name##" + std::string(cvarMainName);
        if (UIWidgets::CVarColorPicker(mainLabel.c_str(), cvarMainName, main_default_color, false,
                                       UIWidgets::ColorPickerRandomButton | UIWidgets::ColorPickerResetButton |
                                           UIWidgets::ColorPickerRainbowCheck,
                                       theme)) {
            main_color = CVarGetColor(cvarMainName, main_default_color);
        }

        std::string extraLabel = "Details##" + std::string(cvarExtraName);
        if (UIWidgets::CVarColorPicker(extraLabel.c_str(), cvarExtraName, extra_default_color, false,
                                       UIWidgets::ColorPickerRandomButton | UIWidgets::ColorPickerResetButton |
                                           UIWidgets::ColorPickerRainbowCheck,
                                       theme)) {
            extra_color = CVarGetColor(cvarExtraName, extra_default_color);
        }
    }
    if (tooltip != NULL && strlen(tooltip) != 0) {
        ImGui::SameLine();
        ImGui::Text(" ?");
        UIWidgets::Tooltip(tooltip);
    }
    UIWidgets::PopStyleCombobox();
}

void InternalRecalculateAvailableChecks(RandomizerRegion startingRegion, RandoAgeTime startingAgeTime) {
    if (!enableAvailableChecks || !GameInteractor::IsSaveLoaded()) {
        return;
    }

    ResetPerformanceTimer(PT_RECALCULATE_AVAILABLE_CHECKS);
    StartPerformanceTimer(PT_RECALCULATE_AVAILABLE_CHECKS);

    const auto& ctx = Rando::Context::GetInstance();
    logic = ctx->GetLogic();

    int16_t entranceIndex = gPlayState->nextEntranceIndex;
    if (startingRegion == RR_ROOT && entranceIndex >= 0 && entranceIndex < ENTR_MAX) {
        // Try to find a mapped entrance
        // e.g. ENTR_DEKU_TREE_0_1 (index 1) is not mapped, but ENTR_DEKU_TREE_ENTRANCE (index 0) is mapped
        const int8_t scene = gEntranceTable[entranceIndex].scene;
        for (; entranceIndex >= 0 && gEntranceTable[entranceIndex].scene == scene; entranceIndex--) {
            const auto entrance = Rando::EntranceShuffler::GetEntranceByIndex(entranceIndex);
            if (entrance != nullptr) {
                startingRegion = entrance->GetOriginalConnectedRegionKey();
                break;
            }
        }
    }

    if (startingAgeTime == RAT_NONE) {
        if (LINK_IS_CHILD && IS_DAY) {
            startingAgeTime = RAT_CHILD_DAY;
        } else if (LINK_IS_CHILD && IS_NIGHT) {
            startingAgeTime = RAT_CHILD_NIGHT;
        } else if (LINK_IS_ADULT && IS_DAY) {
            startingAgeTime = RAT_ADULT_DAY;
        } else if (LINK_IS_ADULT && IS_NIGHT) {
            startingAgeTime = RAT_ADULT_NIGHT;
        }
    }

    std::vector<RandomizerCheck> targetLocations;
    targetLocations.reserve(RC_MAX);
    for (auto& location : Rando::StaticData::GetLocationTable()) {
        RandomizerCheck rc = location.GetRandomizerCheck();
        Rando::ItemLocation* itemLocation = ctx->GetItemLocation(rc);
        itemLocation->SetAvailable(false);
        if (!itemLocation->HasObtained()) {
            targetLocations.emplace_back(rc);
        }
    }

    std::vector<RandomizerCheck> availableChecks =
        ReachabilitySearch(targetLocations, RG_NONE, true, startingRegion, startingAgeTime);
    for (auto& rc : availableChecks) {
        const auto& itemLocation = ctx->GetItemLocation(rc);
        itemLocation->SetAvailable(true);
    }

    totalChecksAvailable = 0;
    for (auto& [rcArea, vec] : checksByArea) {
        areaChecksAvailable[rcArea] = 0;
        for (auto& rc : vec) {
            Rando::ItemLocation* itemLocation = ctx->GetItemLocation(rc);
            if (itemLocation->IsAvailable() && IsVisibleInCheckTracker(rc) && !IsCheckHidden(rc)) {
                areaChecksAvailable[rcArea]++;
            }
        }
        totalChecksAvailable += areaChecksAvailable[rcArea];
    }

    StopPerformanceTimer(PT_RECALCULATE_AVAILABLE_CHECKS);
    SPDLOG_INFO("Recalculate Available Checks Time: {}ms",
                GetPerformanceTimer(PT_RECALCULATE_AVAILABLE_CHECKS).count());
}

void RecalculateAvailableChecks(RandomizerRegion startingRegion /* = RR_ROOT */,
                                RandoAgeTime startingAgeTime /* = RAT_NONE */) {
    recalculateAvailable = true;
    availableChecksStartingRegion = startingRegion;
    availableChecksStartingAgeTime = startingAgeTime;
}

void LoadFromPreset(const nlohmann::json& info) {
    presetLoaded = true;
    presetPos = { info.at("pos").at("x"), info.at("pos").at("y") };
    presetSize = { info.at("size").at("width"), info.at("size").at("height") };
}

void CheckTrackerWindow::Draw() {
    if (!IsVisible()) {
        return;
    }
    DrawElement();
    // Sync up the IsVisible flag if it was changed by ImGui
    SyncVisibilityConsoleVariable();
}

// Marks where each probe landed, so "is the pattern actually on the model" is a
// thing you can see rather than infer from depth numbers. Green passed, red did
// not. Widescreen shows more than the native 320 field, so the horizontal span
// has to come from the active edges rather than 0..SCREEN_WIDTH.
static void SeenDrawProbeMarkers() {
    // Driven from UpdateElement, which runs every frame whether or not the tracker
    // window is open, so the markers stay visible while playing.
    if (!CVarGetInteger(CVAR_TRACKER_CHECK("ProbeOverlay"), 0) || seenProbeDebug.empty()) {
        return;
    }
    float left = OTRGetDimensionFromLeftEdge(0.0f);
    float right = OTRGetDimensionFromRightEdge(SCREEN_WIDTH);
    if (!(right > left)) {
        return;
    }
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImDrawList* draw = ImGui::GetForegroundDrawList();
    for (const auto& entry : seenProbeDebug) {
        if (!entry.wasSampled) {
            continue;
        }
        for (int i = 0; i < entry.pointCount; i++) {
            const auto& point = entry.points[i];
            float u = (point.x - left) / (right - left);
            // Actor_ProjectPos gives +y upward, so probe y counts up from the
            // bottom while pixel rows count down from the top.
            float v = (static_cast<float>(SCREEN_HEIGHT) - point.y) / static_cast<float>(SCREEN_HEIGHT);
            // WorkPos/WorkSize, not Pos/Size: the game renders below the menubar,
            // and using the full window puts every marker a menubar too high.
            ImVec2 at = { viewport->WorkPos.x + u * viewport->WorkSize.x,
                          viewport->WorkPos.y + v * viewport->WorkSize.y };
            draw->AddCircleFilled(at, 4.0f, point.matched ? IM_COL32(0, 255, 0, 255) : IM_COL32(255, 48, 48, 255));
            draw->AddCircle(at, 5.0f, IM_COL32(0, 0, 0, 200));
        }
    }
}

// Everything the sight-disclosure decision was made from, for calibrating the
// probe geometry and tolerances against real scenes instead of screenshots.
static void DrawSeenProbeOverlay() {
    if (!ImGui::CollapsingHeader("Sight Disclosure Probes (debug)")) {
        return;
    }
    UIWidgets::CVarCheckbox("Capture probe diagnostics", CVAR_TRACKER_CHECK("ProbeOverlay"),
                            UIWidgets::CheckboxOptions()
                                .Color(THEME_COLOR)
                                .Tooltip("Records what each depth probe sampled. Costs nothing while closed."));
    if (!CVarGetInteger(CVAR_TRACKER_CHECK("ProbeOverlay"), 0)) {
        return;
    }
    UIWidgets::CVarCheckbox("Re-test already-marked checks", CVAR_TRACKER_CHECK("ProbeRetestMarked"),
                            UIWidgets::CheckboxOptions()
                                .Color(THEME_COLOR)
                                .Tooltip("Keeps probing checks that have already been marked, so geometry can be "
                                         "re-tested from other angles. Cannot re-mark anything."));
    UIWidgets::CVarSliderInt("Coordinate budget", CVAR_TRACKER_CHECK("ProbeBudget"),
                             UIWidgets::IntSliderOptions()
                                 .Color(THEME_COLOR)
                                 .Min(kSeenProbesPerCandidate)
                                 .Max(kSeenMaxProbeCoordinates)
                                 .DefaultValue(kSeenMaxProbeCoordinates)
                                 .ShowButtons(true)
                                 .Tooltip("Lowering this forces candidates to be deferred without needing ten on "
                                          "screen, which is how the scheduling path gets exercised."));
    if (ImGui::Button("Reset peak counters")) {
        seenProbePeakBanked.clear();
    }
    ImGui::Text("Gameplay frame %u | candidates %d | coordinates %d / %d", seenProbeDebugFrame,
                seenProbeCandidatesLastFrame, seenProbeCoordinatesLastFrame, SeenProbeBudget());
    ImGui::Text("rejected (no useful projection) %d | deferred (no budget) %d | matched %d", seenProbeRejectedLastFrame,
                seenProbeDeferredLastFrame, seenProbeMatchesLastFrame);
    ImGui::Text("frames required %d | chest slop %.1f px (min %d) | max distance %.0f | item tolerance %d",
                kSeenFramesRequired, kSeenProbeSlopPixels, kSeenProbeMinTolerance, kSeenMaxProbeDistance,
                kSeenItemDepthTolerance);
    ImGui::Separator();
    for (const auto& geom : seenChestGeometryDebug) {
        const char* name = Rando::StaticData::GetLocation(geom.rc)->GetShortName().c_str();
        if (geom.colReason != nullptr) {
            ImGui::Text("%s geometry: bgId %d rejected - %s", name, geom.bgId, geom.colReason);
            continue;
        }
        ImGui::Text("%s geometry: bgId %d", name, geom.bgId);
        ImGui::Indent();
        ImGui::Text("bounds min (%5d,%5d,%5d) max (%5d,%5d,%5d)", geom.minBounds.x, geom.minBounds.y, geom.minBounds.z,
                    geom.maxBounds.x, geom.maxBounds.y, geom.maxBounds.z);
        ImGui::Text("centre (%7.1f,%7.1f,%7.1f) half (%7.1f,%7.1f,%7.1f) model", geom.centre.x, geom.centre.y,
                    geom.centre.z, geom.halfExtent.x, geom.halfExtent.y, geom.halfExtent.z);
        ImGui::Text("half world (%6.2f,%6.2f,%6.2f)", geom.halfExtent.x * geom.root.scale.x,
                    geom.halfExtent.y * geom.root.scale.y, geom.halfExtent.z * geom.root.scale.z);
        // A non-zero lag is the moving-chest case the root transform avoids.
        ImGui::Text("root pos (%8.2f,%8.2f,%8.2f) lag vs dyna (%7.2f,%7.2f,%7.2f)", geom.root.pos.x, geom.root.pos.y,
                    geom.root.pos.z, geom.root.pos.x - geom.dyna.pos.x, geom.root.pos.y - geom.dyna.pos.y,
                    geom.root.pos.z - geom.dyna.pos.z);
        ImGui::Text("root rot (%6d,%6d,%6d) dyna rot (%6d,%6d,%6d)", geom.root.rot.x, geom.root.rot.y, geom.root.rot.z,
                    geom.dyna.rot.x, geom.dyna.rot.y, geom.dyna.rot.z);
        ImGui::Text("dist %.0f euclid, %.0f axial, slop +/-%.1f world units", geom.distance, geom.axial,
                    SeenProbeSlopWorld(geom.axial));
        if (geom.probeReason != nullptr) {
            ImGui::Text("REJECTED: %s - %d/%d probes on screen, span %.1f x %.1f (need %.1f)", geom.probeReason,
                        geom.projected, kSeenProbesPerCandidate, geom.spanX, geom.spanY, kSeenMinScreenRadius);
        }
        ImGui::Unindent();
    }
    if (!seenChestGeometryDebug.empty()) {
        ImGui::Separator();
    }
    if (seenProbeDebug.empty()) {
        ImGui::TextUnformatted("No candidates registered by a draw this frame.");
        return;
    }
    for (const auto& entry : seenProbeDebug) {
        const char* name = Rando::StaticData::GetLocation(entry.rc)->GetShortName().c_str();
        if (!entry.wasSampled) {
            ImGui::Text("%s [%s] deferred - no coordinate budget", name, entry.isChest ? "chest" : "item");
            continue;
        }
        auto peak = seenProbePeakBanked.find(entry.rc);
        ImGui::Text("%s [%s] %d/%d matched (need %d), banked %d/%d (peak %d)", name, entry.isChest ? "chest" : "item",
                    entry.matches, entry.pointCount, kSeenProbeMatchesRequired, entry.framesBanked, kSeenFramesRequired,
                    peak == seenProbePeakBanked.end() ? 0 : peak->second);
        ImGui::Indent();
        // If "expected implies" disagrees with the measured distance, the probe and
        // the geometry it was built from are not the same point.
        ImGui::Text("dist %.0f euclid, expected implies %.0f axial, window +/-%.0f world units", entry.distance,
                    entry.pointCount > 0 ? SeenDepthToAxial(entry.points[0].expectedZ) : 0.0f,
                    SeenDepthWindowWorld(entry.distance, entry.tolerance));
        for (int i = 0; i < entry.pointCount; i++) {
            const auto& point = entry.points[i];
            // For a chest the accepted band is [near, expected]; sampled should sit
            // inside it. below/above says which end it fell outside.
            const char* where = point.sampledZ < point.nearZ ? "below" : "above";
            ImGui::Text("(%6.1f,%6.1f) near %6d expected %6d sampled %6d %s", point.x, point.y, point.nearZ,
                        point.expectedZ, point.sampledZ, point.matched ? "PASS" : where);
        }
        ImGui::Unindent();
    }
}

void CheckTrackerSettingsWindow::DrawElement() {
    ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, { 8.0f, 8.0f });
    if (ImGui::BeginTable("CheckTrackerSettingsTable", 2, ImGuiTableFlags_BordersH | ImGuiTableFlags_BordersV)) {
        ImGui::TableSetupColumn("General settings", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::TableSetupColumn("Section settings", ImGuiTableColumnFlags_WidthStretch, 200.0f);
        ImGui::TableHeadersRow();
        ImGui::TableNextRow();
        ImGui::TableNextColumn();
        SohGui::GetSohMenu()->MenuDrawItem(backgroundColorWidget,
                                           static_cast<uint32_t>(ImGui::GetContentRegionAvail().x), THEME_COLOR);

        SohGui::GetSohMenu()->MenuDrawItem(windowTypeWidget, static_cast<uint32_t>(ImGui::GetContentRegionAvail().x),
                                           THEME_COLOR);

        UIWidgets::CVarSliderFloat("Font Size", CVAR_TRACKER_CHECK("FontSize"),
                                   UIWidgets::FloatSliderOptions()
                                       .Tooltip("Sets the font size used in the check tracker.")
                                       .Format("%.1f")
                                       .Step(0.1f)
                                       .Min(0.3f)
                                       .Max(2.0f)
                                       .Color(THEME_COLOR)
                                       .DefaultValue(1.0f));

        if (CVarGetInteger(CVAR_TRACKER_CHECK("WindowType"), TRACKER_WINDOW_WINDOW) == TRACKER_WINDOW_FLOATING) {
            UIWidgets::CVarCheckbox("Enable Dragging", CVAR_TRACKER_CHECK("Draggable"),
                                    UIWidgets::CheckboxOptions().Color(THEME_COLOR));
            UIWidgets::CVarCheckbox("Only Enable While Paused", CVAR_TRACKER_CHECK("ShowOnlyPaused"),
                                    UIWidgets::CheckboxOptions().Color(THEME_COLOR));
            UIWidgets::CVarCombobox("Display Mode", CVAR_TRACKER_CHECK("DisplayType"), showMode,
                                    UIWidgets::ComboboxOptions()
                                        .LabelPosition(UIWidgets::LabelPositions::Far)
                                        .ComponentAlignment(UIWidgets::ComponentAlignments::Right)
                                        .Color(THEME_COLOR)
                                        .DefaultIndex(0));
            if (CVarGetInteger(CVAR_TRACKER_CHECK("DisplayType"), TRACKER_DISPLAY_ALWAYS) ==
                TRACKER_DISPLAY_COMBO_BUTTON) {
                UIWidgets::CVarCombobox("Combo Button 1", CVAR_TRACKER_CHECK("ComboButton1"), buttonStrings,
                                        UIWidgets::ComboboxOptions()
                                            .LabelPosition(UIWidgets::LabelPositions::Far)
                                            .ComponentAlignment(UIWidgets::ComponentAlignments::Right)
                                            .Color(THEME_COLOR)
                                            .DefaultIndex(TRACKER_COMBO_BUTTON_L));
                UIWidgets::CVarCombobox("Combo Button 2", CVAR_TRACKER_CHECK("ComboButton2"), buttonStrings,
                                        UIWidgets::ComboboxOptions()
                                            .LabelPosition(UIWidgets::LabelPositions::Far)
                                            .ComponentAlignment(UIWidgets::ComponentAlignments::Right)
                                            .Color(THEME_COLOR)
                                            .DefaultIndex(TRACKER_COMBO_BUTTON_L));
            }
        }
        DrawSeenProbeOverlay();
        ImGui::BeginDisabled(CVarGetInteger(CVAR_SETTING("DisableChanges"), 0));
        SohGui::GetSohMenu()->MenuDrawItem(dungeonSpoilerWidget,
                                           static_cast<uint32_t>(ImGui::GetContentRegionAvail().x), THEME_COLOR);
        ImGui::EndDisabled();

        SohGui::GetSohMenu()->MenuDrawItem(hideUnshuffledShopWidget,
                                           static_cast<uint32_t>(ImGui::GetContentRegionAvail().x), THEME_COLOR);

        SohGui::GetSohMenu()->MenuDrawItem(showGSWidget, static_cast<uint32_t>(ImGui::GetContentRegionAvail().x),
                                           THEME_COLOR);

        SohGui::GetSohMenu()->MenuDrawItem(showLogicWidget, static_cast<uint32_t>(ImGui::GetContentRegionAvail().x),
                                           THEME_COLOR);

        ImGui::BeginDisabled(CVarGetInteger(CVAR_SETTING("DisableChanges"), 0));
        SohGui::GetSohMenu()->MenuDrawItem(checkAvailabilityWidget,
                                           static_cast<uint32_t>(ImGui::GetContentRegionAvail().x), THEME_COLOR);
        ImGui::EndDisabled();

        // Filtering settings
        UIWidgets::CVarCheckbox(
            "Filter Empty Areas", CVAR_TRACKER_CHECK("HideFilteredAreas"),
            UIWidgets::CheckboxOptions()
                .Tooltip("If enabled, will hide area headers that have no locations matching filter")
                .Color(THEME_COLOR)
                .DefaultValue(true));

        ImGui::SeparatorText("Tracker Header Visibility");
        UIWidgets::CVarCheckbox("Hidden Items Toggle", CVAR_TRACKER_CHECK("HiddenItemsToggleVisible"),
                                UIWidgets::CheckboxOptions().Color(THEME_COLOR).DefaultValue(true));
        UIWidgets::CVarCheckbox("Available Checks Toggle", CVAR_TRACKER_CHECK("AvailableChecksToggleVisible"),
                                UIWidgets::CheckboxOptions().Color(THEME_COLOR).DefaultValue(true));
        UIWidgets::CVarCheckbox("Expand/Collapse Buttons", CVAR_TRACKER_CHECK("ExpandCollapseButtonsVisible"),
                                UIWidgets::CheckboxOptions().Color(THEME_COLOR).DefaultValue(false));
        UIWidgets::CVarCheckbox("Search Input", CVAR_TRACKER_CHECK("SearchInputVisible"),
                                UIWidgets::CheckboxOptions().Color(THEME_COLOR).DefaultValue(true));
        UIWidgets::CVarCheckbox("Check Totals", CVAR_TRACKER_CHECK("CheckTotalsVisible"),
                                UIWidgets::CheckboxOptions().Color(THEME_COLOR).DefaultValue(true));

        ImGui::TableNextColumn();

        CheckTracker::ImGuiDrawTwoColorPickerSection("Area Incomplete", CVAR_TRACKER_CHECK("AreaIncomplete.MainColor"),
                                                     CVAR_TRACKER_CHECK("AreaIncomplete.ExtraColor"),
                                                     Color_Area_Incomplete_Main, Color_Area_Incomplete_Extra,
                                                     Color_Main_Default, Color_Area_Incomplete_Extra_Default,
                                                     CVAR_TRACKER_CHECK("AreaIncomplete.Hide"), "", THEME_COLOR);
        CheckTracker::ImGuiDrawTwoColorPickerSection("Area Complete", CVAR_TRACKER_CHECK("AreaComplete.MainColor"),
                                                     CVAR_TRACKER_CHECK("AreaComplete.ExtraColor"),
                                                     Color_Area_Complete_Main, Color_Area_Complete_Extra,
                                                     Color_Main_Default, Color_Area_Complete_Extra_Default,
                                                     CVAR_TRACKER_CHECK("AreaComplete.Hide"), "", THEME_COLOR);
        CheckTracker::ImGuiDrawTwoColorPickerSection(
            "Unchecked", CVAR_TRACKER_CHECK("Unchecked.MainColor"), CVAR_TRACKER_CHECK("Unchecked.ExtraColor"),
            Color_Unchecked_Main, Color_Unchecked_Extra, Color_Main_Default, Color_Unchecked_Extra_Default,
            CVAR_TRACKER_CHECK("Unchecked.Hide"), "Checks you have not interacted with at all.", THEME_COLOR);
        CheckTracker::ImGuiDrawTwoColorPickerSection(
            "Skipped", CVAR_TRACKER_CHECK("Skipped.MainColor"), CVAR_TRACKER_CHECK("Skipped.ExtraColor"),
            Color_Skipped_Main, Color_Skipped_Extra, Color_Main_Default, Color_Skipped_Extra_Default,
            CVAR_TRACKER_CHECK("Skipped.Hide"), "", THEME_COLOR);
        CheckTracker::ImGuiDrawTwoColorPickerSection(
            "Seen", CVAR_TRACKER_CHECK("Seen.MainColor"), CVAR_TRACKER_CHECK("Seen.ExtraColor"), Color_Seen_Main,
            Color_Seen_Extra, Color_Main_Default, Color_Seen_Extra_Default, CVAR_TRACKER_CHECK("Seen.Hide"),
            "Used for shops. Shows item names for shop slots when walking in, and prices when highlighting them in buy "
            "mode.",
            THEME_COLOR);
        CheckTracker::ImGuiDrawTwoColorPickerSection(
            "Scummed", CVAR_TRACKER_CHECK("Scummed.MainColor"), CVAR_TRACKER_CHECK("Scummed.ExtraColor"),
            Color_Scummed_Main, Color_Scummed_Extra, Color_Main_Default, Color_Scummed_Extra_Default,
            CVAR_TRACKER_CHECK("Scummed.Hide"),
            "Checks you collect, but then reload before saving so you no longer have them.", THEME_COLOR);
        CheckTracker::ImGuiDrawTwoColorPickerSection(
            "Category", CVAR_TRACKER_CHECK("Hinted.MainColor"), CVAR_TRACKER_CHECK("Hinted.ExtraColor"),
            Color_Hinted_Main, Color_Hinted_Extra, Color_Main_Default, Color_Hinted_Extra_Default,
            CVAR_TRACKER_CHECK("Hinted.Hide"),
            "Chests whose item category you have seen via Chest Size And Texture Match Contents.", THEME_COLOR);
        CheckTracker::ImGuiDrawTwoColorPickerSection(
            "Collected", CVAR_TRACKER_CHECK("Collected.MainColor"), CVAR_TRACKER_CHECK("Collected.ExtraColor"),
            Color_Collected_Main, Color_Collected_Extra, Color_Main_Default, Color_Collected_Extra_Default,
            CVAR_TRACKER_CHECK("Collected.Hide"), "Checks you have collected without saving or reloading yet.",
            THEME_COLOR);
        CheckTracker::ImGuiDrawTwoColorPickerSection(
            "Saved", CVAR_TRACKER_CHECK("Saved.MainColor"), CVAR_TRACKER_CHECK("Saved.ExtraColor"), Color_Saved_Main,
            Color_Saved_Extra, Color_Main_Default, Color_Saved_Extra_Default, CVAR_TRACKER_CHECK("Saved.Hide"),
            "Checks that you saved the game while having collected.", THEME_COLOR);

        ImGui::PopStyleVar(1);
        ImGui::EndTable();
    }
}

void CheckTrackerWindow::InitElement() {
    SaveManager::Instance->AddInitFunction(InitTrackerData);
    sectionId = SaveManager::Instance->AddSaveFunction("trackerData", 1, SaveFile, true, SECTION_PARENT_NONE);
    SaveManager::Instance->AddLoadFunction("trackerData", 1, LoadFile);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnLoadGame>(CheckTrackerLoadGame);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnExitGame>([](uint32_t fileNum) { Teardown(); });
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnItemReceive>(CheckTrackerItemReceive);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnTransitionEnd>(CheckTrackerTransition);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnShopSlotChange>(CheckTrackerShopSlotChange);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneFlagSet>(CheckTrackerSceneFlagSet);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnFlagSet>(CheckTrackerFlagSet);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnDialogMessage>(CheckTrackerDialogMessage);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnRandoHintRevealed>(CheckTrackerHintRevealed);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPlayDrawBegin>(CheckTrackerSeenItemsFrame);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGraphCallbackPrepare>(CheckTrackerDepthProbePrepare);
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnGraphCallbackConsume>(CheckTrackerDepthProbeConsume);
}

void CheckTrackerWindow::UpdateElement() {
    // Runs every frame regardless of the window's visibility, which is what keeps
    // the probe markers on screen while the menu is closed.
    SeenDrawProbeMarkers();
}

void RegisterCheckTrackerWidgets() {
    backgroundColorWidget = { .name = "Background Color##CheckTracker", .type = WidgetType::WIDGET_CVAR_COLOR_PICKER };
    backgroundColorWidget.CVar(CVAR_TRACKER_CHECK("BgColor"))
        .Options(
            ColorPickerOptions().Color(THEME_COLOR).DefaultValue(Color_Bg_Default).UseAlpha().ShowReset().ShowRandom());
    SohGui::GetSohMenu()->AddSearchWidget({ backgroundColorWidget, "Randomizer", "Check Tracker", "General Settings" });

    windowTypeWidget = { .name = "Window Type##CheckTracker", .type = WidgetType::WIDGET_CVAR_COMBOBOX };
    windowTypeWidget.CVar(CVAR_TRACKER_CHECK("WindowType"))
        .Options(ComboboxOptions()
                     .DefaultIndex(TRACKER_WINDOW_WINDOW)
                     .ComponentAlignment(ComponentAlignments::Right)
                     .LabelPosition(LabelPositions::Far)
                     .Color(THEME_COLOR)
                     .ComboMap(windowType));
    SohGui::GetSohMenu()->AddSearchWidget({ windowTypeWidget, "Randomizer", "Check Tracker", "General Settings" });

    dungeonSpoilerWidget = { .name = "Vanilla/MQ Dungeon Spoilers", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    dungeonSpoilerWidget.CVar(CVAR_TRACKER_CHECK("MQSpoilers"))
        .Options(CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("If enabled, Vanilla/MQ dungeons will show on the tracker immediately. "
                              "Otherwise, Vanilla/MQ dungeon locations must be unlocked."));
    SohGui::GetSohMenu()->AddSearchWidget({ dungeonSpoilerWidget, "Randomizer", "Check Tracker", "General Settings" });

    hideUnshuffledShopWidget = { .name = "Hide Unshuffled Shop Item Checks", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    hideUnshuffledShopWidget.CVar(CVAR_TRACKER_CHECK("HideUnshuffledShopChecks"))
        .Options(
            CheckboxOptions()
                .Color(THEME_COLOR)
                .Tooltip("If enabled, will prevent the tracker from displaying slots with non-shop-item shuffles."))
        .Callback([&](WidgetInfo& info) {
            hideShopUnshuffledChecks = CVarGetInteger(CVAR_TRACKER_CHECK("HideUnshuffledShopChecks"), 0);
            UpdateFilters();
        });
    SohGui::GetSohMenu()->AddSearchWidget(
        { hideUnshuffledShopWidget, "Randomizer", "Check Tracker", "General Settings" });

    showGSWidget = { .name = "Always Show Gold Skulltulas", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    showGSWidget.CVar(CVAR_TRACKER_CHECK("AlwaysShowGSLocs"))
        .Options(CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("If enabled, will show GS locations in the tracker regardless of tokensanity settings."))
        .Callback([&](WidgetInfo& info) {
            alwaysShowGS = !alwaysShowGS;
            UpdateFilters();
        });
    SohGui::GetSohMenu()->AddSearchWidget({ showGSWidget, "Randomizer", "Check Tracker", "General Settings" });

    showLogicWidget = { .name = "Show Logic", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    showLogicWidget.CVar(CVAR_TRACKER_CHECK("ShowLogic"))
        .Options(CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("If enabled, will show a check's logic when hovering over it."));
    SohGui::GetSohMenu()->AddSearchWidget({ showLogicWidget, "Randomizer", "Check Tracker", "General Settings" });

    checkAvailabilityWidget = { .name = "Enable Available Checks", .type = WidgetType::WIDGET_CVAR_CHECKBOX };
    checkAvailabilityWidget.CVar(CVAR_TRACKER_CHECK("EnableAvailableChecks"))
        .Options(CheckboxOptions()
                     .Color(THEME_COLOR)
                     .Tooltip("If enabled, will show the checks that are available to be collected "
                              "with your current progress."))
        .Callback([&](WidgetInfo& info) {
            enableAvailableChecks = CVarGetInteger(CVAR_TRACKER_CHECK("EnableAvailableChecks"), 0);
            RecalculateAvailableChecks();
        });
}

static RegisterMenuInitFunc menuInitFunc(RegisterCheckTrackerWidgets);
} // namespace CheckTracker
