#!/bin/bash
#
# Script Bash per automatizzare la pulizia dei componenti virtuali KiCad
# Wrapper semplificato per kicad_virtual_cleaner.py
#
# Utilizzo:
#   ./clean_virtual.sh file1.csv file2.csv                    # Specifica file CSV, auto-rileva PCB
#   ./clean_virtual.sh file1.csv --pcb progetto.kicad_pcb     # Specifica CSV e PCB
#   ./clean_virtual.sh --auto                                 # Auto-rileva tutto
#   ./clean_virtual.sh -h                                     # Mostra aiuto
#

set -e  # Esce in caso di errore

# Colori per output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Funzione per stampare messaggi colorati
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Funzione di aiuto
show_help() {
    cat << EOF
KiCad Virtual Component Cleaner - Script Bash
==============================================

Rimuove automaticamente gli oggetti con attributo 'virtual' dai file CSV CPL di KiCad.

UTILIZZO:
    $0 [OPZIONI] [FILE_CSV...]

PARAMETRI:
    FILE_CSV...         Uno o più file CSV da pulire

OPZIONI:
    -h, --help          Mostra questo aiuto
    -p, --pcb FILE      Specifica il file .kicad_pcb (default: auto-rileva)
    -q, --quiet         Modalità silenziosa
    -v, --verbose       Modalità verbosa (default)
    --auto              Auto-rileva tutti i file (CSV e PCB)
    --dry-run           Simula l'operazione senza modificare i file

ESEMPI:
    $0 file1.csv file2.csv                    # Specifica file CSV, auto-rileva PCB
    $0 file1.csv --pcb progetto.kicad_pcb     # Specifica CSV e PCB
    $0 *.csv --pcb progetto.kicad_pcb         # Tutti i CSV con PCB specifico
    $0 --auto                                 # Auto-rileva tutto
    $0 --quiet file1.csv                     # Modalità silenziosa
    $0 --dry-run file1.csv file2.csv         # Simula senza modificare

REQUISITI:
    - Python 3.x
    - File kicad_virtual_cleaner.py nella stessa directory

EOF
}

# Variabili default
QUIET=false
DRY_RUN=false
AUTO=false
PCB_FILE=""
CSV_FILES=()
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CLEANER_SCRIPT="$SCRIPT_DIR/kicad_virtual_cleaner.py"

# Parsing argomenti
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -q|--quiet)
            QUIET=true
            shift
            ;;
        -v|--verbose)
            QUIET=false
            shift
            ;;
        -p|--pcb)
            if [[ -n "$2" && "$2" != -* ]]; then
                PCB_FILE="$2"
                shift 2
            else
                print_error "Opzione --pcb richiede un argomento"
                exit 1
            fi
            ;;
        --auto)
            AUTO=true
            shift
            ;;
        --dry-run)
            DRY_RUN=true
            shift
            ;;
        -*)
            print_error "Opzione sconosciuta: $1"
            echo "Usa -h per vedere l'aiuto"
            exit 1
            ;;
        *)
            # Assume che sia un file CSV
            CSV_FILES+=("$1")
            shift
            ;;
    esac
done

# Verifica che Python sia disponibile
if ! command -v python3 &> /dev/null; then
    print_error "Python 3 non trovato. Installa Python 3 per continuare."
    exit 1
fi

# Verifica che lo script Python esista
if [[ ! -f "$CLEANER_SCRIPT" ]]; then
    print_error "Script kicad_virtual_cleaner.py non trovato in: $CLEANER_SCRIPT"
    print_info "Assicurati che il file sia nella stessa directory di questo script."
    exit 1
fi

# Banner
if [[ "$QUIET" != true ]]; then
    echo "=================================================="
    echo "  KiCad Virtual Component Cleaner"
    echo "=================================================="
    echo
fi

# Verifica directory di lavoro
if [[ "$QUIET" != true ]]; then
    print_info "Directory di lavoro: $(pwd)"
fi

# Verifica parametri
if [[ "$AUTO" != true && ${#CSV_FILES[@]} -eq 0 ]]; then
    print_error "Nessun file CSV specificato"
    echo "Usa --auto per auto-rilevamento o specifica i file CSV come argomenti"
    echo "Esempio: $0 file1.csv file2.csv"
    echo "Usa -h per vedere tutti gli esempi"
    exit 1
fi

# Prepara argomenti per lo script Python
PYTHON_ARGS=()

# Aggiungi file CSV se specificati
if [[ ${#CSV_FILES[@]} -gt 0 ]]; then
    for csv_file in "${CSV_FILES[@]}"; do
        PYTHON_ARGS+=("$csv_file")
    done
fi

# Aggiungi file PCB se specificato
if [[ -n "$PCB_FILE" ]]; then
    PYTHON_ARGS+=("--pcb" "$PCB_FILE")
fi

# Aggiungi opzioni
if [[ "$QUIET" == true ]]; then
    PYTHON_ARGS+=("--quiet")
fi

if [[ "$AUTO" == true ]]; then
    PYTHON_ARGS+=("--auto")
fi

# Modalità dry-run (simulazione)
if [[ "$DRY_RUN" == true ]]; then
    print_warning "MODALITÀ DRY-RUN: Nessun file sarà modificato"
    echo
    print_info "Comando che sarebbe eseguito:"
    echo "python3 \"$CLEANER_SCRIPT\" ${PYTHON_ARGS[*]}"
    echo
    print_info "Per eseguire realmente l'operazione, rimuovi --dry-run"
    exit 0
fi

# Mostra informazioni sui file da processare
if [[ "$QUIET" != true ]]; then
    if [[ ${#CSV_FILES[@]} -gt 0 ]]; then
        print_info "File CSV specificati: ${CSV_FILES[*]}"
    fi
    if [[ -n "$PCB_FILE" ]]; then
        print_info "File PCB specificato: $PCB_FILE"
    fi
    if [[ "$AUTO" == true ]]; then
        print_info "Modalità auto-rilevamento attivata"
    fi
    echo
fi

# Esegui lo script Python
if [[ "$QUIET" != true ]]; then
    print_info "Avvio pulizia componenti virtuali..."
    echo
fi

if python3 "$CLEANER_SCRIPT" "${PYTHON_ARGS[@]}"; then
    echo
    print_success "Operazione completata con successo!"
    
    if [[ "$QUIET" != true ]]; then
        echo
        print_info "File di backup creati automaticamente con timestamp"
        print_info "Controlla i file CSV per verificare le modifiche"
        
        # Mostra file di report generati
        REPORT_FILES=(kicad_cleanup_report_*.txt)
        if [[ ${#REPORT_FILES[@]} -gt 0 && -f "${REPORT_FILES[0]}" ]]; then
            LATEST_REPORT=$(ls -t kicad_cleanup_report_*.txt 2>/dev/null | head -1)
            if [[ -n "$LATEST_REPORT" ]]; then
                print_info "Report dettagliato salvato in: $LATEST_REPORT"
            fi
        fi
    fi
    
    exit 0
else
    echo
    print_error "Operazione fallita!"
    exit 1
fi