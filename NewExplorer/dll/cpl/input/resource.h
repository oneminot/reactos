#pragma once

/* metrics */
#define PROPSHEETWIDTH                                  252
#define PROPSHEETHEIGHT                                 228
#define PROPSHEETPADDING                                6
#define SYSTEM_COLUMN                                   (0 * PROPSHEETPADDING)
#define ICONSIZE                                        16

/* IDI */
#define IDI_KEYBOARD_ICO                                202
#define IDI_MARKER_ICO                                  205
#define IDI_MIC_ICO                                     203
#define IDI_KEY_SHORT_ICO                               200
#define IDI_INFO_ICO                                    207
#define IDI_CPLSYSTEM                                   1502

/* IDD */
#define IDD_PROPPAGESETTINGS                            500
#define IDD_INPUT_LANG_PROP                             501
#define IDD_CHANGE_KEY_SEQ                              502
#define IDD_ADD                                         503
#define IDD_KEYSETTINGS                                 504

/* IDC */
#define IDC_KEYLAYOUT_LIST                              1000
#define IDC_SET_DEFAULT                                 1001
#define IDC_ADD_BUTTON                                  1002
#define IDC_REMOVE_BUTTON                               1003
#define IDC_PROP_BUTTON                                 1004
#define IDC_KEY_SET_BTN                                 1005
#define IDC_PRESS_CL_KEY_RB                             1006
#define IDC_PRESS_SHIFT_KEY_RB                          1007
#define IDC_KEY_LISTVIEW                                1008
#define IDC_CHANGE_KEY_SEQ_BTN                          1009
#define IDC_INPUT_LANG_COMBO                            1010
#define IDC_KEYBOARD_LO_COMBO                           1011
#define IDC_USE_SK                                      1012
#define IDC_KB_LAYOUT_IME_COMBO                         1013
#define IDC_INPUT_LANG_STR                              1014
#define IDC_SWITCH_INPUT_LANG_CB                        1015
#define IDC_SWITCH_KBLAYOUTS_CB                         1016
#define IDC_CTRL_LANG                                   1017
#define IDC_CTRL_LAYOUT                                 1018
#define IDC_LEFT_ALT_LANG                               1019
#define IDC_LEFT_ALT_LAYOUT                             1020

/* IDS */
#define IDS_CPLSYSTEMNAME                               1
#define IDS_CPLSYSTEMDESCRIPTION                        2
#define IDS_KEYBOARD                                    3
#define IDS_NONE                                        4
#define IDS_UNKNOWN                                     5
#define IDS_RESTART                                     6
#define IDS_WHATS_THIS                                  7
#define IDS_LANGUAGE                                    8
#define IDS_LAYOUT                                      9
#define IDS_REM_QUESTION                                10
#define IDS_CONFIRMATION                                11
#define IDS_LAYOUT_EXISTS                               12
#define IDS_LAYOUT_EXISTS2                              13
#define IDS_CTRL_SHIFT                                  14
#define IDS_LEFT_ALT_SHIFT                              15
#define IDS_SWITCH_BET_INLANG                           16

/* Layouts */
#define IDS_US_LAYOUT                                   5000
#define IDS_BELGIAN_LAYOUT                              5001
#define IDS_BELGIAN_FRENCH_LAYOUT                       5002
#define IDS_PORTUGUESE_BRAZILIAN_ABNT_LAYOUT            5003
#define IDS_CANADIAN_FRENCH_LAYOUT                      5004
#define IDS_CANADIAN_FRENCH_LEGACY_LAYOUT               5005
#define IDS_DANISH_LAYOUT                               5007
#define IDS_DUTCH_LAYOUT                                5008
#define IDS_FINNISH_LAYOUT                              5009
#define IDS_FRENCH_LAYOUT                               5010
#define IDS_GERMAN_LAYOUT                               5011
#define IDS_GERMAN_IBM_LAYOUT                           5012
#define IDS_ICELANDIC_LAYOUT                            5013
#define IDS_IRISH_LAYOUT                                5014
#define IDS_ITALIAN_LAYOUT                              5015
#define IDS_ITALIAN_142_LAYOUT                          5016
#define IDS_LATIN_AMERICAN_LAYOUT                       5017
#define IDS_NORWEGIAN_LAYOUT                            5018
#define IDS_PORTUGUESE_LAYOUT                           5019
#define IDS_SPANISH_LAYOUT                              5020
#define IDS_SPANISH_VARIANTION_LAYOUT                   5021
#define IDS_SWEDISH_LAYOUT                              5022
#define IDS_SWISS_FRENCH_LAYOUT                         5023
#define IDS_SWISS_GERMAN_LAYOUT                         5024
#define IDS_UNITED_KINGDOM_LAYOUT                       5025
#define IDS_UNITED_STATES_INTERNATIONAL_LAYOUT          5026
#define IDS_UNITED_STATES_DVORAK_FOR_LEFT_HAND_LAYOUT   5027
#define IDS_UNITED_STATES_DVORAK_FOR_RIGHT_HAND_LAYOUT  5028
#define IDS_ALBANIAN_LAYOUT                             5029
#define IDS_CROATIAN_LAYOUT                             5030
#define IDS_CZECH_LAYOUT                                5031
#define IDS_CZECH_QWERTY_LAYOUT                         5032
#define IDS_HUNGARIAN_LAYOUT                            5033
#define IDS_HUNGARIAN_101_KEY_LAYOUT                    5034
#define IDS_POLISH_LAYOUT                               5035
#define IDS_POLISH_214_LAYOUT                           5036
#define IDS_ROMANIAN_LAYOUT                             5037
#define IDS_SERBIAN_LATIN_LAYOUT                        5038
#define IDS_SLOVAK_LAYOUT                               5039
#define IDS_SLOVAK_QWERTY_LAYOUT                        5040
#define IDS_SLOVENIAN_LAYOUT                            5041
#define IDS_ESTONIAN_LAYOUT                             5042
#define IDS_LATVIAN_LAYOUT                              5043
#define IDS_LATVIAN_QWERTY_LAYOUT                       5044
#define IDS_LITHUANIAN_IBM_LAYOUT                       5045
#define IDS_GREEK_LAYOUT                                5046
#define IDS_GREEK_LATIN_LAYOUT                          5047
#define IDS_GREEK_220_LAYOUT                            5048
#define IDS_GREEK_319_LAYOUT                            5049
#define IDS_GREEK_220_LATIN_LAYOUT                      5050
#define IDS_GREEK_319_LATIN_LAYOUT                      5051
#define IDS_BELARUSIAN_LAYOUT                           5052
#define IDS_BULGARIAN_LAYOUT                            5053
#define IDS_BULGARIAN_LATIN_LAYOUT                      5054
#define IDS_RUSSIAN_LAYOUT                              5055
#define IDS_RUSSIAN_TYPEWRITER_LAYOUT                   5056
#define IDS_SERBIAN_CYRILLIC_LAYOUT                     5057
#define IDS_UKRAINIAN_LAYOUT                            5058
#define IDS_TURKISH_F_LAYOUT                            5059
#define IDS_TURKISH_Q_LAYOUT                            5060
#define IDS_JAPANESE_LAYOUT                             5061
#define IDS_JAPANESE_INPUT_SYSTEM_MSIME2002_LAYOUT      5062
#define IDS_KOREAN_LAYOUT                               5063
#define IDS_KOREAN_INPUT_SYSTEM_MSIME2002_LAYOUT        5064
#define IDS_CHINESE_TRADITIONAL_USKEYBOARD_LAYOUT       5065
#define IDS_CHINESE_TRADITIONAL_PHONETIC_LAYOUT         5066
#define IDS_CHINESE_TRADITIONAL_CHANGJIE_LAYOUT         5067
#define IDS_CHINESE_TRADITIONAL_BIG5CODE_LAYOUT         5068
#define IDS_CHINESE_TRADITIONAL_DAYI_LAYOUT             5069
#define IDS_CHINESE_TRADITIONAL_UNICODE_LAYOUT          5070
#define IDS_CHINESE_TRADITIONAL_ALPHANUMERIC_LAYOUT     5071
#define IDS_CHINESE_SIMPLIFIED_USKEYBOARD_LAYOUT        5072
#define IDS_CHINESE_SIMPLIFIED_QUANPIN_LAYOUT           5073
#define IDS_CHINESE_SIMPLIFIED_SHUANGPIN_LAYOUT         5074
#define IDS_CHINESE_SIMPLIFIED_ZHENGMA_LAYOUT           5075
#define IDS_CHINESE_SIMPLIFIED_NEIMA_LAYOUT             5077
#define IDS_THAI_KEDMANEE_LAYOUT                        5079
#define IDS_THAI_PATTACHOTE_LAYOUT                      5080
#define IDS_THAI_KEDMANEE_NONSHIFTLOCK_LAYOUT           5081
#define IDS_THAI_PATTACHOTE_NONSHIFTLOCK_LAYOUT         5082
#define IDS_HEBREW_LAYOUT                               5083
#define IDS_ARABIC_101_LAYOUT                           5084
#define IDS_ARABIC_102_LAYOUT                           5085
#define IDS_ARABIC_102_AZERTY_LAYOUT                    5086
#define IDS_CZECH_PROGRAMMERS_LAYOUT                    5087
#define IDS_LITHUANIAN_LAYOUT                           5088
#define IDS_BELGIAN_COMMA_LAYOUT                        5089
#define IDS_CHINESE_TRADITIONAL_NEWPHONETIC_LAYOUT      5090
#define IDS_CHINESE_SIMPLIFIED_MSPINYINIME30_LAYOUT     5091
#define IDS_UNITED_STATES_DVIRAK_LAYOUT                 5092
#define IDS_CHINESE_TRADITIONAL_NEWCHANGJIE_LAYOUT      5093
#define IDS_ASSAMESE_LAYOUT                             5094
#define IDS_BENGALI_LAYOUT                              5095
#define IDS_DEVANAGARI_INSCRIPT_LAYOUT                  5096
#define IDS_GUJARATI_LAYOUT                             5097
#define IDS_KANNADA_LAYOUT                              5098
#define IDS_MALAYALAM_LAYOUT                            5099
#define IDS_ORIYA_LAYOUT                                5100
#define IDS_PUNJABI_LAYOUT                              5101
#define IDS_TAMIL_LAYOUT                                5102
#define IDS_TELUGU_LAYOUT                               5103
#define IDS_MARATHI_LAYOUT                              5104
#define IDS_HINDI_TRADITIONAL_LAYOUT                    5105
#define IDS_CANTONESE_PHONETIC_LAYOUT                   5107
#define IDS_FAEROESE_LAYOUT                             5108
#define IDS_FYRO_MACEDONIAN_LAYOUT                      5109
#define IDS_CANADIAN_MULTILINGUAL_STD_LAYOUT            5110
#define IDS_CHINESE_TRADITIONAL_QUICK_LAYOUT            5111
#define IDS_CHINESE_TRADITIONAL_ARRAY_LAYOUT            5112
#define IDS_KAZAKH_LAYOUT                               5113
#define IDS_UZBEK_CYRILLIC_LAYOUT                       5114
#define IDS_AZERI_CYRILLIC_LAYOUT                       5115
#define IDS_TATAR_LAYOUT                                5116
#define IDS_AZERI_LATIN_LAYOUT                          5117
#define IDS_VIETNAMESE_LAYOUT                           5118
#define IDS_GEORGIAN_LAYOUT                             5119
#define IDS_ARMENIAN_EASTERN_LAYOUT                     5120
#define IDS_ARMENIAN_WESTERN_LAYOUT                     5121
#define IDS_GREEK_POLYTONIC_LAYOUT                      5122
#define IDS_USENG_TABLE_IBM_ARABIC238L_LAYOUT           5123
#define IDS_FARSI_LAYOUT                                5124
#define IDS_GAELIC_LAYOUT                               5125
#define IDS_PORTUGUESE_BRAZIL_ABNT2_LAYOUT              5126
#define IDS_MONGOLIAN_CYRILLIC_LAYOUT                   5127
#define IDS_KYRGYZ_CYRILLIC_LAYOUT                      5128
#define IDS_URDU_LAYOUT                                 5129
#define IDS_SYRIAC_LAYOUT                               5130
#define IDS_SYRIAC_PHONETIC_LAYOUT                      5131
#define IDS_DIVEHI_PHONETIC_LAYOUT                      5132
#define IDS_DIVEHI_TYPEWRITER_LAYOUT                    5133
#define IDS_BULGARIAN_PHONETIC_CLASSIC_LAYOUT           5134
#define IDS_BULGARIAN_PHONETIC_BDS_LAYOUT               5135
#define IDS_BULGARIAN_BDS_LAYOUT                        5136
#define IDS_GERMAN_RISTOME_LAYOUT                       5137
#define IDS_GERMAN_NEO_11_LAYOUT                        5138
#define IDS_GERMAN_DE_ERGO_LAYOUT                       5139
#define IDS_BURMESE_LAYOUT                              5140
#define IDS_UKRAINIAN_STUDENT_LAYOUT                    5141

/* EOF */
