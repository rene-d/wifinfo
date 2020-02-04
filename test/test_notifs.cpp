#include "mock.h"
#include <ESP8266HTTPClient.h>

#include "tic.cpp"
#include "httpreq.cpp"

TEST(notifs, http)
{
    tinfo_init(1800, false);

    strcpy(config.jeedom.host, "");
    strcpy(config.emoncms.host, "");
    strcpy(config.httpReq.host, "");

    strcpy(config.httpReq.host, "sql.home");
    config.httpReq.port = 88;
    config.httpReq.freq = 15;
    config.httpReq.trigger_hchp = 1;
    config.httpReq.trigger_seuils = 0;
    config.httpReq.trigger_adps = 0;
    config.httpReq.seuil_haut = 5900;
    config.httpReq.seuil_bas = 4200;

    // syntaxe 1: %HCHC%
    /*
    strcpy(config.httpReq.path, "/tic.php?hchc=%HCHC%&hchp=%HCHP%&papp=%PAPP%&pct=%%");
    HTTPClient::begin_called = 0;
    http_notif("GEN");
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tic.php?hchc=52890470&hchp=49126843&papp=1800&pct=%");
    */

    // syntaxe 2: $HCHC
    strcpy(config.httpReq.path, "/tic.php?hchc=$HCHC&hchp=$HCHP&papp=$PAPP");
    HTTPClient::begin_called = 0;
    http_notif("GEN");
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tic.php?hchc=52890470&hchp=49126843&papp=1800");
    ASSERT_EQ(HTTPClient::begin_port, 88);
    ASSERT_EQ(HTTPClient::begin_host, "sql.home");

    // pas de requête http
    strcpy(config.httpReq.host, "");
    HTTPClient::begin_called = 0;
    http_notif("GEN");
    ASSERT_EQ(HTTPClient::begin_called, 0);
}

TEST(notifs, http_timer)
{
    tinfo_init(1234, false);

    strcpy(config.jeedom.host, "");
    strcpy(config.emoncms.host, "");
    strcpy(config.httpReq.host, "");

    strcpy(config.httpReq.host, "sql.home");
    config.httpReq.port = 88;
    strcpy(config.httpReq.path, "/tic.php?p=$PAPP&t=$_type");
    config.httpReq.freq = 15;
    config.httpReq.trigger_hchp = 1;
    config.httpReq.trigger_seuils = 0;
    config.httpReq.trigger_adps = 0;
    config.httpReq.seuil_haut = 5900;
    config.httpReq.seuil_bas = 4200;

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
    ASSERT_EQ(HTTPClient::begin_url, "/tic.php?p=1234&t=GEN");

    // pas d'échéance timer: pas de requête
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);

    // échéance timer: il y a une requête
    tinfo_init(4321, false);
    timer_http.trigger();
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
    ASSERT_EQ(HTTPClient::begin_url, "/tic.php?p=4321&t=GEN");
}

TEST(notifs, http_ptec)
{
    tinfo_init();

    strcpy(config.jeedom.host, "");
    strcpy(config.emoncms.host, "");
    strcpy(config.httpReq.host, "");

    strcpy(config.httpReq.host, "sql.home");
    config.httpReq.port = 88;
    strcpy(config.httpReq.path, "/tic.php?p=$PAPP&ptec=$PTEC&t=$_type");
    config.httpReq.freq = 15;
    config.httpReq.trigger_hchp = 0;
    config.httpReq.trigger_seuils = 0;
    config.httpReq.trigger_adps = 0;
    config.httpReq.seuil_haut = 5900;
    config.httpReq.seuil_bas = 4200;

    // active les notifs de période en cours
    config.httpReq.trigger_hchp = 1;

    HTTPClient::begin_called = 0;
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);

    // test passage en heures creuses
    // notifs activées: il y a une requête
    tinfo_init(1800, true);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tic.php?p=1800&ptec=HC..&t=HC");

    // repassage en heures pleines
    // notifs activées: il y a une requête
    tinfo_init(1000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
    ASSERT_EQ(HTTPClient::begin_url, "/tic.php?p=1000&ptec=HP..&t=HP");

    // désactive les notifs de période en cours
    config.httpReq.trigger_hchp = 0;

    // test passage en heures creuses
    // notifs désactivées: pas de requête
    tinfo_init(1800, true);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);

    // repassage en heures pleines
    // notifs désactivées: pas de requête
    tinfo_init(1800, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
}

TEST(notifs, http_seuils)
{
    tinfo_init();

    strcpy(config.jeedom.host, "");
    strcpy(config.emoncms.host, "");
    strcpy(config.httpReq.host, "");

    strcpy(config.httpReq.host, "sql.home");
    config.httpReq.port = 88;
    strcpy(config.httpReq.path, "/tic.php?p=$PAPP&t=$_type");
    config.httpReq.freq = 15;
    config.httpReq.trigger_hchp = 0;
    config.httpReq.trigger_seuils = 0;
    config.httpReq.trigger_adps = 0;
    config.httpReq.seuil_haut = 5900;
    config.httpReq.seuil_bas = 4200;

    // active les notifs de seuils
    config.httpReq.trigger_seuils = 1;

    HTTPClient::begin_called = 0;
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 0);

    // dépassement seuil haut (sans repasser par le seuil bas)

    tinfo_init(6000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_url, "/tic.php?p=6000&t=HAUT");

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
    ASSERT_EQ(HTTPClient::begin_url, "/tic.php?p=4000&t=BAS");

    tinfo_init(5000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);

    tinfo_init(4000, false);
    tic_notifs();
    ASSERT_EQ(HTTPClient::begin_called, 2);
}

TEST(notifs, jeedom)
{
    tinfo_init();

    strcpy(config.jeedom.host, "");
    strcpy(config.emoncms.host, "");
    strcpy(config.httpReq.host, "");

    strcpy(config.jeedom.host, "jeedom.home");
    config.jeedom.port = 80;
    strcpy(config.jeedom.url, "/url.php");
    strcpy(config.jeedom.adco, "");
    config.jeedom.freq = 300;

    // notification normale
    HTTPClient::begin_called = 0;
    jeedom_notif();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_port, 80);
    ASSERT_EQ(HTTPClient::begin_host, "jeedom.home");
    ASSERT_EQ(HTTPClient::begin_url, "/url.php?api=&ADCO=111111111111&OPTARIF=HC..&ISOUSC=30&HCHC=052890470&HCHP=049126843&PTEC=HP..&IINST=008&IMAX=042&PAPP=01890&HHPHC=D&MOTDETAT=000000");

    // ADCO fixé
    strcpy(config.jeedom.adco, "12345678");
    HTTPClient::begin_called = 0;
    jeedom_notif();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_port, 80);
    ASSERT_EQ(HTTPClient::begin_host, "jeedom.home");
    ASSERT_EQ(HTTPClient::begin_url, "/url.php?api=&ADCO=12345678&OPTARIF=HC..&ISOUSC=30&HCHC=052890470&HCHP=049126843&PTEC=HP..&IINST=008&IMAX=042&PAPP=01890&HHPHC=D&MOTDETAT=000000");

    // pas de jeedom configuré: pas d'envoi http
    strcpy(config.jeedom.host, "");
    HTTPClient::begin_called = 0;
    jeedom_notif();
    ASSERT_EQ(HTTPClient::begin_called, 0);
}

TEST(notifs, emoncms)
{
    tinfo_init();

    strcpy(config.jeedom.host, "");
    strcpy(config.emoncms.host, "");
    strcpy(config.httpReq.host, "");

    strcpy(config.emoncms.host, "emoncms.home");
    config.emoncms.port = 8080;
    strcpy(config.emoncms.url, "/e.php");
    strcpy(config.emoncms.apikey, "key");
    config.emoncms.node = 1;
    config.emoncms.freq = 300;

    // notification normale
    HTTPClient::begin_called = 0;
    emoncms_notif();
    ASSERT_EQ(HTTPClient::begin_called, 1);
    ASSERT_EQ(HTTPClient::begin_port, 8080);
    ASSERT_EQ(HTTPClient::begin_host, "emoncms.home");
    ASSERT_EQ(HTTPClient::begin_url, "/e.php?node=1&apikey=key&json={ADCO:111111111111,OPTARIF:2,ISOUSC:30,HCHC:52890470,HCHP:49126843,PTEC:3,IINST:8,IMAX:42,PAPP:1890,HHPHC:68,MOTDETAT:0}");

    // pas de jeedom configuré: pas d'envoi http
    strcpy(config.emoncms.host, "");
    emoncms_notif();
    ASSERT_EQ(HTTPClient::begin_called, 1);
}
