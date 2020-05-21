/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2015-2016 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <string.h>

#include "gs-content-rating.h"

const gchar *
gs_content_rating_system_to_str (GsContentRatingSystem system)
{
	if (system == GS_CONTENT_RATING_SYSTEM_INCAA)
		return "INCAA";
	if (system == GS_CONTENT_RATING_SYSTEM_ACB)
		return "ACB";
	if (system == GS_CONTENT_RATING_SYSTEM_DJCTQ)
		return "DJCTQ";
	if (system == GS_CONTENT_RATING_SYSTEM_GSRR)
		return "GSRR";
	if (system == GS_CONTENT_RATING_SYSTEM_PEGI)
		return "PEGI";
	if (system == GS_CONTENT_RATING_SYSTEM_KAVI)
		return "KAVI";
	if (system == GS_CONTENT_RATING_SYSTEM_USK)
		return "USK";
	if (system == GS_CONTENT_RATING_SYSTEM_ESRA)
		return "ESRA";
	if (system == GS_CONTENT_RATING_SYSTEM_CERO)
		return "CERO";
	if (system == GS_CONTENT_RATING_SYSTEM_OFLCNZ)
		return "OFLCNZ";
	if (system == GS_CONTENT_RATING_SYSTEM_RUSSIA)
		return "RUSSIA";
	if (system == GS_CONTENT_RATING_SYSTEM_MDA)
		return "MDA";
	if (system == GS_CONTENT_RATING_SYSTEM_GRAC)
		return "GRAC";
	if (system == GS_CONTENT_RATING_SYSTEM_ESRB)
		return "ESRB";
	if (system == GS_CONTENT_RATING_SYSTEM_IARC)
		return "IARC";
	return NULL;
}

/* data obtained from https://en.wikipedia.org/wiki/Video_game_rating_system */
static char *
get_esrb_string (const gchar *source, const gchar *translate)
{
	if (g_strcmp0 (source, translate) == 0)
		return g_strdup (source);
	/* TRANSLATORS: This is the formatting of English and localized name
 	   of the rating e.g. "Adults Only (solo adultos)" */
	return g_strdup_printf (_("%s (%s)"), source, translate);
}

/* data obtained from https://en.wikipedia.org/wiki/Video_game_rating_system */
gchar *
gs_utils_content_rating_age_to_str (GsContentRatingSystem system, guint age)
{
	if (system == GS_CONTENT_RATING_SYSTEM_INCAA) {
		if (age >= 18)
			return g_strdup ("+18");
		if (age >= 13)
			return g_strdup ("+13");
		return g_strdup ("ATP");
	}
	if (system == GS_CONTENT_RATING_SYSTEM_ACB) {
		if (age >= 18)
			return g_strdup ("R18+");
		if (age >= 15)
			return g_strdup ("MA15+");
		return g_strdup ("PG");
	}
	if (system == GS_CONTENT_RATING_SYSTEM_DJCTQ) {
		if (age >= 18)
			return g_strdup ("18");
		if (age >= 16)
			return g_strdup ("16");
		if (age >= 14)
			return g_strdup ("14");
		if (age >= 12)
			return g_strdup ("12");
		if (age >= 10)
			return g_strdup ("10");
		return g_strdup ("L");
	}
	if (system == GS_CONTENT_RATING_SYSTEM_GSRR) {
		if (age >= 18)
			return g_strdup ("限制");
		if (age >= 15)
			return g_strdup ("輔15");
		if (age >= 12)
			return g_strdup ("輔12");
		if (age >= 6)
			return g_strdup ("保護");
		return g_strdup ("普通");
	}
	if (system == GS_CONTENT_RATING_SYSTEM_PEGI) {
		if (age >= 18)
			return g_strdup ("18");
		if (age >= 16)
			return g_strdup ("16");
		if (age >= 12)
			return g_strdup ("12");
		if (age >= 7)
			return g_strdup ("7");
		if (age >= 3)
			return g_strdup ("3");
		return NULL;
	}
	if (system == GS_CONTENT_RATING_SYSTEM_KAVI) {
		if (age >= 18)
			return g_strdup ("18+");
		if (age >= 16)
			return g_strdup ("16+");
		if (age >= 12)
			return g_strdup ("12+");
		if (age >= 7)
			return g_strdup ("7+");
		if (age >= 3)
			return g_strdup ("3+");
		return NULL;
	}
	if (system == GS_CONTENT_RATING_SYSTEM_USK) {
		if (age >= 18)
			return g_strdup ("18");
		if (age >= 16)
			return g_strdup ("16");
		if (age >= 12)
			return g_strdup ("12");
		if (age >= 6)
			return g_strdup ("6");
		return g_strdup ("0");
	}
	/* Reference: http://www.esra.org.ir/ */
	if (system == GS_CONTENT_RATING_SYSTEM_ESRA) {
		if (age >= 18)
			return g_strdup ("+18");
		if (age >= 15)
			return g_strdup ("+15");
		if (age >= 12)
			return g_strdup ("+12");
		if (age >= 7)
			return g_strdup ("+7");
		if (age >= 3)
			return g_strdup ("+3");
		return NULL;
	}
	if (system == GS_CONTENT_RATING_SYSTEM_CERO) {
		if (age >= 18)
			return g_strdup ("Z");
		if (age >= 17)
			return g_strdup ("D");
		if (age >= 15)
			return g_strdup ("C");
		if (age >= 12)
			return g_strdup ("B");
		return g_strdup ("A");
	}
	if (system == GS_CONTENT_RATING_SYSTEM_OFLCNZ) {
		if (age >= 18)
			return g_strdup ("R18");
		if (age >= 16)
			return g_strdup ("R16");
		if (age >= 15)
			return g_strdup ("R15");
		if (age >= 13)
			return g_strdup ("R13");
		return g_strdup ("G");
	}
	if (system == GS_CONTENT_RATING_SYSTEM_RUSSIA) {
		if (age >= 18)
			return g_strdup ("18+");
		if (age >= 16)
			return g_strdup ("16+");
		if (age >= 12)
			return g_strdup ("12+");
		if (age >= 6)
			return g_strdup ("6+");
		return g_strdup ("0+");
	}
	if (system == GS_CONTENT_RATING_SYSTEM_MDA) {
		if (age >= 18)
			return g_strdup ("M18");
		if (age >= 16)
			return g_strdup ("ADV");
		return get_esrb_string ("General", _("General"));
	}
	if (system == GS_CONTENT_RATING_SYSTEM_GRAC) {
		if (age >= 18)
			return g_strdup ("18");
		if (age >= 15)
			return g_strdup ("15");
		if (age >= 12)
			return g_strdup ("12");
		return get_esrb_string ("ALL", _("ALL"));
	}
	if (system == GS_CONTENT_RATING_SYSTEM_ESRB) {
		if (age >= 18)
			return get_esrb_string ("Adults Only", _("Adults Only"));
		if (age >= 17)
			return get_esrb_string ("Mature", _("Mature"));
		if (age >= 13)
			return get_esrb_string ("Teen", _("Teen"));
		if (age >= 10)
			return get_esrb_string ("Everyone 10+", _("Everyone 10+"));
		if (age >= 6)
			return get_esrb_string ("Everyone", _("Everyone"));

		return get_esrb_string ("Early Childhood", _("Early Childhood"));
	}
	/* IARC = everything else */
	if (age >= 18)
		return g_strdup ("18+");
	if (age >= 16)
		return g_strdup ("16+");
	if (age >= 12)
		return g_strdup ("12+");
	if (age >= 7)
		return g_strdup ("7+");
	if (age >= 3)
		return g_strdup ("3+");
	return NULL;
}

/*
 * parse_locale:
 * @locale: (transfer full): a locale to parse
 * @language_out: (out) (optional) (nullable): return location for the parsed
 *    language, or %NULL to ignore
 * @territory_out: (out) (optional) (nullable): return location for the parsed
 *    territory, or %NULL to ignore
 * @codeset_out: (out) (optional) (nullable): return location for the parsed
 *    codeset, or %NULL to ignore
 * @modifier_out: (out) (optional) (nullable): return location for the parsed
 *    modifier, or %NULL to ignore
 *
 * Parse @locale as a locale string of the form
 * `language[_territory][.codeset][@modifier]` — see `man 3 setlocale` for
 * details.
 *
 * On success, %TRUE will be returned, and the components of the locale will be
 * returned in the given addresses, with each component not including any
 * separators. Otherwise, %FALSE will be returned and the components will be set
 * to %NULL.
 *
 * @locale is modified, and any returned non-%NULL pointers will point inside
 * it.
 *
 * Returns: %TRUE on success, %FALSE otherwise
 */
static gboolean
parse_locale (gchar *locale  /* (transfer full) */,
	      const gchar **language_out,
	      const gchar **territory_out,
	      const gchar **codeset_out,
	      const gchar **modifier_out)
{
	gchar *separator;
	const gchar *language = NULL, *territory = NULL, *codeset = NULL, *modifier = NULL;

	separator = strrchr (locale, '@');
	if (separator != NULL) {
		modifier = separator + 1;
		*separator = '\0';
	}

	separator = strrchr (locale, '.');
	if (separator != NULL) {
		codeset = separator + 1;
		*separator = '\0';
	}

	separator = strrchr (locale, '_');
	if (separator != NULL) {
		territory = separator + 1;
		*separator = '\0';
	}

	language = locale;

	/* Parse failure? */
	if (*language == '\0') {
		language = NULL;
		territory = NULL;
		codeset = NULL;
		modifier = NULL;
	}

	if (language_out != NULL)
		*language_out = language;
	if (territory_out != NULL)
		*territory_out = territory;
	if (codeset_out != NULL)
		*codeset_out = codeset;
	if (modifier_out != NULL)
		*modifier_out = modifier;

	return (language != NULL);
}

/* data obtained from https://en.wikipedia.org/wiki/Video_game_rating_system */
GsContentRatingSystem
gs_utils_content_rating_system_from_locale (const gchar *locale)
{
	g_autofree gchar *locale_copy = g_strdup (locale);
	const gchar *territory;

	/* Default to IARC for locales which can’t be parsed. */
	if (!parse_locale (locale_copy, NULL, &territory, NULL, NULL))
		return GS_CONTENT_RATING_SYSTEM_IARC;

	/* Argentina */
	if (g_strcmp0 (territory, "AR") == 0)
		return GS_CONTENT_RATING_SYSTEM_INCAA;

	/* Australia */
	if (g_strcmp0 (territory, "AU") == 0)
		return GS_CONTENT_RATING_SYSTEM_ACB;

	/* Brazil */
	if (g_strcmp0 (territory, "BR") == 0)
		return GS_CONTENT_RATING_SYSTEM_DJCTQ;

	/* Taiwan */
	if (g_strcmp0 (territory, "TW") == 0)
		return GS_CONTENT_RATING_SYSTEM_GSRR;

	/* Europe (but not Finland or Germany), India, Israel,
	 * Pakistan, Quebec, South Africa */
	if ((g_strcmp0 (territory, "GB") == 0) ||
	    g_strcmp0 (territory, "AL") == 0 ||
	    g_strcmp0 (territory, "AD") == 0 ||
	    g_strcmp0 (territory, "AM") == 0 ||
	    g_strcmp0 (territory, "AT") == 0 ||
	    g_strcmp0 (territory, "AZ") == 0 ||
	    g_strcmp0 (territory, "BY") == 0 ||
	    g_strcmp0 (territory, "BE") == 0 ||
	    g_strcmp0 (territory, "BA") == 0 ||
	    g_strcmp0 (territory, "BG") == 0 ||
	    g_strcmp0 (territory, "HR") == 0 ||
	    g_strcmp0 (territory, "CY") == 0 ||
	    g_strcmp0 (territory, "CZ") == 0 ||
	    g_strcmp0 (territory, "DK") == 0 ||
	    g_strcmp0 (territory, "EE") == 0 ||
	    g_strcmp0 (territory, "FR") == 0 ||
	    g_strcmp0 (territory, "GE") == 0 ||
	    g_strcmp0 (territory, "GR") == 0 ||
	    g_strcmp0 (territory, "HU") == 0 ||
	    g_strcmp0 (territory, "IS") == 0 ||
	    g_strcmp0 (territory, "IT") == 0 ||
	    g_strcmp0 (territory, "LZ") == 0 ||
	    g_strcmp0 (territory, "XK") == 0 ||
	    g_strcmp0 (territory, "LV") == 0 ||
	    g_strcmp0 (territory, "FL") == 0 ||
	    g_strcmp0 (territory, "LU") == 0 ||
	    g_strcmp0 (territory, "LT") == 0 ||
	    g_strcmp0 (territory, "MK") == 0 ||
	    g_strcmp0 (territory, "MT") == 0 ||
	    g_strcmp0 (territory, "MD") == 0 ||
	    g_strcmp0 (territory, "MC") == 0 ||
	    g_strcmp0 (territory, "ME") == 0 ||
	    g_strcmp0 (territory, "NL") == 0 ||
	    g_strcmp0 (territory, "NO") == 0 ||
	    g_strcmp0 (territory, "PL") == 0 ||
	    g_strcmp0 (territory, "PT") == 0 ||
	    g_strcmp0 (territory, "RO") == 0 ||
	    g_strcmp0 (territory, "SM") == 0 ||
	    g_strcmp0 (territory, "RS") == 0 ||
	    g_strcmp0 (territory, "SK") == 0 ||
	    g_strcmp0 (territory, "SI") == 0 ||
	    g_strcmp0 (territory, "ES") == 0 ||
	    g_strcmp0 (territory, "SE") == 0 ||
	    g_strcmp0 (territory, "CH") == 0 ||
	    g_strcmp0 (territory, "TR") == 0 ||
	    g_strcmp0 (territory, "UA") == 0 ||
	    g_strcmp0 (territory, "VA") == 0 ||
	    g_strcmp0 (territory, "IN") == 0 ||
	    g_strcmp0 (territory, "IL") == 0 ||
	    g_strcmp0 (territory, "PK") == 0 ||
	    g_strcmp0 (territory, "ZA") == 0)
		return GS_CONTENT_RATING_SYSTEM_PEGI;

	/* Finland */
	if (g_strcmp0 (territory, "FI") == 0)
		return GS_CONTENT_RATING_SYSTEM_KAVI;

	/* Germany */
	if (g_strcmp0 (territory, "DE") == 0)
		return GS_CONTENT_RATING_SYSTEM_USK;

	/* Iran */
	if (g_strcmp0 (territory, "IR") == 0)
		return GS_CONTENT_RATING_SYSTEM_ESRA;

	/* Japan */
	if (g_strcmp0 (territory, "JP") == 0)
		return GS_CONTENT_RATING_SYSTEM_CERO;

	/* New Zealand */
	if (g_strcmp0 (territory, "NZ") == 0)
		return GS_CONTENT_RATING_SYSTEM_OFLCNZ;

	/* Russia: Content rating law */
	if (g_strcmp0 (territory, "RU") == 0)
		return GS_CONTENT_RATING_SYSTEM_RUSSIA;

	/* Singapore */
	if (g_strcmp0 (territory, "SQ") == 0)
		return GS_CONTENT_RATING_SYSTEM_MDA;

	/* South Korea */
	if (g_strcmp0 (territory, "KR") == 0)
		return GS_CONTENT_RATING_SYSTEM_GRAC;

	/* USA, Canada, Mexico */
	if ((g_strcmp0 (territory, "US") == 0) ||
	    g_strcmp0 (territory, "CA") == 0 ||
	    g_strcmp0 (territory, "MX") == 0)
		return GS_CONTENT_RATING_SYSTEM_ESRB;

	/* everything else is IARC */
	return GS_CONTENT_RATING_SYSTEM_IARC;
}

static const gchar *content_rating_strings[GS_CONTENT_RATING_SYSTEM_LAST][7] = {
	{ "3+", "7+", "12+", "16+", "18+", NULL }, /* GS_CONTENT_RATING_SYSTEM_UNKNOWN */
	{ "ATP", "+13", "+18", NULL }, /* GS_CONTENT_RATING_SYSTEM_INCAA */
	{ "PG", "MA15+", "R18+", NULL }, /* GS_CONTENT_RATING_SYSTEM_ACB */
	{ "L", "10", "12", "14", "16", "18", NULL }, /* GS_CONTENT_RATING_SYSTEM_DJCTQ */
	{ "普通", "保護", "輔12", "輔15", "限制", NULL }, /* GS_CONTENT_RATING_SYSTEM_GSRR */
	{ "3", "7", "12", "16", "18", NULL }, /* GS_CONTENT_RATING_SYSTEM_PEGI */
	{ "3+", "7+", "12+", "16+", "18+", NULL }, /* GS_CONTENT_RATING_SYSTEM_KAVI */
	{ "0", "6", "12", "16", "18", NULL}, /* GS_CONTENT_RATING_SYSTEM_USK */
	{ "+3", "+7", "+12", "+15", "+18", NULL }, /* GS_CONTENT_RATING_SYSTEM_ESRA */
	{ "A", "B", "C", "D", "Z", NULL }, /* GS_CONTENT_RATING_SYSTEM_CERO */
	{ "G", "R13", "R15", "R16", "R18", NULL }, /* GS_CONTENT_RATING_SYSTEM_OFLCNZ */
	{ "0+", "6+", "12+", "16+", "18+", NULL }, /* GS_CONTENT_RATING_SYSTEM_RUSSIA */
	{ "General", "ADV", "M18", NULL }, /* GS_CONTENT_RATING_SYSTEM_MDA */
	{ "ALL", "12", "15", "18", NULL }, /* GS_CONTENT_RATING_SYSTEM_GRAC */
	{ "Early Childhood", "Everyone", "Everyone 10+", "Teen", "Mature", "Adults Only", NULL }, /* GS_CONTENT_RATING_SYSTEM_ESRB */
	{ "3+", "7+", "12+", "16+", "18+", NULL }, /* GS_CONTENT_RATING_SYSTEM_IARC */
};

gchar **
gs_utils_content_rating_get_values (GsContentRatingSystem system)
{
	g_return_val_if_fail ((int) system < GS_CONTENT_RATING_SYSTEM_LAST, NULL);

	/* IARC is the fallback for everything */
	if (system == GS_CONTENT_RATING_SYSTEM_UNKNOWN)
		system = GS_CONTENT_RATING_SYSTEM_IARC;

	/* ESRB is special as it requires localised suffixes */
	if (system == GS_CONTENT_RATING_SYSTEM_ESRB) {
		g_auto(GStrv) esrb_ages = g_new0 (gchar *, 7);

		esrb_ages[0] = get_esrb_string (content_rating_strings[system][0], _("Early Childhood"));
		esrb_ages[1] = get_esrb_string (content_rating_strings[system][1], _("Everyone"));
		esrb_ages[2] = get_esrb_string (content_rating_strings[system][2], _("Everyone 10+"));
		esrb_ages[3] = get_esrb_string (content_rating_strings[system][3], _("Teen"));
		esrb_ages[4] = get_esrb_string (content_rating_strings[system][4], _("Mature"));
		esrb_ages[5] = get_esrb_string (content_rating_strings[system][5], _("Adults Only"));
		esrb_ages[6] = NULL;

		return g_steal_pointer (&esrb_ages);
	}

	return g_strdupv ((gchar **) content_rating_strings[system]);
}

static guint content_rating_ages[GS_CONTENT_RATING_SYSTEM_LAST][7] = {
	{ 3, 7, 12, 16, 18 }, /* GS_CONTENT_RATING_SYSTEM_UNKNOWN */
	{ 0, 13, 18 }, /* GS_CONTENT_RATING_SYSTEM_INCAA */
	{ 0, 15, 18 }, /* GS_CONTENT_RATING_SYSTEM_ACB */
	{ 0, 10, 12, 14, 16, 18 }, /* GS_CONTENT_RATING_SYSTEM_DJCTQ */
	{ 0, 6, 12, 15, 18 }, /* GS_CONTENT_RATING_SYSTEM_GSRR */
	{ 3, 7, 12, 16, 18 }, /* GS_CONTENT_RATING_SYSTEM_PEGI */
	{ 3, 7, 12, 16, 18 }, /* GS_CONTENT_RATING_SYSTEM_KAVI */
	{ 0, 6, 12, 16, 18 }, /* GS_CONTENT_RATING_SYSTEM_USK */
	{ 3, 7, 12, 15, 18 }, /* GS_CONTENT_RATING_SYSTEM_ESRA */
	{ 0, 12, 15, 17, 18 }, /* GS_CONTENT_RATING_SYSTEM_CERO */
	{ 0, 13, 15, 16, 18 }, /* GS_CONTENT_RATING_SYSTEM_OFLCNZ */
	{ 0, 6, 12, 16, 18 }, /* GS_CONTENT_RATING_SYSTEM_RUSSIA */
	{ 0, 16, 18 }, /* GS_CONTENT_RATING_SYSTEM_MDA */
	{ 0, 12, 15, 18 }, /* GS_CONTENT_RATING_SYSTEM_GRAC */
	{ 0, 6, 10, 13, 17, 18 }, /* GS_CONTENT_RATING_SYSTEM_ESRB */
	{ 3, 7, 12, 16, 18 }, /* GS_CONTENT_RATING_SYSTEM_IARC */
};

const guint *
gs_utils_content_rating_get_ages (GsContentRatingSystem system)
{
	g_assert (system < GS_CONTENT_RATING_SYSTEM_LAST);
	return content_rating_ages[system];
}

const struct {
	const gchar		*id;
	MctAppFilterOarsValue	 value;
	guint			 csm_age;
} id_to_csm_age[] =  {
/* v1.0 */
{ "violence-cartoon",	MCT_APP_FILTER_OARS_VALUE_NONE,		0 },
{ "violence-cartoon",	MCT_APP_FILTER_OARS_VALUE_MILD,		3 },
{ "violence-cartoon",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	4 },
{ "violence-cartoon",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	6 },
{ "violence-fantasy",	MCT_APP_FILTER_OARS_VALUE_NONE,		0 },
{ "violence-fantasy",	MCT_APP_FILTER_OARS_VALUE_MILD,		3 },
{ "violence-fantasy",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	7 },
{ "violence-fantasy",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	8 },
{ "violence-realistic",	MCT_APP_FILTER_OARS_VALUE_NONE,		0 },
{ "violence-realistic",	MCT_APP_FILTER_OARS_VALUE_MILD,		4 },
{ "violence-realistic",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	9 },
{ "violence-realistic",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	14 },
{ "violence-bloodshed",	MCT_APP_FILTER_OARS_VALUE_NONE,		0 },
{ "violence-bloodshed",	MCT_APP_FILTER_OARS_VALUE_MILD,		9 },
{ "violence-bloodshed",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	11 },
{ "violence-bloodshed",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	18 },
{ "violence-sexual",	MCT_APP_FILTER_OARS_VALUE_NONE,		0 },
{ "violence-sexual",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	18 },
{ "drugs-alcohol",	MCT_APP_FILTER_OARS_VALUE_NONE,		0 },
{ "drugs-alcohol",	MCT_APP_FILTER_OARS_VALUE_MILD,		11 },
{ "drugs-alcohol",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	13 },
{ "drugs-narcotics",	MCT_APP_FILTER_OARS_VALUE_NONE,		0 },
{ "drugs-narcotics",	MCT_APP_FILTER_OARS_VALUE_MILD,		12 },
{ "drugs-narcotics",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	14 },
{ "drugs-tobacco",	MCT_APP_FILTER_OARS_VALUE_NONE,		0 },
{ "drugs-tobacco",	MCT_APP_FILTER_OARS_VALUE_MILD,		10 },
{ "drugs-tobacco",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	13 },
{ "sex-nudity",		MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "sex-nudity",		MCT_APP_FILTER_OARS_VALUE_MILD,		12 },
{ "sex-nudity",		MCT_APP_FILTER_OARS_VALUE_MODERATE,	14 },
{ "sex-themes",		MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "sex-themes",		MCT_APP_FILTER_OARS_VALUE_MILD,		13 },
{ "sex-themes",		MCT_APP_FILTER_OARS_VALUE_MODERATE,	14 },
{ "sex-themes",		MCT_APP_FILTER_OARS_VALUE_INTENSE,	15 },
{ "language-profanity",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "language-profanity",	MCT_APP_FILTER_OARS_VALUE_MILD,		8  },
{ "language-profanity",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	11 },
{ "language-profanity",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	14 },
{ "language-humor",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "language-humor",	MCT_APP_FILTER_OARS_VALUE_MILD,		3  },
{ "language-humor",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	8  },
{ "language-humor",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	14 },
{ "language-discrimination", MCT_APP_FILTER_OARS_VALUE_NONE,	0  },
{ "language-discrimination", MCT_APP_FILTER_OARS_VALUE_MILD,	9  },
{ "language-discrimination", MCT_APP_FILTER_OARS_VALUE_MODERATE,10 },
{ "language-discrimination", MCT_APP_FILTER_OARS_VALUE_INTENSE,	11 },
{ "money-advertising",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "money-advertising",	MCT_APP_FILTER_OARS_VALUE_MILD,		7  },
{ "money-advertising",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	8  },
{ "money-advertising",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	10 },
{ "money-gambling",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "money-gambling",	MCT_APP_FILTER_OARS_VALUE_MILD,		7  },
{ "money-gambling",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	10 },
{ "money-gambling",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	18 },
{ "money-purchasing",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "money-purchasing",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	15 },
{ "social-chat",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "social-chat",	MCT_APP_FILTER_OARS_VALUE_MILD,		4  },
{ "social-chat",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	10 },
{ "social-chat",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	13 },
{ "social-audio",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "social-audio",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	15 },
{ "social-contacts",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "social-contacts",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	12 },
{ "social-info",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "social-info",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	13 },
{ "social-location",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "social-location",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	13 },
/* v1.1 additions */
{ "social-info",	MCT_APP_FILTER_OARS_VALUE_MILD,		0  },
{ "social-info",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	13 },
{ "money-purchasing",	MCT_APP_FILTER_OARS_VALUE_MILD,		12 },
{ "social-chat",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	14 },
{ "sex-homosexuality",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "sex-homosexuality",	MCT_APP_FILTER_OARS_VALUE_MILD,		10 },
{ "sex-homosexuality",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	13 },
{ "sex-homosexuality",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	18 },
{ "sex-prostitution",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "sex-prostitution",	MCT_APP_FILTER_OARS_VALUE_MILD,		12 },
{ "sex-prostitution",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	14 },
{ "sex-prostitution",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	18 },
{ "sex-adultery",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "sex-adultery",	MCT_APP_FILTER_OARS_VALUE_MILD,		8  },
{ "sex-adultery",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	10 },
{ "sex-adultery",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	18 },
{ "sex-appearance",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "sex-appearance",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	10 },
{ "sex-appearance",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	15 },
{ "violence-worship",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "violence-worship",	MCT_APP_FILTER_OARS_VALUE_MILD,		13 },
{ "violence-worship",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	15 },
{ "violence-worship",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	18 },
{ "violence-desecration", MCT_APP_FILTER_OARS_VALUE_NONE,	0  },
{ "violence-desecration", MCT_APP_FILTER_OARS_VALUE_MILD,	13 },
{ "violence-desecration", MCT_APP_FILTER_OARS_VALUE_MODERATE,	15 },
{ "violence-desecration", MCT_APP_FILTER_OARS_VALUE_INTENSE,	18 },
{ "violence-slavery",	MCT_APP_FILTER_OARS_VALUE_NONE,		0  },
{ "violence-slavery",	MCT_APP_FILTER_OARS_VALUE_MILD,		13 },
{ "violence-slavery",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	15 },
{ "violence-slavery",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	18 },

/* EOS customisation to add at least one CSM ↔ OARS mapping for ages 16 and 17,
 * as these are used in many locale-specific ratings systems. Without them,
 * mapping (e.g.) OFLCNZ R16 → CSM 16 → OARS → CSM gives CSM 15, which then maps
 * back to OFLCNZ R15, which is not what we want. The addition of these two
 * mappings should not expose younger users to content they would not have seen
 * with the default upstream mappings; it instead slightly raises the age at
 * which users are allowed to see intense content in these two categories.
 *
 * See https://phabricator.endlessm.com/T23897#666769. */
{ "drugs-alcohol",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	16 },
{ "drugs-narcotics",	MCT_APP_FILTER_OARS_VALUE_INTENSE,	17 },
{ NULL, 0, 0 } };

/**
 * as_content_rating_id_value_to_csm_age:
 * @id: the subsection ID e.g. "violence-cartoon"
 * @value: the #AsContentRatingValue, e.g. %MCT_APP_FILTER_OARS_VALUE_INTENSE
 *
 * Gets the Common Sense Media approved age for a specific rating level.
 *
 * Returns: The age in years, or 0 for no details.
 *
 * Since: 0.5.12
 **/
guint
as_content_rating_id_value_to_csm_age (const gchar *id, MctAppFilterOarsValue value)
{
	guint i;
	for (i = 0; id_to_csm_age[i].id != NULL; i++) {
		if (value == id_to_csm_age[i].value &&
		    g_strcmp0 (id, id_to_csm_age[i].id) == 0)
			return id_to_csm_age[i].csm_age;
	}
	return 0;
}

/**
 * as_content_rating_id_csm_age_to_value:
 * @id: the subsection ID e.g. "violence-cartoon"
 * @age: the age
 *
 * Gets the #MctAppFilterOarsValue for a given age.
 *
 * Returns: the #MctAppFilterOarsValue
 **/
MctAppFilterOarsValue
as_content_rating_id_csm_age_to_value (const gchar *id, guint age)
{
	MctAppFilterOarsValue value;
	guint i;

	value = MCT_APP_FILTER_OARS_VALUE_UNKNOWN;

	for (i = 0; id_to_csm_age[i].id != NULL; i++) {
		if (age >= id_to_csm_age[i].csm_age &&
		    g_strcmp0 (id, id_to_csm_age[i].id) == 0)
			value = MAX (value, id_to_csm_age[i].value);
	}
	return value;
}
