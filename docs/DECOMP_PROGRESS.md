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

## Segunda etapa: fila automática de funções ainda não recuperadas

O mesmo comando `make decomp-audit` agora também executa
[`decomp_triage.py`](../tools/recovery/decomp_triage.py) após o inventário.
O GitHub Actions entrega:

- `decomp_priorities.csv`: **todos** os candidatos do Assembly, ordenados
  pela frequência observada de chamadas diretas (`call`/`bal`) e por
  referências de ponteiros em tabelas, com o arquivo/linha original.
- `decomp_pointer_hypotheses.csv`: rótulos presentes em dados de endereços
  (`.long`, `.word`, etc.) mas ausentes do inventário de candidatos.
  **Não são automaticamente promovidos a funções**; podem ser dados,
  callbacks internos ou destinos de task a verificar.
- `decomp_triage.json`: pontuações, agrupamento exploratório por subsistema,
  nomes de fontes C parecidos (só sugestões, não equivalência), e próximo
  endereço conhecido quando houver.
- `DECOMP_TRIAGE.md`: as prioridades para revisão e a distribuição por área.

O **próximo endereço candidato** é apenas o endereço de entrada da próxima
função identificável, **não** o limite final real da função anterior. Pode
haver blocos compartilhados, labels intermediários, funções sem símbolo ou
dados alinhados. Não contabilizamos tamanhos de funções a partir dessa lacuna.

A pontuação `5 * chamadas diretas + 2 * referências por ponteiros`
é apenas uma heurística de prioridade (popularidade de callsite), e não mede
completude, tamanho ou dificuldade. Referências baseadas em `.long` são
propostas, sujeitas a inspeção manual. O cálculo inicial de 899 candidatos
e 0,4449% de **vínculos restritos** mantém-se separado dessas hipóteses.

O passo de revisão é: verificar destino na ROM, delimitar o fluxo de controle
real, identificar o C por semântica, anexar evidência de ROM e comparar estado
contra uma execução de referência. Só então ampliar o manifesto de cobertura.
