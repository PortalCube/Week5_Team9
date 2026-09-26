param(
    [switch]$Check
)

$ErrorActionPreference = 'Stop'

$clangFormat = 'clang-format'

$files = git ls-files -- '*.c' '*.cc' '*.cpp' '*.cxx' '*.h' '*.hh' '*.hpp' '*.hxx' |
    Where-Object { $_ -notlike 'Source/ThirdParty/*' }

if ($Check)
{
    & $clangFormat --dry-run --Werror --style=file $files

    if ($LASTEXITCODE -ne 0)
    {
        exit $LASTEXITCODE
    }
}
else
{
    & $clangFormat -i --style=file $files
}