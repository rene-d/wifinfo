#include "mock.h"

#include "teleinfo.h"

// mauvais crc (étiquette ADCO)
static const std::string test_trame_ko = "\
\x02\
\nADCO 111111111111 A\r\
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

static const std::string test_trame_partielle_debut = "\
\x02\
\nADCO 111111111111 #\r\
\nOPTARIF HC.. <\r\
\nISOUSC 30 9\r\
\nHCHC 052890";

static const std::string test_trame_partielle_fin = "\
ST 008 _\r\
\nIMAX 042 E\r\
\nPAPP 01890 3\r\
\nHHPHC D /\r\
\nMOTDETAT 000000 B\r\
\x03";

static const std::string test_trame_incomplete = "\
\x02\
\nADCO 111111111111 A\r\
\nOPTARIF HC.. <\r\
\nISOUSC 30 9\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nPTEC HP.. \
\n E\r\
\nPAPP 01890 3\r\
\nHHPHC D /\r\
\nMOTDETAT 000000 B\r\
\x03";

static const std::string test_trame_mauvais_groupe = "\
\x02\
\nADCO 111111111111 A\r\
\nOPTARIF HC.. <\r\
\nISOUSC 30 9\r\
\nH\r\
\nHCHP 049126843 8\r\
\nPTEC HP..  \r\
\nIINST 008 _\r\
\nIMAX 042 E\r\
\nPAPP 01890 3\r\
\nHHPHC D /\r\
\nMOTDETAT 000000 B\r\
\x03";

static const std::string test_trame_trop_longue = "\
\x02\
\nADCO 111111111111 A\r\
\nOPTARIF HC.. <\r\
\nISOUSC 30 9\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nHCHC 052890470 )\r\
\nHCHP 049126843 8\r\
\nPTEC HP.. \
\n E\r\
\nPAPP 01890 3\r\
\nHHPHC D /\r\
\nMOTDETAT 000000 B\r\
\x03";

static const std::string test_trame_groupe_trop_long = "\
\x02\
\nADCO 111111111111 A\r\
\nOPTARIF HC.. <\r\
\nISOUSC 30 9\r\
\nHCHC AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\
\nPTEC HP.. \
\n E\r\
\nPAPP 01890 3\r\
\nHHPHC D /\r\
\nMOTDETAT 000000 B\r\
\x03";

TEST(teleinfo, decode_ok)
{
    TeleinfoDecoder tinfo_decode;

    for (auto c : trame_teleinfo)
    {
        tinfo_decode.put(c);
    }

    // la trame doit être décodée
    ASSERT_TRUE(tinfo_decode.ready());

    // la valeur IINST doit être 008
    ASSERT_STREQ(tinfo_decode.get_value("IINST"), "008");
    ASSERT_STREQ(tinfo_decode.get_value("IINST", "123"), "008");

    // la valeur inexistante peut être remplacée par une valeur par défaut
    ASSERT_STREQ(tinfo_decode.get_value("unknown"), nullptr);
    ASSERT_STREQ(tinfo_decode.get_value("unknown", "123"), "123");
}

TEST(teleinfo, decode_int)
{
    const char *value;

    value = "000123";
    ASSERT_TRUE(Teleinfo::get_integer(value));
    ASSERT_STREQ(value, "123");

    value = "000";
    ASSERT_TRUE(Teleinfo::get_integer(value));
    ASSERT_STREQ(value, "0");

    value = "000123a";
    ASSERT_FALSE(Teleinfo::get_integer(value));
    ASSERT_STREQ(value, "000123a");
}

TEST(teleinfo, decode_ko_getters)
{
    TeleinfoDecoder tinfo_decode;
    Teleinfo tinfo;
    const char *label;
    const char *value;
    const char *state;
    String js;
    char raw[Teleinfo::MAX_FRAME_SIZE];

    for (auto c : test_trame_ko)
    {
        tinfo_decode.put(c);
    }

    // la trame ne doit pas décodée
    ASSERT_FALSE(tinfo_decode.ready());

    // ni copiée
    tinfo.copy_from(tinfo_decode);
    ASSERT_TRUE(tinfo.is_empty());

    // pas de valeur
    ASSERT_EQ(tinfo.get_value("ADCO"), nullptr);

    state = nullptr;
    ASSERT_FALSE(tinfo.get_value_next(label, value, &state));

    // pas de données
    tinfo.get_frame_array_json(js);
    ASSERT_EQ(js, "[]");

    tinfo.get_frame_dict_json(js);
    ASSERT_EQ(js, "{}");

    tinfo.get_frame_ascii(raw, sizeof(raw));
    ASSERT_STREQ(raw, "");
}

TEST(teleinfo, decode_ko)
{
    TeleinfoDecoder tinfo_decode;

    for (auto c : test_trame_ko)
    {
        tinfo_decode.put(c);
    }

    // la trame ne doit pas décodée
    ASSERT_FALSE(tinfo_decode.ready());

    // trame partielle début
    for (auto c : test_trame_partielle_debut)
    {
        tinfo_decode.put(c);
    }
    ASSERT_FALSE(tinfo_decode.ready());
    ASSERT_TRUE(tinfo_decode.is_empty());

    // la réception d'une trame ok doit réussir
    for (auto c : trame_teleinfo)
    {
        tinfo_decode.put(c);
    }
    ASSERT_TRUE(tinfo_decode.ready());

    // trame partielle fin
    for (auto c : test_trame_partielle_fin)
    {
        tinfo_decode.put(c);
    }
    ASSERT_FALSE(tinfo_decode.ready());

    // la réception d'une trame ok doit réussir
    for (auto c : trame_teleinfo)
    {
        tinfo_decode.put(c);
    }
    ASSERT_TRUE(tinfo_decode.ready());

    // la réception d'une trame incomplète doit échouer
    for (auto c : test_trame_incomplete)
    {
        tinfo_decode.put(c);
    }
    ASSERT_FALSE(tinfo_decode.ready());

    // la réception d'une trame incomplète doit échouer
    for (auto c : test_trame_trop_longue)
    {
        tinfo_decode.put(c);
    }
    ASSERT_FALSE(tinfo_decode.ready());

    // tout et n'importe quoi
    for (int c = 0; c <= 127; ++c)
    {
        tinfo_decode.put(c);
    }
    ASSERT_FALSE(tinfo_decode.ready());

    // pas de place pour le crc dans un groupe
    for (auto c : test_trame_mauvais_groupe)
    {
        tinfo_decode.put(c);
    }
    ASSERT_FALSE(tinfo_decode.ready());

    // test_trame_groupe_trop_long
    for (auto c : test_trame_groupe_trop_long)
    {
        tinfo_decode.put(c);
    }
    ASSERT_FALSE(tinfo_decode.ready());

    // la réception d'une trame ok doit réussir après tous ces échecs
    for (auto c : trame_teleinfo)
    {
        tinfo_decode.put(c);
    }
    ASSERT_TRUE(tinfo_decode.ready());
}

TEST(teleinfo, copy)
{
    TeleinfoDecoder tinfo_decode;
    Teleinfo tinfo;

    for (auto c : trame_teleinfo)
    {
        tinfo_decode.put(c);
    }

    tinfo.copy_from(tinfo_decode);

    // la trame ne doit pas décodée
    ASSERT_STREQ(tinfo.get_value("PAPP"), "01890");

    // le timestamp est celui par défaut
    ASSERT_EQ(tinfo.get_timestamp_iso8601(), test_time_marker());
}

TEST(teleinfo, json)
{
    TeleinfoDecoder tinfo_decode;
    Teleinfo tinfo;

    for (auto c : trame_teleinfo)
    {
        tinfo_decode.put(c);
    }

    tinfo.copy_from(tinfo_decode);

    // la trame ne doit pas décodée
    ASSERT_STREQ(tinfo.get_value("PAPP"), "01890");

    String output;

    tinfo.get_frame_array_json(output);
    //std::cout << output << std::endl;
    auto j1 = json::parse(output.s);
    ASSERT_TRUE(j1.is_array());
    ASSERT_EQ(j1.size(), 12);
    ASSERT_EQ(j1[1]["va"], "111111111111");

    tinfo.get_frame_dict_json(output);
    //std::cout << output << std::endl;
    auto j2 = json::parse(output.s);
    ASSERT_EQ(j2["OPTARIF"], "HC..");
    ASSERT_EQ(j2["HCHC"], 52890470);
    ASSERT_EQ(j2["HCHP"], 49126843);
    ASSERT_EQ(j2["PTEC"], "HP..");
    ASSERT_EQ(j2["MOTDETAT"], 0);
}

TEST(teleinfo, ascii)
{
    TeleinfoDecoder tinfo_decode;
    Teleinfo tinfo;

    for (auto c : trame_teleinfo)
    {
        tinfo_decode.put(c);
    }

    tinfo.copy_from(tinfo_decode);

    char output[Teleinfo::MAX_FRAME_SIZE];
    size_t sz = tinfo.get_frame_ascii(output, sizeof(output));
    //std::cout << sz << std::endl;
    //std::cout << output << std::endl;
    ASSERT_NE(sz, 0);
}

TEST(teleinfo, iterate)
{
    TeleinfoDecoder tinfo_decode;
    Teleinfo tinfo;

    for (auto c : trame_teleinfo)
    {
        tinfo_decode.put(c);
    }

    tinfo.copy_from(tinfo_decode);

    const char *label;
    const char *value;
    const char *state = nullptr;
    size_t nb = 0;
    while (tinfo.get_value_next(label, value, &state))
    {
        // std::cout << label << " : " << value << std::endl;
        if (strcmp(label, "PAPP") == 0)
        {
            ASSERT_STREQ(value, "01890");
        }
        ++nb;
    }

    // 11 valeurs dans la trame de téléinformation
    ASSERT_EQ(nb, 11);
}
