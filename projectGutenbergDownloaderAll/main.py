import os
import ftplib
import tempfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from threading import Lock

# Imposta il mirror FTP e il percorso principale
FTP_SERVER = "ftp.mirrorservice.org"
FTP_PATH = "/sites/ftp.ibiblio.org/pub/docs/books/gutenberg/"
OUTPUT_FILE = "all_txt_combined.txt"

# Crea un lock per garantire l'accesso esclusivo al file di output
output_lock = Lock()

# Funzione per scaricare tutti i file .txt da una directory specifica (ricorsiva)
def download_txt_from_folder(folder):
    try:
        with ftplib.FTP(FTP_SERVER) as ftp:
            ftp.login()  # Login anonimo
            ftp.cwd(folder)

            # Esplora ricorsivamente la directory e scarica tutti i file .txt
            def explore_and_download(ftp, current_path):
                ftp.cwd(current_path)
                items = ftp.nlst()

                for item in items:
                    try:
                        ftp.cwd(item)  # Verifica se è una directory
                        explore_and_download(ftp, f"{current_path}/{item}")  # Ricorsione per sottodirectory
                        ftp.cwd("..")  # Torna indietro
                    except ftplib.error_perm:
                        # Se non è una directory, verifica se è un file .txt
                        if item.endswith(".txt"):
                            with tempfile.TemporaryFile() as temp_file:
                                ftp.retrbinary(f"RETR {item}", temp_file.write)
                                temp_file.seek(0)
                                file_content = temp_file.read().decode('utf-8', errors='ignore')

                                # Scrivi immediatamente nel file di output
                                with output_lock:
                                    with open(OUTPUT_FILE, "a", encoding="utf-8") as outfile:
                                        outfile.write(file_content)
                                        outfile.write("\n\n" + "="*80 + "\n\n")  # Separatore tra file
                                print(f"Scaricato e scritto: {current_path}/{item}")

            explore_and_download(ftp, folder)  # Avvia l'esplorazione

    except Exception as e:
        print(f"Errore nell'accesso alla directory {folder}: {e}")

# Funzione principale per scaricare tutti i .txt dalle cartelle principali
def download_and_combine_all_txt():
    # Crea un pool di thread per ogni cartella principale (0-9)
    folder_paths = [f"{FTP_PATH}{i}" for i in range(10)]
    with ThreadPoolExecutor(max_workers=len(folder_paths)) as executor:
        # Lancia un thread per ogni cartella principale
        future_to_folder = {executor.submit(download_txt_from_folder, folder): folder for folder in folder_paths}

        for future in as_completed(future_to_folder):
            folder = future_to_folder[future]
            try:
                future.result()  # Attende il completamento del thread
            except Exception as e:
                print(f"Errore nel download dei contenuti della cartella {folder}: {e}")

    print(f"Download e combinazione completati. File salvato come {OUTPUT_FILE}")

# Esegui lo script
download_and_combine_all_txt()
