/**
 * Microtuning functions
 */

#ifdef TESTS
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#endif

#include <cassert>
#include <cmath>

#include "libMTSClient.h"

/**
 * Pull all 128 MTS-ESP frequencies into an array.
 */
static void getMTSESPFrequencies(double *frequency)
{
    MTSClient *client = MTS_RegisterClient();
    int i;
    for (i = 0; i <= 127; i++)
    {
        frequency[i] = MTS_NoteToFrequency(client, i, 0);
    }
    MTS_DeregisterClient(client);
}

/**
 * Try to infer the scale size (number of notes in the scale) and period
 * (interval which the scale repeats at) from a table of 128 frequencies.  For
 * example for 12TET the scale size is 12 and the period is 2 (the octave).
 *
 * The scale size and period are used in extendFrequencies to extend the
 * MTS-ESP frequency table to higher frequencies. This allows using more than
 * 128 tonewheels without using multichannel MTS-ESP to specify all the
 * frequencies.
 *
 * This function works for any whole-number period less than 100; I would guess
 * that the most common cases would be 2 (any octave repeating scale) or 3
 * (e.g. for Bohlen-Pierce). It cannot find non-integer periods, e.g. scales
 * with stretched octaves. In case it cannot find the period it returns -1 for
 * scale size and period.
 */
static void inferScaleSize(double *frequency, int *scaleSizeRet, int *periodRet)
{
    int scaleSize = 0;
    int period;
    int i, j;
    double target;
    for (period = 2; period <= 100; period++)
    {
        for (i = 0; i <= 126; i++)
        {
            if (frequency[i] > 10.0)
            { /* Do not use very small frequencies to avoid numerical errors */
                target = period * frequency[i];
                for (j = i; j <= 126; j++)
                {
                    if ((std::fabs(frequency[j] - target) < 1e-6) &&
                        (std::fabs(frequency[j + 1] - period * frequency[i + 1]) < 1e-6))
                    { /* If frequencies j, i and j+1, i+1 are related by period */
                        scaleSize = j - i;
                        goto end;
                    }
                }
            }
        }
    }
end:
    if (scaleSize == 0)
    {
        scaleSize = -1;
        period = -1;
    }
    *scaleSizeRet = scaleSize;
    *periodRet = period;
};

/**
 * Extend the 128-element MTS-ESP frequency table to a given length.
 *
 * Uses the inferred scale size and period to extend to higher frequencies. If
 * scale size and period cannot be inferred, the higher frequencies are all set
 * to the last available MTS-ESP frequency.
 */
static void extendFrequencies(double *frequency, int length)
{
    int scaleSize, period;
    inferScaleSize(frequency, &scaleSize, &period);
    assert(scaleSize <= 128);
    if (scaleSize > 0)
    {
        for (int i = 128; i < length; i++)
        {
            frequency[i] = period * frequency[i - scaleSize];
        }
    }
    else
    {
        for (int i = 128; i < length; i++)
        {
            frequency[i] = frequency[127];
        }
    }
}

/**
 * Get frequency table of given length.
 *
 * Pulls 128 MTS-ESP frequencies then extends them to the requested length.
 */
void getFrequencies(double *frequency, int length)
{
    assert(length >= 128);
    getMTSESPFrequencies(frequency);
    extendFrequencies(frequency, length);
}

#ifdef TESTS
TEST_CASE("Testing getMTSESPFrequencies")
{
    double frequency[128] = {0.0};
    getMTSESPFrequencies(frequency);
    double a = 32.70319566257483;
    double b = 5919.91076338615039;
    CHECK(frequency[24] == a);
    CHECK(frequency[24 + 12] == 2 * a);
    CHECK(frequency[114 - 12] == b / 2);
    CHECK(frequency[114] == b);
}

TEST_CASE("Testing extendFrequencies")
{
    double frequency[256] = {0.0};
    getMTSESPFrequencies(frequency);
    CHECK(frequency[128] == 0.0);
    CHECK(frequency[255] == 0.0);
    extendFrequencies(frequency, 256);
    CHECK(frequency[128] == 13289.75032255824408);
    CHECK(frequency[255] == 20390018.00521029531956);
}

TEST_CASE("Testing getFrequencies")
{
    double frequency[256] = {0.0};
    getFrequencies(frequency, 256);
    CHECK(frequency[0] == 8.1757989156437070);
    CHECK(frequency[128] == 13289.75032255824408);
    CHECK(frequency[255] == 20390018.00521029531956);
}

TEST_CASE("Testing inferPeriod 12TET")
{
    double frequency[128] = {0.0};
    getMTSESPFrequencies(frequency);
    int scaleSize, period;
    inferScaleSize(frequency, &scaleSize, &period);
    CHECK(scaleSize == 12);
    CHECK(period == 2);
}

TEST_CASE("Testing inferPeriod 19TET")
{
    double frequency[128] = {
        29.312923989933967, 30.40204690044455,  31.531636400145715, 32.70319566257484,
        33.91828425814626,  35.17851972424315,  36.485579088320186, 37.84120251603355,
        39.24719397002958,  40.705425330123695, 42.21783710195307,  43.78644284901423,
        45.41332995495425,  47.1006643910718,   48.850691536306506, 50.66574130559508,
        52.54822903192615,  54.50066098286453,  56.5256353093695,   58.625847979867935,
        60.8040938008891,   63.06327280029143,  65.40639132514968,  67.83656851629252,
        70.3570394484863,   72.97115817664037,  75.6824050320671,   78.49438794005916,
        81.41085066024739,  84.43567420390615,  87.57288569802846,  90.8266599099085,
        94.2013287821436,   97.70138307261301,  101.33148261119015, 105.0964580638523,
        109.00132196572906, 113.051270618739,   117.25169595973587, 121.6081876017782,
        126.12654560058286, 130.81278265029937, 135.67313703258498, 140.71407889697258,
        145.94231635328075, 151.3648100641342,  156.9887758801183,  162.82170132049478,
        168.8713484078123,  175.14577139605697, 181.653319819817,   188.4026575642872,
        195.40276614522597, 202.6629652223803,  210.1929161277046,  218.00264393145807,
        226.102541237478,   234.50339191947174, 243.21637520355648, 252.2530912011658,
        261.62556530059874, 271.34627406516995, 281.42815779394516, 291.8846327065615,
        302.7296201282684,  313.9775517602366,  325.64340264098956, 337.7426968156246,
        350.29154279211394, 363.306639639634,   376.8053151285744,  390.80553229045194,
        405.3259304447606,  420.3858322554092,  436.00528786291613, 452.205082474956,
        469.0067838389435,  486.43275040711296, 504.5061824023316,  523.2511306011975,
        542.6925481303399,  562.8563155878903,  583.769265413123,   605.4592402565368,
        627.9551035204732,  651.2868052819791,  675.4853936312492,  700.5830855842279,
        726.613279279268,   753.6106302571488,  781.6110645809039,  810.6518608895212,
        840.7716645108184,  872.0105757258323,  904.410164949912,   938.013567677887,
        972.8655008142259,  1009.0123648046632, 1046.502261202395,  1085.3850962606798,
        1125.7126311757806, 1167.538530826246,  1210.9184805130735, 1255.9102070409465,
        1302.5736105639583, 1350.9707872624983, 1401.1661711684558, 1453.226558558536,
        1507.2212605142977, 1563.2221291618077, 1621.3037217790425, 1681.5433290216367,
        1744.0211514516645, 1808.820329899824,  1876.027135355774,  1945.7310016284518,
        2018.0247296093264, 2093.00452240479,   2170.7701925213596, 2251.4252623515613,
        2335.0770616524906, 2421.8369610261457, 2511.8204140818916, 2605.1472211279147,
        2701.9415745249967, 2802.3323423369116, 2906.453117117072,  3014.4425210285954,
    };
    int scaleSize, period;
    inferScaleSize(frequency, &scaleSize, &period);
    CHECK(scaleSize == 19);
    CHECK(period == 2);
}

TEST_CASE("Testing inferPeriod Bohlen-Pierce")
{
    double frequency[128] = {
        1.642790111337959,  1.7876550383579546, 1.9452944692397185, 2.116834898712101,
        2.3035021480099007, 2.506630134033841,  2.7276704014688447, 2.968202495465875,
        3.229945250624673,  3.5147690673915934, 3.8247092871631745, 4.161980730690898,
        4.5289935265878265, 4.928370334013877,  5.362965115073864,  5.835883407719154,
        6.350504696136302,  6.910506444029701,  7.519890402101522,  8.183011204406533,
        8.904607486397621,  9.689835751874021,  10.544307202174782, 11.474127861489524,
        12.485942192072697, 13.586980579763477, 14.785111002041633, 16.088895345221594,
        17.507650223157466, 19.051514088408908, 20.731519332089103, 22.559671206304564,
        24.549033613219596, 26.713822459192862, 29.06950725562206,  31.63292160652436,
        34.42238358446859,  37.45782657621811,  40.76094173929046,  44.35533300612493,
        48.26668603566481,  52.52295066947242,  57.15454226522675,  62.19455799626734,
        67.67901361891373,  73.64710083965883,  80.14146737757864,  87.20852176686623,
        94.89876481957306,  103.26715075340576, 112.37347972865436, 122.28282521787138,
        133.0659990183748,  144.80005810699447, 157.56885200841734, 171.46362679568034,
        186.583673988802,   203.0370408567413,  220.94130251897647, 240.42440213273596,
        261.62556530059874, 284.69629445871936, 309.80145226021745, 337.12043918596305,
        366.84847565361434, 399.1979970551244,  434.40017432098335, 472.706556025252,
        514.390880387041,   559.7510219664063,  609.1111225702239,  662.8239075569297,
        721.2732063982082,  784.8766959017962,  854.088883376158,   929.4043567806523,
        1011.3613175578897, 1100.5454269608429, 1197.5939911653738, 1303.2005229629508,
        1418.1196680757569, 1543.1726411611228, 1679.2530658992198, 1827.3333677106714,
        1988.4717226707905, 2163.8196191946245, 2354.630087705388,  2562.2666501284752,
        2788.21307034196,   3034.0839526736704, 3301.636280882529,  3592.7819734961236,
        3909.6015688888547, 4254.35900422727,   4629.517923483374,  5037.759197697659,
        5482.0001031320135, 5965.415168012371,  6491.458857583873,  7063.890263116174,
        7686.799950385435,  8364.63921102588,   9102.25185802101,   9904.908842647597,
        10778.34592048837,  11728.804706666562, 12763.077012681826, 13888.553770450122,
        15113.277593092977, 16446.00030939606,  17896.24550403713,  19474.376572751644,
        21191.670789348515, 23060.399851156275, 25093.91763307764,  27306.75557406303,
        29714.726527942792, 32335.037761465104, 35186.41411999968,  38289.23103804547,
        41665.66131135036,  45339.832779278986, 49338.00092818817,  53688.736512111325,
        58423.129718254924, 63575.01236804554,  69181.19955346883,  75281.7528992329,
    };
    int scaleSize, period;
    inferScaleSize(frequency, &scaleSize, &period);
    CHECK(scaleSize == 13);
    CHECK(period == 3);
}

TEST_CASE("Testing inferPeriod p4.scl")
{
    double frequency[128] = {
        5.5107356640386095e-11, 1.1021471328077219e-10, 1.6532206992115805e-10,
        2.755367832019303e-10,  3.8575149648270303e-10, 7.715029929654061e-10,
        1.1572544894481074e-09, 1.9287574824135143e-09, 2.7002604753789237e-09,
        5.400520950757821e-09,  8.100781426136721e-09,  1.3501302376894546e-08,
        1.890182332765239e-08,  3.780364665530478e-08,  5.6705469982957235e-08,
        9.45091166382619e-08,   1.3231276329356686e-07, 2.6462552658713373e-07,
        3.9693828988070107e-07, 6.615638164678341e-07,  9.26189343054969e-07,
        1.852378686109938e-06,  2.77856802916491e-06,   4.630946715274843e-06,
        6.48332540138479e-06,   1.2966650802769545e-05, 1.9449976204154344e-05,
        3.241662700692385e-05,  4.538327780969346e-05,  9.076655561938692e-05,
        0.0001361498334290805,  0.00022691638904846718, 0.00031768294466785453,
        0.0006353658893357091,  0.0009530488340035646,  0.0015884147233392717,
        0.002223780612674981,   0.004447561225349957,   0.006671341838024934,
        0.011118903063374885,   0.015566464288724843,   0.031132928577449724,
        0.04669939286617458,    0.07783232144362429,    0.108965250021074,
        0.217930500042148,      0.32689575006322197,    0.5448262501053698,
        0.7627567501475179,     1.5255135002950357,     2.288270250442553,
        3.8137837507375876,     5.339297251032623,      10.678594502065254,
        16.017891753097885,     26.69648625516313,      37.37508075722839,
        74.75016151445678,      112.1252422716852,      186.87540378614193,
        261.62556530059874,     523.2511306011975,      784.8766959017962,
        1308.1278265029932,     1831.3789571041907,     3662.7579142083814,
        5494.136871312571,      9156.89478552096,       12819.652699729331,
        25639.305399458663,     38458.958099188036,     64098.2634986467,
        89737.56889810541,      179475.13779621082,     269212.70669431624,
        448687.8444905268,      628162.9822867368,      1256325.9645734737,
        1884488.9468602128,     3140814.9114336832,     4397140.876007163,
        8794281.752014326,      13191422.628021503,     21985704.380035803,
        30779986.132050168,     61559972.264100336,     92339958.3961506,
        153899930.66025075,     215459902.9243514,      430919805.8487017,
        646379708.7730533,      1077299514.621754,      1508219320.4704573,
        3016438640.9409146,     4524657961.411378,      7541096602.352284,
        10557535243.293213,     21115070486.586426,     31672605729.879673,
        52787676216.46591,      73902746703.05237,      147805493406.10474,
        221708240109.15796,     369513733515.2618,      517319226921.3671,
        1034638453842.7343,     1551957680764.0994,     2586596134606.8345,
        3621234588449.5737,     7242469176899.147,      10863703765348.707,
        18106172942247.86,      25348642119147.035,     50697284238294.07,
        76045926357441.02,      126743210595735.16,     177440494834029.47,
        354880989668057.1,      532321484502085.0,      887202474170142.5,
        1242083463838201.2,     2484166927676402.5,     3726250391514599.0,
        6210417319191003.0,     8694584246867417.0,     1.7389168493734834e+16,
        2.6083752740602216e+16, 4.347292123433707e+16,
    };
    int scaleSize, period;
    inferScaleSize(frequency, &scaleSize, &period);
    CHECK(scaleSize == 4);
    CHECK(period == 7);
}

TEST_CASE("Testing inferPeriod bagpipe4.scl (period 1190 cents)")
{
    double frequency[128] = {
        2.3943234311985675, 2.6603593679984088, 2.837716659198303,  3.19243124159809,
        3.5471458239978766, 3.724503115197771,  4.232058920873153,  3.7030515557640085,
        4.2320589208731505, 4.761066285982294,  5.290073651091439,  5.642745227830868,
        6.348088381309726,  7.053431534788587,  7.406103111528017,  8.415368110219507,
        7.363447096442065,  8.415368110219505,  9.467289123996942,  10.519210137774383,
        11.220490813626006, 12.623052165329256, 14.025613517032506, 14.726894192884133,
        16.733798312970627, 14.642073523849303, 16.73379831297063,  18.825523102091957,
        20.917247891213293, 22.31173108396084,  25.10069746945594,  27.88966385495105,
        29.284147047698607, 33.27483745353056,  29.115482771839236, 33.27483745353056,
        37.434192135221885, 41.5935468169132,   44.36644993804073,  49.91225618029583,
        55.45806242255093,  58.23096554367847,  66.16637698451645,  57.895579861451886,
        66.16637698451645,  74.43717410758099,  82.70797123064557,  88.22183597935526,
        99.24956547677465,  110.27729497419405, 115.79115972290377, 131.5705733911144,
        115.12425171722516, 131.5705733911144,  148.0168950650038,  164.46321673889307,
        175.42743118815258, 197.3558600866716,  219.28428898519078, 230.24850343445033,
        261.62556530059874, 228.92236963802384, 261.62556530059874, 294.3287609631735,
        327.0319566257485,  348.834087067465,   392.4383479508981,  436.0426088343311,
        457.8447392760477,  520.2374258519455,  455.2077476204522,  520.2374258519455,
        585.2671040834387,  650.2967823149316,  693.649901135927,   780.3561387779183,
        867.062376419909,   910.4154952409044,  1034.4821575295762, 905.1718878383793,
        1034.4821575295762, 1163.7924272207729, 1293.1026969119705, 1379.309543372768,
        1551.7232362943641, 1724.1369292159607, 1810.3437756767587, 2057.0479574677925,
        1799.9169627843191, 2057.0479574677925, 2314.178952151269,  2571.3099468347427,
        2742.7306099570565, 3085.5719362016885, 3428.4132624463195, 3599.833925568636,
        4090.4004660915975, 3579.1004078301467, 4090.4004660915975, 4601.700524353047,
        5113.000582614502,  5453.867288122131,  6135.600699137396,  6817.33411015266,
        7158.200815660293,  8133.682985980809,  7116.972612733206,  8133.682985980809,
        9150.393359228408,  10167.103732476018, 10844.910647974411, 12200.52447897121,
        13556.13830996801,  14233.945225466412, 16173.67283835827,  14151.963733563483,
        16173.67283835825,  18195.38194315303,  20217.09104794783,  21564.89711781103,
        24260.509257537404, 26956.121397263774, 28303.927467126967, 32161.038674991334,
        28140.90884061741,  32161.038674991334, 36181.16850936524,  40201.298343739145,
    };
    int scaleSize, period;
    inferScaleSize(frequency, &scaleSize, &period);
    CHECK(scaleSize == -1);
    CHECK(period == -1);
}

TEST_CASE("Testing extendFrequencies bagpipe4.scl (period 1190 cents)")
{
    double frequency[256] = {
        2.3943234311985675, 2.6603593679984088, 2.837716659198303,  3.19243124159809,
        3.5471458239978766, 3.724503115197771,  4.232058920873153,  3.7030515557640085,
        4.2320589208731505, 4.761066285982294,  5.290073651091439,  5.642745227830868,
        6.348088381309726,  7.053431534788587,  7.406103111528017,  8.415368110219507,
        7.363447096442065,  8.415368110219505,  9.467289123996942,  10.519210137774383,
        11.220490813626006, 12.623052165329256, 14.025613517032506, 14.726894192884133,
        16.733798312970627, 14.642073523849303, 16.73379831297063,  18.825523102091957,
        20.917247891213293, 22.31173108396084,  25.10069746945594,  27.88966385495105,
        29.284147047698607, 33.27483745353056,  29.115482771839236, 33.27483745353056,
        37.434192135221885, 41.5935468169132,   44.36644993804073,  49.91225618029583,
        55.45806242255093,  58.23096554367847,  66.16637698451645,  57.895579861451886,
        66.16637698451645,  74.43717410758099,  82.70797123064557,  88.22183597935526,
        99.24956547677465,  110.27729497419405, 115.79115972290377, 131.5705733911144,
        115.12425171722516, 131.5705733911144,  148.0168950650038,  164.46321673889307,
        175.42743118815258, 197.3558600866716,  219.28428898519078, 230.24850343445033,
        261.62556530059874, 228.92236963802384, 261.62556530059874, 294.3287609631735,
        327.0319566257485,  348.834087067465,   392.4383479508981,  436.0426088343311,
        457.8447392760477,  520.2374258519455,  455.2077476204522,  520.2374258519455,
        585.2671040834387,  650.2967823149316,  693.649901135927,   780.3561387779183,
        867.062376419909,   910.4154952409044,  1034.4821575295762, 905.1718878383793,
        1034.4821575295762, 1163.7924272207729, 1293.1026969119705, 1379.309543372768,
        1551.7232362943641, 1724.1369292159607, 1810.3437756767587, 2057.0479574677925,
        1799.9169627843191, 2057.0479574677925, 2314.178952151269,  2571.3099468347427,
        2742.7306099570565, 3085.5719362016885, 3428.4132624463195, 3599.833925568636,
        4090.4004660915975, 3579.1004078301467, 4090.4004660915975, 4601.700524353047,
        5113.000582614502,  5453.867288122131,  6135.600699137396,  6817.33411015266,
        7158.200815660293,  8133.682985980809,  7116.972612733206,  8133.682985980809,
        9150.393359228408,  10167.103732476018, 10844.910647974411, 12200.52447897121,
        13556.13830996801,  14233.945225466412, 16173.67283835827,  14151.963733563483,
        16173.67283835825,  18195.38194315303,  20217.09104794783,  21564.89711781103,
        24260.509257537404, 26956.121397263774, 28303.927467126967, 32161.038674991334,
        28140.90884061741,  32161.038674991334, 36181.16850936524,  40201.298343739145,
    };
    CHECK(frequency[128] == 0.0);
    CHECK(frequency[255] == 0.0);

    extendFrequencies(frequency, 256);
    CHECK(frequency[128] == frequency[127]);
    CHECK(frequency[255] == frequency[127]);
}
#endif
