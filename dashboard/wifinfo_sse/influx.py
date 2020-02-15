#!/usr/bin/env python3
# rene-d 2020

# à utiliser avec un dashboard comme celui-ci:
#   https://www.kozodo.com/blog/techno/article.php?id=32

"""
Store SSE in the InfluxDB database
"""

import logging
import time
import requests
from influxdb import InfluxDBClient
from sseclient import SSEClient
import json
import click


# Tarifs réglementés EDF fin 2019

# TVA à 5.5%
ABO = 17.16  # Heures Creuses - 06kVA
CTA = 3.63  # Contribution Tarifaire d'Acheminement Electricité

# TVA à 20%
TARIF_HC = 0.0778  # Heures Creuses - 06kVA
TARIF_HP = 0.1103  # Heures Pleines - 06kVA
TCFE = 0.00969  # Taxe sur la Consommation Finale d'Electricité
CSPE = 0.02250  # Contribution au Service Public d'Electricité

TVA_055 = 1.055  # TVA 5.5%
TVA_20 = 1.20  # TVA 20%


@click.command()
@click.option(
    "-freq", "frequency", help="fréquence d'écriture dans la base InfluxDb", type=click.IntRange(5, 3600), default=10,
)
@click.option(
    "-module", "wifinfo_addr", help="adresse IP du module WifInfo", default="192.168.4.1",
)
@click.option("-host", "influxdb_addr", help="adresse IP du module WifInfo", default="localhost")
def main(frequency, wifinfo_addr, influxdb_addr):

    tarif_hc_ttc = (TARIF_HC + TCFE + CSPE) * TVA_20
    tarif_hp_ttc = (TARIF_HP + TCFE + CSPE) * TVA_20

    print("tarif_hc_ttc: ", tarif_hc_ttc)
    print("tarif_hp_ttc: ", tarif_hp_ttc)

    # connexion à la base de données InfluxDB
    client = InfluxDBClient(influxdb_addr, 8086)
    database = "teleinfo"

    while True:
        try:
            if not {"name": database} in client.get_list_database():
                # client.create_database(database)
                client.query(
                    f'CREATE DATABASE "{database}" WITH DURATION 30d REPLICATION 1 NAME "month"', method="POST"
                )
            client.switch_database(database)
        except requests.exceptions.ConnectionError:
            logging.info("InfluxDB is not reachable. Waiting 5 seconds to retry.")
            time.sleep(5)
        else:
            break

    # source SSE
    messages = SSEClient(f"http://{wifinfo_addr}/sse/tinfo.json")

    last_write = 0
    for msg in messages:

        now = int(time.time() / frequency)
        if now == last_write:
            print(f"\033[2m{msg}\033[0m")
            continue

        print(msg)

        last_write = now

        data = json.loads(msg.data)
        points = [
            {
                "measurement": "conso",
                "tags": {"host": data["ADCO"]},
                "time": data["timestamp"],
                "fields": {
                    "COSTHC": round(data["HCHC"] / 1000 * tarif_hc_ttc, 4),
                    "COSTHP": round(data["HCHP"] / 1000 * tarif_hp_ttc, 4),
                    "HCHC": data["HCHC"],
                    "HCHP": data["HCHP"],
                    "PAPP": data["PAPP"],
                    "IINST": data["IINST"],
                    "PTEC": data["PTEC"],
                },
            }
        ]
        # print(json.dumps(points, separators=(",", ":")))
        client.write_points(points)


if __name__ == "__main__":
    exit(main())
