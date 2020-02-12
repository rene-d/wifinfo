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
        self._isousc = 30
        self.adps = False
        self.adco = "012345678123"

    @property
    def adco(self):
        return self._adco

    @adco.setter
    def adco(self, value):
        value = str(value)[0:12]
        self._adco = "0" * (12 - len(value)) + value

    def bascule(self):
        """
        bascule de période tarifaire
        """
        self.heures_creuses = not self.heures_creuses

    @property
    def uptime(self):
        """
        retourne le temps depuis le lancement du programme
        """
        # "0 days 00 h 13 m 17 sec"
        now = int(time.monotonic())
        return f"{now // 86400} days {(now // 3600) % 24} h {(now // 60) % 60:02d} m {now % 60} sec"

    @property
    def timestamp(self):
        """
        retourne l'heure courante au format ISO8601
        """
        return datetime.datetime.now().strftime("%Y-%m-%dT%H:%M:%SZ")

    @property
    def iinst(self):
        """"
        calcule l'intensité instantanée et met à jour les index et la puissance apparente
        """
        return f"{self.iinst_value:03d}"

    @property
    def iinst_value(self):
        """"
        calcule l'intensité instantanée et met à jour les index et la puissance apparente
        """
        now = time.monotonic()
        self.intensite = 20 * abs(math.sin(now / 100))
        delta = (now - self.intervalle) * self.papp_value / 3600.0
        self.intervalle = now
        if self.heures_creuses:
            self.index_hchc += delta
        else:
            self.index_hchp += delta
        return round(self.intensite)

    @property
    def papp(self, text=False):
        """
        calcule la Puissance APParente en VA avec un cos φ aléatoire
        """
        return f"{self.papp_value:05d}"

    @property
    def papp_value(self):
        """
        calcule la Puissance APParente en VA avec un cos φ aléatoire
        """
        cos_phi = random.uniform(0.85, 1.15)
        return round(self.intensite * 230 * cos_phi)

    @property
    def hchc(self):
        """
        retourne l'index Heures Creuses
        """
        return f"{self.hchc_value:09d}"

    @property
    def hchc_value(self):
        """
        retourne l'index Heures Creuses
        """
        return round(self.index_hchc)

    @property
    def hchp(self, text=False):
        """
        retourne l'index Heures Pleines
        """
        return f"{self.hchp_value:09d}"

    @property
    def hchp_value(self, text=False):
        """
        retourne l'index Heures Pleines
        """
        return round(self.index_hchp)

    @property
    def ptec(self):
        """
        retourne la Période Tarifaire En Cours
        """
        if self.heures_creuses:
            return "HC.."
        else:
            return "HP.."

    @property
    def adps(self):
        """
        retourne l'Avertissement de Dépassement de Puissance Souscrite
        """
        if self._adps == 0:
            return None
        return f"{round(self._adps):03d}"

    @adps.setter
    def adps(self, value):
        """
        positionne l'Avertissement de Dépassement de Puissance Souscrite
        """
        if isinstance(value, bool):
            self._adps = [0, self._isousc + 1][value]
        elif not (self._isousc <= value <= 999):
            self._adps = 0
        else:
            self._adps = value

    @property
    def isousc(self):
        """
        retourne l'Intensité SOUSCrite
        """
        return f"{self._isousc:02d}"

    @property
    def isousc_value(self):
        """
        retourne l'Intensité SOUSCrite
        """
        return self._isousc

    def trame(self):
        """
        construit une trame telle qu'elle esr envoyée par un compteur
        """
        intensite = self.iinst
        tinfo = "\x02"
        tinfo += group("ADCO", self.adco)
        tinfo += group("OPTARIF", "HC..")
        tinfo += group("ISOUSC", self.isousc)
        tinfo += group("HCHC", self.hchc)
        tinfo += group("HCHP", self.hchp)
        tinfo += group("PTEC", self.ptec)
        tinfo += group("IINST", intensite)
        tinfo += group("IMAX", "042")
        tinfo += group("PAPP", self.papp)
        if self.adps:
            tinfo += group("ADPS", self.adps)
        tinfo += group("HHPHC", "D")
        tinfo += group("MOTDETAT", "000000")
        tinfo += "\x03"

        return tinfo.encode("ascii")

    def json_dict(self):
        """
        retourne les valeurs sous forme de dictionnaire JSON
        """
        intensite = self.iinst_value
        d = {
            "_UPTIME": int(time.monotonic()),
            "timestamp": self.timestamp,
            "ADCO": self.adco,
            "OPTARIF": "HC..",
            "ISOUSC": self.isousc_value,
            "HCHC": self.hchc_value,
            "HCHP": self.hchp_value,
            "PTEC": self.ptec,
            "IINST": intensite,
            "IMAX": 42,
            "PAPP": self.papp_value,
            "HHPHC": "D",
            "MOTDETAT": 0,
        }
        if self.adps:
            d["ADPS"] = self.adps
        return json.dumps(d, separators=(",", ":"))

    def json_array(self):
        """
        retourne les valeurs sous forme de tableau JSON
        """
        i = self.iinst(True)
        d = [
            {"na": "timestamp", "va": tic.timestamp},
            {"na": "ADCO", "va": self.adco},
            {"na": "OPTARIF", "va": "HC.."},
            {"na": "ISOUSC", "va": "30"},
            {"na": "HCHC", "va": tic.hchc},
            {"na": "HCHP", "va": tic.hchp},
            {"na": "PTEC", "va": tic.ptec},
            {"na": "IINST", "va": i},
            {"na": "IMAX", "va": "042"},
            {"na": "PAPP", "va": tic.papp},
            {"na": "HHPHC", "va": "D"},
            {"na": "MOTDETAT", "va": "000000"},
        ]
        if self.is_adps:
            d.append({"na": "ADPS", "va": self.adps})
        return json.dumps(d, indent=4)


tic = SimuTic()
