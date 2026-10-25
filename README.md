# Gargantua

Aplicacao C++17 de visualizacao de um buraco negro com OpenGL 4.3, GLFW, GLEW e GLM. Os shaders GLSL fazem o tracado de raios, a lente gravitacional, o disco de acrecao e o pos-processamento.

## Requisitos

- Windows x64
- Visual Studio 2026 com as ferramentas de desenvolvimento C++
- CMake 3.21 ou superior
- vcpkg

O manifesto `vcpkg.json` instala GLFW, GLEW, GLM e `glslangValidator` (ferramenta de validacao dos shaders).
Se ainda nao tiver o vcpkg, instale-o uma vez:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
& C:\dev\vcpkg\bootstrap-vcpkg.bat
```

## Configurar e compilar

Abra um PowerShell na pasta do projeto, onde estao `CMakeLists.txt` e `CMakePresets.json`. Ajuste `VCPKG_ROOT` caso o vcpkg esteja instalado em outro local:

```powershell
$env:VCPKG_ROOT = 'C:\dev\vcpkg'
cmake --preset vs2026-x64
```

O primeiro comando de configuracao instala as dependencias do manifesto. Depois, compile uma das configuracoes:

```powershell
cmake --build --preset release
cmake --build --preset debug
```

Os executaveis ficam em `build-vs2026\Release\Gargantua.exe` e `build-vs2026\Debug\Gargantua.exe`. Os shaders sao copiados para a pasta correspondente ao lado do executavel.

Os presets devem ser usados a partir da raiz do projeto, nao de `build-vs2026\Release`. Se o terminal estiver em outra pasta, compile informando o diretorio de build:

```powershell
cmake --build 'C:\caminho\para\Gargantua\build-vs2026' --config Release
```

## Validar shaders

Para compilar/validar todos os shaders com `glslangValidator`:

```powershell
cmake --build --preset release --target validate_shaders
```

O alvo valida os shaders para OpenGL. Ele nao inicia a janela do aplicativo.

## Executar

Inicie o executavel na pasta de configuracao desejada:

```powershell
.\build-vs2026\Release\Gargantua.exe
```

## Controles

### Navegacao e simulacao

- **W/A/S/D**: mover a camera.
- **Espaco / Ctrl esquerdo**: subir / descer.
- **Shift esquerdo**: mover mais rapido.
- **Mouse**: olhar ao redor.
- **Tab**: liberar ou capturar o mouse.
- **Roda do mouse**: ajustar o campo de visao.
- **Esc**: sair.
- **P**: pausar/retomar a animacao do disco e a deriva do buraco negro.
- **5 / 6**: diminuir/aumentar a velocidade de rotacao do disco. O valor zero para a animacao do disco.
- **F5**: recarregar os shaders.

### Opcoes e ajustes

- **F3**: abrir/fechar o menu grafico exibido no titulo da janela.
- No menu F3, **setas para cima/baixo** selecionam uma opcao e **esquerda/direita** alteram o valor.
- Opcoes do menu: escala da resolucao interna, anti-aliasing FXAA, bloom, VSync e velocidade da deriva do buraco negro.
- A deriva varia de **0,0x** (parado) a **3,0x**. O buraco negro segue uma linha reta a velocidade constante, sem orbitar; o multiplicador altera a velocidade. Como a trajetoria e ilimitada, mova a camera para acompanha-lo se ele sair do enquadramento.
- **F2**: alternar VSync fora do menu.
- **7 / 8**: diminuir/aumentar a escala da resolucao interna.
- **B**: alternar bloom.
- **1 / 2**: diminuir/aumentar exposicao.
- **3 / 4**: diminuir/aumentar intensidade do bloom.
- **9 / 0**: diminuir/aumentar a qualidade do tracado de raios.
- **Z / X**: diminuir/aumentar intensidade do disco.
- **C / V**: diminuir/aumentar intensidade do efeito Doppler.
- **N / M**: diminuir/aumentar a mistura da paleta artistica.

O titulo da janela mostra a taxa de quadros (FPS). Aumentar a escala de renderizacao ou a qualidade do tracado de raios pode reduzir o desempenho.
