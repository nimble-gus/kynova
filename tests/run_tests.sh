#!/bin/sh
# Corre todas las pruebas del front-end de Kynova.
#   tests/valid/*.kyn   deben terminar con código 0.
#   tests/invalid/*.kyn deben terminar con código 1 y su salida debe contener
#                       el texto indicado en la primera línea "// ESPERADO: ...".

KYNOVA=${1:-./kynova}
DIR=$(dirname "$0")
pass=0
fail=0

for f in "$DIR"/valid/*.kyn; do
    out=$("$KYNOVA" -q "$f" 2>&1)
    if [ $? -eq 0 ]; then
        echo "  OK    válido    $f"
        pass=$((pass + 1))
    else
        echo "  FALLA válido    $f"
        echo "$out" | sed 's/^/        /'
        fail=$((fail + 1))
    fi
done

for f in "$DIR"/invalid/*.kyn; do
    expected=$(sed -n 's|^// ESPERADO: ||p' "$f" | head -n 1)
    out=$("$KYNOVA" -q "$f" 2>&1)
    code=$?
    if [ $code -eq 1 ] && printf '%s' "$out" | grep -qF -- "$expected"; then
        echo "  OK    inválido  $f"
        pass=$((pass + 1))
    else
        echo "  FALLA inválido  $f (código $code, se esperaba el mensaje: \"$expected\")"
        echo "$out" | sed 's/^/        /'
        fail=$((fail + 1))
    fi
done

echo
echo "Pruebas: $pass OK, $fail fallidas"
[ $fail -eq 0 ]
