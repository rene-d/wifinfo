// module téléinformation client
// rene-d 2020

#include "tic.h"
#include "config.h"
#include "httpreq.h"
#include "led.h"
#include "sse.h"
#include "strncpy_s.h"
#include "teleinfo.h"
#include <PolledTimeout.h>

// les différente notifications que httpreq peut envoyer
//
#define HTTP_NOTIF_TYPE_MAJ "MAJ"   // mise à jour (notif régulière)
#define HTTP_NOTIF_TYPE_PTEC "PTEC" // changement de période tarifaire
#define HTTP_NOTIF_TYPE_HAUT "HAUT" // dépassement seuil haut
#define HTTP_NOTIF_TYPE_BAS "BAS"   // retour au seuil bas
#define HTTP_NOTIF_TYPE_ADPS "ADPS" // quand l'étiquette ADPS est présente
#define HTTP_NOTIF_TYPE_NORM "NORM" // retour d'un ADPS

enum Seuil
{
    BAS,
    HAUT
};

extern SseClients sse_clients;

static char periode_en_cours[8] = {0};
static bool init_periode_en_cours = true;
static Seuil seuil_en_cours = BAS;
static bool etat_adps = false;

static esp8266::polledTimeout::periodicMs timer_http(esp8266::polledTimeout::periodicMs::neverExpires);
static esp8266::polledTimeout::periodicMs timer_emoncms(esp8266::polledTimeout::periodicMs::neverExpires);
static esp8266::polledTimeout::periodicMs timer_jeedom(esp8266::polledTimeout::periodicMs::neverExpires);
static esp8266::polledTimeout::periodicMs timer_sse(esp8266::polledTimeout::periodicMs::neverExpires);

Teleinfo tinfo;
static TeleinfoDecoder tinfo_decoder;

static void tic_get_json_dict_notif(String &data, const char *notif);
static void http_notif(const char *notif);
static void http_notif_periode_en_cours();
static void http_notif_seuils();
static void http_notif_adps();
static void jeedom_notif();
static void emoncms_notif();

void tic_decode(int c)
{
    tinfo_decoder.put(c);

    if (tinfo_decoder.ready())
    {
        if (config.options & OPTION_LED_TINFO)
        {
            led_on();
        }
        tinfo.copy_from(tinfo_decoder);

        Serial.printf("teleinfo: [%lu] %s  %s  %s  %s\n",
                      millis(),
                      tinfo.get_value("PTEC", "?"),
                      tinfo.get_value("HCHP", "?"),
                      tinfo.get_value("HCHC", "?"),
                      tinfo.get_value("PAPP", "?"));

        tic_notifs();

        if (config.options & OPTION_LED_TINFO)
        {
            led_off();
        }
    }
}

// appelée chaque fois qu'une trame de teleinfo valide est reçue
void tic_notifs()
{
    if (config.httpreq.host[0] != 0)
    {
        if (config.httpreq.trigger_ptec)
        {
            http_notif_periode_en_cours();
        }

        if (config.httpreq.trigger_seuils)
        {
            http_notif_seuils();
        }

        if (config.httpreq.trigger_adps)
        {
            http_notif_adps();
        }

        if (timer_http)
        {
            http_notif(HTTP_NOTIF_TYPE_MAJ);
        }
    }

    if (timer_jeedom)
    {
        jeedom_notif();
    }

    if (timer_emoncms)
    {
        emoncms_notif();
    }

    if (timer_sse && (sse_clients.count() != 0))
    {
        String data;
        tic_get_json_dict(data);
        sse_clients.handle_clients(&data);
    }
}

void tic_make_timers()
{
    // http
    if ((config.httpreq.freq == 0) || (config.httpreq.host[0] == 0) || (config.httpreq.port == 0))
    {
        timer_http.resetToNeverExpires();
        Serial.println("timer_http disabled");
    }
    else
    {
        timer_http.reset(config.httpreq.freq * 1000);
        Serial.printf("timer_http enabled, freq=%d s\n", config.httpreq.freq);
    }

    // jeedom
    if ((config.jeedom.freq == 0) || (config.jeedom.host[0] == 0) || (config.jeedom.port == 0))
    {
        timer_jeedom.resetToNeverExpires();
        Serial.println("timer_jeedom disabled");
    }
    else
    {
        timer_jeedom.reset(config.jeedom.freq * 1000);
        Serial.printf("timer_jeedom enabled, freq=%d s\n", config.jeedom.freq);
    }

    // emoncms
    if ((config.emoncms.freq == 0) || (config.emoncms.host[0] == 0) || (config.emoncms.port == 0))
    {
        timer_emoncms.resetToNeverExpires();
        Serial.println("timer_emoncms disabled");
    }
    else
    {
        timer_emoncms.reset(config.emoncms.freq * 1000);
        Serial.printf("timer_emoncms enabled, freq=%d s\n", config.emoncms.freq);
    }

    if (config.sse_freq == 0)
    {
        timer_sse.reset(esp8266::polledTimeout::periodicMs::alwaysExpired);
        Serial.println("timer_sse always expired");
    }
    else
    {
        timer_sse.reset(config.sse_freq * 1000);
        Serial.printf_P(PSTR("timer_sse enabled, freq=%d s\n"), config.sse_freq);
    }
}

void tic_get_json_array(String &data)
{
    if (tinfo.is_empty())
    {
        data = "[]";
        return;
    }

    // la trame en JSON fait environ 360 à 373 octets selon ADPS pour un abo HC/HP
    JSONTableBuilder js(data, 400);
    const char *label;
    const char *value;
    const char *state = nullptr;

    js.append("timestamp", tinfo.get_timestamp_iso8601());

    while (tinfo.get_value_next(label, value, &state))
    {
        js.append(label, value);
    }

    js.finalize();
}

void tic_get_json_dict_notif(String &data, const char *notif)
{

    if (tinfo.is_empty())
    {
        data = "{}";
        return;
    }

    // la trame fait 217 à 230 selon ADPS pour un abo HC/HP
    JSONBuilder js(data, 256);
    const char *label;
    const char *value;
    const char *state = nullptr;

    js.append(FPSTR("timestamp"), tinfo.get_timestamp_iso8601().c_str());

    if (notif != nullptr)
    {
        js.append(FPSTR("notif"), notif);
    }

    while (tinfo.get_value_next(label, value, &state))
    {
        bool is_number = tinfo.get_integer(value);

        if (is_number)
            js.append_without_quote(label, value);
        else
            js.append(label, value);
    }

    js.finalize();
}

void tic_get_json_dict(String &data)
{
    tic_get_json_dict_notif(data, nullptr);
}

// interface pour webserver http://wifinfo/<ETIQUETTE>
const char *tic_get_value(const char *label)
{
    return tinfo.get_value(label, nullptr, true);
}

static void http_add_value_ex(String &uri, const char *label, const char *notif)
{
    if (strcasecmp_P(label, PSTR("type")) == 0)
    {
        uri += notif;
    }
    else if (strcasecmp_P(label, PSTR("notif")) == 0)
    {
        uri += notif;
    }
    else if (strcasecmp_P(label, PSTR("date")) == 0)
    {
        uri += tinfo.get_timestamp_iso8601();
    }
    else if (strcasecmp_P(label, PSTR("timestamp")) == 0)
    {
        uri += tinfo.get_timestamp();
    }
    else if (strcasecmp_P(label, PSTR("chipid")) == 0)
    {
        char buf[16];
        snprintf_P(buf, sizeof(buf), PSTR("0x%06X"), ESP.getChipId());
        uri += buf;
    }
    else
    {
        uri += tinfo.get_value(label, "", true);
    }
}

void http_notif(const char *notif)
{
    String uri;
    char label[16];

    uri.reserve(CFG_HTTPREQ_HOST_LENGTH + 32);

    // formate l'URL
    for (const char *p = config.httpreq.url; *p; ++p)
    {
        if (*p == '~')
        {
            ++p;
            size_t i = 0;
            while (*p && (*p != '~') && (i < (sizeof(label) - 1)))
            {
                label[i++] = *p++;
            }
            label[i] = 0;

            if (i == 0)
            {
                uri += "~";
            }
            else
            {
                http_add_value_ex(uri, label, notif);
            }
        }
        else if (*p == '$')
        {
            ++p;
            if (*p == '$')
            {
                uri += "$";
            }
            else
            {
                size_t i = 0;
                while ((isalpha(*p) || (*p == '_')) && (i < (sizeof(label) - 1)))
                {
                    label[i++] = *p++;
                }
                label[i] = 0;
                --p; // revient sur le dernier caractère du label

                http_add_value_ex(uri, label, notif);
            }
        }
        else
        {
            uri += *p;
        }
    }

    if (config.httpreq.use_post)
    {
        String data;

        tic_get_json_dict_notif(data, notif);

        Serial.printf_P(PSTR("http_notif: POST %s\n"), notif);

        http_request(config.httpreq.host, config.httpreq.port, uri, data.c_str());
    }
    else
    {
        Serial.printf_P(PSTR("http_notif: GET %s\n"), notif);
        http_request(config.httpreq.host, config.httpreq.port, uri);
    }
}

void http_notif_periode_en_cours()
{
    const char *PTEC = tinfo.get_value("PTEC");
    if (PTEC == NULL)
    {
        return;
    }

    // a-t-on un changement de période ?
    if (strncmp(periode_en_cours, PTEC, sizeof(periode_en_cours)) != 0)
    {
        strncpy_s(periode_en_cours, PTEC, sizeof(periode_en_cours) - 1);
        if (init_periode_en_cours)
        {
            // premier passage: on mémorise la période courante
            init_periode_en_cours = false;
        }
        else
        {
            http_notif(HTTP_NOTIF_TYPE_PTEC);
        }
    }
}

void http_notif_adps()
{
    const char *ADPS = tinfo.get_value("ADPS");

    if (ADPS == NULL)
    {
        if (etat_adps == true)
        {
            // on était en ADPS: on signale et on rebascule en état normal
            etat_adps = false;
            http_notif(HTTP_NOTIF_TYPE_NORM);
        }
    }
    else
    {
        if (etat_adps == false)
        {
            // on vient de passer en ADPS: on signale et on bascule dans l'état ADPS
            etat_adps = true;
            http_notif(HTTP_NOTIF_TYPE_ADPS);
        }
    }
}

void http_notif_seuils()
{
    const char *PAPP = tinfo.get_value("PAPP");
    if (PAPP == NULL)
    {
        return;
    }

    long papp = strtol(PAPP, nullptr, 10);
    if (papp == 0)
    {
        return;
    }

    if ((papp >= config.httpreq.seuil_haut) && (seuil_en_cours == BAS))
    {
        seuil_en_cours = HAUT;
        http_notif(HTTP_NOTIF_TYPE_HAUT);
    }
    else if ((papp <= config.httpreq.seuil_bas) && (seuil_en_cours == HAUT))
    {
        seuil_en_cours = BAS;
        http_notif(HTTP_NOTIF_TYPE_BAS);
    }
}

// Do a http post to jeedom server
void jeedom_notif()
{
    if (config.jeedom.host[0] == 0)
    {
        return;
    }

    const char *label;
    const char *value;
    const char *state = nullptr;

    String url;

    url = *config.jeedom.url ? config.jeedom.url : "/";
    url += "?";

    url += F("api=");
    url += config.jeedom.apikey;

    while (tinfo.get_value_next(label, value, &state))
    {
        if (strcmp(label, "ADCO") == 0)
        {
            // Config identifiant forcée ?
            if (config.jeedom.adco[0] != 0)
            {
                value = config.jeedom.adco;
            }
        }

        url += F("&");
        url += label;
        url += F("=");
        url += value;
    }

    http_request(config.jeedom.host, config.jeedom.port, url);
}

// construct the JSON (without " ???) part of emoncms url
void tic_emoncms_data(String &url)
{
    const char *label;
    const char *value;
    const char *state = nullptr;

    bool first_item = true;
    url += "{";

    while (tinfo.get_value_next(label, value, &state))
    {
        // On first item, do not add , separator
        if (first_item)
        {
            first_item = false;
        }
        else
        {
            url += ",";
        }

        url += label;
        url += ":";

        // EMONCMS ne sait traiter que des valeurs numériques, donc ici il faut faire une
        // table de mappage, tout à fait arbitraire, mais c"est celle-ci dont je me sers
        // depuis mes débuts avec la téléinfo
        if (!strcmp(label, "OPTARIF"))
        {
            // L'option tarifaire choisie (Groupe "OPTARIF") est codée sur 4 caractères alphanumériques
            /* J'ai pris un nombre arbitraire codé dans l'ordre ci-dessous
                je mets le 4eme char à 0, trop de possibilités
                BASE => Option Base.
                HC.. => Option Heures Creuses.
                EJP. => Option EJP.
                BBRx => Option Tempo
                */
            const char *p = value;

            if (*p == 'B' && *(p + 1) == 'A' && *(p + 2) == 'S')
            {
                url += "1";
            }
            else if (*p == 'H' && *(p + 1) == 'C')
            {
                url += "2";
            }
            else if (*p == 'E' && *(p + 1) == 'J' && *(p + 2) == 'P')
            {
                url += "3";
            }
            else if (*p == 'B' && *(p + 1) == 'B' && *(p + 2) == 'R')
            {
                url += "4";
            }
            else
            {
                url += "0";
            }
        }
        else if (!strcmp(label, "HHPHC"))
        {
            // L'horaire heures pleines/heures creuses (Groupe "HHPHC") est codé par un caractère A à Y
            // J'ai choisi de prendre son code ASCII
            int code = *value;
            url += String(code);
        }
        else if (!strcmp(label, "PTEC"))
        {
            // La période tarifaire en cours (Groupe "PTEC"), est codée sur 4 caractères
            /* J'ai pris un nombre arbitraire codé dans l'ordre ci-dessous
                TH.. => Toutes les Heures.
                HC.. => Heures Creuses.
                HP.. => Heures Pleines.
                HN.. => Heures Normales.
                PM.. => Heures de Pointe Mobile.
                HCJB => Heures Creuses Jours Bleus.
                HCJW => Heures Creuses Jours Blancs (White).
                HCJR => Heures Creuses Jours Rouges.
                HPJB => Heures Pleines Jours Bleus.
                HPJW => Heures Pleines Jours Blancs (White).
                HPJR => Heures Pleines Jours Rouges.
                */
            if (!strcmp(value, "TH"))
            {
                url += "1";
            }
            else if (!strcmp(value, "HC"))
            {
                url += "2";
            }
            else if (!strcmp(value, "HP"))
            {
                url += "3";
            }
            else if (!strcmp(value, "HN"))
            {
                url += "4";
            }
            else if (!strcmp(value, "PM"))
            {
                url += "5";
            }
            else if (!strcmp(value, "HCJB"))
            {
                url += "6";
            }
            else if (!strcmp(value, "HCJW"))
            {
                url += "7";
            }
            else if (!strcmp(value, "HCJR"))
            {
                url += "8";
            }
            else if (!strcmp(value, "HPJB"))
            {
                url += "9";
            }
            else if (!strcmp(value, "HPJW"))
            {
                url += "10";
            }
            else if (!strcmp(value, "HPJR"))
            {
                url += "11";
            }
            else
            {
                url += "0";
            }
        }
        else
        {
            tinfo.get_integer(value);
            url += value;
        }
    }

    url += "}";
}

// emoncmsPost (called by main sketch on timer, if activated)
void emoncms_notif()
{
    // Some basic checking
    if (config.emoncms.host[0] == 0)
    {
        return;
    }

    String url;

    url = *config.emoncms.url ? config.emoncms.url : "/";
    url += "?";

    if (config.emoncms.node > 0)
    {
        url += F("node=");
        url += String(config.emoncms.node);
        url += "&";
    }

    url += F("apikey=");
    url += config.emoncms.apikey;

    //append json list of values
    url += F("&json=");

    tic_emoncms_data(url); //Get Teleinfo list of values

    // And submit all to emoncms
    http_request(config.emoncms.host, config.emoncms.port, url);
}

void tic_dump()
{
    char raw[Teleinfo::MAX_FRAME_SIZE];
    tinfo.get_frame_ascii(raw, sizeof(raw));
    Serial.println(tinfo.get_timestamp_iso8601());
    Serial.println(raw);
}
