import csv
import sys

def filtra_csv(input_file, output_file):
    """
    Filtra un file CSV, mantenendo solo le righe con un valore non vuoto
    nella colonna 'LCSC'. La prima riga (intestazione) viene sempre mantenuta.
    """
    try:
        with open(input_file, mode='r', newline='', encoding='utf-8') as infile:
            reader = csv.DictReader(infile)
            fieldnames = reader.fieldnames
            
            # Se la colonna non esiste, solleva un errore
            if "LCSC" not in fieldnames:
                print("Error: 'LCSC' column is missing in the file.")
                return

            filtered_rows = []
            # Itera su ogni riga del file CSV
            for row in reader:
                # Controlla se la colonna 'LCSC' non è vuota
                if row.get("LCSC"):
                    filtered_rows.append(row)

        # Scrivi le righe filtrate in un nuovo file CSV
        with open(output_file, mode='w', newline='', encoding='utf-8') as outfile:
            writer = csv.DictWriter(outfile, fieldnames=fieldnames)
            
            # Scrive l'intestazione
            writer.writeheader()
            
            # Scrive le righe filtrate
            writer.writerows(filtered_rows)
        
        print(f"Operation complete with success! New file is saved as '{output_file}'.")
        print(f"Found and saved {len(filtered_rows)} rows with the 'LCSC' column.")

    except FileNotFoundError:
        print(f"Error: file '{input_file}' was not found.")
    except Exception as e:
        print(f"An error occurs: {e}")

# Questa parte gestisce gli argomenti da riga di comando
if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python filtra_csv.py <input_file.csv> <output_file.csv>")
    else:
        input_csv = sys.argv[1]
        output_csv = sys.argv[2]
        filtra_csv(input_csv, output_csv)
