#!/usr/bin/env python3
"""
MP3 Cleaner for Helix Decoder
Removes ID3 tags and converts MP3 files to Helix-compatible format:
- 44100 Hz sample rate
- Stereo (2 channels)
- 128 kbps bitrate
- No metadata (ID3v1, ID3v2)
"""

import os
import subprocess
import sys
from pathlib import Path

def get_data_dir():
    """Encontra a pasta data (um nível acima da pasta tools)"""
    script_dir = Path(__file__).parent
    data_dir = script_dir.parent / 'data'

    if not data_dir.exists():
        print(f"Error: 'data' folder not found at {data_dir}")
        print("Please ensure your folder structure is:")
        print("  sketch/")
        print("    ├── your_sketch.ino")
        print("    ├── tools/")
        print("    │   └── clean_mp3.py")
        print("    └── data/")
        print("        └── *.mp3")
        return None
    return data_dir

def check_ffmpeg():
    """Verifica se ffmpeg está instalado"""
    try:
        result = subprocess.run(['ffmpeg', '-version'],
                               capture_output=True, check=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def clean_mp3_file(input_file, output_file=None):
    """
    Limpa um arquivo MP3 para formato compatível com Helix
    """
    if output_file is None:
        output_file = input_file

    temp_file = input_file.with_suffix('.tmp.mp3')

    # Comando ffmpeg para remover TODOS os metadados e converter para formato padrão
    cmd = [
        'ffmpeg',
        '-i', str(input_file),
        '-acodec', 'libmp3lame',
        '-ar', '44100',           # Taxa de amostragem
        '-ac', '2',               # Estéreo
        '-b:a', '128k',           # Bitrate
        '-map_metadata', '-1',    # Remove metadados
        '-write_id3v1', '0',      # Não escreve ID3v1
        '-id3v2_version', '0',    # Não escreve ID3v2
        '-fflags', '+bitexact',   # Modo exato
        '-flags', '+bitexact',
        '-y',                     # Sobrescreve
        str(temp_file)
    ]

    try:
        # Executa conversão
        result = subprocess.run(cmd, capture_output=True, text=True)

        if result.returncode != 0:
            print(f"  ✗ ffmpeg error: {result.stderr[:200]}")
            if temp_file.exists():
                temp_file.unlink()
            return False

        # Verifica se o arquivo resultante começa com frame MP3 válido (0xFF)
        with open(temp_file, 'rb') as f:
            header = f.read(4)
            if not header or header[0] != 0xFF:
                # Se ainda tem ID3, pula manualmente
                with open(temp_file, 'rb') as f:
                    data = f.read()

                # Remove tag ID3 se existir
                if len(data) > 10 and data[:3] == b'ID3':
                    size = ((data[6] & 0x7F) << 21) | \
                           ((data[7] & 0x7F) << 14) | \
                           ((data[8] & 0x7F) << 7) | \
                           (data[9] & 0x7F)
                    tag_size = size + 10
                    data = data[tag_size:]

                    # Escreve arquivo limpo
                    with open(temp_file, 'wb') as f_out:
                        f_out.write(data)

        # Substitui o original
        if output_file == input_file:
            input_file.unlink()
        temp_file.rename(output_file)
        return True

    except Exception as e:
        print(f"  ✗ Exception: {e}")
        if temp_file.exists():
            temp_file.unlink()
        return False

def main():
    print("=" * 60)
    print("MP3 Cleaner for Helix Decoder")
    print("Removes ID3 tags and converts to Helix-compatible format")
    print("=" * 60)

    # Verifica ffmpeg
    if not check_ffmpeg():
        print("\n✗ Error: ffmpeg is not installed or not in PATH")
        print("\nInstall ffmpeg:")
        print("  - Windows: https://ffmpeg.org/download.html")
        print("  - Mac: brew install ffmpeg")
        print("  - Linux: sudo apt install ffmpeg")
        sys.exit(1)

    # Encontra pasta data
    data_dir = get_data_dir()
    if not data_dir:
        sys.exit(1)

    # Encontra arquivos MP3
    mp3_files = list(data_dir.glob('*.mp3')) + list(data_dir.glob('*.MP3'))

    if not mp3_files:
        print(f"\nNo MP3 files found in {data_dir}")
        sys.exit(0)

    print(f"\nFound {len(mp3_files)} MP3 file(s) in: {data_dir}")
    print("-" * 60)

    # Mostra arquivos
    for f in mp3_files:
        print(f"  • {f.name}")

    # Confirmação
    print("\n" + "=" * 60)
    print("WARNING: This will OVERWRITE the original files!")
    print("Format: 44100 Hz, Stereo, 128 kbps, no metadata")
    print("=" * 60)

    # Auto-confirm para uso não-interativo
    response = 'y'

    # Processa arquivos
    print("\nProcessing files...")
    print("-" * 60)

    success_count = 0
    for mp3_file in mp3_files:
        print(f"  Converting: {mp3_file.name}...")
        if clean_mp3_file(mp3_file):
            success_count += 1
            print(f"    [OK] Done")
        else:
            print(f"    [FAIL]")

    # Resultado
    print("-" * 60)
    print(f"\nCompleted: {success_count}/{len(mp3_files)} files processed successfully")

    # Verificação final
    print("\nVerifying MP3 headers...")
    print("-" * 60)
    all_good = True
    for mp3_file in mp3_files:
        with open(mp3_file, 'rb') as f:
            header = f.read(4)
            if header and header[0] == 0xFF:
                print(f"  [OK] {mp3_file.name}: 0x{header[0]:02X} {header[1]:02X} {header[2]:02X} {header[3]:02X}")
            else:
                print(f"  [FAIL] {mp3_file.name}: 0x{header[0]:02X}")
                all_good = False

    if all_good:
        print("\n" + "=" * 60)
        print("[OK] All MP3 files are clean and ready for Helix decoder!")
        print("=" * 60)
    else:
        print("\n⚠ Some files still have issues. You may need to re-encode them manually.")

if __name__ == "__main__":
    main()
