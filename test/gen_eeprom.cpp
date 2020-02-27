// module téléinformation client
// rene-d 2020

#include "config.cpp"
#include <getopt.h>
#include <unistd.h>

#ifndef DEBUG
EmptySerialClass EmptySerial;
#endif

void tic_make_timers()
{
}

// class EmulateWebServer : public ESP8266WebServer
// {
// public:
//     virtual bool hasArg(const String &name) const override
//     {
//         printf("has name %s\n", name.c_str());
//         return true;
//     }
//     virtual String arg(const String &name) const override
//     {
//         printf("get name %s\n", name.c_str());
//         return "1";
//     }
// };

int main(int argc, char *argv[])
{
    int opt;
    const char *output_file = nullptr;
    // EmulateWebServer server;

    config_reset();

    while ((opt = getopt(argc, argv, "ho:")) != -1)
    {
        switch (opt)
        {
        case 'h':
            printf("Crée une configuration valide pour test eeprom.py\n");
            printf("Usage: %s [-h | -o <FILENAME>]\n", argv[0]);
            exit(0);
            break;
        case 'o':
            output_file = optarg;
            break;
        }
    }

    //config_handle_form(server);

    strncpy_s(config.ssid, "reseauwifi", CFG_SSID_LENGTH);
    strncpy_s(config.psk, "motdepasse", CFG_PSK_LENGTH);
    strncpy_s(config.ap_psk, "motdepasse", CFG_PSK_LENGTH);
    config.httpreq.freq = 300;
    config.httpreq.trigger_ptec = 1;

    config_save();

    if (output_file != nullptr)
    {
        FILE *f = fopen(output_file, "wb");
        fwrite(&config, 1, sizeof(config), f);
        fclose(f);
    }
    else
    {
        if (isatty(1))
        {
            fprintf(stderr, "Ne peut écrire le fichier binaire dans un terminal.\n");
            exit(1);
        }
        else
        {
            write(1, &config, sizeof(config));
        }
    }

    return 0;
}
