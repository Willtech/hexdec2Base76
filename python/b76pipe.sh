#!/bin/ksh
#
#  b76pipe — Base76 7‑bit binary pipeline encoder/decoder
#
#  PIPELINE (forward)
#    1. ASCII message
#    2. ASCII → hex → Base76        (hex2base76.py)
#    3. Base76 char → index (0–75)  (base76todec.py)
#    4. index → 7‑bit binary
#    5. 7‑bit bits → ASCII '0'/'1'
#    6. '0'/'1' → ASCII 48/49 → DEC → Base76 (dec2base76.py)
#
#  PIPELINE (reverse, -r)
#    1. Base76 → DEC                (base76todec.py)
#    2. DEC 48/49 → ASCII '0'/'1'
#    3. '0'/'1' → 7‑bit groups
#    4. 7‑bit → index (0–75)
#    5. index → Base76 char         (dec2base76.py)
#    6. Base76 → hex → ASCII        (base76tohex.py)
#
#------------------------------------------------------------------------------
#  AUTHOR
#      Graduate. Damian Williamson
#      Willtech, Swan Hill, Victoria, Australia.
#
#  AI COLLABORATION
#      Microsoft Copilot — pipeline design, commentary, and integration sketch.
#------------------------------------------------------------------------------

PYTHON="${PYTHON:-python3}"

# Adjust these paths to match your repo layout
DEC2B76="./dec2base76.py"
B76TODEC="./base76todec.py"
HEX2B76="./hex2base76.py"
B76TOHEX="./base76tohex.py"

usage() {
    echo "Usage: $0 [-r] [-i infile] [-o outfile] [message]" >&2
    exit 1
}

reverse=0
infile=""
outfile=""

# Parse flags
while getopts "ri:o:" opt; do
    case "$opt" in
        r) reverse=1 ;;
        i) infile="$OPTARG" ;;
        o) outfile="$OPTARG" ;;
        *) usage ;;
    esac
done
shift $((OPTIND - 1))

# Read input message
if [ -n "$infile" ]; then
    msg=$(cat "$infile")
else
    msg="$*"
fi

#----------------------------- Python wrappers -------------------------------#

dec2b76() {
    "$PYTHON" "$DEC2B76" "$1"
}

b76todec() {
    "$PYTHON" "$B76TODEC" "$1"
}

ascii_to_b76() {
    hex=$(printf "%s" "$1" | hexdump -ve '1/1 "%02x"')
    "$PYTHON" "$HEX2B76" "$hex"
}

b76_to_ascii() {
    hex=$("$PYTHON" "$B76TOHEX" "$1")
    printf "%b" "$(echo "$hex" | sed 's/../\\x&/g')"
}

#----------------------------- Forward pipeline ------------------------------#

if [ "$reverse" -eq 0 ]; then

    #
    # 1. ASCII → Base76
    #
    b76_1=$(ascii_to_b76 "$msg")

    #
    # 2. Base76 → 7‑bit binary
    #
    bin=""
    i=1
    len=${#b76_1}
    while [ "$i" -le "$len" ]; do
        c=$(printf "%s" "$b76_1" | cut -c "$i")
        idx=$(b76todec "$c")
        bits=$(printf "%07d" "$(echo "obase=2;$idx" | bc)")
        bin="${bin}${bits}"
        i=$((i + 1))
    done

    #
    # 3. 7‑bit binary → ASCII '0'/'1'
    #
    ascii01=""
    i=1
    blen=${#bin}
    while [ "$i" -le "$blen" ]; do
        bit=$(printf "%s" "$bin" | cut -c "$i")
        if [ "$bit" = "0" ]; then
            ascii01="${ascii01}0"
        else
            ascii01="${ascii01}1"
        fi
        i=$((i + 1))
    done

    #
    # 4. ASCII '0'/'1' → ASCII codes 48/49 → DEC → Base76
    #
    out=""
    i=1
    alen=${#ascii01}
    while [ "$i" -le "$alen" ]; do
        ch=$(printf "%s" "$ascii01" | cut -c "$i")
        if [ "$ch" = "0" ]; then
            out="${out}$(dec2b76 48)"
        else
            out="${out}$(dec2b76 49)"
        fi
        i=$((i + 1))
    done

#----------------------------- Reverse pipeline ------------------------------#

else
    encoded="$msg"

    #
    # 1. Base76 → DEC → ASCII '0'/'1'
    #
    ascii01=""
    i=1
    elen=${#encoded}
    while [ "$i" -le "$elen" ]; do
        c=$(printf "%s" "$encoded" | cut -c "$i")
        dec=$(b76todec "$c")
        case "$dec" in
            48) ascii01="${ascii01}0" ;;
            49) ascii01="${ascii01}1" ;;
            *)  echo "Invalid encoded bit: $c (DEC=$dec)" >&2; exit 1 ;;
        esac
        i=$((i + 1))
    done

    #
    # 2. ASCII '0'/'1' → 7‑bit groups → Base76 index
    #
    bin="$ascii01"
    b76_1=""
    while [ ${#bin} -ge 7 ]; do
        chunk=$(printf "%s" "$bin" | cut -c 1-7)
        bin=$(printf "%s" "$bin" | cut -c 8-)
        idx=$(echo "ibase=2;$chunk" | bc)
        b76_1="${b76_1}$(dec2b76 "$idx")"
    done

    #
    # 3. Base76 → ASCII
    #
    out=$(b76_to_ascii "$b76_1")
fi

#----------------------------- Output handling -------------------------------#

if [ -n "$outfile" ]; then
    printf "%s" "$out" >"$outfile"
else
    printf "%s\n" "$out"
fi
