# Medição auditável da decompilação de Sonic the Fighters

Esta página registra a primeira execução automática do inventário de funções
Assembly e suas ligações a recuperações C com evidência explícita.

**Primeiro snapshot:** 13 de outubro de 2026. Inventário calculado em
[GitHub Actions, execução 38374400772](https://github.com/celsowm/stfdecomp/actions/runs/38374400772).
Os CSV/JSON/Markdown completos estão anexados ao workflow, não são ROMs.

| Contagem | Primeira medição |
| --- | ---: |
| Linhas de Assembly (rom_code1..4.s) | 201.526 |
| Labels distintos definidos | 17.846 |
| Funções candidatas: alvos de call/bal ou símbolos numéricos sub_/func_ | **899** |
| Funções candidatas associadas a pelo menos um intervalo C documentado | **4** |
| Taxa de **mapeamento restrito** do inventário | **0,4449%** |
| Intervalos parciais com endereço original + fonte C + referência de validação | 12 |
| União desses intervalos, sem contar sobreposições duas vezes | 2.504 bytes |
| Fração do espaço nominal de endereços da ROM de programa (1 MiB) | 0,2388% |
| Funções provadamente recompiladas C com equivalência binária à ROM | **0 documentadas** |
| **Percentual real de decompilação integral** | **Ainda não mensurável** |

## Como reproduzir

```sh
make decomp-audit
# Saídas:
# build/decomp-progress/decomp_progress.json
# build/decomp-progress/decomp_function_inventory.csv
# build/decomp-progress/DECOMP_PROGRESS.md
```

Opcionalmente, depois de gerar a tabela verdadeira de símbolos do executável
original com a toolchain i960:

```sh
python tools/recovery/decomp_progress.py --nm build/rom_code1.nm
```

Cada execução da [CI decomp-progress](https://github.com/celsowm/stfdecomp/actions/workflows/decomp-progress.yml)
publica os arquivos gerados como *artifact* e imprime um resumo na página do workflow.

## Definição dos indicadores

1. **Funções candidatas**: identificadores definidos na seção de código, usados
   como alvos diretos de `call`/`bal`, ou identificados como `sub_ADDRESS`.
   Não incluem todos os destinos de ponteiros de função, entradas de tasks,
   rotinas sem símbolos, nem necessariamente correspondem 1:1 a funções
   independentes. O contador *não é* o total real de funções do jogo.
2. **Mapeamento de recuperação C**: o inventário só associa um candidato
   quando ele está relacionado a um intervalo definido em
   [decomp_evidence.json](../tools/recovery/decomp_evidence.json), que exige
   endereço original, fonte C existente, referência de evidência e status
   `partial`. Código C ainda sem entrada comprovada no manifesto não é
   contabilizado: por isso 0,4449% é **incompleto, não o progresso real**.
3. **Bytes de corredor**: as assinaturas de `validate_sfight_rom.py` delimitam
   12 segmentos do i960 com referência direta. Somar seus tamanhos não mede
   equivalência; o SHA-256 se refere ao *original*, não aos bytes que seriam
   compilados a partir das funções C. O denominador de 1 MiB inclui dados.
4. **Funções completas byte-match**: nenhum manifesto certifica ainda um
   *rebuild* C de uma função inteira idêntica à original. Isso **não** significa
   que todo o C seja incorreto; significa que a verificação total ainda não
   foi obtida/documentada.

O Makefile principal ainda usa os fontes de Assembly para reconstruir a
ROM. A biblioteca C da recuperação é compilada/testada em paralelo.
Passar na suíte de testes sintéticos não é o mesmo que obter equivalência
byte-exata, temporal ou visual.

## Passos para melhorar a confiabilidade

- Usar `make symbols` e `--nm` para atribuir endereços exatos aos nomes de
  função não numéricos; incluir também entradas de ponteiros de task.
- Deduzir início e **fim de cada função** a partir de fluxo de controle,
  informações de linker e referências da ROM. Distinguir dados embutidos.
- Adicionar evidências para outras rotinas em `src/recovered/`, especialmente
  as que ainda não foram vinculadas a entradas originais. Nunca inferir
  conclusão a partir de nome ou existência do arquivo C.
- Ampliar captura e replay de estado i960/TGP contra a execução original;
  atribuir *full semantic match* somente depois de verificar todos os
  caminhos relevantes.
- Vincular C na produção da ROM e medir o conjunto de funções completas que
  passam em um comparador de bytes. Só então divulgar uma taxa de
  decompilação com denominador de instruções/funções **reais**.

Os 899 candidatos, 12 trechos e 4 vínculos constituem o **baseline
reproduzível**, não uma meta nem a conclusão de que falta 99,55% do jogo.
