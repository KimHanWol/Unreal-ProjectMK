# 상점 기획 (판매 · 레시피 구매)

## 1. 목적

현재 루프는 `채굴 → 광물 수집` 에서 끊겨 있다. 광물 `SellPrice` 가 전부 0 이고 판매 경로가 없어, 코인은 Recycler 스킬로만 들어온다.
상점은 `채굴 → 지상 복귀 → 판매 → 코인 → 스킬/제작 → 더 깊이` 로 루프를 닫는 지점이다.

- 지상 복귀의 이유를 만든다 (산소 회복 + 판매 + 자동 저장).
- 깊이에 따른 광물 가치 차이로 "더 내려갈지 / 돌아갈지" 판단을 만든다.
- 사망 페널티(광물 전부 손실)가 의미를 갖게 한다. 팔기 전 광물은 위험 자산이다.

## 2. 상점 위치와 진입

- 지표면에 배치된 `Shop` 블록(BlockData `Shop`, TileIndex 56)이 상점이다.
- 진입 방식: 캐릭터가 Shop 블록을 향해 드릴 방향을 두고 상호작용하면 상점 메뉴가 열린다.
  - 기존 `IInteractable` 은 GAS 이벤트 태그 기반이고 `GA_Drill` 에 인터랙션이 이식되어 있으므로 같은 경로를 사용한다.
  - `IInteractable::CanInteract` 는 수직 속도 0 (지면에 서 있음) 을 요구한다. 비행 중에는 열리지 않는다.
- 상점이 열려 있는 동안 캐릭터 입력(이동, 드릴)은 막는다. 스킬트리(K)와 동일한 메뉴 입력 모드를 사용한다.
- 상점에서 멀어지거나 ESC 로 닫는다.

## 3. 메뉴 구성

`EMenuContentsType` 에 `Shop` 을 추가하고, 기존 `UMKMenuBase` / `UMKMenuContentsBase` 구조를 그대로 쓴다.

| 탭 | 내용 |
|---|---|
| 판매 | 판매가가 있는 광물·아이템 목록, 개당 가격, 보유 수량, 합계. `개별 판매` / `전체 판매` |
| 레시피 구매 | 아직 해금하지 않은 레시피 목록. 해금가, 재료, 구매 버튼 (확정) |
| 제작 | 상점에 넣지 않는다. 별도 제작 UI(C) 에서 한다 (확정, `Consumables.md` 참고) |
| 스킬 | 상점에 넣지 않는다. 스킬트리는 별도 메뉴(K)로 유지한다 (확정) |

슬롯 UI 는 기존 `UItemSlotWidget` / `UMKSlotBase` / `UMKTooltipBase` 를 재사용한다.

## 4. 판매 규칙

- 판매 가능 아이템: `ItemDataTable` 의 `bIsOre == true` 인 아이템.
- 판매 가격: `ItemDataTable.SellPrice` × 수량.
- 판매 수익은 `USkillComponent::TryAddCoin(Price, false)` 로 지급한다. `CoinGainMultiplier` 는 채굴 중 코인 드롭량에만 적용하고 판매 대금에는 적용하지 않는다 (확정).
- 판매 후 인벤토리 변경 → 기존 `OnInventoryChangedDelegate` 로 UI 와 이동속도 페널티가 갱신된다.
- 판매 · 제작 직후 자동 저장한다 (세이브 시스템의 저장 시점에 `상점 거래` 추가).

### 가격표 초안

깊이 티어를 가정한 초안이다. 깊이별 지층 작업(추천 2번) 때 스폰 깊이와 함께 다시 조정한다.

| 아이템 | 티어 | SellPrice |
|---|---|---|
| Ore_Coal | 1 | 2 |
| Ore_Copper | 1 | 4 |
| Ore_Iron | 2 | 8 |
| Ore_Gold | 2 | 15 |
| Ore_Emerald | 3 | 25 |
| Ore_Diamond | 3 | 40 |
| Ore_Titanium | 4 | 60 |
| Ore_Lithium | 4 | 80 |
| Slime | - | 판매 불가 (제작 재료) |

기준: 현재 스킬 가격(`SkillDataTable.Price`)과 비교해 "지상 왕복 2~3회에 스킬 1개" 정도가 되도록 맞춘다. 스킬 가격은 에디터에서 확인 후 확정한다.

## 5. 레시피 구매 규칙

- 소모품 자체는 상점에서 팔지 않는다. 레시피(`ShopRecipeDataTable.UnlockPrice`)를 코인으로 사면 제작이 해금된다.
- 해금가 0 인 레시피는 처음부터 해금되어 있다.
- 레시피 목록과 수치는 `Consumables.md` 를 따른다.

## 6. 구현 구조 (제안)

1. 데이터
   - `ItemDataTable.SellPrice` 채우기
   - `ShopRecipeDataTable` 실제 레시피 추가
   - `EMenuContentsType::Shop` 추가
2. 상호작용
   - Shop 블록 액터가 `IInteractable` 을 구현하고, 상호작용 이벤트 태그로 상점 메뉴를 연다.
   - 블록은 `ABlockBase` 하나의 클래스로 스폰되므로, `ABlockBase` 에 인터페이스를 붙이고 BlockData 로 상점 여부를 판단하거나, LevelManager 가 Shop 타일에 전용 액터를 스폰하는 방식 중 선택한다 (결정 필요).
3. 판매 로직
   - `UInventoryComponent` 에 `TrySellItem(FName ItemUID, int32 ItemCount)` / `TrySellAllOreItems()` 추가
   - 코인 지급은 `USkillComponent::TryAddCoin` 경유
4. UI
   - `UShopMenuContents` (`UMKMenuContentsBase` 계열), 판매 탭 / 제작 탭
5. 사운드 (데이터 추가 완료)
   - `Shop_Open`, `Shop_Sell`, `Shop_Craft`, `UI_Click`, `UI_MenuOpen`, `UI_MenuClose`
6. 세이브
   - 판매 · 제작 완료 시 자동 저장

## 7. 결정 사항

| 항목 | 결정 |
|---|---|
| 판매 수익 코인 배율 | 적용하지 않음 |
| 스킬트리 위치 | 상점과 분리, K 로 연다 |
| 상호작용 | 전용 `AShopActor` + `IInteractable` 확장. 클릭 또는 E 키 |
| 가격 | 4장 초안 값을 ItemDataTable 에 반영, 이후 기획에서 조정 |
| 제작 | 상점은 레시피 판매만. 제작은 별도 제작 UI. 소모품은 판매하지 않음 |

## 8. 구현 현황

- 판매 탭: `UShopMenuContents` / `UShopSellSlotWidget`. 위젯 블루프린트 없이 C++ 기본 레이아웃으로 동작한다. 같은 이름의 위젯(`SellSlotListPanel`, `TotalPriceText`, `SellAllButton`, `ItemIconImage`, `ItemNameText`, `ItemPriceText`, `SellButton`)을 가진 WBP 로 교체할 수 있다.
- 메뉴 등록: `UMKMenuBase::RuntimeMenuContentsClasses` 기본값에 `UShopMenuContents` 가 들어 있어, `MenuContentsCanvasPanel` 에 Shop 컨텐츠가 없으면 런타임에 생성한다.
- 진입: Shop 타일 블록 위에 `AShopActor` 가 함께 생성된다. 클릭 또는 `Interact`(E) 키로 열리며, 거리 40 이내이고 지면에 서 있을 때만 열린다.
- 레시피 구매: `UShopMenuContents` 아래쪽 목록 / `UShopRecipeSlotWidget`. WBP 로 교체할 때 위젯 이름은 `RecipeSlotListPanel`, 슬롯은 `ResultItemIconImage`, `RecipeNameText`, `RequiredItemsText`, `UnlockPriceText`, `UnlockButton`.
- 테스트용 치트: `OpenShop` (거리 조건 무시).
