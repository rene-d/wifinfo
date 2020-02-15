#include "mock.h"
#include "mock_time.h"
#include <ESP8266HTTPClient.h>

#include "tic.cpp"
#include "httpreq.cpp"

static const Teleinfo empty_tinfo{};

//
static void test_reset_timers()
{
    timer_emoncms.resetToNeverExpires();
    timer_jeedom.resetToNeverExpires();
    timer_http.resetToNeverExpires();
}

// configure les trois types de notifications
static void test_config_notif(bool emoncms, bool jeedom, bool httpreq)
{
    test_reset_timers();

    if (emoncms)
    {
        strcpy(config.emoncms.host, "emoncms.home");
        config.emoncms.port = 8080;
        strcpy(config.emoncms.url, "/e.php");
        strcpy(config.emoncms.apikey, "key");
        config.emoncms.node = 1;
        config.emoncms.freq = 300;
    }
    else
    {
        strcpy(config.emoncms.host, "");
    }

    if (jeedom)
    {
        strcpy(config.jeedom.host, "jeedom.home");
        config.jeedom.port = 80;
        strcpy(config.jeedom.url, "/url.php");
        strcpy(config.jeedom.adco, "");
        config.jeedom.freq = 300;
    }
    else
    {
        strcpy(config.jeedom.host, "");
    }

    if (httpreq)
    {
        strcpy(config.httpReq.host, "sql.home");
        config.httpReq.port = 88;
        strcpy(config.httpReq.url, "/tinfo.php?hchc=$HCHC&hchp=$HCHP&papp=$PAPP");
        config.httpReq.freq = 15;
        config.httpReq.trigger_ptec = 0;
        config.httpReq.trigger_seuils = 0;
        config.httpReq.trigger_adps = 0;
        config.httpReq.seuil_haut = 5900;
        config.httpReq.seuil_bas = 4200;
    }
    else
    {
        strcpy(config.httpReq.host, "");
    }
}

// vérification de la génération des trames téléinfo de test
//
TEST(tic, tinfo_builder)
{
    ASSERT_TRUE(empty_tinfo.is_empty());

    tinfo.copy_from(empty_tinfo);
    ASSERT_TRUE(tinfo.is_empty());

    tinfo.copy_from(empty_tinfo);
    tinfo_init(100, true, 0);
    ASSERT_FALSE(tinfo.is_empty());

    tinfo.copy_from(empty_tinfo);
    tinfo_init(200, false, 0);
    ASSERT_FALSE(tinfo.is_empty());

    tinfo.copy_from(empty_tinfo);
    tinfo_init(300, true, 100);
    ASSERT_FALSE(tinfo.is_empty());

    tinfo.copy_from(empty_tinfo);
    tinfo_init(400, false, 200);
    ASSERT_FALSE(tinfo.is_empty());
}

// test du cas général
//
TEST(tic, notif_aucun)
{
    test_config_notif(false, false, false);

    tinfo.copy_from(empty_tinfo);
    ASSERT_TRUE(tinfo.is_empty());

    for (auto c : trame_teleinfo)
    {
        tic_decode(c);
    }

    ASSERT_FALSE(tinfo.is_empty());
}

// test du cas général avec les 3 notifs
//
TEST(tic, notif_tous)
{
    test_config_notif(true, true, true);

    tinfo.copy_from(empty_tinfo);
    ASSERT_TRUE(tinfo.is_empty());

    HTTPClient::begin_called = 0;

    timer_emoncms.trigger();
    timer_jeedom.trigger();
    timer_http.trigger();

    for (auto c : trame_teleinfo)
    {
        tic_decode(c);
    }

    ASSERT_FALSE(tinfo.is_empty());
    ASSERT_EQ(HTTPClient::begin_called, 3);
}

// test de la génération de la requête http
//
TEST(notifs, http_notif)
{
    test_config_notif(false, false, true);

    tinfo_init(1800, false);

    // syntaxe 1: ~HCHC~
    strcpy(config.httpReq.url, "/tinfo.php?hchc=~HCHC~&hchp=~HCHP~&papp=~PAPP~&o=$OPTARIF&tilde=~~");
    HTTPClient::begin_called = 0;
    http_notif("MAJ");
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?hchc=52890470&hchp=49126843&papp=1800&o=HC&tilde=~");
    ASSERT_EQ(HTTPClient::begin_port, 88);
    ASSERT_EQ(HTTPClient::begin_host, "sql.home");

    // syntaxe 2: $HCHC
    strcpy(config.httpReq.url, "/tinfo.php?hchc=$HCHC&hchp=$HCHP&papp=$PAPP&ptec=$PTEC&dollar=$$");
    HTTPClient::begin_called = 0;
    http_notif("MAJ");
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?hchc=52890470&hchp=49126843&papp=1800&ptec=HP&dollar=$");

    // les étiquettes spéciales (non case sensitive)
    strcpy(config.httpReq.url, "/maj?id=$ChipID&i=$PAPP&y=$TYPE&t=$TimeStamp&d=$Date&r=$rien&blah");
    HTTPClient::begin_called = 0;
    http_notif("XYZ");
    String url = "/maj?id=0x0012AB&i=1800&y=XYZ&t=" + String(mock_time_timestamp()) + "&d=" + String(mock_time_marker()) + "&r=&blah";
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, url);
    ASSERT_EQ(HTTPClient::begin_port, 88);
    ASSERT_EQ(HTTPClient::begin_host, "sql.home");
}

// test du déclenchement de la notif http
//
TEST(notifs, http_timer)
{
    test_config_notif(false, false, true);
    strcpy(config.httpReq.url, "/tinfo.php?p=$PAPP&t=$type");

    tinfo_init(1234, false);

    // pas de timer: pas de requête
    HTTPClient::begin_called = 0;
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);

    // échéance timer: il y a une requête
    timer_http.trigger();
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?p=1234&t=MAJ");

    // pas d'échéance timer: pas de requête
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);

    // échéance timer: il y a une requête
    tinfo_init(4321, false);
    timer_http.trigger();
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?p=4321&t=MAJ");

    // pas de requête http même si le timer se déclenche
    strcpy(config.httpReq.host, "");
    HTTPClient::begin_called = 0;
    timer_http.trigger();
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);
}

// test du déclenchement sur changement de Période Tarifaire En Cours
//
TEST(notifs, http_ptec)
{
    test_config_notif(false, false, true);
    strcpy(config.httpReq.url, "/tinfo.php?p=$PAPP&ptec=$PTEC&t=$type");
    config.httpReq.trigger_ptec = 1; // active les notifs de PTEC

    tinfo_init();

    HTTPClient::begin_called = 0;
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);

    // test passage en heures creuses
    // notifs activées: il y a une requête
    tinfo_init(1800, true);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?p=1800&ptec=HC&t=PTEC");

    // pas de changement: pas de notif supplémentaire
    tinfo_init(2800, true);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);

    // repassage en heures pleines
    // notifs activées: il y a une requête
    tinfo_init(1000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?p=1000&ptec=HP&t=PTEC");

    // désactive les notifs de période en cours
    config.httpReq.trigger_ptec = 0;

    // test passage en heures creuses
    // notifs désactivées: pas de requête
    tinfo_init(1900, true);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);

    // repassage en heures pleines
    // notifs désactivées: pas de requête
    tinfo_init(1100, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
}

TEST(notifs, http_seuils)
{
    test_config_notif(false, false, true);

    tinfo_init();

    // active les notifs de seuils
    strcpy(config.httpReq.url, "/tinfo.php?p=$PAPP&t=$type");
    config.httpReq.trigger_seuils = 1;

    HTTPClient::begin_called = 0;
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);

    // dépassement seuil haut (sans repasser par le seuil bas)

    tinfo_init(6000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?p=6000&t=HAUT");

    tinfo_init(5000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);

    tinfo_init(6000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);

    // retour seuil bas (sans repasser au-dessus du seuil haut)

    tinfo_init(4000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?p=4000&t=BAS");

    tinfo_init(5000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);

    tinfo_init(4000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
}

// test de la notification d'Avertissement de Dépassement de Puissance Souscrite
//
TEST(notifs, http_adps)
{
    test_config_notif(false, false, true);

    // active les notifications de dépassement
    strcpy(config.httpReq.url, "/tinfo.php?p=$PAPP&t=$type");
    config.httpReq.trigger_adps = 1;

    HTTPClient::begin_called = 0;

    tinfo_init(5000, true, 0);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);

    tinfo_init(6000, true, 0);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);

    tinfo_init(7000, true, 10);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?p=7000&t=ADPS");

    tinfo_init(7200, true, 11);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);

    tinfo_init(4000, true, 0);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
    ASSERT_EQ(HTTPClient::begin_url, "/tinfo.php?p=4000&t=NORM");

    tinfo_init(1000, true, 0);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
}

TEST(notifs, jeedom)
{
    test_config_notif(false, true, false);

    tinfo_init();

    // notification normale
    HTTPClient::begin_called = 0;
    jeedom_notif();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_port, 80);
    ASSERT_EQ(HTTPClient::begin_host, "jeedom.home");
    ASSERT_EQ(HTTPClient::begin_url, "/url.php?api=&ADCO=111111111111&OPTARIF=HC&ISOUSC=30&HCHC=052890470&HCHP=049126843&PTEC=HP&IINST=008&IMAX=042&PAPP=01890&HHPHC=D&MOTDETAT=000000");

    // ADCO fixé
    strcpy(config.jeedom.adco, "12345678");
    HTTPClient::begin_called = 0;
    jeedom_notif();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_port, 80);
    ASSERT_EQ(HTTPClient::begin_host, "jeedom.home");
    ASSERT_EQ(HTTPClient::begin_url, "/url.php?api=&ADCO=12345678&OPTARIF=HC&ISOUSC=30&HCHC=052890470&HCHP=049126843&PTEC=HP&IINST=008&IMAX=042&PAPP=01890&HHPHC=D&MOTDETAT=000000");

    // pas de jeedom configuré: pas d'envoi http
    strcpy(config.jeedom.host, "");
    HTTPClient::begin_called = 0;
    jeedom_notif();
    ASSERT_EQ(HTTPClient::begin_called, 0);
}

TEST(notifs, emoncms)
{
    tinfo_init();

    test_config_notif(true, false, false);

    // notification normale
    HTTPClient::begin_called = 0;
    emoncms_notif();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_port, 8080);
    ASSERT_EQ(HTTPClient::begin_host, "emoncms.home");
    ASSERT_EQ(HTTPClient::begin_url, "/e.php?node=1&apikey=key&json={ADCO:111111111111,OPTARIF:2,ISOUSC:30,HCHC:52890470,HCHP:49126843,PTEC:3,IINST:8,IMAX:42,PAPP:1890,HHPHC:68,MOTDETAT:0}");

    // pas de emoncms configuré: pas d'envoi http
    strcpy(config.emoncms.host, "");
    emoncms_notif();
    ASSERT_EQ(HTTPClient::begin_called, 1);
}

TEST(tic, json_empty)
{
    String data;

    tinfo.copy_from(empty_tinfo);

    // pas de données
    tic_get_json_array(data);
    ASSERT_EQ(data, "[]");

    tic_get_json_dict(data);
    ASSERT_EQ(data, "{}");
}

TEST(tic, json)
{
    TeleinfoDecoder tinfo_decode;

    for (auto c : trame_teleinfo)
    {
        tinfo_decode.put(c);
    }

    tinfo.copy_from(tinfo_decode);

    // la trame ne doit pas décodée
    ASSERT_STREQ(tinfo.get_value("PAPP"), "01890");

    String output;

    tic_get_json_array(output);
    // std::cout << output << std::endl;
    auto j1 = json::parse(output.s);
    ASSERT_TRUE(j1.is_array());
    ASSERT_EQ(j1.size(), 12);
    ASSERT_EQ(j1[1]["va"], "111111111111");

    tic_get_json_dict(output);
    // std::cout << output << std::endl;
    auto j2 = json::parse(output.s);
    ASSERT_EQ(j2["OPTARIF"], "HC");
    ASSERT_EQ(j2["HCHC"], 52890470);
    ASSERT_EQ(j2["HCHP"], 49126843);
    ASSERT_EQ(j2["PTEC"], "HP");
    ASSERT_EQ(j2["MOTDETAT"], 0);
}
