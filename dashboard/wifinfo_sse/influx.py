#!/usr/bin/env python3
# module téléinformation client
# rene-d 2020

# à utiliser avec un dashboard comme celui-ci:
#   https://www.kozodo.com/blog/techno/article.php?id=32

"""
Store SSE in the InfluxDB database
"""

import json
import logging
import socket
import sys
import time

import click
import requests
from influxdb import InfluxDBClient
from sseclient import SSEClient

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


class Sonde:
    def __init__(self):
        """
        Initialise la sonde
        """

        self.tarif_hc_ttc = (TARIF_HC + TCFE + CSPE) * TVA_20
        self.tarif_hp_ttc = (TARIF_HP + TCFE + CSPE) * TVA_20
        print("tarif_hc_ttc: ", self.tarif_hc_ttc)
        print("tarif_hp_ttc: ", self.tarif_hp_ttc)

    def connect(self, influxdb_addr):
        """
        Connexion à la base de données InfluxDB
        """

        self.client = InfluxDBClient(influxdb_addr, 8086)
        database = "teleinfo"
        while True:
            try:
                if not {"name": database} in self.client.get_list_database():
                    # client.create_database(database)
                    self.client.query(
                        f'CREATE DATABASE "{database}" WITH DURATION 30d REPLICATION 1 NAME "month"',
                        method="POST",
                    )
                self.client.switch_database(database)
            except requests.exceptions.ConnectionError:
                logging.info("InfluxDB is not reachable. Waiting 5 seconds to retry.")
                time.sleep(5)
            else:
                break

    def insert_data(self, data):
        """
        Ajoute un point de mesure
        """

        points = [
            {
                "measurement": "conso",
                "tags": {"host": data["ADCO"]},
                "time": data["timestamp"],
                "fields": {
                    # "COSTHC": round(data["HCHC"] / 1000 * self.tarif_hc_ttc, 4),
                    # "COSTHP": round(data["HCHP"] / 1000 * self.tarif_hp_ttc, 4),
                    "HCHC": data["HCHC"],
                    "HCHP": data["HCHP"],
                    "PAPP": data["PAPP"],
                    "IINST": data["IINST"],
                    "PTEC": data["PTEC"],
                },
            }
        ]
        # print(json.dumps(points, separators=(",", ":")))
        self.client.write_points(points)

    def sse(self, frequency, wifinfo_addr):
        """
        Se connecte au serveur SSE et stocke les mesures qu'il envoie
        """

        while True:
            try:
                # source SSE
                messages = SSEClient(f"http://{wifinfo_addr}/tic", timeout=31)

                last_write = 0
                for msg in messages:

                    sys.stdout.flush()

                    now = int(time.time() / frequency)
                    if now == last_write:
                        # trop fréquent: on ignore le message
                        print(f"\033[2m{msg}\033[0m")
                        continue

                    print(msg)
                    last_write = now
                    self.insert_data(json.loads(msg.data))

            except (socket.timeout, requests.exceptions.ConnectionError) as exception:
                print(exception)
                print("retry in 10s")
                time.sleep(10)

    def req(self, frequency, wifinfo_addr):
        """
        Polle le module à intervalle régulier
        """
        last_timestamp = None

        while True:
            rep = requests.get(f"http://{wifinfo_addr}/json", timeout=5)
            if rep.status_code == 200:
                data = rep.json()
                if data["timestamp"] != last_timestamp:
                    print(rep.content)
                    last_timestamp = data["timestamp"]
                    self.insert_data(data)
            else:
                print("HTTP code", rep.status_code)

            time.sleep(frequency)


@click.command()
@click.option(
    "-freq",
    "frequency",
    envvar="FREQUENCY",
    help="fréquence d'écriture dans la base InfluxDb",
    type=click.IntRange(5, 3600),
    default=10,
)
@click.option(
    "-module",
    "wifinfo_addr",
    envvar="WIFINFO",
    help="adresse IP du module WifInfo",
    default="192.168.4.1",
)
@click.option(
    "-host", "influxdb_addr", help="adresse IP du module WifInfo", default="influxdb"
)
def main(frequency, wifinfo_addr, influxdb_addr):

    sonde = Sonde()

    sonde.connect(influxdb_addr)

    if frequency >= 30:
        sonde.req(frequency, wifinfo_addr)
    else:
        sonde.sse(frequency, wifinfo_addr)


if __name__ == "__main__":
    main()
