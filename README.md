# Duck Hunt

Jogo 2D em C++17 e OpenGL 3.3. Usa GLFW, GLM e stb_image; o CMake baixa essas dependencias automaticamente na primeira configuracao. Os recursos sao copiados para junto do executavel, entao o jogo nao depende do diretorio do codigo-fonte nem da pasta de trabalho usada para inicia-lo.

## Compilar e empacotar

Requisitos: CMake 3.20 ou mais recente, Git, um compilador C/C++ com suporte a C++17 e os componentes de desenvolvimento do OpenGL. No Windows, use o Visual Studio 2022 com a carga de trabalho **Desenvolvimento para desktop com C++**. No macOS, instale o Xcode Command Line Tools e o CMake. No Ubuntu/Debian:

```sh
sudo apt update
sudo apt install build-essential cmake git libgl1-mesa-dev xorg-dev
```

Na raiz do repositorio, configure e compile em modo Release:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Para gerar um ZIP pronto para distribuir, contendo o executavel e apenas os recursos necessarios:

```sh
cmake --build build --config Release --target package
```

O pacote fica em `build/package/`. No Windows, ele inclui o runtime C++ estatico, os recursos do jogo e o executavel; nao inclui arquivos intermediarios da compilacao. Extraia o ZIP e execute `ProcessamentoGrafico.exe`.

O executavel gerado e especifico para o sistema operacional e a arquitetura usados na compilacao; nao existe um unico binario que rode em Windows, Linux e macOS. Quem clonar o repositorio pode compilar e empacotar localmente com os comandos acima. O jogo requer suporte a OpenGL 3.3.

Na primeira configuracao, o CMake baixa GLFW, GLM e stb_image; e necessario ter conexao com a internet. Para executar diretamente da pasta de build:

- Windows com Visual Studio: `build\bin\Release\ProcessamentoGrafico.exe`
- Linux/macOS: `build/bin/Release/ProcessamentoGrafico`

Os arquivos de configuracao, imagens e shaders sao colocados automaticamente ao lado do executavel.

## Arquivos do jogo

- `duckhunt/src/main.cpp`: inicializacao e ciclo principal.
- `duckhunt/src/game.cpp`: regras, movimento, colisao, tiros, controles e rodadas.
- `duckhunt/src/renderer.cpp`: recursos graficos, animacao e desenho.
- `duckhunt/src/resources.cpp`: configuracao, shaders e carregamento de imagens.
- `duckhunt/shaders/`: shaders de vertices e fragmentos.
- `duckhunt/assets/`: imagens PNG/GIF. GIFs usam os tempos de quadro do proprio arquivo.
- `duckhunt/config/game.cfg`: dimensoes da janela, velocidade, rodadas, tiros e arquivos dos sprites.

## Controles

- Enter: iniciar no menu ou voltar ao menu depois do fim da partida.
- Clique esquerdo: atirar no pato.
- R: reiniciar a partida durante o jogo ou no menu.
- Esc: sair.
- Cada rodada permite a quantidade de tiros definida em `duckhunt/config/game.cfg`. Errar todos os tiros encerra a partida; acertar o pato avanca para a rodada seguinte.
- A arma acompanha o mouse na parte inferior da tela. Acertar todos os patos mostra o cao comemorando por 2 segundos e depois a tela de vitoria; errar os tiros mostra a tela de derrota.
