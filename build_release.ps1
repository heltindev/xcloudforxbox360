$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot
$project = Join-Path $root 'XCloud360.vcxproj'
$msbuild = Join-Path $env:WINDIR 'Microsoft.NET\Framework\v4.0.30319\MSBuild.exe'
$xextool = Join-Path $root 'tools\xextool\XexTool-windows-x64\XexTool.exe'

if (!(Test-Path $msbuild)) { throw 'MSBuild 4.0 nao localizado.' }
if (!(Test-Path $project)) { throw 'Projeto XCloud360.vcxproj ausente.' }
if (!(Test-Path $xextool)) {
    throw 'XexTool ausente. Baixe Team-Resurgent/XexTool e extraia XexTool.exe em tools\xextool\XexTool-windows-x64\.'
}
if ([string]::IsNullOrWhiteSpace($env:XEDK)) {
    throw 'Ambiente XEDK nao configurado. Configure o Xbox 360 SDK antes de compilar.'
}
# Gera a fonte a partir do SDK instalado. O arquivo .xpr gerado
# permanece local e NAO precisa ser publicado no GitHub.
$fontSource = Join-Path $root 'assets\Fonts\SegoeUI_24.xpr'
if (!(Test-Path $fontSource)) {
    $bundler = Join-Path $env:XEDK 'bin\win32\bundler.exe'
    $rdf = Join-Path $env:XEDK 'Source\Samples\Media\Fonts\SegoeUI_24.rdf'
    if (!(Test-Path $bundler) -or !(Test-Path $rdf)) {
        throw 'Nao encontrei Bundler ou SegoeUI_24.rdf no XDK.'
    }
    New-Item -ItemType Directory -Path (Split-Path $fontSource) -Force | Out-Null
    Write-Host 'Gerando fonte Segoe UI localmente com SDK Bundler...'
    & $bundler $rdf -o $fontSource
    if ($LASTEXITCODE -ne 0 -or !(Test-Path $fontSource)) {
        throw 'Falha ao gerar o arquivo de fonte .xpr.'
    }
}
$atgLib = Join-Path $root 'obj\AtgFramework\Release\AtgFramework.lib'
if (!(Test-Path $atgLib)) {
    $atgProj = Join-Path $env:XEDK 'Source\Samples\Common\AtgFramework2010.vcxproj'
    if (!(Test-Path $atgProj)) { throw 'AtgFramework2010.vcxproj nao encontrado no XDK.' }
    $atgFolder = Join-Path $root 'obj\AtgFramework\Release'
    New-Item -ItemType Directory -Path $atgFolder -Force | Out-Null
    $atgFolder = $atgFolder + '\'
    Write-Host 'Preparando biblioteca ATG para a fonte suavizada...'
    & $msbuild $atgProj /t:Build '/p:Configuration=Release;Platform=Xbox 360' ('/p:OutDir=' + $atgFolder) ('/p:IntDir=' + $atgFolder) /v:quiet
    if ($LASTEXITCODE -ne 0 -or !(Test-Path $atgLib)) { throw 'Falha ao compilar biblioteca ATG.' }
}
Write-Host 'Etapa 1/3: compilando Xbox 360 Release...'
& $msbuild $project /t:Build '/p:Configuration=Release;Platform=Xbox 360' /v:minimal
if ($LASTEXITCODE -ne 0) { throw "Compilacao falhou (codigo $LASTEXITCODE)" }

$source = Join-Path $root 'Release\XCloud360.xex'
$dist = Join-Path $root 'dist'
$target = Join-Path $dist 'XCloud360.xex'
if (!(Test-Path $source)) { throw 'Compilou, mas XEX nao foi localizado.' }
New-Item -ItemType Directory -Force -Path $dist | Out-Null

Write-Host 'Etapa 2/3: garantindo XEX Retail para Xbox 360 RGH...'
# MSBuild pode preservar a saida anterior quando nao ha mudancas.
# Se a pasta Release ja contem Retail, evitamos reconverter o mesmo XEX.
$sourceInfo = (& $xextool -l $source | Out-String)
if ($LASTEXITCODE -ne 0) { throw 'Nao foi possivel inspecionar XEX de Release.' }
if ($sourceInfo -match '(?m)^\s+Retail\s*$' -and
    $sourceInfo -match 'All Media Types') {
    Copy-Item -LiteralPath $source -Destination $target -Force
} else {
    & $xextool -m r -e e -r a -o $target $source
    if ($LASTEXITCODE -ne 0) { throw 'Conversao Retail falhou.' }
}

$info = (& $xextool -l $target | Out-String)
if ($LASTEXITCODE -ne 0 -or $info -notmatch '(?m)^\s+Retail\s*$' -or
    $info -notmatch 'All Media Types') {
    throw 'ERRO: validacao do XEX retail falhou; pacote nao sera criado.'
}
# Fonte de exportacao solicitada pelo usuario: Release/XCloud360.xex.
# Esta COPIA e Retail validada, nao o Devkit original.
Copy-Item -LiteralPath $target -Destination $source -Force

# Polished, anti-aliased Segoe UI in an XPR. Bitmap font remains
# compiled into the XEX as a fallback if resources fail to load.
$fontSource = Join-Path $root 'assets\Fonts\SegoeUI_24.xpr'
$fontDir = Join-Path $dist 'Media\Fonts'
if (!(Test-Path $fontSource)) { throw 'Fonte suavizada XPR ausente no assets\Fonts.' }
New-Item -ItemType Directory -Path $fontDir -Force | Out-Null
Copy-Item -LiteralPath $fontSource -Destination (Join-Path $fontDir 'SegoeUI_24.xpr') -Force
$testNote = Join-Path $dist 'LEIA_PRIMEIRO.txt'
@'
XCloud360 v0.9 - DIAGNOSTICO DNS UDP E TCP / RGH3
XEX convertido para RETAIL com All Media Types.
Copie toda a pasta, incluindo Media/Fonts para ter letras suaves.
Se a fonte externa nao carregar, ha letras bitmap de reserva.
Inicie no Aurora/XeXMenu. Aperte A para diagnostico de rede.
Anote o texto exato: ETAPA, ERRO e ORIGEM.
Teste A: www.xbox.com:443 (DNS Xbox; fallback DNS UDP Google).
Teste B: 1.1.1.1:443 (TCP sem DNS). UDP DNS consulta 8.8.8.8:53 e 8.8.4.4:53; erro 10060 indica timeout.
DNS primario/secundario sao ajustados nas configuracoes do Xbox 360.
O XEX usa DNS configurado no console; NAO impõe endereco fixo.
O teste TCP tem limite de tempo e nao negocia TLS.
Isto testa DNS UDP e TCP; NAO testa HTTPS/TLS, login ou streaming.
Exclusivo Xbox 360 RGH3: valide DNS/UDP/TCP no hardware real.
B volta, BACK sai. Nenhuma senha/token e solicitada.
Ainda NAO faz login Microsoft, navegador xCloud nem Fortnite.
'@ | Set-Content -LiteralPath $testNote -Encoding Ascii

Write-Host 'Etapa 3/3: criando pacote...'
$zip = Join-Path $root 'XCloud360-RGH3.zip'
if (Test-Path $zip) { Remove-Item $zip -Force }
Compress-Archive -Path (Join-Path $dist '*') -DestinationPath $zip
$hash = (Get-FileHash -Algorithm SHA256 $target).Hash
Write-Host ('SUCESSO (RGH Retail): ' + $source)
Write-Host ('COPIA PARA DISTRIBUICAO: ' + $target)
Write-Host ('Pacote RGH: ' + $zip)
Write-Host ('SHA256: ' + $hash)
Write-Host 'XexTool: Retail + All Media Types verificados.'
