#!/usr/bin/env python3
# module téléinformation client
# rene-d 2020

import struct
import pathlib
import json
import yaml
import click
import subprocess
import re
import tempfile


def get_eeprom_base():
    """ lit l'adresse de base de l'eeprom """
    output = subprocess.run(
        "esptool.py --no-stub flash_id", shell=True, stderr=subprocess.DEVNULL, stdout=subprocess.PIPE
    ).stdout
    mem = re.search(rb"Detected flash size: (\w+)(?=\n)", output, re.DOTALL)
    if not mem:
        print("Impossible de déterminer la taille de la flash")
        return

    mem = mem.group(1).decode()
    print(f"Taille flash détectée: {mem}")

    # cf. framework-arduinoespressif8266/tools/sdk/ld/eagle.flash.*.ld
    # ATTENTION : ceci est valable pour les boards esp12 et esp1_1m configurés dans platformio.ini
    if mem == "4MB":
        base = 0x405FB000
    elif mem == "1MB":
        base = 0x402FB000
    else:
        click.echo(click.style("Taille non gérée", fg="red"))
        return

    return base - 0x40200000


def write_eeprom(config):
    """ sérialise la conf dans la page de 1Ko """

    emoncms = struct.pack(
        "<33s33s33sHBI",
        config["emon_host"].encode(),
        config["emon_apikey"].encode(),
        config["emon_url"].encode(),
        config["emon_port"],
        config["emon_node"],
        config["emon_freq"],
    )

    jeedom = struct.pack(
        "<33s49s65s13sHI",
        config["jdom_host"].encode(),
        config["jdom_apikey"].encode(),
        config["jdom_url"].encode(),
        config["jdom_adco"].encode(),
        config["jdom_port"],
        config["jdom_freq"],
    )

    httpreq = struct.pack(
        "<33s151sHIBHH",
        config["httpreq_host"].encode(),
        config["httpreq_url"].encode(),
        config["httpreq_port"],
        config["httpreq_freq"],
        config["httpreq_trigger_adps"] | config["httpreq_trigger_ptec"] << 1 | config["httpreq_trigger_seuils"] << 2,
        config["httpreq_seuil_haut"],
        config["httpreq_seuil_bas"],
    )

    eeprom = struct.pack(
        "<33s65s17s65s65sIHH129s128s256s256s",
        config["ssid"].encode(),
        config["psk"].encode(),
        config["host"].encode(),
        config["ap_psk"].encode(),
        config["ota_auth"].encode(),
        config["cfg_led_tinfo"],
        config["ota_port"],
        config["sse_freq"],
        b"",  # filler
        emoncms,
        jeedom,
        httpreq,
    )

    assert len(eeprom) == 1022
    crc = 0xFFFF
    for a in eeprom:
        crc = crc ^ a
        for i in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc = crc >> 1
    eeprom += struct.pack("<H", crc)

    assert len(eeprom) == 1024
    return eeprom


def read_eeprom(eeprom):
    """ désérialise la conf """

    config = {}

    d = struct.unpack("<33s65s17s65s65sIHH129s128s256s256sH", eeprom)
    config["ssid"] = d[0].rstrip(b"\0").decode()
    config["psk"] = d[1].rstrip(b"\0").decode()
    config["host"] = d[2].rstrip(b"\0").decode()
    config["ap_psk"] = d[3].rstrip(b"\0").decode()
    config["ota_auth"] = d[4].rstrip(b"\0").decode()
    config["cfg_led_tinfo"] = d[5] & 1
    config["ota_port"] = d[6]
    config["sse_freq"] = d[7]

    emoncms = struct.unpack_from("<33s33s33sHBI", d[9])
    config["emon_host"] = emoncms[0].rstrip(b"\0").decode()
    config["emon_apikey"] = emoncms[1].rstrip(b"\0").decode()
    config["emon_url"] = emoncms[2].rstrip(b"\0").decode()
    config["emon_port"] = emoncms[3]
    config["emon_node"] = emoncms[4]
    config["emon_freq"] = emoncms[5]

    jeedom = struct.unpack_from("<33s49s65s13sHI", d[10])
    config["jdom_host"] = jeedom[0].rstrip(b"\0").decode()
    config["jdom_apikey"] = jeedom[1].rstrip(b"\0").decode()
    config["jdom_url"] = jeedom[2].rstrip(b"\0").decode()
    config["jdom_adco"] = jeedom[3].rstrip(b"\0").decode()
    config["jdom_port"] = jeedom[4]
    config["jdom_freq"] = jeedom[5]

    httpreq = struct.unpack_from("<33s151sHIBHH", d[11])
    config["httpreq_host"] = httpreq[0].rstrip(b"\0").decode()
    config["httpreq_url"] = httpreq[1].rstrip(b"\0").decode()
    config["httpreq_port"] = httpreq[2]
    config["httpreq_freq"] = httpreq[3]
    config["httpreq_trigger_adps"] = httpreq[4] & 1
    config["httpreq_trigger_ptec"] = (httpreq[4] & 2) >> 1
    config["httpreq_trigger_seuils"] = (httpreq[4] & 4) >> 2
    config["httpreq_seuil_haut"] = httpreq[5]
    config["httpreq_seuil_bas"] = httpreq[6]

    config["crc"] = f"0x{d[12]:04x}"

    return config


@click.group()
def cli_write():
    pass


@cli_write.command()
@click.option("-i", "--input", help="fichier source", default="config.yaml", type=click.File("rb"))
@click.option("-f", "--force", help="pas de question", is_flag=True)
def write(input, force):
    """ Charge une configuration en EEPROM """
    suffix = pathlib.Path(input.name).suffix.lower()

    if suffix == ".bin":
        eeprom_file = input
    else:
        if suffix == ".json":
            config = json.load(input)
        elif suffix == ".yaml" or suffix == ".yml":
            config = yaml.full_load(input)
        else:
            print(f"Extension de fichier non reconnue: {suffix}")
            return 2

        eeprom_file = tempfile.NamedTemporaryFile("w+b", prefix="eeprom", suffix=".bin")
        eeprom_file.write(write_eeprom(config))
        eeprom_file.flush()
        click.echo(click.style(yaml.safe_dump(config, sort_keys=False), fg="cyan"))

    input.close()

    base = get_eeprom_base()
    if not base:
        return 2

    cmd = f"esptool.py --after hard_reset write_flash 0x{base:x} {eeprom_file.name}"
    click.echo(click.style(cmd, fg="bright_black"))

    if not force:
        click.confirm("Voulez-vous contnuer ?", abort=True)
    subprocess.run(cmd, shell=True)


@click.group()
def cli_read():
    pass


@cli_read.command()
@click.option("-o", "--output", help="fichier destination", default="config.yaml", type=click.File("wb"))
def read(output):
    """ Lit la configuration depuis l'EEPROM """
    base = get_eeprom_base()
    if not base:
        return 2

    output.close()

    suffix = pathlib.Path(output.name).suffix
    if suffix == ".yaml" or suffix == ".yml" or suffix == ".json":
        eeprom_file = tempfile.NamedTemporaryFile("w+b", prefix="eeprom", suffix=".bin")
    else:
        eeprom_file = output

    cmd = f"esptool.py --after no_reset read_flash 0x{base:x} 0x400 {eeprom_file.name}"
    click.echo(click.style(cmd, fg="bright_black"))
    subprocess.run(cmd, shell=True)

    if suffix == ".yaml" or suffix == ".yml":
        eeprom = open(eeprom_file.name, "rb").read()
        output = open(output.name, "w")
        yaml.safe_dump(read_eeprom(eeprom), output, sort_keys=False)
    elif suffix == ".json":
        eeprom = open(eeprom_file.name, "rb").read()
        output = open(output.name, "w")
        json.dump(read_eeprom(eeprom), output, indent=4)
    else:
        pass

    print(f"Configuration sauvée: {output.name}")


@click.group()
def cli_dump():
    pass


@cli_dump.command()
@click.option("-i", "--input", help="fichier source", default="config.bin", type=click.File("rb"))
@click.option("-f", "--format", help="fichier source", default="yaml", type=click.Choice(["yaml", "json"]))
def dump(input, format):
    """ Convertit le fichier EEPROM config.bin """
    eeprom = input.read()
    input.close()

    if format == "json":
        print(json.dumps(read_eeprom(eeprom), indent=4))
    else:
        print(yaml.safe_dump(read_eeprom(eeprom), sort_keys=False))


cli = click.CommandCollection(sources=[cli_write, cli_read, cli_dump])

if __name__ == "__main__":
    cli()
