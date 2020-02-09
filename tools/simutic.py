import datetime
import math
import time
import random
import json

"""
Simulation de Téléinformation Client
"""


def group(label, value):
    """
    construit un groupe d'une trame de téléinformation
    """
    sum = 0
    for c in label:
        sum += ord(c)
    sum += ord(" ")
    for c in value:
        sum += ord(c)
    sum = (sum & 63) + 32
    return "\x0A" + label + " " + str(value) + " " + chr(sum) + "\x0D"


class SimuTic:
    """
    simule un compteur en option HP/HC
    """

    def __init__(self):
        self.intervalle = 0
        self.heures_creuses = False
        self.index_hchc = 52000000
        self.index_hchp = 49000000
        self.intensite = 0
        self.adps = 0
        self.adco = "012345678123"

    def bascule(self):
        """
        bascule de période tarifaire
        """
        self.heures_creuses = not self.heures_creuses

    def depassement(self, a=0):
        """
        positionne l'Avertissement de Dépassement de Puissance Souscrite
        """
        self.adps = int(a)

    def uptime(self):
        """
        retourne le temps depuis le lancement du programme
        """
        # "0 days 00 h 13 m 17 sec"
        now = int(time.monotonic())
        return f"{now // 86400} days {(now // 3600) % 24} h {(now // 60) % 60:02d} m {now % 60} sec"

    def timestamp(self):
        """
        retourne l'heure courante au format ISO8601
        """
        return datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ")

    def iinst(self, text=False):
        """"
        calcule l'intensité instantanée et met à jour les index et la puissance apparente
        """
        now = time.monotonic()
        self.intensite = 20 * abs(math.sin(now / 100))
        delta = (now - self.intervalle) * (self.papp()) / 3600.0
        self.intervalle = now
        if self.heures_creuses:
            self.index_hchc += delta
        else:
            self.index_hchp += delta
        if text:
            return f"{round(self.intensite):03d}"
        return round(self.intensite)

    def papp(self, text=False):
        """
        calcule la Puissance APParente en VA avec un cos φ aléatoire
        """
        cos_phi = random.uniform(0.85, 1.15)
        va = round(self.intensite * 230 * cos_phi)
        if text:
            return f"{va:05d}"
        return va

    def hchc(self, text=False):
        """
        retourne l'index Heures Creuses
        """
        if text:
            return f"{round(self.index_hchc):09d}"
        return round(self.index_hchc)

    def hchp(self, text=False):
        """
        retourne l'index Heures Pleines
        """
        if text:
            return f"{round(self.index_hchp):09d}"
        return round(self.index_hchp)

    def ptec(self):
        """
        retourne la Période Tarifaire En Cours
        """
        if self.heures_creuses:
            return "HC.."
        else:
            return "HP.."

    def trame(self):
        """
        construit une trame telle qu'elle esr envoyée par un compteur
        """
        i = self.iinst(True)
        tinfo = "\x02"
        tinfo += group("ADCO", self.adco)
        tinfo += group("OPTARIF", "HC..")
        tinfo += group("ISOUSC", "30")
        tinfo += group("HCHC", self.hchc(True))
        tinfo += group("HCHP", self.hchp(True))
        tinfo += group("PTEC", self.ptec())
        tinfo += group("IINST", i)
        tinfo += group("IMAX", "042")
        tinfo += group("PAPP", self.papp(True))
        if self.adps > 0:
            tinfo += group("ADPS", f"{self.adps:03d}")
        tinfo += group("HHPHC", "D")
        tinfo += group("MOTDETAT", "000000")
        tinfo += "\x03"

        return tinfo.encode("ascii")

    def json_dict(self):
        """
        retourne les valeurs sous forme de dictionnaire JSON
        """
        i = self.iinst()
        d = {
            "_UPTIME": int(time.monotonic()),
            "timestamp": self.timestamp(),
            "ADCO": self.adco,
            "OPTARIF": "HC..",
            "ISOUSC": 30,
            "HCHC": self.hchc(),
            "HCHP": self.hchp(),
            "PTEC": self.ptec(),
            "IINST": i,
            "IMAX": 42,
            "PAPP": self.papp(),
            "HHPHC": "D",
            "MOTDETAT": 0,
        }
        if self.adps > 0:
            d["ADPS"] = self.adps
        return json.dumps(d, separators=(",", ":"))

    def json_array(self):
        """
        retourne les valeurs sous forme de tableau JSON
        """
        i = self.iinst(True)
        d = [
            {"na": "timestamp", "va": tic.timestamp()},
            {"na": "ADCO", "va": self.adco},
            {"na": "OPTARIF", "va": "HC.."},
            {"na": "ISOUSC", "va": "30"},
            {"na": "HCHC", "va": tic.hchc(True)},
            {"na": "HCHP", "va": tic.hchp(True)},
            {"na": "PTEC", "va": tic.ptec()},
            {"na": "IINST", "va": i},
            {"na": "IMAX", "va": "042"},
            {"na": "PAPP", "va": tic.papp(True)},
            {"na": "HHPHC", "va": "D"},
            {"na": "MOTDETAT", "va": "000000"},
        ]
        if self.adps > 0:
            d.append({"na": "ADPS", "va": f"{self.adps:03d}"})
        return json.dumps(d, indent=4)


tic = SimuTic()
