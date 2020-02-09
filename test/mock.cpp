#include "mock.h"

#include "teleinfo.h"
#include "config.h"
#include "sse.h"

// -----------------------------------

extern Teleinfo tinfo;
SseClients sse_clients;

// -----------------------------------

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

class TeleinfoBuilder
{
    std::string frame_;

public:
    TeleinfoBuilder()
    {
        reset();
    }

    void reset()
    {
        frame_ = '\x02';
    }

    void add_group(const char *label, const char *value)
    {
        if (frame_[frame_.length() - 1] == '\x03')
            return;

        frame_ += '\n';
        frame_ += label;
        frame_ += ' ';
        frame_ += value;
        frame_ += ' ';

        int sum = 32; // l'espace de séparation
        for (const char *p = label; *p; sum += *p++)
            ;
        for (const char *p = value; *p; sum += *p++)
            ;
        frame_ += (sum & 63) + 32;

        frame_ += '\r';
    }

    void add_group(const char *label, uint32_t value, uint8_t width)
    {
        char s[16];
        if (width > 15)
            width = 15;
        s[width] = 0;
        while (width != 0)
        {
            s[--width] = '0' + (value % 10);
            value /= 10;
        }
        add_group(label, s);
    }

    const std::string &get() const
    {
        if (frame_[frame_.length() - 1] != '\x03')
            const_cast<std::string &>(frame_) += '\x03';
        return frame_;
    }
};

// initialise Teleinfo avec la trame d'exemple
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

void tinfo_init(uint32_t papp, bool heures_creuses, uint32_t adps)
{
    TeleinfoBuilder trame;

    trame.add_group("ADCO", "111111111111");
    trame.add_group("OPTARIF", "HC..");
    trame.add_group("ISOUSC", "30");
    trame.add_group("ISOUSC", "30");
    trame.add_group("HCHC", 52890470, 9);
    trame.add_group("HCHP", 49126843, 9);
    trame.add_group("PTEC", heures_creuses ? "HC.." : "HP..");
    trame.add_group("IINST", papp / 230, 3);
    trame.add_group("IMAX", 42, 3);
    trame.add_group("PAPP", papp, 5);
    if (adps != 0)
        trame.add_group("ADPS", adps, 5);
    trame.add_group("HHPHC", "A");
    trame.add_group("MOTDETAT", 0, 6);

    // décode la trame
    TeleinfoDecoder decode;
    for (auto c : trame.get())
    {
        decode.put(c);
    }
    ASSERT_TRUE(decode.ready());

    tinfo.copy_from(decode);
}
