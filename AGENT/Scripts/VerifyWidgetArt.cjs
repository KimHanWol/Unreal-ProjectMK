const fs = require('fs');
const sharp = require('C:/Users/wssh0/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');

async function main() {
  const manifest = JSON.parse(fs.readFileSync('Content/Asset/WidgetArtV1/Source/manifest.json'));
  const hardAlphaFailures = [];
  for (const entry of manifest) {
    const data = await sharp(entry.file).ensureAlpha().raw().toBuffer();
    for (let index = 3; index < data.length; index += 4) {
      if (data[index] !== 0 && data[index] !== 255) {
        hardAlphaFailures.push(entry.name);
        break;
      }
    }
  }
  const source = fs.readFileSync('Source/ProjectMK/Actor/Block/BlockBase.cpp', 'utf8');
  const drillScaleApplied = source.includes('TEXT("DrillContact"), GetActorLocation(), 1.15f');
  const quickSlotSource = fs.readFileSync('Source/ProjectMK/UI/PlaytestQuickSlotWidget.cpp', 'utf8');
  const quickSlotNamesCollapsed = quickSlotSource.includes('ItemNameText->SetVisibility(ESlateVisibility::Collapsed)');
  const craftingSource = fs.readFileSync('Source/ProjectMK/UI/CraftingMenuContents.cpp', 'utf8');
  const twoColumnCrafting = craftingSource.includes('RecipeIndex / 2, RecipeIndex % 2');
  const commonWidgetHeader = fs.readFileSync('Source/ProjectMK/UI/MKUserWidget.h', 'utf8');
  const commonWidgetSource = fs.readFileSync('Source/ProjectMK/UI/MKUserWidget.cpp', 'utf8');
  const reusableLayoutSystem = [
    'enum class EMKTextRole',
    'struct FMKUILayout',
    'BuildSectionHeader',
    'BuildSectionLabel',
    'BuildItemIconFrame',
    'BuildActionButton',
  ].every(symbol => commonWidgetHeader.includes(symbol));
  const actionButtonsDoNotAutoWrap = commonWidgetSource.includes('ApplyTextRole(OutLabel, EMKTextRole::Button);');
  const uiSources = fs.readdirSync('Source/ProjectMK/UI')
    .filter(name => name.endsWith('.cpp'))
    .map(name => fs.readFileSync(`Source/ProjectMK/UI/${name}`, 'utf8'))
    .join('\n');
  const authoredFontSizes = [...uiSources.matchAll(/Font\.Size\s*=\s*(\d+)/g)].map(match => Number(match[1]));
  const minimumAuthoredFontSize = authoredFontSizes.length ? Math.min(...authoredFontSizes) : null;
  const shopRecipeSource = fs.readFileSync('Source/ProjectMK/UI/ShopRecipeSlotWidget.cpp', 'utf8');
  const shopSellSource = fs.readFileSync('Source/ProjectMK/UI/ShopSellSlotWidget.cpp', 'utf8');
  const skillTreeSource = fs.readFileSync('Source/ProjectMK/UI/SkillTreeMenuContents.cpp', 'utf8');
  const currencySource = fs.readFileSync('Source/ProjectMK/UI/CurrencyWidget.cpp', 'utf8');
  const simplifiedRecipePurchase = shopRecipeSource.includes('BuildCurrencyButton')
    && !shopRecipeSource.includes('RequiredItemsFormat');
  const inventoryBasedSelling = shopSellSource.includes('BuildCurrencyAmount')
    && shopSellSource.includes('묶음 판매');
  const dataDrivenSkillTree = skillTreeSource.includes('RequiredSkill')
    && skillTreeSource.includes('AddSkillConnection')
    && skillTreeSource.includes('CalculateSkillDepth');
  const framedGlobalCurrency = currencySource.includes('CurrencyPanel')
    && currencySource.includes('BuildCurrencyAmount');
  const visibleCoinLabels = [...uiSources.matchAll(/(?:TEXT|NSLOCTEXT)\([^\n]*코인/g)].length;
  const layoutV2 = JSON.parse(fs.readFileSync('output/widget-art/layout-v2-application.json', 'utf8'));
  const characterStatusGap = layoutV2.characterStatus.gap;
  const failures = [];
  if (manifest.length !== 16) failures.push(`Expected 16 assets, found ${manifest.length}`);
  if (hardAlphaFailures.length) failures.push(`Soft alpha: ${hardAlphaFailures.join(', ')}`);
  if (!drillScaleApplied) failures.push('Drill contact scale is not 1.15 blocks');
  if (!quickSlotNamesCollapsed) failures.push('Quick slot item names are not collapsed');
  if (!twoColumnCrafting) failures.push('Crafting recipes are not arranged in two columns');
  if (!reusableLayoutSystem) failures.push('Reusable UI layout system is incomplete');
  if (!actionButtonsDoNotAutoWrap) failures.push('Action button labels can auto-wrap');
  if (minimumAuthoredFontSize !== null && minimumAuthoredFontSize < 12) failures.push(`UI font size below 12: ${minimumAuthoredFontSize}`);
  if (!simplifiedRecipePurchase) failures.push('Recipe purchase still exposes crafting materials or lacks direct currency price');
  if (!inventoryBasedSelling) failures.push('Shop selling does not expose stack and currency affordances');
  if (!dataDrivenSkillTree) failures.push('Skill tree is not generated from prerequisite data');
  if (!framedGlobalCurrency) failures.push('Global currency HUD is not using the shared currency component');
  if (visibleCoinLabels) failures.push(`Visible coin labels remain in UI source: ${visibleCoinLabels}`);
  if (characterStatusGap < 8) failures.push(`Character status overlaps portrait: ${characterStatusGap}`);
  const report = {
    assets: manifest.length,
    hardAlphaFailures,
    drillContactWidthInBlocks: drillScaleApplied ? 1.15 : null,
    quickSlotNamesCollapsed,
    twoColumnCrafting,
    reusableLayoutSystem,
    actionButtonsDoNotAutoWrap,
    minimumAuthoredFontSize,
    simplifiedRecipePurchase,
    inventoryBasedSelling,
    dataDrivenSkillTree,
    framedGlobalCurrency,
    visibleCoinLabels,
    characterStatusGap,
    failures,
  };
  fs.writeFileSync('output/widget-art/validation-report.json', JSON.stringify(report, null, 2) + '\n');
  console.log(JSON.stringify(report));
  if (failures.length) process.exitCode = 1;
}
main().catch(error => { console.error(error); process.exitCode = 1; });
