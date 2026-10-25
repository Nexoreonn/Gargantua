# Gargantua

Aplicação C++17 de visualização de um buraco negro com OpenGL 4.3, GLFW, GLEW e GLM. Os shaders GLSL fazem o traçado de raios, a lente gravitacional, o disco de acreção e o pós-processamento. Isso também pode servir como um benchmark para testar sua máquina, mas, por enquanto, ficará limitado à visualização do FPS.

## Requisitos

- Windows x64
- Visual Studio 2026 com as ferramentas de desenvolvimento C++
- CMake 3.21 ou superior
- vcpkg

O manifesto `vcpkg.json` instala GLFW, GLEW, GLM, Dear ImGui (interface do menu gráfico) e `glslangValidator` (ferramenta de validação dos shaders).

Se ainda não tiver o vcpkg, instale-o uma vez:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:\dev\vcpkg
& C:\dev\vcpkg\bootstrap-vcpkg.bat
```

## Configurar e compilar

Abra um PowerShell na pasta do projeto, onde estão `CMakeLists.txt` e `CMakePresets.json`. Ajuste `VCPKG_ROOT` caso o vcpkg esteja instalado em outro local:

```powershell
$env:VCPKG_ROOT = 'C:\dev\vcpkg'
cmake --preset vs2026-x64
```

O primeiro comando de configuração instala as dependências do manifesto. Depois, compile uma das configurações:

```powershell
cmake --build --preset release
cmake --build --preset debug
```

Os executáveis ficam em `build-vs2026\Release\Gargantua.exe` e `build-vs2026\Debug\Gargantua.exe`. Os shaders são copiados para a pasta correspondente, ao lado do executável.

Os presets devem ser usados a partir da raiz do projeto, e não de `build-vs2026\Release`. Se o terminal estiver em outra pasta, compile informando o diretório de build:

```powershell
cmake --build 'C:\caminho\para\Gargantua\build-vs2026' --config Release
```

## Validar shaders

Para compilar e validar todos os shaders com `glslangValidator`:

```powershell
cmake --build --preset release --target validate_shaders
```

O alvo valida os shaders para OpenGL. Ele não inicia a janela do aplicativo.

## Executar

Inicie o executável na pasta da configuração desejada:

```powershell
.\build-vs2026\Release\Gargantua.exe
```

## Controles

### Navegação e simulação

- **W/A/S/D:** mover a câmera.
- **Espaço / Ctrl esquerdo:** subir / descer.
- **Shift esquerdo:** mover mais rápido.
- **Mouse:** olhar ao redor.
- **Tab:** liberar ou capturar o mouse.
- **Roda do mouse:** ajustar o campo de visão.
- **Esc:** sair.
- **P:** pausar ou retomar a animação do disco e a deriva do buraco negro.
- **F1:** alternar entre câmera livre e câmera em órbita. No modo orbital, a câmera acompanha e aponta para o buraco negro; **W/S** ajustam a distância, **A/D** giram ao redor dele e o mouse controla a orientação.
- **5 / 6:** diminuir ou aumentar a velocidade de rotação do disco. O valor zero interrompe a animação do disco.
- **F5:** recarregar os shaders.

### Opções e ajustes

- **F3:** abrir ou fechar o painel gráfico dentro da janela.
- O painel também permite ativar ou desativar o modo de câmera em órbita.
- O painel inspirado em menus de jogos permite ajustar com o mouse ou teclado a escala da resolução interna, FXAA, bloom, VSync, intensidade da lente gravitacional, qualidade do traçado e velocidades de animação.
- A lente gravitacional usa passos de integração menores perto da esfera de fótons e destaca o anel de fótons; o controle de intensidade permite ajustar a deflexão visual.
- A deriva varia de **0,0x** (parado) a **3,0x**. O buraco negro segue uma linha reta a velocidade constante, sem orbitar; o multiplicador altera sua velocidade. Como a trajetória é ilimitada, mova a câmera para acompanhá-lo caso ele saia do enquadramento.
- **F2:** alternar o VSync fora do menu.
- **7 / 8:** diminuir ou aumentar a escala da resolução interna.
- **B:** alternar o bloom.
- **1 / 2:** diminuir ou aumentar a exposição.
- **3 / 4:** diminuir ou aumentar a intensidade do bloom.
- **9 / 0:** diminuir ou aumentar a qualidade do traçado de raios.
- **Z / X:** diminuir ou aumentar a intensidade do disco.
- **C / V:** diminuir ou aumentar a intensidade do efeito Doppler.
- **N / M:** diminuir ou aumentar a mistura da paleta artística.

O título da janela mostra a taxa de quadros (FPS). Aumentar a escala de renderização ou a qualidade do traçado de raios pode reduzir o desempenho.