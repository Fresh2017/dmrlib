"use strict";

(function($, dmr) {
    if (window.DMR === undefined) {
        window.DMR = {};
    }
    if (DMR.ID === undefined) {
        DMR.ID = {};
    }

    /* ISO-3166-2 with modifications */
    var iso3166 = {
        'af': 'Afghanistan',
        'ax': 'Aland Islands',
        'al': 'Albania',
        'dz': 'Algeria',
        'as': 'American Samoa',
        'ad': 'Andorra',
        'ao': 'Angola',
        'ai': 'Anguilla',
        'aq': 'Antarctica',
        'ag': 'Antigua & Barbuda',
        'ar': 'Argentina',
        'am': 'Armenia',
        'aw': 'Aruba',
        'au': 'Australia',
        'at': 'Austria',
        'az': 'Azerbaijan',
        'bs': 'Bahamas',
        'bh': 'Bahrain',
        'bd': 'Bangladesh',
        'bb': 'Barbados',
        'by': 'Belarus',
        'be': 'Belgium',
        'bz': 'Belize',
        'bj': 'Benin',
        'bm': 'Bermuda',
        'bt': 'Bhutan',
        'bo': 'Bolivia',
        'ba': 'Bosnia & Herzegovina',
        'bw': 'Botswana',
        'bv': 'Bouvet Island',
        'br': 'Brazil',
        'io': 'British Indian Ocean',
        'bn': 'Brunei Darussalam',
        'bg': 'Bulgaria',
        'bf': 'Burkina Faso',
        'bi': 'Burundi',
        'kh': 'Cambodia',
        'cm': 'Cameroon',
        'ca': 'Canada',
        'cv': 'Cape Verde',
        'ky': 'Cayman Islands',
        'cf': 'Central African Republic',
        'td': 'Chad',
        'cl': 'Chile',
        'cn': 'China',
        'cx': 'Christmas Island',
        'cc': 'Cocos Islands',
        'co': 'Colombia',
        'km': 'Comoros',
        'cg': 'Congo',
        'cd': 'Congo',
        'ck': 'Cook Islands',
        'cr': 'Costa Rica',
        'ci': 'Cote D\'Ivoire',
        'hr': 'Croatia',
        'cu': 'Cuba',
        'cy': 'Cyprus',
        'cz': 'Czech Republic',
        'dk': 'Denmark',
        'dj': 'Djibouti',
        'dm': 'Dominica',
        'do': 'Dominican Republic',
        'ec': 'Ecuador',
        'eg': 'Egypt',
        'sv': 'El Salvador',
        'gq': 'Equatorial Guinea',
        'er': 'Eritrea',
        'ee': 'Estonia',
        'et': 'Ethiopia',
        'fk': 'Falkland Islands',
        'fo': 'Faroe Islands',
        'fj': 'Fiji',
        'fi': 'Finland',
        'fr': 'France',
        'gf': 'French Guiana',
        'pf': 'French Polynesia',
        'tf': 'French Southern Territories',
        'ga': 'Gabon',
        'gm': 'Gambia',
        'ge': 'Georgia',
        'de': 'Germany',
        'gh': 'Ghana',
        'gi': 'Gibraltar',
        'gr': 'Greece',
        'gl': 'Greenland',
        'gd': 'Grenada',
        'gp': 'Guadeloupe',
        'gu': 'Guam',
        'gt': 'Guatemala',
        'gg': 'Guernsey',
        'gn': 'Guinea',
        'gw': 'Guinea-Bissau',
        'gy': 'Guyana',
        'ht': 'Haiti',
        'hm': 'Heard Island',
        'va': 'Vatican City',
        'hn': 'Honduras',
        'hk': 'Hong Kong',
        'hu': 'Hungary',
        'is': 'Iceland',
        'in': 'India',
        'id': 'Indonesia',
        'ir': 'Iran',
        'iq': 'Iraq',
        'ie': 'Ireland',
        'im': 'Isle Of Man',
        'il': 'Israel',
        'it': 'Italy',
        'jm': 'Jamaica',
        'jp': 'Japan',
        'je': 'Jersey',
        'jo': 'Jordan',
        'kz': 'Kazakhstan',
        'ke': 'Kenya',
        'ki': 'Kiribati',
        'kr': 'Korea',
        'kw': 'Kuwait',
        'kg': 'Kyrgyzstan',
        'la': 'Lao People\'s Democratic Republic',
        'lv': 'Latvia',
        'lb': 'Lebanon',
        'ls': 'Lesotho',
        'lr': 'Liberia',
        'ly': 'Libyan Arab Jamahiriya',
        'li': 'Liechtenstein',
        'lt': 'Lithuania',
        'lu': 'Luxembourg',
        'mo': 'Macao',
        'mk': 'Macedonia',
        'mg': 'Madagascar',
        'mw': 'Malawi',
        'my': 'Malaysia',
        'mv': 'Maldives',
        'ml': 'Mali',
        'mt': 'Malta',
        'mh': 'Marshall Islands',
        'mq': 'Martinique',
        'mr': 'Mauritania',
        'mu': 'Mauritius',
        'yt': 'Mayotte',
        'mx': 'Mexico',
        'fm': 'Micronesia',
        'md': 'Moldova',
        'mc': 'Monaco',
        'mn': 'Mongolia',
        'me': 'Montenegro',
        'ms': 'Montserrat',
        'ma': 'Morocco',
        'mz': 'Mozambique',
        'mm': 'Myanmar',
        'na': 'Namibia',
        'nr': 'Nauru',
        'np': 'Nepal',
        'nl': 'Netherlands',
        'an': 'Netherlands Antilles',
        'nc': 'New Caledonia',
        'nz': 'New Zealand',
        'ni': 'Nicaragua',
        'ne': 'Niger',
        'ng': 'Nigeria',
        'nu': 'Niue',
        'nf': 'Norfolk Island',
        'mp': 'Northern Mariana Islands',
        'no': 'Norway',
        'om': 'Oman',
        'pk': 'Pakistan',
        'pw': 'Palau',
        'ps': 'Palestinian Territory',
        'pa': 'Panama',
        'pg': 'Papua New Guinea',
        'py': 'Paraguay',
        'pe': 'Peru',
        'ph': 'Philippines',
        'pn': 'Pitcairn',
        'pl': 'Poland',
        'pt': 'Portugal',
        'pr': 'Puerto Rico',
        'qa': 'Qatar',
        're': 'Reunion',
        'ro': 'Romania',
        'ru': 'Russian Federation',
        'rw': 'Rwanda',
        'bl': 'Saint Barthelemy',
        'sh': 'Saint Helena',
        'kn': 'Saint Kitts And Nevis',
        'lc': 'Saint Lucia',
        'mf': 'Saint Martin',
        'pm': 'Saint Pierre And Miquelon',
        'vc': 'Saint Vincent And Grenadines',
        'ws': 'Samoa',
        'sm': 'San Marino',
        'st': 'Sao Tome And Principe',
        'sa': 'Saudi Arabia',
        'sn': 'Senegal',
        'rs': 'Serbia',
        'sc': 'Seychelles',
        'sl': 'Sierra Leone',
        'sg': 'Singapore',
        'sk': 'Slovakia',
        'si': 'Slovenia',
        'sb': 'Solomon Islands',
        'so': 'Somalia',
        'za': 'South Africa',
        'gs': 'South Georgia And Sandwich Isl.',
        'es': 'Spain',
        'lk': 'Sri Lanka',
        'sd': 'Sudan',
        'sr': 'Suriname',
        'sj': 'Svalbard And Jan Mayen',
        'sz': 'Swaziland',
        'se': 'Sweden',
        'ch': 'Switzerland',
        'sy': 'Syrian Arab Republic',
        'tw': 'Taiwan',
        'tj': 'Tajikistan',
        'tz': 'Tanzania',
        'th': 'Thailand',
        'tl': 'Timor-Leste',
        'tg': 'Togo',
        'tk': 'Tokelau',
        'to': 'Tonga',
        'tt': 'Trinidad And Tobago',
        'tn': 'Tunisia',
        'tr': 'Turkey',
        'tm': 'Turkmenistan',
        'tc': 'Turks And Caicos Islands',
        'tv': 'Tuvalu',
        'ug': 'Uganda',
        'ua': 'Ukraine',
        'ae': 'United Arab Emirates',
        'gb': 'United Kingdom',
        'us': 'United States',
        'um': 'United States Outlying Islands',
        'uy': 'Uruguay',
        'uz': 'Uzbekistan',
        'vu': 'Vanuatu',
        've': 'Venezuela',
        'vn': 'Viet Nam',
        'vg': 'Virgin Islands',
        'vi': 'Virgin Islands',
        'wf': 'Wallis And Futuna',
        'eh': 'Western Sahara',
        'ye': 'Yemen',
        'zm': 'Zambia',
        'zw': 'Zimbabwe'
    };

    var parseId = function(id) {
        return parseInt(id, 10).toString();
    }

    DMR.country = function(id) {
        id = parseId(id);
        console.log('country?', id);
        if (DMR.ID[id] !== undefined && DMR.ID[id].country !== undefined) {
            return DMR.ID[id].country;
        } else if (id.length > 3) {
            return DMR.country(id.substr(0, id.length - 1));
        }
        return null;
    };

    DMR.countryName = function(id) {
        var country = DMR.country(id);
        if (country !== null) {
            return iso3166[country];
        }
        return null;
    };

    DMR.description = function(id) {
        id = parseId(id);
        var bits = [], prefix;
        if (DMR.ID[id]) {
            if (DMR.ID[id].name) {
                /* This is a person */
                bits.push(DMR.ID[id].name);
                if (DMR.ID[id].call) {
                    bits.push('(' + DMR.ID[id].call + ')');
                }
            } else {
                /* This is a group (or something else) */
                if (DMR.ID[id].special) {
                    bits.push(DMR.ID[id].special);
                }
                if (DMR.ID[id].country) {
                    bits.push(iso3166[DMR.ID[id].country]);
                }
                if (DMR.ID[id].area) {
                    bits.push('(' + DMR.ID[id].area + ')');
                }
            }
        } else if (id > 999) {
            prefix = (id % 1000);
            if (DMR.ID[prefix] && DMR.ID[prefix].country) {
                bits.push(iso3166[DMR.ID[prefix].country]);
            }
        }
        if (bits.length) {
            return bits.join(' ');
        }
        return id.toString();
    };
 
    var group_ids = {
        /* Special purpose */
        '1'       : {special: 'Local'},
        '9'       : {special: 'DMR Reflector'},
        '91'      : {special: 'World-wide'},
        '92'      : {special: 'Europe'},
        '93'      : {special: 'North America'},
        '95'      : {special: 'Australia, New Zeland'},
        '899'     : {special: 'Repeater Testing'},

        /* By language */
        '3026'    : {'language': 'en_CA'}, // 'Canada English (4326)',
        '3027'    : {'language': 'fr_CA'}, // 'Canada Francais (4327)',
        '7551'    : {'language': 'nl_BE'}, // 'Belgium Vlaams',
        '910'     : {'language': 'de_DE'},
        '911'     : {'language': 'fr_FR'},
        '913'     : {'language': 'en_US'},
        '914'     : {'language': 'es_ES'},
        '915'     : {'language': 'pt_PT'},
        '920'     : {'language': 'de_AT'},
        '921'     : {'language': 'fr_BE'},
        '922'     : {'language': 'nl_NL'},
        '923'     : {'language': 'en_GB'},

        /* By 'country' */
        '204'     : {'country': 'nl'},
        '2041'    : {'country': 'nl', 'area': 'North'},
        '2042'    : {'country': 'nl', 'area': 'Center'},
        '2043'    : {'country': 'nl', 'area': 'South'},
        '2044'    : {'country': 'nl', 'area': 'East'},
        '2049881' : {'country': 'nl', 'area': 'XRF088 A'},
        '2049882' : {'country': 'nl', 'area': 'XRF088 B'},

        '206'     : {'country': 'be'},
        '2061'    : {'country': 'be', 'area': 'North'},
        '2062'    : {'country': 'be', 'area': 'South'},
        '2063'    : {'country': 'be', 'area': 'East'},
        '2064'    : {'country': 'be', 'area': 'On demand 1'},
        '2065'    : {'country': 'be', 'area': 'On demand 2'},
        '2066'    : {'country': 'be', 'area': 'On demand 3'},
        '2067'    : {'country': 'be', 'area': 'On demand 4'},
        '2068'    : {'country': 'be', 'area': 'On demand 5'},
        '2069'    : {'country': 'be', 'area': 'On demand 6'},

        '214'     : {'country': 'es'},

        '222'     : {'country': 'it'},
        '2220'    : {'country': 'it', 'area': 'Reg-Zona0'},
        '2221'    : {'country': 'it', 'area': 'Reg-Zona1'},
        '2222'    : {'country': 'it', 'area': 'Reg-Zona2'},
        '2223'    : {'country': 'it', 'area': 'Reg-Zona3'},
        '2224'    : {'country': 'it', 'area': 'Reg-Zona4'},
        '2225'    : {'country': 'it', 'area': 'Reg-Zona5'},
        '2226'    : {'country': 'it', 'area': 'Reg-Zona6'},
        '2227'    : {'country': 'it', 'area': 'Reg-Zona7'},
        '2228'    : {'country': 'it', 'area': 'Reg-Zona8'},
        '2229'    : {'country': 'it', 'area': 'Reg-Zona9'},

        '228'     : {'country': 'ch'},

        '235'     : {'country': 'gb'},
        '2350'    : {'country': 'gb', 'area': '4400'},
        '2351'    : {'country': 'gb', 'area': '1'},
        '2352'    : {'country': 'gb', 'area': 'Wide'},
        '2353'    : {'country': 'gb', 'area': 'DV4 talk'},
        '2354'    : {'country': 'gb', 'area': 'Ireland'},
        '2355'    : {'country': 'gb', 'area': 'Scotland'},
        '2356'    : {'country': 'gb', 'area': 'Wales'},
        '2357'    : {'country': 'gb', 'area': '7'},
        '2358'    : {'country': 'gb', 'area': 'Test'},

        '240'     : {special: 'Märket'},

        '250'     : {'country': 'ru'},
        '2501'    : {'country': 'ru', 'area': 'Global'},
        '2502'    : {'country': 'ru', 'area': 'XRF250'},
        '2505'    : {'country': 'ru', 'area': 'Radiocult'},

        '255'     : {'country': 'ua'},

        '262'     : {'country': 'de'},

        '310'     : {'country': 'us'},
        '3100'    : {'country': 'us', 'area': 'Nation wide'},
        '3106'    : {'country': 'us', 'area': 'California'},
        '31090'   : {'country': 'us', 'area': '0 (4639)'},
        '31091'   : {'country': 'us', 'area': '1 (4641)'},
        '31092'   : {'country': 'us', 'area': '2 (4642)'},
        '31093'   : {'country': 'us', 'area': '3 (4643)'},
        '31094'   : {'country': 'us', 'area': '4 (4644)'},
        '31095'   : {'country': 'us', 'area': '5 (4645)'},
        '31096'   : {'country': 'us', 'area': '6 (4646)'},
        '31097'   : {'country': 'us', 'area': '7 (4647)'},
        '31098'   : {'country': 'us', 'area': '9 (4649)'},
        '31099'   : {'country': 'us', 'area': '8 (4648)'},
        '314700'  : {'country': 'us', 'area': 'Tennessee Statewide'},

        '334'     : {'country': 'mx'},
        '3341'    : {'country': 'mx', 'area': '1'},
        '3342'    : {'country': 'mx', 'area': '2'},
        '3343'    : {'country': 'mx', 'area': '3'},

        '734'     : {'country': 've'}
    };

    /* Merge group ids into DMR.ID */
    $.each(group_ids, function(id, data) {
        if (DMR.ID[id] === undefined) {
            DMR.ID[id] = data;
        }
    });
})(jQuery);
