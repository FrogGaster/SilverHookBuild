$ErrorActionPreference = "Stop"

$repoRoot = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$sourcePath = Join-Path $repoRoot "ImGui DirectX 11 Kiero Hook\main.cpp"
$source = [System.IO.File]::ReadAllText($sourcePath)
$expression = [regex]'FindPattern\(const_cast\s*<char\*>\("(?<pattern>(?:\\x[0-9A-Fa-f]{2})+)"\),\s*const_cast\s*<char\*>\("(?<mask>[x?]+)"\)\)'
$matches = $expression.Matches($source).Count
$updated = $expression.Replace($source, {
    param($match)
    return 'FIND_PATTERN("' + $match.Groups['pattern'].Value + '", "' + $match.Groups['mask'].Value + '")'
})

$imguiExpression = [regex]'(?<prefix>ImGui::(?:Begin|Button|Checkbox|Text|TextWrapped|BeginCombo|InputText|InputInt|SliderInt)\s*\()\s*(?<literal>(?:u8)?"(?:\\.|[^"\\])*")'
$imguiMatches = $imguiExpression.Matches($updated).Count
$updated = $imguiExpression.Replace($updated, {
    param($match)
    return $match.Groups['prefix'].Value + 'OBFUSCATE(' + $match.Groups['literal'].Value + ')'
})

$diagnosticExpression = [regex]'(?<prefix>(?:WriteLog|printm|printLog)\s*\()\s*(?<literal>"(?:\\.|[^"\\])*")'
$diagnosticMatches = $diagnosticExpression.Matches($updated).Count
$updated = $diagnosticExpression.Replace($updated, {
    param($match)
    return $match.Groups['prefix'].Value + 'OBFUSCATE(' + $match.Groups['literal'].Value + ')'
})

$statusExpression = [regex]'(?<prefix>gBoostStatus\s*=\s*)(?<literal>"(?:\\.|[^"\\])*")'
$statusMatches = $statusExpression.Matches($updated).Count
$updated = $statusExpression.Replace($updated, {
    param($match)
    return $match.Groups['prefix'].Value + 'OBFUSCATE(' + $match.Groups['literal'].Value + ')'
})

$countryPrefixExpression = [regex]'(?<prefix>strstr\(rawName,\s*)(?<literal>"(?:\\.|[^"\\])*")'
$countryPrefixMatches = $countryPrefixExpression.Matches($updated).Count
$updated = $countryPrefixExpression.Replace($updated, {
    param($match)
    return $match.Groups['prefix'].Value + 'OBFUSCATE(' + $match.Groups['literal'].Value + ')'
})

$encoding = [System.Text.UTF8Encoding]::new($true)
[System.IO.File]::WriteAllText($sourcePath, $updated, $encoding)
Write-Host "Obfuscated $matches patterns, $imguiMatches ImGui strings, $diagnosticMatches diagnostics, $statusMatches status strings, and $countryPrefixMatches country prefixes."
