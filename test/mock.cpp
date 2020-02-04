#include "mock.h"

#include <ESP8266HTTPClient.h>
#include <PolledTimeout.h>
#include "teleinfo.h"
#include "config.h"
#include "sse.h"

#include <cassert>

extern Teleinfo tinfo;
_Config config;
SseClients sse_clients;

SerialClass Serial;

const std::string trame_teleinfo = "\
\x02\
\nADCO 111111111111 #\r\
\nOPTARIF HC.. <\r\
\nISOUSC 30 9\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nPTEC HP..  \r\
\nIINST 008 _\r\
\nIMAX 042 E\r\
\nPAPP 01890 3\r\
\nHHPHC D /\r\
\nMOTDETAT 000000 B\r\
\x03";

static const char trame_teleinfo_papp[] = "\
\x02\
\nADCO 111111111111 #\r\
\nOPTARIF HC.. <\r\
\nISOUSC 30 9\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nPTEC H%c.. %c\r\
\nIINST 008 _\r\
\nIMAX 042 E\r\
\nPAPP %05u %c\r\
\nHHPHC D /\r\
\nMOTDETAT 000000 B\r\
\x03";

void tinfo_init()
{
    TeleinfoDecoder decode;
    for (auto c : trame_teleinfo)
    {
        decode.put(c);
    }
    assert(decode.ready());
    tinfo.copy_from(decode);
}

void tinfo_init(uint32_t papp, bool heures_creuses)
{
    char buf[Teleinfo::MAX_FRAME_SIZE];

    // recalcule le checksum pour "PAPP <papp> "
    papp = papp % 100000;
    int cksum = 0;
    cksum = 'P' + 'A' + 'P' + 'P' + ' ';
    uint32_t v = papp;
    for (int i = 0; i < 5; ++i)
    {
        cksum += '0' + v % 10;
        v /= 10;
    }
    cksum = (cksum & 63) + 32;
    snprintf(buf, sizeof(buf), trame_teleinfo_papp,
             heures_creuses ? 'C' : 'P',
             heures_creuses ? 'S' : ' ', // checksum pour HC.. et HP..
             papp, cksum);

    // décode la trame
    TeleinfoDecoder decode;
    for (const char *c = buf; *c; ++c)
    {
        decode.put(*c);
    }
    assert(decode.ready());
    tinfo.copy_from(decode);
}

int HTTPClient::begin_called = 0;
std::string HTTPClient::begin_host;
uint16_t HTTPClient::begin_port;
std::string HTTPClient::begin_url;

void HTTPClient::begin(WiFiClient &, const char *host, uint16_t port, const String &url)
{
    begin_called++;
    begin_host = host;
    begin_port = port;
    begin_url = url;
}
