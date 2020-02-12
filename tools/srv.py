#!/usr/bin/env python3

# https://github.com/Mikhus/canvas-gauges/
# https://www.chartjs.org/


import time
import flask
from flask_cors import CORS, cross_origin
from simutic import tic

app = flask.Flask(__name__)

app.config["SECRET_KEY"] = "the quick brown fox jumps over the lazy dog"
app.config["CORS_HEADERS"] = "Content-Type"

cors = CORS(app, resources={r"/": {"origins": "http://localhost:5000"}})


def event_stream():
    while True:
        time.sleep(3)
        d = tic.json_dict()
        print("send:", d)
        yield f"data: {d}\n\n"


@app.route("/")
def home():
    return flask.redirect("/index.htm")


@app.route("/hc")
def bascule_ptec():
    tic.bascule()
    return tic.ptec()


@app.route("/tinfo.json")
def tinfo_json():
    d = tic.json_array()
    return d


@app.route("/spiffs.json")
def spiffs_json():
    d = {
        "files": [
            {"na": "/index.htm.gz", "va": 6468},
            {"na": "/favicon.ico", "va": 1150},
            {"na": "/css/wifinfo.css.gz", "va": 21983},
            {"na": "/js/wifinfo.js.gz", "va": 56176},
            {"na": "/js/Chart.min.js.gz", "va": 52811},
            {"na": "/js/gauge.min.js.gz", "va": 13599},
            {"na": "/version", "va": 8},
            {"na": "/fonts/glyphicons.woff2", "va": 18028},
            {"na": "/fonts/glyphicons.woff", "va": 23424},
        ],
        "spiffs": [{"Total": 957314, "Used": 197537, "ram": 37200}],
    }
    return flask.jsonify(d)


@app.route("/config.json")
def config_json():
    d = {
        "ssid": "monsuperwifi",
        "psk": "motdepasseinviolable",
        "host": "WifInfo-123456",
        "ap_psk": "",
        "emon_host": "emoncms.org",
        "emon_port": "80",
        "emon_url": "/input/post.json",
        "emon_apikey": "",
        "emon_node": "0",
        "emon_freq": "0",
        "ota_auth": "OTA_WifInfo",
        "ota_port": "8266",
        "jdom_host": "jeedom.local",
        "jdom_port": "80",
        "jdom_url": "/plugins/teleinfo/core/php/jeeTeleinfo.php",
        "jdom_apikey": "",
        "jdom_adco": "",
        "jdom_freq": "0",
        "httpreq_host": "192.168.1.23",
        "httpreq_port": "8080",
        "httpreq_url": "/tinfo.php?hchp=$HCHP;hchc=$HCHC;papp=$PAPP;iinst=$IINST;type=$_type",
        "httpreq_freq": "300",
        "httpreq_trigger_ptec": 1,
        "httpreq_trigger_adps": 1,
        "httpreq_trigger_seuils": 0,
        "httpreq_seuil_haut": 5700,
        "httpreq_seuil_bas": 4200,
    }
    return flask.jsonify(d)


@app.route("/wifiscan.json")
def wifiscan_json():
    d = [
        {
            "ssid": "orange",
            "rssi": -84,
            "bssi": "11:22:33:00:00:00",
            "channel": 1,
            "encryptionType": 7,
        },
        {
            "ssid": "FreeWifi",
            "rssi": -74,
            "bssi": "11:22:33:00:00:00",
            "channel": 6,
            "encryptionType": 8,
        },
    ]
    return flask.jsonify(d)


@app.route("/system.json")
def system_json():
    d = [
        {"na": "Uptime", "va": tic.uptime()},
        {"na": "Wi-Fi RSSI", "va": "-72 dB"},
        {"na": "Wi-Fi network", "va": "monsuperwifi"},
        {"na": "Adresse MAC station", "va": "cc:dd:ee:11:22:33"},
        {"na": "Nb reconnexions Wi-Fi", "va": "1"},
        {"na": "WifInfo Version", "va": "develop"},
        {"na": "Compilé le", "va": "Feb  6 2020 23:21:40"},
        {"na": "SDK Version", "va": "2.2.2-dev(38a443e)"},
        {"na": "Chip ID", "va": "0x123456"},
        {"na": "Boot Version", "va": "0x1F"},
        {"na": "Flash Real Size", "va": "4.00 MB"},
        {"na": "Firmware Size", "va": "404.47 kB"},
        {"na": "Free Size", "va": "2.60 MB"},
        {"na": "SPIFFS Total", "va": "934.88 kB"},
        {"na": "SPIFFS Used", "va": "192.91 kB"},
        {"na": "SPIFFS Occupation", "va": "20%"},
        {"na": "Free RAM", "va": "34.00 kB"},
    ]
    return flask.jsonify(d)


@app.route("/config_form.json", methods=["POST"])
def config_form():
    for name, value in flask.request.values.items():
        print(f"{name} = {value}")
    return "OK"


@app.route("/sse/tinfo.json", methods=["GET", "POST"])
@cross_origin()
def stream():
    return flask.Response(event_stream(), mimetype="text/event-stream")


@app.route("/<path:path>")
def files(path):
    return flask.send_from_directory("../data_src/", path)


if __name__ == "__main__":
    app.debug = True
    app.run(threaded=True)
