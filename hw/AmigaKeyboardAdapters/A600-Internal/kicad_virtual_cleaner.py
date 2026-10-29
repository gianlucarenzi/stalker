#!/usr/bin/env python3
"""
KiCad Virtual Component Cleaner
===============================

Script principale per rimuovere automaticamente gli oggetti con attributo 'virtual',
tag 'fiducial' e oggetti solo su layer di serigrafia dai file CSV di posizionamento 
componenti (CPL) basandosi sul file .kicad_pcb

Autore: Script automatico per pulizia componenti virtuali
Data: 2025-10-27
"""

import re
import csv
import os
import shutil
import sys
import argparse
from datetime import datetime
from pathlib import Path


class KiCadVirtualCleaner:
    """Classe principale per la pulizia degli oggetti virtuali"""
    
    def __init__(self, verbose=True):
        self.verbose = verbose
        self.virtual_references = []
        self.fiducial_references = []
        self.silkscreen_only_references = []
        self.all_excluded_references = []
        self.processed_files = []
        
    def log(self, message, level="INFO"):
        """Log dei messaggi con timestamp"""
        if self.verbose:
            timestamp = datetime.now().strftime("%H:%M:%S")
            print(f"[{timestamp}] {level}: {message}")
    
    def extract_excluded_references(self, kicad_pcb_file):
        """
        Estrae i riferimenti degli oggetti da escludere dal file .kicad_pcb:
        - Oggetti con attributo 'virtual'
        - Oggetti con tag 'fiducial'
        - Oggetti che esistono solo sui layer F.SilkS o B.SilkS
        """
        self.log(f"Analizzando il file {kicad_pcb_file}...")
        virtual_references = []
        fiducial_references = []
        silkscreen_only_references = []
        
        if not os.path.exists(kicad_pcb_file):
            self.log(f"Errore: File {kicad_pcb_file} non trovato", "ERROR")
            return [], [], []
        
        try:
            with open(kicad_pcb_file, 'r', encoding='utf-8') as f:
                content = f.read()
            
            lines = content.split('\n')
            i = 0
            
            while i < len(lines):
                line = lines[i].strip()
                
                # Se troviamo un modulo, analizziamo il suo contenuto
                if line.startswith('(module '):
                    module_content = []
                    paren_count = 0
                    
                    # Raccogliamo tutto il contenuto del modulo
                    j = i
                    while j < len(lines):
                        current_line = lines[j]
                        module_content.append(current_line)
                        
                        # Contiamo le parentesi per trovare la fine del modulo
                        paren_count += current_line.count('(') - current_line.count(')')
                        
                        if j > i and paren_count == 0:
                            break
                        j += 1
                    
                    # Analizziamo il modulo
                    module_text = '\n'.join(module_content)
                    reference = None
                    
                    # Cerchiamo fp_text reference in questo modulo
                    for line_content in module_content:
                        if 'fp_text reference' in line_content:
                            # Estraiamo il riferimento
                            match = re.search(r'fp_text reference\s+(\S+)', line_content)
                            if match:
                                reference = match.group(1)
                                break
                    
                    if reference:
                        ref_upper = reference.upper()
                        # Verifica se ha attributo virtual
                        if '(attr virtual)' in module_text:
                            virtual_references.append(reference)
                            self.log(f"Trovato oggetto virtuale: {reference}")
                        
                        # Verifica se ha tag fiducial
                        elif '(tags fiducial)' in module_text or 'fiducial' in module_text.lower():
                            fiducial_references.append(reference)
                            self.log(f"Trovato oggetto fiducial: {reference}")
                        
                        # Rileva LOGO su serigrafia (F.SilkS o B.SilkS)
                        elif 'LOGO' in ref_upper and self._is_module_on_silkscreen(module_text):
                            silkscreen_only_references.append(reference)
                            self.log(f"Trovato LOGO su serigrafia: {reference}")
                        
                        # Verifica se esiste solo sui layer di serigrafia
                        elif self._is_silkscreen_only_module(module_text):
                            silkscreen_only_references.append(reference)
                            self.log(f"Trovato oggetto solo su serigrafia: {reference}")
                    
                    i = j + 1
                else:
                    i += 1
                    
        except Exception as e:
            self.log(f"Errore durante la lettura del file: {e}", "ERROR")
            return [], [], []
        
        self.virtual_references = virtual_references
        self.fiducial_references = fiducial_references
        self.silkscreen_only_references = silkscreen_only_references
        
        # Combina tutti i riferimenti da escludere
        all_excluded = virtual_references + fiducial_references + silkscreen_only_references
        self.all_excluded_references = list(set(all_excluded))  # Rimuovi duplicati
        
        self.log(f"Trovati {len(virtual_references)} oggetti virtuali")
        self.log(f"Trovati {len(fiducial_references)} oggetti fiducial")
        self.log(f"Trovati {len(silkscreen_only_references)} oggetti solo su serigrafia")
        self.log(f"Totale oggetti da escludere: {len(self.all_excluded_references)}")
        
        return virtual_references, fiducial_references, silkscreen_only_references
    
    def _is_silkscreen_only_module(self, module_text):
        """
        Verifica se un modulo esiste solo sui layer di serigrafia (F.SilkS o B.SilkS)
        """
        # Cerca tutti i layer menzionati nel modulo
        layer_pattern = r'\(layer\s+([^)]+)\)'
        layers = re.findall(layer_pattern, module_text)
        
        # Filtra solo i layer fisici (esclude Fab, SilkS, ecc.)
        physical_layers = []
        for layer in layers:
            layer = layer.strip()
            # Layer fisici tipici di KiCad
            if layer in ['F.Cu', 'B.Cu', 'In1.Cu', 'In2.Cu', 'In3.Cu', 'In4.Cu']:
                physical_layers.append(layer)
        
        # Se non ci sono layer fisici, probabilmente è solo grafico
        if not physical_layers:
            # Verifica se ha solo layer di serigrafia
            silkscreen_layers = [layer for layer in layers if 'SilkS' in layer]
            if silkscreen_layers:
                return True
        
        return False

    def _is_module_on_silkscreen(self, module_text):
        """Ritorna True se il modulo menziona F.SilkS o B.SilkS in qualunque elemento"""
        layer_pattern = r'\(layer\s+([^)]+)\)'
        layers = re.findall(layer_pattern, module_text)
        for layer in layers:
            if 'SilkS' in layer:
                return True
        return False
    
    def create_backup(self, file_path):
        """Crea un backup del file con timestamp"""
        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        backup_path = f"{file_path}.backup_{timestamp}"
        shutil.copy2(file_path, backup_path)
        self.log(f"Backup creato: {backup_path}")
        return backup_path
    
    def clean_csv_file(self, csv_file, excluded_references):
        """
        Rimuove le righe corrispondenti ai riferimenti esclusi dal file CSV
        preservando esattamente il formato originale
        """
        if not os.path.exists(csv_file):
            self.log(f"File {csv_file} non trovato, saltato", "WARNING")
            return False, 0
        
        self.log(f"Processando {csv_file}...")
        
        # Crea backup
        backup_path = self.create_backup(csv_file)
        
        lines_to_keep = []
        removed_count = 0
        removed_items = []
        
        try:
            # Leggi il file riga per riga per preservare il formato originale
            with open(csv_file, 'r', encoding='utf-8') as f:
                lines = f.readlines()
            
            # Processa ogni riga
            for i, line in enumerate(lines):
                line = line.rstrip('\n\r')  # Rimuovi solo i caratteri di fine riga
                
                if i == 0:  # Header
                    lines_to_keep.append(line)
                    continue
                
                if not line.strip():  # Riga vuota
                    continue
                
                # Estrai il designator dalla riga
                # Gestisce sia formato con virgolette che senza
                if line.startswith('"'):
                    # Formato con virgolette: "DESIGNATOR","..."
                    end_quote = line.find('"', 1)
                    if end_quote > 0:
                        designator = line[1:end_quote]
                    else:
                        designator = line.split(',')[0].strip('"')
                else:
                    # Formato senza virgolette: DESIGNATOR,...
                    designator = line.split(',')[0]
                
                if designator in excluded_references:
                    # Determina il tipo di oggetto rimosso
                    obj_type = "virtuale"
                    if designator in self.fiducial_references:
                        obj_type = "fiducial"
                    elif designator in self.silkscreen_only_references:
                        obj_type = "solo serigrafia"
                    
                    self.log(f"  Rimossa riga per oggetto {obj_type}: {designator}")
                    removed_count += 1
                    removed_items.append(f"{designator} ({obj_type})")
                else:
                    lines_to_keep.append(line)
            
            # Scrivi il file preservando il formato originale
            with open(csv_file, 'w', encoding='utf-8') as f:
                for line in lines_to_keep:
                    f.write(line + '\n')
            
            self.log(f"File {csv_file} aggiornato: rimosse {removed_count} righe")
            
            # Salva informazioni per il report
            file_info = {
                'name': csv_file,
                'backup': backup_path,
                'removed_count': removed_count,
                'removed_items': removed_items,
                'original_lines': len(lines_to_keep) + removed_count,
                'final_lines': len(lines_to_keep)
            }
            self.processed_files.append(file_info)
            
            return True, removed_count
            
        except Exception as e:
            self.log(f"Errore durante l'elaborazione di {csv_file}: {e}", "ERROR")
            # Ripristina il backup in caso di errore
            if os.path.exists(backup_path):
                shutil.copy2(backup_path, csv_file)
                self.log(f"File ripristinato dal backup", "INFO")
            return False, 0
    
    def find_csv_files(self, base_name=None, directory="."):
        """
        Trova automaticamente i file CSV CPL nella directory
        """
        csv_files = []
        
        if base_name:
            # Cerca file con pattern specifico
            patterns = [
                f"{base_name}_cpl_top.csv",
                f"{base_name}_cpl_bot.csv",
                f"{base_name}_cpl_bottom.csv"
            ]
            
            for pattern in patterns:
                file_path = os.path.join(directory, pattern)
                if os.path.exists(file_path):
                    csv_files.append(file_path)
        else:
            # Cerca tutti i file CSV che contengono "cpl"
            for file in os.listdir(directory):
                if file.lower().endswith('.csv') and 'cpl' in file.lower():
                    csv_files.append(os.path.join(directory, file))
        
        return csv_files
    
    def find_pcb_file(self, directory="."):
        """
        Trova automaticamente il file .kicad_pcb nella directory
        """
        pcb_files = [f for f in os.listdir(directory) if f.endswith('.kicad_pcb')]
        
        if len(pcb_files) == 1:
            return pcb_files[0]
        elif len(pcb_files) > 1:
            self.log(f"Trovati più file .kicad_pcb: {pcb_files}", "WARNING")
            return None
        else:
            self.log("Nessun file .kicad_pcb trovato", "ERROR")
            return None
    
    def generate_report(self):
        """Genera un report dettagliato dell'operazione"""
        report_lines = []
        report_lines.append("=" * 80)
        report_lines.append("REPORT PULIZIA COMPONENTI KICAD")
        report_lines.append("=" * 80)
        report_lines.append(f"Data e ora: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        report_lines.append("")
        
        # Oggetti trovati per categoria
        report_lines.append("OGGETTI IDENTIFICATI PER CATEGORIA:")
        report_lines.append("-" * 40)
        
        if self.virtual_references:
            report_lines.append(f"OGGETTI VIRTUALI ({len(self.virtual_references)}):")
            for i, obj in enumerate(self.virtual_references, 1):
                report_lines.append(f"  {i:2d}. {obj}")
            report_lines.append("")
        
        if self.fiducial_references:
            report_lines.append(f"OGGETTI FIDUCIAL ({len(self.fiducial_references)}):")
            for i, obj in enumerate(self.fiducial_references, 1):
                report_lines.append(f"  {i:2d}. {obj}")
            report_lines.append("")
        
        if self.silkscreen_only_references:
            report_lines.append(f"OGGETTI SOLO SU SERIGRAFIA ({len(self.silkscreen_only_references)}):")
            for i, obj in enumerate(self.silkscreen_only_references, 1):
                report_lines.append(f"  {i:2d}. {obj}")
            report_lines.append("")
        
        report_lines.append(f"TOTALE OGGETTI DA ESCLUDERE: {len(self.all_excluded_references)}")
        report_lines.append("")
        
        # File processati
        report_lines.append("FILE PROCESSATI:")
        report_lines.append("-" * 40)
        total_removed = 0
        
        for file_info in self.processed_files:
            report_lines.append(f"File: {file_info['name']}")
            report_lines.append(f"  - Righe originali: {file_info['original_lines']}")
            report_lines.append(f"  - Righe rimosse: {file_info['removed_count']}")
            report_lines.append(f"  - Righe finali: {file_info['final_lines']}")
            report_lines.append(f"  - Backup: {file_info['backup']}")
            
            if file_info['removed_items']:
                report_lines.append(f"  - Oggetti rimossi: {', '.join(file_info['removed_items'])}")
            
            report_lines.append("")
            total_removed += file_info['removed_count']
        
        # Riepilogo
        report_lines.append("RIEPILOGO:")
        report_lines.append("-" * 40)
        report_lines.append(f"Totale oggetti virtuali: {len(self.virtual_references)}")
        report_lines.append(f"Totale oggetti fiducial: {len(self.fiducial_references)}")
        report_lines.append(f"Totale oggetti solo serigrafia: {len(self.silkscreen_only_references)}")
        report_lines.append(f"Totale righe rimosse: {total_removed}")
        report_lines.append(f"File processati: {len(self.processed_files)}")
        report_lines.append("")
        report_lines.append("OPERAZIONE COMPLETATA CON SUCCESSO!")
        report_lines.append("=" * 80)
        
        return "\n".join(report_lines)
    
    def clean_project(self, csv_files, kicad_pcb_file):
        """
        Metodo principale per pulire un progetto KiCad
        """
        self.log("Avvio pulizia componenti KiCad (virtuali, fiducial, solo serigrafia)")
        
        # Verifica che il file PCB esista
        if not os.path.exists(kicad_pcb_file):
            self.log(f"File PCB non trovato: {kicad_pcb_file}", "ERROR")
            return False
        
        # Estrai riferimenti da escludere
        virtual_refs, fiducial_refs, silkscreen_refs = self.extract_excluded_references(kicad_pcb_file)
        
        if not self.all_excluded_references:
            # Niente da togliere: i CSV vanno già bene, non è un errore
            self.log("Nessun oggetto da escludere trovato. Operazione terminata.")
            return True
        
        # Verifica che i file CSV esistano
        existing_csv_files = []
        for csv_file in csv_files:
            if os.path.exists(csv_file):
                existing_csv_files.append(csv_file)
            else:
                self.log(f"File CSV non trovato: {csv_file}", "WARNING")
        
        if not existing_csv_files:
            self.log("Nessun file CSV valido trovato", "ERROR")
            return False
        
        self.log(f"File CSV da processare: {existing_csv_files}")
        
        # Processa ogni file CSV
        success_count = 0
        for csv_file in existing_csv_files:
            success, removed = self.clean_csv_file(csv_file, self.all_excluded_references)
            if success:
                success_count += 1
        
        # Genera e mostra report
        report = self.generate_report()
        print("\n" + report)
        
        # Salva report su file
        report_file = f"kicad_cleanup_report_{datetime.now().strftime('%Y%m%d_%H%M%S')}.txt"
        with open(report_file, 'w', encoding='utf-8') as f:
            f.write(report)
        self.log(f"Report salvato in: {report_file}")
        
        return success_count > 0


def main():
    """Funzione principale con interfaccia a riga di comando"""
    parser = argparse.ArgumentParser(
        description="Rimuove automaticamente oggetti virtuali, fiducial e solo-serigrafia dai file CSV CPL di KiCad",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Esempi di utilizzo:
  python kicad_virtual_cleaner.py file1.csv file2.csv                    # Specifica file CSV, auto-rileva PCB
  python kicad_virtual_cleaner.py file1.csv --pcb progetto.kicad_pcb     # Specifica CSV e PCB
  python kicad_virtual_cleaner.py *.csv --pcb progetto.kicad_pcb         # Tutti i CSV con PCB specifico
  python kicad_virtual_cleaner.py --auto                                 # Auto-rileva tutto

Oggetti rimossi automaticamente:
  - Oggetti con attributo 'virtual'
  - Oggetti con tag 'fiducial'
  - Oggetti che esistono solo sui layer F.SilkS o B.SilkS
        """
    )
    
    parser.add_argument('csv_files', nargs='*',
                       help='File CSV da pulire (uno o più file)')
    parser.add_argument('-p', '--pcb', 
                       help='File .kicad_pcb da analizzare (default: auto-rileva)')
    parser.add_argument('-q', '--quiet', action='store_true',
                       help='Modalità silenziosa (meno output)')
    parser.add_argument('--auto', action='store_true',
                       help='Auto-rileva tutti i file (CSV e PCB)')
    
    args = parser.parse_args()
    
    # Inizializza cleaner
    cleaner = KiCadVirtualCleaner(verbose=not args.quiet)
    
    # Determina file CSV
    csv_files = args.csv_files
    
    if args.auto or not csv_files:
        # Auto-rileva file CSV
        auto_csv = cleaner.find_csv_files()
        if auto_csv:
            if not csv_files:
                csv_files = auto_csv
                cleaner.log(f"File CSV auto-rilevati: {csv_files}")
            else:
                cleaner.log(f"File CSV specificati: {csv_files}")
        else:
            cleaner.log("Nessun file CSV trovato per auto-rilevamento", "ERROR")
            if not csv_files:
                sys.exit(1)
    
    if not csv_files:
        print("Errore: Nessun file CSV specificato")
        print("Usa --auto per auto-rilevamento o specifica i file CSV come argomenti")
        sys.exit(1)
    
    # Determina file PCB
    pcb_file = args.pcb
    if not pcb_file:
        pcb_file = cleaner.find_pcb_file()
        if pcb_file:
            cleaner.log(f"File PCB auto-rilevato: {pcb_file}")
        else:
            print("Errore: Nessun file .kicad_pcb trovato o specificato")
            print("Usa -p per specificare il file PCB")
            sys.exit(1)
    
    # Verifica che il file PCB esista
    if not os.path.exists(pcb_file):
        print(f"Errore: File PCB non trovato: {pcb_file}")
        sys.exit(1)
    
    # Esegui pulizia
    success = cleaner.clean_project(csv_files, pcb_file)
    
    if success:
        print("\n✓ Operazione completata con successo!")
        sys.exit(0)
    else:
        print("\n✗ Operazione fallita")
        sys.exit(1)


if __name__ == "__main__":
    main()