#pragma once

#include <Arduino.h>
#include <sys/time.h>
#include <time.h>

#define STX 0x02 // start of text
#define ETX 0x03 // end of text
#define EOT 0x04 // end of transmission
#define LF 0x0A  // line feed
#define CR 0x0D  // carriage return

/*
<STX>
<LF>ADCO 111111111111 #<CR>
<LF>OPTARIF HC.. <<CR>
<LF>ISOUSC 30 9<CR>
<LF>HCHC 052890470 )<CR>
<LF>HCHP 049126843 8<CR>
<LF>PTEC HP..  <CR>
<LF>IINST 008 _<CR>
<LF>IMAX 042 E<CR>
<LF>PAPP 01890 3<CR>
<LF>HHPHC D /<CR>
<LF>MOTDETAT 000000 B<CR>
<ETX>
*/

class Teleinfo
{
public:
    static const size_t MAX_FRAME_SIZE = 350;

protected:
    char frame_[MAX_FRAME_SIZE]; // buffer de mémorisation de la trame
    size_t offset_{0};           // offset courant (i.e. longueur de la trame)
    timeval timestamp_;          // date du début de la trame

public:
    void copy_from(const Teleinfo &tinfo)
    {
        offset_ = tinfo.offset_;
        memmove(frame_, tinfo.frame_, offset_);
        timestamp_ = tinfo.timestamp_;
    }

    bool is_empty() const
    {
        return offset_ == 0;
    }

    const char *get_value(const char *label, const char *default_value = nullptr, bool remove_leading_zeros = false) const
    {
        const char *p = frame_;
        const char *end = frame_ + offset_;

        while (p < end)
        {
            const char *current_label = p;

            // cherche la fin de l'étiquette
            while (*p != 0)
            {
                if (p >= end)
                    return default_value;
                ++p;
            }

            //saute le 0 sépateur étiquette-valeur
            ++p;
            if (p >= end)
                return default_value;

            const char *current_value = p;

            // cherche la fin de la valeur
            while (*p != 0)
            {
                if (p >= end)
                    return default_value;
                ++p;
            }
            ++p; //saute le 0

            if (strcmp(current_label, label) == 0)
            {
                if (remove_leading_zeros)
                    get_integer(current_value);

                return current_value;
            }
        }

        return default_value;
    }

    bool get_value_next(const char *&label, const char *&value, char const **state) const
    {
        const char *p = frame_;
        const char *end = frame_ + offset_;

        if (state == nullptr)
            return false;
        if (*state != nullptr && (*state < frame_ || *state >= end))
            return false;

        if (*state != nullptr)
            p = *state;

        label = p;

        // cherche la fin de l'étiquette
        while (*p != 0)
        {
            if (p >= end)
                return false;
            ++p;
        }

        //saute le 0 sépateur étiquette-valeur
        ++p;
        if (p >= end)
            return false;

        value = p;

        // cherche la fin de la valeur
        while (*p != 0)
        {
            if (p >= end)
                return false;
            ++p;
        }
        ++p; //saute le 0

        *state = p;
        return true;
    }

    String get_timestamp_iso8601() const
    {
        struct tm *tm = localtime(&timestamp_.tv_sec);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", tm);
        return buf;
    }

    void get_frame_array_json(String &data) const
    {
        if (offset_ == 0)
        {
            data = "[]";
            return;
        }

        data = "[{\"na\":\"timestamp\",\"va\":\"";
        data += get_timestamp_iso8601();
        data += "\", \"fl\":8,\"ck\":0}";

        const char *p = frame_;
        const char *end = frame_ + offset_;
        while (p < end)
        {
            data += ",{\"na\":\"";

            data += p;
            p += strlen(p) + 1;
            data += "\",\"va\":\"";

            if (p >= end)
            {
                data += "ERROR\"}]";
                break;
            }
            data += p;
            data += "\",\"ck\":\"\",\"fl\":8}";
            p += strlen(p) + 1;
        }
        data += "]";
    }

    void get_frame_dict_json(String &data) const
    {
        data = "{\"_UPTIME\":";
        data += millis() / 1000;
        data += ",\"timestamp\":\"";
        data += get_timestamp_iso8601();
        data += "\"";

        const char *p = frame_;
        const char *end = frame_ + offset_;
        while (p < end)
        {
            data += ",\"";
            data += p;

            p += strlen(p) + 1;
            data += "\":";

            if (p >= end)
            {
                data += "\"ERROR\"";
                break;
            }

            if (get_integer(p))
            {
                data += p;
            }
            else
            {
                data += "\"";
                data += p;
                data += "\"";
            }

            p += strlen(p) + 1;
        }
        data += "}";
    }

    size_t get_frame_ascii(char *frame, size_t size) const
    {
        size_t j = 0;
        bool label = true;
        for (size_t i = 0; i < offset_; ++i)
        {
            int c = frame_[i];
            if (c == 0)
            {
                if (label)
                {
                    frame[j++] = ' ';
                }
                else
                {
                    frame[j++] = LF;
                }
                label = !label;
            }
            else
            {
                frame[j++] = c;
            }
            if (j >= size - 2)
            {
                break;
            }
        }
        if (j != 0 && frame[j - 1] != LF)
        {
            // par construction, on doit avoir un LF à la fin
            // sinon la frame est mal formée
            j = 0;
        }
        frame[j] = 0;

        return j;
    }

    static bool get_integer(const char *&value)
    {
        // il faut que value ne contienne que des chiffres
        for (const char *c = value; *c; ++c)
        {
            if (!isdigit(*c))
            {
                return false;
            }
        }
        // supprime les 0, sauf le dernier
        while (*value == '0' && *(value + 1) != 0)
            ++value;
        return true;
    }
};

class TeleinfoDecoder : public Teleinfo
{
    size_t offset_start_group_{0}; // offset de début d'un groupe (état wait_cr)
    enum
    {
        wait_stx,
        wait_lf_or_etx,
        wait_cr
    } state_{wait_stx}; // automate de réception
    bool ready_{false}; // trame complète, on peut la lire

public:
    bool ready() const
    {
        return ready_;
    }

    void put(int c)
    {
        ready_ = false;

        if (c == STX)
        {
            // début de trame, on réinitialise et on attend un LF (ou un ETX à la rigueur...)
            offset_ = 0;
            state_ = wait_lf_or_etx;
            ready_ = false;
            gettimeofday(&timestamp_, nullptr);
        }
        else if (c == LF)
        {
            // début de valeur: on doit être dans l'état adéquant
            if (state_ == wait_lf_or_etx)
            {
                // on passe dans l'état lecture jusqu'au CR
                state_ = wait_cr;
                offset_start_group_ = offset_;
            }
            else
            {
                // sinon, erreur et on attend le début de la trame suivante
                state_ = wait_stx;
            }
        }
        else if (c == CR)
        {
            // fin de donnée
            if (state_ == wait_cr)
            {
                // 4 octets au moins (STX-LF-groupe-crc) en début de trame
                // 2 octets au moins pour finir la trame
                if ((offset_ >= 4) && (offset_ < (MAX_FRAME_SIZE - 2)))
                {
                    if (offset_start_group_ < offset_ - 3)
                    {
                        // séparateur après la donnée
                        int sep = frame_[offset_ - 2];

                        int checksum = frame_[offset_ - 1];

                        // calcul du checksum sur étiquette-séparateur-donnée
                        // (mode de calcul n°1, cf. doc Enedis)
                        int sum = 0;
                        for (size_t i = offset_start_group_; i < offset_ - 2; ++i)
                        {
                            sum += frame_[i];

                            if (frame_[i] == sep)
                            {
                                frame_[i] = 0;
                                sep = -1; // valeur impossible, empêche un deuxième séparateur
                            }
                        }
                        sum = (sum & 63) + 32;

                        if (sum == checksum)
                        {
                            --offset_;               // supprime le checksum
                            frame_[offset_ - 1] = 0; // écrase le dernier séparateur avec 0
                            state_ = wait_lf_or_etx;
                        }
                        else
                        {
                            // mauvais checksum: reinit
                            state_ = wait_stx;
                        }
                    }
                    else
                    {
                        // pas assez de caractères: reinit
                        state_ = wait_stx;
                    }
                }
                else
                {
                    // frame trop longue: reinit
                    state_ = wait_stx;
                }
            }
            else
            {
                // mauvais état: reinit
                state_ = wait_stx;
            }
        }
        else if (c == ETX)
        {
            if (state_ == wait_lf_or_etx)
            {
                ready_ = true;
                state_ = wait_stx;
                //validate_frame();
            }
            else
            {
                // reinit
                state_ = wait_stx;
            }
        }
        else if (c == EOT)
        {
            // cas d'interruption de la trame
            state_ = wait_stx;
        }
        else
        {
            // caractère quelconque
            if (state_ == wait_cr)
            {
                if (offset_ < MAX_FRAME_SIZE)
                {
                    frame_[offset_++] = c;
                }
                else
                {
                    // reinit
                    state_ = wait_stx;
                }
            }
            else
            {
                // reinit
                state_ = wait_stx;
            }
        }
    }
};
