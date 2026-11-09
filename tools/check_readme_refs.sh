#!/bin/sh
# check_readme_refs.sh — confere as referências de código do README.md (alvo: make readme-check).
#
# 1. Em cada linha de tabela do README que cita âncoras [REQ:<ID>] e links "arquivo#Lx" (ou "#Lx-Ly"):
#    - cada ID precisa existir como "[REQ:<ID>]" no arquivo de algum link da linha, a no máximo 3 linhas
#      da linha citada;
#    - cada link precisa ter alguma âncora da linha por perto (pega número de linha desatualizado).
# 2. Lista os IDs obrigatórios que não aparecem no README.
# 3. Cada bloco ```c cuja primeira linha é "// arquivo#Lx-Ly" precisa ser cópia exata dessas linhas.
#
# Sai com 0 quando está tudo certo; senão, com o número de erros (no máximo 255).
cd "$(dirname "$0")/.." || exit 1
README=README.md
[ -f "$README" ] || { echo "README.md não encontrado"; exit 1; }

erros=0
falha() {
    echo "ERRO: $1"
    erros=$((erros + 1))
}

# Linha (número) de cada âncora [REQ:ID] num arquivo.
ancoras() { grep -n "\[REQ:$2\]" "$1" 2>/dev/null | cut -d: -f1; }

perto() { # $1 = linha citada, $2.. = linhas de âncora
    citada=$1
    shift
    for a in "$@"; do
        d=$((citada - a))
        [ "$d" -lt 0 ] && d=$((-d))
        [ "$d" -le 3 ] && return 0
    done
    return 1
}

# ------------------------------------------------------------------ 1. tabelas
tmp_linhas=$(mktemp 2>/dev/null || echo /tmp/readme_check_$$)
grep -n '^|.*REQ:' "$README" > "$tmp_linhas"
total_linhas=0
while IFS= read -r registro; do
    total_linhas=$((total_linhas + 1))
    num=${registro%%:*}
    linha=${registro#*:}
    ids=$(printf '%s\n' "$linha" | grep -o 'REQ:[A-Za-z0-9_-]*' | sed 's/REQ://' | sort -u)
    links=$(printf '%s\n' "$linha" | grep -o '([^)( ]*#L[0-9][0-9]*[^)( ]*)' | tr -d '()' | sort -u)
    [ -z "$links" ] && { falha "README:$num cita $ids sem link de arquivo com linha"; continue; }
    for id in $ids; do
        achou=0
        for link in $links; do
            arquivo=${link%%#*}
            inicio=$(printf '%s' "${link#*#L}" | sed 's/[^0-9].*//')
            [ -f "$arquivo" ] || continue
            if perto "$inicio" $(ancoras "$arquivo" "$id"); then achou=1; fi
        done
        [ "$achou" -eq 1 ] || falha "README:$num: âncora [REQ:$id] não está a até 3 linhas de nenhum link da linha"
    done
    for link in $links; do
        arquivo=${link%%#*}
        inicio=$(printf '%s' "${link#*#L}" | sed 's/[^0-9].*//')
        [ -f "$arquivo" ] || { falha "README:$num: arquivo $arquivo não existe"; continue; }
        ok=0
        for id in $ids; do
            if perto "$inicio" $(ancoras "$arquivo" "$id"); then ok=1; fi
        done
        [ "$ok" -eq 1 ] || falha "README:$num: o link $link não tem nenhuma âncora da linha por perto"
    done
done < "$tmp_linhas"
rm -f "$tmp_linhas"

# ------------------------------------------------------------------ 2. IDs obrigatórios
OBRIGATORIOS="RF1 RF2 RF3 RF4 RF5 RF6 RNF1 RNF2 RNF3 RNF4 RNF5 RNF6
MAP-ruidos FILTER-ruidos FOLD-ruidos MAP-particulas FILTER-particulas MAP-rachaduras FOLD-rachaduras
FOLD-eventos FILTER-eventos FILTER-interativos FOLD-interativos FOLD-paredes
PURA-regras_nova_posicao PURA-regras_criatura_ve_jogador PURA-regras_ruido_percebido
PURA-regras_proximo_estado_criatura PURA-regras_placar PURA-regras_integridade_vidro
PURA-regras_avaliar_circuito PURA-regras_proxima_fase"
for id in $OBRIGATORIOS; do
    grep -qE "REQ:$id([^A-Za-z0-9_-]|$)" "$README" || falha "ID obrigatório ausente do README: $id"
done

# ------------------------------------------------------------------ 3. trechos de código
blocos=$(awk '
    /^```c[ ]*$/ { dentro = 1; primeira = 1; next }
    /^```/ && dentro { dentro = 0; next }
    dentro && primeira { primeira = 0; if (match($0, /\/\/ [^ ]+#L[0-9]+-L[0-9]+/)) print NR ":" substr($0, RSTART + 3, RLENGTH - 3); next }
' "$README")
for bloco in $blocos; do
    num=${bloco%%:*}
    ref=${bloco#*:}
    arquivo=${ref%%#*}
    faixa=${ref#*#L}
    ini=${faixa%%-L*}
    fim=${faixa#*-L}
    [ -f "$arquivo" ] || { falha "README:$num: trecho de $arquivo, que não existe"; continue; }
    esperado=$(sed -n "${ini},${fim}p" "$arquivo")
    copiado=$(awk -v n="$num" 'NR > n && /^```/ { exit } NR > n { print }' "$README")
    [ "$esperado" = "$copiado" ] || falha "README:$num: o trecho não é cópia exata de $arquivo#L$ini-L$fim"
done

echo "readme-check: $total_linhas linhas de tabela, $(printf '%s\n' "$blocos" | grep -c ':') trechos de código, $erros erro(s)"
[ "$erros" -gt 255 ] && erros=255
exit "$erros"
