#include "tic.h"
#include "teleinfo.h"
#include "config.h"
#include "httpreq.h"
#include "sse.h"
#include "led.h"
#include <PolledTimeout.h>

extern SseClients sse_clients;

static char periode_en_cours[8] = {0};
static bool init_periode_en_cours = true;
static enum { BAS,
              HAUT } seuil_en_cours = BAS;
static bool etat_adps = false;

static esp8266::polledTimeout::periodicMs timer_http(esp8266::polledTimeout::periodicMs::neverExpires);
static esp8266::polledTimeout::periodicMs timer_emoncms(esp8266::polledTimeout::periodicMs::neverExpires);
static esp8266::polledTimeout::periodicMs timer_jeedom(esp8266::polledTimeout::periodicMs::neverExpires);

Teleinfo tinfo;
static TeleinfoDecoder tinfo_decoder;

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
        if (config.config & CONFIG_LED_TINFO)
            led_on();

        tinfo.copy_from(tinfo_decoder);

        Serial.printf("teleinfo: [%lu] %s  %s  %s  %s\n",
                      millis(),
                      tinfo.get_value("PTEC", "?"),
                      tinfo.get_value("HCHP", "?"),
                      tinfo.get_value("HCHC", "?"),
                      tinfo.get_value("PAPP", "?"));

        tic_notifs();

        if (config.config & CONFIG_LED_TINFO)
            led_off();
    }
}

// appelée chaque fois qu'une trame de teleinfo valide est reçue
void tic_notifs()
{
    if (config.httpReq.host[0] != 0)
    {
        if (config.httpReq.trigger_ptec)
        {
            http_notif_periode_en_cours();
        }

        if (config.httpReq.trigger_seuils)
        {
            http_notif_seuils();
        }

        if (config.httpReq.trigger_adps)
        {
            http_notif_adps();
        }

        if (timer_http)
        {
            http_notif("MAJ");
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

    if (sse_clients.has_clients())
    {
        String data;
        tic_get_json_dict(data);
        sse_clients.handle_clients(&data);
    }
}

void tic_make_timers()
{
    // http
    if (config.httpReq.freq == 0 || config.httpReq.host[0] == 0 || config.httpReq.port == 0)
    {
        timer_http.resetToNeverExpires();
        Serial.println("timer_http disabled");
    }
    else
    {
        timer_http.reset(config.httpReq.freq * 1000);
        Serial.printf("timer_http enabled, freq=%d s\n", config.httpReq.freq);
    }

    // jeedom
    if (config.jeedom.freq == 0 || config.jeedom.host[0] == 0 || config.jeedom.port == 0)
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
    if (config.emoncms.freq == 0 || config.emoncms.host[0] == 0 || config.emoncms.port == 0)
    {
        timer_emoncms.resetToNeverExpires();
        Serial.println("timer_emoncms disabled");
    }
    else
    {
        timer_emoncms.reset(config.emoncms.freq * 1000);
        Serial.printf("timer_emoncms enabled, freq=%d s\n", config.emoncms.freq);
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

void tic_get_json_dict(String &data)
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

    js.append(FPSTR("_UPTIME"), millis() / 1000);
    js.append(FPSTR("timestamp"), tinfo.get_timestamp_iso8601().c_str());

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

const char *tic_get_value(const char *label)
{
    return tinfo.get_value(label);
}

void http_notif(const char *notif)
{
    String uri;
    uri.reserve(strlen(config.httpReq.url) + 32);

    char label[16];

    for (const char *p = config.httpReq.url; *p; ++p)
    {
        if (*p == '~')
        {
            ++p;
            size_t i = 0;
            while (*p && *p != '~' && i < sizeof(label) - 1)
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
                if (strcmp(label, "_type") == 0)
                {
                    uri += notif;
                }
                else
                {
                    uri += tinfo.get_value(label, "null", true);
                }
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
                while ((isalpha(*p) || *p == '_') && i < sizeof(label) - 1)
                {
                    label[i++] = *p++;
                }
                label[i] = 0;
                --p; // revient sur le dernier caractère du label

                if (strcmp(label, "_type") == 0)
                {
                    uri += notif;
                }
                else
                {
                    uri += tinfo.get_value(label, "null", true);
                }
            }
        }
        else
        {
            uri += *p;
        }
    }

    Serial.printf("http_notif: %s\n", notif);
    http_request(config.httpReq.host, config.httpReq.port, uri);
}

void http_notif_periode_en_cours()
{
    const char *PTEC = tinfo.get_value("PTEC");
    if (PTEC == NULL)
        return;

    // a-t-on un changement de période ?
    if (strncmp(periode_en_cours, PTEC, sizeof(periode_en_cours)) != 0)
    {
        strncpy(periode_en_cours, PTEC, sizeof(periode_en_cours));
        if (init_periode_en_cours)
        {
            // premier passage: on mémorise la période courante
            init_periode_en_cours = false;
        }
        else
        {
            http_notif("PTEC");
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
            http_notif("NORM");
        }
    }
    else
    {
        if (etat_adps == false)
        {
            // on vient de passer en ADPS: on signale et on bascule en mode AsDPS
            etat_adps = true;
            http_notif("ADPS");
        }
    }
}

void http_notif_seuils()
{
    const char *PAPP = tinfo.get_value("PAPP");
    if (PAPP == NULL)
        return;

    long papp = atol(PAPP);
    if (papp == 0)
        return;

    if ((papp >= config.httpReq.seuil_haut) && (seuil_en_cours == BAS))
    {
        seuil_en_cours = HAUT;
        http_notif("HAUT");
    }
    else if ((papp <= config.httpReq.seuil_bas) && (seuil_en_cours == HAUT))
    {
        seuil_en_cours = BAS;
        http_notif("BAS");
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
                value = config.jeedom.adco;
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
            first_item = false;
        else
            url += ",";

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
                url += "1";
            else if (*p == 'H' && *(p + 1) == 'C' && *(p + 2) == '.')
                url += "2";
            else if (*p == 'E' && *(p + 1) == 'J' && *(p + 2) == 'P')
                url += "3";
            else if (*p == 'B' && *(p + 1) == 'B' && *(p + 2) == 'R')
                url += "4";
            else
                url += "0";
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
            if (!strcmp(value, "TH.."))
                url += "1";
            else if (!strcmp(value, "HC.."))
                url += "2";
            else if (!strcmp(value, "HP.."))
                url += "3";
            else if (!strcmp(value, "HN.."))
                url += "4";
            else if (!strcmp(value, "PM.."))
                url += "5";
            else if (!strcmp(value, "HCJB"))
                url += "6";
            else if (!strcmp(value, "HCJW"))
                url += "7";
            else if (!strcmp(value, "HCJR"))
                url += "8";
            else if (!strcmp(value, "HPJB"))
                url += "9";
            else if (!strcmp(value, "HPJW"))
                url += "10";
            else if (!strcmp(value, "HPJR"))
                url += "11";
            else
                url += "0";
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
    if (config.emoncms.host[0] == 0 || tinfo.is_empty())
        return;

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
