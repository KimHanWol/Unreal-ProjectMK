# 소모성 아이템 기획

## 1. 방향

소모품은 광물을 **판매(코인)** 대신 **사용**으로 바꾸는 두 번째 출구다. "팔까, 만들까"라는 선택을 만든다.
게임의 핵심 긴장인 세 가지를 직접 건드리는 아이템을 우선한다.

| 긴장 | 소모품이 주는 해법 |
|---|---|
| 산소 (깊이 제한) | 더 오래, 더 깊이 머무르게 한다 |
| 붕괴 (위협·보상 손실) | 붕괴를 막거나, 원하는 방향으로 일으킨다 |
| 사망 페널티 (광물 손실) | 위험한 도전을 할 수 있게 보험을 준다 |

붕괴·발파로 부서진 블록은 드롭이 없다. 그래서 **붕괴 제어 소모품**이 이 게임에서 가장 가치 있는 축이다.

## 2. 확정 아이템 (구현됨)

수치는 초안이다. 데이터는 `ItemDataTable`, 레시피는 `ShopRecipeDataTable` 에 있다.

| 아이템 (행 이름) | 레시피 | 결과 | 해금가 | 효과 |
|---|---|---|---|---|
| 산소 캡슐 (`OxygenCapsule`) | Ore_Coal ×3, Ore_Copper ×1 | 1 | 20 | 최대 산소의 40% 회복. 산소가 가득하면 실패 |
| 응급 키트 (`RepairKit`) | Ore_Copper ×2, Ore_Iron ×1 | 1 | 30 | 체력 +50. 체력이 가득하면 실패 |
| 귀환 신호탄 (`RecallFlare`) | Ore_Iron ×2, Ore_Gold ×1 | 1 | 60 | 3초 동안 움직이지 않으면 스폰 지점으로 귀환 (광물 유지). 대기 중 움직이면 취소되고 소모하지 않는다 |
| 버팀목 (`SupportBeam`) | Ore_Iron ×3 | 2 | 40 | **캐릭터가 서 있는 칸**에 버팀목 블록 설치. 캐릭터는 통과한다 (`bIsPassableByCharacter`). Rock 처럼 **지지 앵커**가 되어 붙은 덩어리가 떨어지지 않는다. 드릴로 캐면 버팀목 1개로 회수 |
| 완충 젤 (`CushionGel`) | Slime ×2, Ore_Coal ×1 | 1 | 40 | **캐릭터가 밟고 있는 블록**(서 있는 칸 바로 아래)에만 바른다. 발밑이 비었거나 채굴 불가 블록이면 사용 실패. 그 블록 위로 떨어진 블록은 충격 없이 멈춘다 |
| 발파 폭약 (`BlastCharge`) | Ore_Coal ×5, Ore_Copper ×2 | 1 | 50 | 서 있는 칸에 설치, 2초 뒤 반경 1칸(3×3) 블록 파괴. **Rock 도 부순다.** 터진 블록은 **광물이 나오지 않는다** (붕괴와 같음). 주변 블록은 채굴과 같게 낙하 판정을 받는다. 캐릭터는 대미지를 받지 않는다 |
| 보험 증서 (`Insurance`) | Ore_Diamond ×1 | 1 | 100 | 소지형. 사망 시 1개 소모하고 코인·광물 손실을 막는다. 숫자 키로 쓰지 않는다 |

- 효과 종류는 `EConsumableEffectType`, 수치는 행의 `EffectValue` / `EffectDuration` / `PlaceBlockKey` 로 정한다.
  - 산소 캡슐: `EffectValue` = 회복 비율
  - 응급 키트: `EffectValue` = 회복량
  - 귀환 신호탄: `EffectDuration` = 대기 시간
  - 버팀목: `PlaceBlockKey` = 설치할 `BlockDataTable` 행 (`SupportBeam`, TileIndex 900)
  - 발파 폭약: `EffectDuration` = 폭발 지연, `EffectValue` = 반경(칸)
- 소모품은 판매가 0 이라 상점 판매 목록에 나오지 않는다.
- Border, Shop 블록은 `bIsExplosionProof` 라 폭약으로도 부서지지 않는다.

### 제외 / 보류

| 아이템 | 상태 |
|---|---|
| 굳힘 수지 | 제외 |
| 구조 스캐너 | 제외 |
| 드릴 과열제 | 보류 |
| 자석 증폭기 | 보류 |

## 3. 사용 규칙

- **숫자 키 1~8** 이 인벤토리 슬롯 1~8 의 아이템을 사용한다 (`QuickSlot1`~`QuickSlot8` 액션 매핑).
- **대상 칸:** 설치형은 모두 캐릭터 기준으로 정한다. 버팀목·발파 폭약은 서 있는 칸, 완충 젤은 밟고 있는 칸. 바라보는 방향은 쓰지 않는다.
- **공중 제한:** 설치형(버팀목, 완충 젤, 발파 폭약)은 땅에 서 있을 때만 쓸 수 있다. **드릴도 비행·낙하 중에는 동작하지 않는다.**
- 사용할 수 없으면 `Item_UseFailed` 사운드를 내고 소모하지 않는다.
- 귀환 대기 중에는 다른 소모품을 쓸 수 없다. 사망하면 귀환이 취소된다.

## 4. 레시피와 제작

- 소모품은 상점에서 팔지 않는다. **상점에서 레시피를 사야 제작이 해금된다.**
- 상점: 광물·아이템 판매 + 레시피 구매. 해금가 0 인 레시피는 처음부터 해금된 것으로 보고 상점 목록에 나오지 않는다.
- 제작은 **별도 제작 UI** 에서 한다 (C 키로 토글).
- 해금한 레시피는 세이브 데이터 `UnlockedRecipeKeys` 에 저장되고, 레시피 구매·제작 시 자동 저장한다.

## 5. 코드 구조

| 클래스 | 역할 |
|---|---|
| `UConsumableComponent` (캐릭터) | `TryUseInventorySlot`, `TryUseItem`, `TryConsumeInsurance`, 귀환 타이머. 델리게이트 `OnConsumableUsedDelegate`, `OnRecallStateChangedDelegate` |
| `UCraftingComponent` (캐릭터) | 레시피 해금·제작. `TryUnlockRecipe`, `TryCraftRecipe`, `IsRecipeUnlocked`, `CanUnlockRecipe`, `CanCraftRecipe`, `GetUnlockedRecipeKeys`, `GetLockedRecipeKeys` (모두 블루프린트 호출 가능) |
| `ABlastChargeActor` | 설치된 폭약. 깜빡이다가 `ULevelManagerSubsystem::ApplyExplosion` 호출 |
| `ULevelManagerSubsystem` | `TryPlaceBlock` (통과 가능 블록은 폰 겹침 검사 생략), `ApplyExplosion`, `IsBlockCellEmpty`, `GetBlockAtWorldLocation`, `GetBlockBelowWorldLocation`. 지지 앵커 판정, 완충 블록 착지 처리 |
| `UShopMenuContents` | 판매 목록 + 레시피 구매 목록 (`UShopRecipeSlotWidget`). 기본 레이아웃을 코드로 만들고, WBP 로 덮어쓸 수 있다 (`RecipeSlotListPanel`) |
| `UCraftingMenuContents` | 제작 UI. 기본 목업 레이아웃을 제공하며 WBP 로 교체할 수 있다. 레시피 해금 상태, 재료 보유 / 필요 수량과 제작 결과를 표시한다 |

### 제작 UI (WBP) 연결 방법

기본 제작 화면은 `UMKMenuBase` 에 자동 등록되어 C 키로 열린다. 별도 레이아웃이 필요할 때 아래 방법으로 교체한다.

1. `UCraftingMenuContents` 를 부모로 WBP 를 만든다 (`MenuContentsType` 은 자동으로 `Crafting`).
2. 메뉴 WBP 의 `MenuContentsCanvasPanel` 안에 배치한다. C 키로 열린다.
3. 사용할 함수: `GetUnlockedRecipeKeys`, `CanCraftRecipe`, `TryCraftRecipe`.
4. `BP_OnCraftingRefreshed` 이벤트에서 목록을 다시 그린다 (메뉴가 열릴 때, 인벤토리·레시피가 바뀔 때 호출).

### 퀵슬롯 UI

숫자 키는 인벤토리 슬롯 순서를 그대로 쓴다. PIE 목업에서는 하단에 인벤토리 슬롯 0~7 을 표시하고 클릭으로도 사용할 수 있다. 테스트 패널과 세이브 분리 규칙은 [PlaytestMockup.md](PlaytestMockup.md) 를 참고한다.

## 6. 치트 (테스트용)

- `GiveItem <행 이름> [개수]`
- `UseQuickSlot <1~8>`: 해당 슬롯 사용 결과와 남은 개수를 출력
- `UnlockRecipe <레시피 행>`: 코인을 써서 해금
- `CraftRecipe <레시피 행>`

## 7. 남은 작업

- **Slime 획득처가 없다.** 완충 젤은 몬스터가 들어오기 전까지 제작할 수 없다.
- 아이콘은 임시 광석 스프라이트, 버팀목 블록 스프라이트는 임시 타일(`world_tileset_Sprite_39`)이다.
- 새 사운드 행(`Item_Use`, `Item_UseFailed`, `Blast_Explode`, `Recipe_Unlock`, `Recall_Start`, `Recall_Complete`)은 음원이 비어 있다.
- 완충 젤 표시는 블록 색조(`CushionTintColor`)뿐이다.
- 몬스터 추가 시: 폭약과 낙하 블록으로 몬스터를 잡을 수 있게 `ApplyExplosion` 과 낙하 충돌에 대상을 추가한다.
- 테스트용 레시피 `Recipe_1` (Ore_Test) 이 해금가 0 으로 남아 있어 제작 목록에 항상 보인다.
