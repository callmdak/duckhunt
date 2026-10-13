# Duck Hunt

Prototipo 2D em OpenGL 3.3, GLFW e GLM. O jogo usa projecao ortografica, sprites texturizados, animacao por spritesheet, camadas com parallax, hitbox retangular e controle pelo mouse.

## Controles

- Enter: iniciar no menu; voltar ao menu depois do fim da partida.
- Clique esquerdo: atirar no pato.
- R: reiniciar a partida durante o jogo ou no menu.
- Esc: sair.
- Cada rodada permite a quantidade de tiros definida em `config/game.cfg`. O jogo termina depois do numero configurado de rodadas.
- A arma acompanha o mouse na parte inferior da tela. Acertar todos os patos mostra o cao comemorando por 2 segundos e depois a tela de vitoria; errar algum mostra a tela de derrota.

O menu inicial, as instrucoes, a rodada atual, os acertos, os tiros restantes e o resultado final aparecem desenhados dentro da janela. O titulo da janela permanece como "Duck Hunt".

## Estrutura para apresentacao

- `src/main.cpp`: carrega configuracao e sprites, cria VAO/VBO/EBO, executa o loop, desenha textos com uma fonte bitmap, move o pato, trata os tiros e colisao, controla menu/rodadas e desenha as camadas.
- `shaders/vertex.glsl`: aplica `projection * model` e encaminha as coordenadas de textura.
- `shaders/fragment.glsl`: amostra a textura do sprite e aplica a cor de modulacao.
- `assets/`: sprites editaveis em texto ou imagens PNG/GIF, convertidos para texturas RGBA na inicializacao. GIFs animados usam os tempos de quadro do proprio arquivo.
- `config/game.cfg`: dimensoes da janela, velocidade, quantidade de rodadas e tiros e arquivos dos sprites.

O quad unitario usa vertices com posicao 2D e UV. A matriz ortografica mapeia coordenadas de pixel da janela para o espaco de recorte; cada sprite recebe sua propria matriz de modelo para posicao e escala.

## Build

Abra a pasta no VS Code com a extensao CMake Tools, configure o projeto e compile o alvo `ProcessamentoGrafico`. Os shaders, a configuracao e os sprites sao localizados a partir da pasta do projeto, mesmo quando o executavel inicia no diretorio de build.