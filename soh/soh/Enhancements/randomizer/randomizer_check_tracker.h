#pragma once

#include <nlohmann/json.hpp>
#include "randomizerTypes.h"
#include "soh/SohGui/UIWidgets.hpp"

namespace CheckTracker {

class CheckTrackerSettingsWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    ~CheckTrackerSettingsWindow(){};

  protected:
    void InitElement() override{};
    void DrawElement() override;
    void UpdateElement() override{};
};

class CheckTrackerWindow final : public Ship::GuiWindow {
  public:
    using GuiWindow::GuiWindow;
    void Draw() override;
    ~CheckTrackerWindow(){};

  protected:
    void InitElement() override;
    void DrawElement() override;
    void UpdateElement() override;
};

// Converts an index into a Little Endian bitmask, as follows:
// 00: 0000000100000000
// 01: 0000001000000000
//...
// 06: 0100000000000000
// 07: 1000000000000000
// 08: 0000000000000001
// 09: 0000000000000010
//...
// 14: 0000000001000000
// 15: 0000000010000000
// repeat...
#define INDEX_TO_16BIT_LITTLE_ENDIAN_BITMASK(idx) (0x8000 >> (7 - (idx % 8) + ((idx % 16) / 8) * 8))

void Teardown();
void UpdateAllOrdering();
bool IsVisibleInCheckTracker(RandomizerCheck rc);
bool IsCheckShuffled(RandomizerCheck rc);
void InitTrackerData(bool isDebug);
RandomizerCheckArea GetCheckArea();
uint16_t GetTotalChecks();
uint16_t GetTotalChecksGotten();
bool IsAreaSpoiled(RandomizerCheckArea rcArea);
void SetAreaSpoiled(RandomizerCheckArea rcArea);
void UpdateInventoryChecks();
void UpdateAreas(RandomizerCheckArea area);
void UpdateAllOrdering();
void UpdateAllAreas();
void RecalculateAllAreaTotals();
void SpoilAreaFromCheck(RandomizerCheck rc);
void RecalculateAvailableChecks(RandomizerRegion startingRegion = RR_ROOT, RandoAgeTime startingAgeTime = RAT_NONE);
void LoadFromPreset(const nlohmann::json& info);
} // namespace CheckTracker

// Called from the draw paths that actually display a check's identity, while the
// model matrix that drew it is still current. Registering here rather than by
// scanning actor lists is what ties the evidence to a real draw submission.
#ifdef __cplusplus
extern "C" {
#endif
struct EnBox;
// limbIndex selects the chest's category-bearing body/lid surfaces.
void CheckTracker_RegisterChestProbes(struct EnBox* box, int32_t limbIndex);
void CheckTracker_RegisterItemProbes(Actor* actor, struct GetItemEntry entry);
#ifdef __cplusplus
}
#endif
