/*
 * librairie Teleinfo
 * Copyright (c) 2014-2020 rene-d. All right reserved.
 */
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

// Quelques rappels sur le courant alternatif monophasé:
//  HCHP/HCHC : index en watt*heure (Wh) d'énergie consommée
//  IINST : intensité instantanée en ampère (A)
//  PAPP = puissance apparente en volt-ampère VA = U * I
//  W = puissance active en watt (W) = U * I * cos Φ
//  en France, la tension nominale est 230V à 50Hz

class Teleinfo
{
public:
    static const size_t MAX_FRAME_SIZE = 350;

protected:
    char frame_[MAX_FRAME_SIZE]; // buffer de mémorisation de la trame
    size_t size_{0};             // offset courant (i.e. longueur de la trame)
    timeval timestamp_{0, 0};    // date du début de la trame

    struct conso
    {
        uint32_t date_ms;
        uint32_t watt_heure;
    };

    static const size_t CONSO_MAX = 48; // il y a une mesure toute les 1.4 s
    conso consos_[CONSO_MAX];           // avec 48, on couvre 67 s
    ssize_t i_consos_{0};
    conso conso0_{0, 0};

public:
    Teleinfo()
    {
        memset(&consos_, 0, sizeof(consos_));
    }

    void copy_from(const Teleinfo &tinfo)
    {
        size_ = tinfo.size_;
        memmove(frame_, tinfo.frame_, size_);
        timestamp_ = tinfo.timestamp_;

        if (!is_empty())
        {
            uint32_t wh = get_value_int("HCHP") + get_value_int("HCHC");

            if (conso0_.date_ms == 0)
            {
                // mémorise une valeur initiale pour avoir des valeurs
                // un peu moins grandes dans le tableau consos_[]
                conso0_.date_ms = timestamp_.tv_sec - 1;
                conso0_.watt_heure = wh - 1;
            }

            consos_[i_consos_].date_ms = (timestamp_.tv_sec - conso0_.date_ms) * 1000 + timestamp_.tv_usec / 1000;
            consos_[i_consos_].watt_heure = wh - conso0_.watt_heure;

            i_consos_ = (i_consos_ + 1) % CONSO_MAX;
        }
    }

    // retourne l'estimation de la puissance en watt en dérivant les index
    // de consommation d'énergie qui sont en W.h
    //
    // faute de pouvoir faire beaucoup de calculs et de connaître les valeurs futures,
    // on estime que la variation de la puissance est linéaire sur l'intervalle de temps
    //
    // il faut une période de 60s pour avoir une mesure si la puissance est de 60 W:
    // en 60s l'index va monter de 1 Wh, résolution des index HC/HP.
    uint32_t watt(uint32_t periode = 60) const
    {
        ssize_t actuel = (CONSO_MAX + i_consos_ - 1) % CONSO_MAX;
        uint32_t now = consos_[actuel].date_ms;
        ssize_t premier = actuel;

        periode = periode * 1000; // secondes -> millisecondes

        // cherche la valeur la plus éloignée dans le tableau circulaire
        // tout en restant dans la période
        do
        {
            ssize_t suivant = (CONSO_MAX + premier - 1) % CONSO_MAX;
            if (consos_[suivant].watt_heure == 0)
            {
                break;
            }
            if (now - consos_[suivant].date_ms > periode)
            {
                break;
            }
            premier = suivant;
        } while (premier != i_consos_);

        if (consos_[actuel].date_ms == consos_[premier].date_ms)
        {
            // i.e. actuel == premier (sauf si problème de mesure)
            // pas assez de valeur
            return 0;
        }

#if 0
        // ajustement affine avec toutes les valeurs trouvées

        size_t i = premier;
        int n = 0;
        double m_x = 0;
        double m_y = 0;
        double s_x2 = 0;
        double s_y2 = 0;
        double s_xy = 0;
        double x_prev = consos_[premier].date_ms;

        while (true)
        {
            n += 1;

            double x = consos_[i].date_ms;
            double y = consos_[i].watt_heure;

            if (x < x_prev)
            {
                x += 1000000;
            }
            x_prev = x;

            x = x / 3600000.; // convertit x de millisecondes en heures

            m_x += x;
            m_y += y;

            s_x2 += x * x;
            s_y2 += y * y;
            s_xy += x * y;

            if (i == actuel)
            {
                break;
            }
            i = (i + 1) % CONSO_MAX;
        }

        m_x = m_x / n;                        // moyenne(x)
        m_y = m_y / n;                        // moyenne(y)
        double cov_xy = s_xy / n - m_x * m_y; // Cov(x, y)
        double v_x = s_x2 / n - m_x * m_x;    // V(x)
        double v_y = s_y2 / n - m_y * m_y;    // V(x)

        double r = cov_xy / sqrt(v_x * v_y); // coeff de corrélation linéaire
        double a = cov_xy / v_x;             // Y = a X + b
        double b = m_y - a * m_x;            //

        uint32_t w = round(a);
#else
        // considère uniquement les première et dernière (actuelle) valeurs

        uint32_t wh = consos_[actuel].watt_heure - consos_[premier].watt_heure;
        int32_t t = now - consos_[premier].date_ms;
        if (t < 0)
        {
            t += 1000000;
        }
        uint32_t w = (3600000 * wh) / t;
#endif

        // for (int i = 0; i < CONSO_MAX; ++i)
        // {
        //     char flag = ' ';
        //     if (i == i_consos_)
        //         flag = 'i';
        //     if (i == actuel)
        //         flag = 'a';
        //     if (i == premier)
        //         flag = 'p';
        //     printf("%2d %c %10d %10d\n", i, flag, consos_[i].date_ms, consos_[i].watt_heure);
        // }
        // printf("\tactuel=%2zu  dernier=%2zu   W=%u   a=%.3lf r=%.3lf\n", actuel, premier, w, 0.,0.);

        return w;
    }

    bool is_empty() const
    {
        return size_ == 0;
    }

    const char *get_value(const char *label, const char *default_value = nullptr, bool remove_leading_zeros = false) const
    {
        const char *p = frame_;
        const char *end = frame_ + size_;

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

    uint32_t get_value_int(const char *label, uint32_t default_value = 0) const
    {
        const char *value = get_value(label, nullptr, true);
        if (value == nullptr)
        {
            return default_value;
        }

        // convertit la chaîne en entier
        uint32_t u = 0;
        for (const char *c = value; *c; ++c)
        {
            int8_t digit = (*c - '0');
            if ((digit > 9) || (digit < 0))
            {
                return default_value;
            }
            u = digit + u * 10;
        }
        return u;
    }

    bool get_value_next(const char *&label, const char *&value, char const **state) const
    {
        const char *p = frame_;
        const char *end = frame_ + size_;

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

    time_t get_timestamp() const
    {
        return timestamp_.tv_sec;
    }

    String get_timestamp_iso8601() const
    {
        struct tm *tm = localtime(&timestamp_.tv_sec);
        char buf[32]; // bien suffisant pour date
        strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", tm);
        return String(buf);
    }

    String get_seconds() const
    {
        char buf[32]; // bien suffisant pour les secondes
        snprintf(buf, sizeof(buf), "%ld.%03d", timestamp_.tv_sec, (int)(timestamp_.tv_usec / 1000));
        return String(buf);
    }

    size_t get_frame_ascii(char *frame, size_t size) const
    {
        size_t j = 0;
        bool label = true;
        for (size_t i = 0; i < size_; ++i)
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
    size_t offset_{0};             // offset courant (i.e. longueur de la trame)
    size_t offset_start_group_{0}; // offset de début d'un groupe (état wait_cr)
    enum
    {
        wait_stx,
        wait_lf_or_etx,
        wait_cr
    } state_{wait_stx}; // automate de réception

    int (*time_cb_)(struct timeval *, void *){static_cast<int (*)(struct timeval *, void *)>(::gettimeofday)};

public:
    void set_time_cb(int (*cb)(struct timeval *, void *))
    {
        time_cb_ = cb;
    }

    // trame complète, on peut la lire
    bool ready() const
    {
        return size_ != 0;
    }

    void put(int c)
    {
        size_ = 0;

        if (c == STX)
        {
            // début de trame, on réinitialise et on attend un LF (ou un ETX à la rigueur...)
            offset_ = 0;
            state_ = wait_lf_or_etx;
            time_cb_(&timestamp_, nullptr);
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
                        char sep = frame_[offset_ - 2];

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
                            --offset_; // supprime le checksum

                            // supprime les . qui terminent certaines valeurs (PTEC par exemple)
                            while ((offset_ >= 2) && (frame_[offset_ - 2] == '.'))
                            {
                                --offset_;
                            }

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
                size_ = offset_;
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
