# Bass Presence

Plugin VST3 que faz o grave (808, baixo, kick) **aparecer em celular, fone e caixa pequena**, sem alterar o som original.

Alto-falantes pequenos não reproduzem sub-grave (40-80 Hz). Mas o cérebro reconstrói a nota grave a partir dos **harmônicos** (efeito de *fundamental ausente*). O Bass Presence gera esses harmônicos e soma ao sinal original, tudo dentro de um único plugin: **sem send, sem canal paralelo, sem som duplicado**.

## Como funciona

```
entrada ──(seco)────────────────────────────────────────┐
   │                                                     ▼
   └─> LP (fonte) ─> saturação (tanh) ─> HP (harmônicos) ─> nível ─> (+) ─> saída
```

1. O sinal seco passa **intacto** para a saída.
2. Uma cópia interna passa por um **low-pass** (isola o grave, que é a fonte dos harmônicos).
3. A cópia é **saturada** com `tanh` (gera harmônicos ímpares; o *Warmth* adiciona um bias que gera harmônicos pares).
4. Um **high-pass** remove o grave fundamental da cópia, sobrando só os harmônicos (que o celular consegue tocar).
5. A cópia é misturada baixinho com o sinal original.

Os filtros são de 4ª ordem (dois estágios de 12 dB/oitava em cascata).

## Parâmetros

| Parâmetro | Faixa | Padrão | O que faz |
|---|---|---|---|
| **Fonte LP (Hz)** | 60 - 400 | 150 | Quanto do grave alimenta o saturador. |
| **Harmônicos HP (Hz)** | 60 - 250 | 90 | Corta o grave fundamental da cópia. Se o 808 ainda "pesa" duplicado, suba. |
| **Drive (dB)** | 0 - 30 | 12 | Intensidade da saturação (mais drive = mais harmônicos). |
| **Nível dos harmônicos (dB)** | -40 - 0 | -15 | Quanto da cópia é somada ao original. É o controle principal. |
| **Warmth (pares)** | 0 - 1 | 0.3 | Adiciona harmônicos pares (som mais quente/analógico). |

## Uso no FL Studio

1. Coloque o **Bass Presence** no insert do 808 (ou do baixo).
2. Comece com os valores padrão e ajuste o **Nível dos harmônicos** ouvindo no celular.
3. Se o 808 ficou muito "sujo", reduza o **Drive** ou o **Nível**.
4. Se o grave soa duplicado ou com fase estranha, suba o **Harmônicos HP**.

Dica: dê **Solo** no plugin ajustando o nível para ouvir o efeito, mas lembre que a soma com o seco é o que importa. Compare sempre com uma referência profissional no mesmo volume.

## Instalação

### Opção 1: baixar do GitHub Actions

Em **Actions > Build VST3 > último run > Artifacts**, baixe `BassPresence-VST3-Windows` e copie `Bass Presence.vst3` para:

```
C:\Program Files\Common Files\VST3\
```

No FL Studio: **Add > More plugins > Refresh plugin list** (ou *Find installed plugins*).

### Opção 2: compilar localmente (Windows)

Requisitos: [CMake](https://cmake.org/) 3.22+, Visual Studio Build Tools (carga **Desktop development with C++**) e Git.

```
git clone https://github.com/<seu-usuario>/bass-presence.git
cd bass-presence
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

O plugin sai em `build\BassPresence_artefacts\Release\VST3\Bass Presence.vst3`. A primeira compilação baixa o JUCE e demora alguns minutos.

## Estrutura

```
bass-presence/
├── CMakeLists.txt
├── Source/
│   └── Plugin.cpp
├── .github/workflows/build.yml
├── LICENSE
└── README.md
```

## Limitações

- Interface genérica (sliders do JUCE). Sem editor gráfico próprio.
- Só VST3, só Windows no workflow de CI (o CMake também compila em macOS/Linux, mas não foi testado).
- Os filtros de baixa frequência têm pequena defasagem de fase em relação ao seco; com o nível baixo isso é inaudível, mas pode alterar o timbre em ajustes extremos.
- Não substitui boa mixagem: separar kick e baixo por frequência e cortar sub inútil (< 30 Hz) continua valendo.

## Licença

Código sob licença MIT (ver `LICENSE`). O JUCE tem licença própria (AGPLv3 ou comercial): confira antes de distribuir binários.
