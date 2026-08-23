#pragma once

#include "soh/Enhancements/custom-message/text.h"
#include "soh/Enhancements/item-tables/ItemTableTypes.h"
#include "static_data.h"
#include "option.h"

namespace Rando {
class ItemLocation {
  public:
    ItemLocation();
    explicit ItemLocation(RandomizerCheck rc_);
    RandomizerCheck GetRandomizerCheck() const;
    bool IsAddedToPool() const;
    void AddToPool();
    void RemoveFromPool();
    const Item& GetPlacedItem() const;
    const Text& GetPlacedItemName() const;
    RandomizerGet GetPlacedRandomizerGet() const;
    void SetPlacedItem(RandomizerGet item);
    RandomizerGet& RefPlacedItem();
    void SetDelayedItem(RandomizerGet item);
    RandomizerRegion GetParentRegionKey() const;
    void SetParentRegion(RandomizerRegion region);
    std::set<RandomizerArea> GetAreas() const;
    RandomizerArea GetFirstArea() const;
    RandomizerArea GetRandomArea() const;
    void MergeAreas(std::set<RandomizerArea> newAreas);
    void PlaceVanillaItem();
    void ApplyPlacedItemEffect() const;
    void SaveDelayedItem();
    uint16_t GetPrice() const;
    void SetPrice(uint16_t price_);
    bool HasCustomPrice() const;
    void SetCustomPrice(uint16_t price_);
    bool CanBePurchased() const;
    bool HasObtained() const;
    void SetCheckStatus(RandomizerCheckStatus status_);
    RandomizerCheckStatus GetCheckStatus();
    void SetSeenCategory(GetItemCategory seenCategory_);
    int16_t GetSeenCategory() const;
    void SetIsSkipped(bool isSkipped_);
    bool GetIsSkipped();
    bool IsHintable() const;
    void SetAsHintable();
    bool IsAHintAccessible() const;
    void SetHintAccesible();
    const std::vector<RandomizerHint>& GetHintedBy() const;
    void AddHintedBy(RandomizerHint hintKey);
    bool IsHidden() const;
    bool IsExcluded();
    OptionValue& GetExcludedOption();
    void SetExcludedOption(uint8_t val);
    void SetHidden(bool hidden_);
    bool IsVisible() const;
    void SetVisible(bool visibleInImGui_);
    bool IsWothCandidate() const;
    void SetWothCandidate();
    bool IsFoolishCandidate() const;
    void SetBarrenCandidate();
    void ResetVariables();
    bool IsAvailable() const;
    void SetAvailable(bool isAvailable_);

  private:
    RandomizerCheck rc;
    std::vector<RandomizerHint> hintedBy = {};
    bool hintAccesible = false;
    bool isHintable = false;
    bool addedToPool = false;
    RandomizerGet placedItem = RG_NONE;
    RandomizerGet delayedItem = RG_NONE;
    OptionValue excludedOption = OptionValue(RO_LOCATION_INCLUDE);
    uint16_t price = 0;
    RandomizerRegion parentRegion = RR_NONE;
    std::set<RandomizerArea> areas = {};
    bool hasCustomPrice = false;
    bool hidden = false;
    bool visibleInImGui = false;
    bool wothCandidate = false;
    bool barrenCandidate = false;
    RandomizerCheckStatus status = RCSHOW_UNCHECKED;
    // The category a CSMC chest was drawing when the player saw it, or -1 for
    // none. Qualifies RCSHOW_CATEGORY: recomputing it later would track the
    // chest's live downgrades and change what a check reports unlooked-at.
    int16_t seenCategory = -1;
    bool isSkipped = false;
    bool isAvailable = false;
};
} // namespace Rando