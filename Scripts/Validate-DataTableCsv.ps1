param(
    [string]$CsvRoot = (Join-Path (Split-Path -Parent $PSScriptRoot) "Content\Data\DataTable\DataSource")
)

Set-StrictMode -Version 3.0
$ErrorActionPreference = "Stop"

$script:Issues = New-Object System.Collections.Generic.List[string]
$script:AssetReferencePattern = "^(?:[A-Za-z0-9_]+')?/(?:Game|Engine|Script)/.+\..+(?:')?$"
$script:EnumValues = @{
    ECharacterAnimationType = @("Idle", "Run", "Fly", "Fall", "Drill_Side", "Drill_Up", "Drill_Down")
    ESFXType = @("None", "Floating", "Drill", "Break_Dirt")
    ESFXInstanceParamType = @("None", "DestroyedBlockCount", "Max")
    ESFXContinuousParamType = @("None", "HP", "Flying", "Drill")
}

function New-ScalarSchema {
    param(
        [string]$Kind,
        [bool]$Nullable = $false,
        [string[]]$Values = @(),
        [hashtable]$Node = $null
    )

    return @{
        kind = $Kind
        nullable = $Nullable
        values = $Values
        node = $Node
    }
}

function New-ObjectNodeSchema {
    param(
        [hashtable]$Properties,
        [string[]]$Required = @(),
        [bool]$AllowAdditional = $false
    )

    return @{
        kind = "object"
        properties = $Properties
        required = $Required
        allowAdditional = $AllowAdditional
    }
}

function New-ArrayNodeSchema {
    param(
        [hashtable]$Item
    )

    return @{
        kind = "array"
        item = $Item
    }
}

$AnimationTextureNode = New-ObjectNodeSchema -Properties @{
    IdleTexture = New-ScalarSchema -Kind "softObject" -Nullable $true
    RunTexture = New-ScalarSchema -Kind "softObject" -Nullable $true
    FlyTexture = New-ScalarSchema -Kind "softObject" -Nullable $true
    FallTexture = New-ScalarSchema -Kind "softObject" -Nullable $true
    DrillSideTexture = New-ScalarSchema -Kind "softObject" -Nullable $true
    DrillUpTexture = New-ScalarSchema -Kind "softObject" -Nullable $true
    DrillDownTexture = New-ScalarSchema -Kind "softObject" -Nullable $true
}

$script:TableSchemas = @{
    "BlockData.csv" = @{
        columns = [ordered]@{
            "---" = New-ScalarSchema -Kind "name"
            TileIndex = New-ScalarSchema -Kind "int"
            bHasCollision = New-ScalarSchema -Kind "bool"
            TileSprite = New-ScalarSchema -Kind "softObject" -Nullable $true
            BlockDurability = New-ScalarSchema -Kind "float"
            bIsMineable = New-ScalarSchema -Kind "bool"
            SpawnableItemDataList = New-ScalarSchema -Kind "ueArray" -Nullable $true -Node (New-ArrayNodeSchema -Item (New-ObjectNodeSchema -Required @("SpawnableItemKey", "SpawnProbability", "ItemSprite") -Properties @{
                SpawnableItemKey = New-ScalarSchema -Kind "name"
                SpawnProbability = New-ScalarSchema -Kind "float"
                ItemSprite = New-ScalarSchema -Kind "softObject" -Nullable $true
            }))
            bNeedTobeHide = New-ScalarSchema -Kind "bool"
        }
    }
    "Item.csv" = @{
        columns = [ordered]@{
            "---" = New-ScalarSchema -Kind "name"
            ItemIcon = New-ScalarSchema -Kind "softObject" -Nullable $true
            bIsOre = New-ScalarSchema -Kind "bool"
            SellPrice = New-ScalarSchema -Kind "int"
        }
    }
    "Character.csv" = @{
        columns = [ordered]@{
            "---" = New-ScalarSchema -Kind "name"
            CharacterName = New-ScalarSchema -Kind "text"
            AnimationTextures = New-ScalarSchema -Kind "ueStruct" -Nullable $true -Node $AnimationTextureNode
            InitialInventoryItems = New-ScalarSchema -Kind "ueArray" -Nullable $true -Node (New-ArrayNodeSchema -Item (New-ObjectNodeSchema -Required @("ItemKey", "ItemCount") -Properties @{
                ItemKey = New-ScalarSchema -Kind "name"
                ItemCount = New-ScalarSchema -Kind "int"
            }))
        }
    }
    "ShopRecipe.csv" = @{
        columns = [ordered]@{
            "---" = New-ScalarSchema -Kind "name"
            DisplayName = New-ScalarSchema -Kind "text"
            RequiredItems = New-ScalarSchema -Kind "ueArray" -Nullable $true -Node (New-ArrayNodeSchema -Item (New-ObjectNodeSchema -Required @("ItemKey", "ItemCount") -Properties @{
                ItemKey = New-ScalarSchema -Kind "name"
                ItemCount = New-ScalarSchema -Kind "int"
            }))
            ResultItemKey = New-ScalarSchema -Kind "name"
        }
    }
    "Sound.csv" = @{
        columns = [ordered]@{
            "---" = New-ScalarSchema -Kind "name"
            SFXType = New-ScalarSchema -Kind "enum" -Values $script:EnumValues.ESFXType
            SoundSource = New-ScalarSchema -Kind "softObject" -Nullable $true
            InitialParams = New-ScalarSchema -Kind "ueArray" -Nullable $true -Node (New-ArrayNodeSchema -Item (New-ScalarSchema -Kind "enum" -Values $script:EnumValues.ESFXInstanceParamType))
            ContinuousParams = New-ScalarSchema -Kind "ueArray" -Nullable $true -Node (New-ArrayNodeSchema -Item (New-ScalarSchema -Kind "enum" -Values $script:EnumValues.ESFXContinuousParamType))
        }
    }
}

function Add-Issue {
    param(
        [string]$Path,
        [string]$Message
    )

    $script:Issues.Add(("{0}: {1}" -f $Path, $Message)) | Out-Null
}

function Test-NullLike {
    param(
        [object]$Value
    )

    if ($null -eq $Value) {
        return $true
    }

    if ($Value -isnot [string]) {
        return $false
    }

    return [string]::IsNullOrWhiteSpace($Value)
}

function Test-AssetReference {
    param(
        [string]$Value
    )

    return $Value -match $script:AssetReferencePattern
}

function Test-IntegerString {
    param(
        [string]$Value
    )

    $parsed = 0L
    return [int64]::TryParse($Value, [ref]$parsed)
}

function Test-FloatString {
    param(
        [string]$Value
    )

    $parsed = 0.0
    return [double]::TryParse(
        $Value,
        [System.Globalization.NumberStyles]::Float,
        [System.Globalization.CultureInfo]::InvariantCulture,
        [ref]$parsed
    )
}

function Test-BoolString {
    param(
        [string]$Value
    )

    return $Value -match "^(?i:true|false|1|0)$"
}

function Split-TopLevel {
    param(
        [string]$Text
    )

    $items = New-Object System.Collections.Generic.List[string]
    $buffer = New-Object System.Text.StringBuilder
    $depthParen = 0
    $inQuotes = $false

    for ($i = 0; $i -lt $Text.Length; $i++) {
        $char = $Text[$i]
        $shouldAppend = $true

        if ($char -eq '"') {
            if ($inQuotes -and $i + 1 -lt $Text.Length -and $Text[$i + 1] -eq '"') {
                [void]$buffer.Append('""')
                $i++
                continue
            }
            $inQuotes = -not $inQuotes
            [void]$buffer.Append($char)
            continue
        }

        if (-not $inQuotes) {
            if ($char -eq '(') {
                $depthParen++
            }
            elseif ($char -eq ')') {
                $depthParen--
            }
            elseif ($char -eq ',' -and $depthParen -eq 0) {
                $items.Add($buffer.ToString().Trim()) | Out-Null
                $buffer.Clear() | Out-Null
                $shouldAppend = $false
            }
        }

        if ($shouldAppend) {
            [void]$buffer.Append($char)
        }
    }

    $final = $buffer.ToString().Trim()
    if ($final.Length -gt 0) {
        $items.Add($final) | Out-Null
    }

    return ,([string[]]$items.ToArray())
}

function Parse-UnrealQuotedString {
    param(
        [string]$Text
    )

    $trimmed = $Text.Trim()
    if ($trimmed.Length -ge 2 -and $trimmed.StartsWith('"') -and $trimmed.EndsWith('"')) {
        return $trimmed.Substring(1, $trimmed.Length - 2).Replace('""', '"')
    }

    return $trimmed
}

function Parse-UnrealStruct {
    param(
        [string]$Text,
        [string]$Path
    )

    $trimmed = $Text.Trim()
    if ($trimmed -eq "") {
        return $null
    }

    if (-not ($trimmed.StartsWith("(") -and $trimmed.EndsWith(")"))) {
        Add-Issue -Path $Path -Message "Expected Unreal struct syntax wrapped with parentheses."
        return $null
    }

    $inner = $trimmed.Substring(1, $trimmed.Length - 2)
    if ([string]::IsNullOrWhiteSpace($inner)) {
        return @{}
    }

    $properties = @{}
    foreach ($entry in (Split-TopLevel -Text $inner)) {
        $eqIndex = $entry.IndexOf("=")
        if ($eqIndex -lt 1) {
            Add-Issue -Path $Path -Message ("Invalid struct entry '{0}'." -f $entry)
            continue
        }

        $key = $entry.Substring(0, $eqIndex).Trim()
        $rawValue = $entry.Substring($eqIndex + 1).Trim()
        $properties[$key] = Parse-UnrealQuotedString -Text $rawValue
    }

    return $properties
}

function Parse-UnrealArray {
    param(
        [string]$Text,
        [string]$Path
    )

    $trimmed = $Text.Trim()
    if ($trimmed -eq "") {
        return @()
    }

    if (-not ($trimmed.StartsWith("(") -and $trimmed.EndsWith(")"))) {
        Add-Issue -Path $Path -Message "Expected Unreal array syntax wrapped with parentheses."
        return $null
    }

    $inner = $trimmed.Substring(1, $trimmed.Length - 2)
    if ([string]::IsNullOrWhiteSpace($inner)) {
        return @()
    }

    $items = New-Object System.Collections.Generic.List[object]
    foreach ($entry in (Split-TopLevel -Text $inner)) {
        if ($entry.StartsWith("(") -and $entry.EndsWith(")")) {
            $items.Add((Parse-UnrealStruct -Text $entry -Path $Path)) | Out-Null
        }
        else {
            $items.Add((Parse-UnrealQuotedString -Text $entry)) | Out-Null
        }
    }

    return ,([object[]]$items.ToArray())
}

function Validate-Node {
    param(
        [object]$Node,
        [hashtable]$Schema,
        [string]$Path
    )

    switch ($Schema.kind) {
        "object" {
            if ($null -eq $Node -or $Node -isnot [hashtable]) {
                Add-Issue -Path $Path -Message "Expected an object node."
                return
            }

            foreach ($requiredName in $Schema.required) {
                if (-not $Node.ContainsKey($requiredName)) {
                    Add-Issue -Path $Path -Message ("Missing required key '{0}'." -f $requiredName)
                }
            }

            foreach ($propertyName in $Schema.properties.Keys) {
                if ($Node.ContainsKey($propertyName)) {
                    Validate-Value -Value $Node[$propertyName] -Schema $Schema.properties[$propertyName] -Path ("{0}.{1}" -f $Path, $propertyName)
                }
            }

            if (-not $Schema.allowAdditional) {
                foreach ($propertyName in $Node.Keys) {
                    if (-not $Schema.properties.ContainsKey($propertyName)) {
                        Add-Issue -Path ("{0}.{1}" -f $Path, $propertyName) -Message "Unexpected extra key."
                    }
                }
            }
        }
        "array" {
            if ($null -eq $Node -or $Node -isnot [System.Array]) {
                Add-Issue -Path $Path -Message "Expected an array node."
                return
            }

            for ($index = 0; $index -lt $Node.Count; $index++) {
                Validate-Node -Node $Node[$index] -Schema $Schema.item -Path ("{0}[{1}]" -f $Path, $index)
            }
        }
        default {
            Validate-Value -Value $Node -Schema $Schema -Path $Path
        }
    }
}

function Validate-Value {
    param(
        [object]$Value,
        [hashtable]$Schema,
        [string]$Path
    )

    if (Test-NullLike -Value $Value) {
        if ($Schema.nullable) {
            return
        }

        Add-Issue -Path $Path -Message "Value is empty."
        return
    }

    if ($Value -isnot [string]) {
        $Value = [string]$Value
    }

    if ($Value -eq "None") {
        if (($Schema.kind -eq "softObject" -or $Schema.kind -eq "softClass") -and $Schema.nullable) {
            return
        }
    }

    switch ($Schema.kind) {
        "string" { return }
        "name" { return }
        "text" { return }
        "int" {
            if (-not (Test-IntegerString -Value $Value)) {
                Add-Issue -Path $Path -Message "Expected an integer."
            }
            return
        }
        "float" {
            if (-not (Test-FloatString -Value $Value)) {
                Add-Issue -Path $Path -Message "Expected a float using '.' as the decimal separator."
            }
            return
        }
        "bool" {
            if (-not (Test-BoolString -Value $Value)) {
                Add-Issue -Path $Path -Message "Expected a boolean. Allowed values: true, false, 1, 0."
            }
            return
        }
        "enum" {
            if ($Schema.values -notcontains $Value) {
                Add-Issue -Path $Path -Message ("Unknown enum value. Allowed values: {0}" -f ($Schema.values -join ", "))
            }
            return
        }
        "softObject" {
            if (-not (Test-AssetReference -Value $Value)) {
                Add-Issue -Path $Path -Message "Expected a valid Unreal object reference."
            }
            return
        }
        "softClass" {
            if (-not (Test-AssetReference -Value $Value)) {
                Add-Issue -Path $Path -Message "Expected a valid Unreal class reference."
            }
            return
        }
        "ueStruct" {
            $node = Parse-UnrealStruct -Text $Value -Path $Path
            if ($null -ne $node) {
                Validate-Node -Node $node -Schema $Schema.node -Path $Path
            }
            return
        }
        "ueArray" {
            $node = Parse-UnrealArray -Text $Value -Path $Path
            if ($null -ne $node) {
                Validate-Node -Node $node -Schema $Schema.node -Path $Path
            }
            return
        }
        default {
            Add-Issue -Path $Path -Message ("Unknown schema kind '{0}'." -f $Schema.kind)
            return
        }
    }
}

function Validate-CsvFile {
    param(
        [System.IO.FileInfo]$File
    )

    $tableSchema = $script:TableSchemas[$File.Name]
    if ($null -eq $tableSchema) {
        Add-Issue -Path $File.Name -Message "No registered schema for this CSV file."
        return
    }

    $headerLine = Get-Content -LiteralPath $File.FullName -TotalCount 1 -Encoding UTF8
    if ([string]::IsNullOrWhiteSpace($headerLine)) {
        Add-Issue -Path $File.Name -Message "Header row is missing."
        return
    }

    $headers = @($headerLine.Split(",") | ForEach-Object { $_.Trim() })
    $expectedHeaders = @($tableSchema.columns.Keys)

    foreach ($expectedHeader in $expectedHeaders) {
        if ($headers -notcontains $expectedHeader) {
            Add-Issue -Path $File.Name -Message ("Missing required column '{0}'." -f $expectedHeader)
        }
    }

    foreach ($header in $headers) {
        if ($expectedHeaders -notcontains $header) {
            Add-Issue -Path $File.Name -Message ("Unknown column '{0}'." -f $header)
        }
    }

    $rows = @(Import-Csv -LiteralPath $File.FullName -Encoding UTF8)
    for ($rowIndex = 0; $rowIndex -lt $rows.Count; $rowIndex++) {
        $row = $rows[$rowIndex]
        foreach ($columnName in $expectedHeaders) {
            $cellValue = $row.$columnName
            $cellPath = "{0} row {1} column {2}" -f $File.Name, ($rowIndex + 1), $columnName
            Validate-Value -Value $cellValue -Schema $tableSchema.columns[$columnName] -Path $cellPath
        }
    }
}

if (-not (Test-Path -LiteralPath $CsvRoot)) {
    Write-Host ("CSV root not found: {0}" -f $CsvRoot) -ForegroundColor Red
    exit 1
}

$csvFiles = @(Get-ChildItem -LiteralPath $CsvRoot -Filter *.csv -File | Sort-Object Name)
if ($csvFiles.Count -eq 0) {
    Write-Host ("No CSV files found under {0}" -f $CsvRoot)
    exit 0
}

foreach ($csvFile in $csvFiles) {
    Validate-CsvFile -File $csvFile
}

if ($script:Issues.Count -gt 0) {
    Write-Host "CSV schema validation failed:" -ForegroundColor Red
    foreach ($issue in $script:Issues) {
        Write-Host ("- {0}" -f $issue) -ForegroundColor Red
    }
    exit 1
}

Write-Host ("CSV schema validation passed for {0} file(s)." -f $csvFiles.Count) -ForegroundColor Green
exit 0
