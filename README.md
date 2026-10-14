# Duck Hunt

Jogo 2D em C++17 e OpenGL 3.3.

## Compilar

Requisitos: CMake 3.20+, Git, compilador C++17 e OpenGL. O CMake baixa GLFW, GLM e stb_image; é necessária conexão com a internet.

```sh
cmake -S . -B build
cmake --build build --config Release
```

Execute `build/bin/Release/duckhunt.exe` no Windows ou `build/bin/Release/duckhunt` no Linux/macOS.

Para gerar um ZIP distribuível:

```sh
cmake --build build --config Release --target package
```

## Controles

- **Enter:** iniciar ou voltar ao menu.
- **Clique esquerdo:** atirar.
- **R:** reiniciar.
- **Esc:** sair.

Acertar o pato avança a rodada; errar todos os tiros encerra a partida.
