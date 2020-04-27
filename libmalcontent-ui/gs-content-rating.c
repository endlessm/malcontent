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

const gchar *
gs_content_rating_key_value_to_str (const gchar *id, MctAppFilterOarsValue value)
{
	guint i;
	const struct {
		const gchar		*id;
		MctAppFilterOarsValue	 value;
		const gchar		*desc;
	} tab[] =  {
	{ "violence-cartoon",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No cartoon violence") },
	{ "violence-cartoon",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Cartoon characters in unsafe situations") },
	{ "violence-cartoon",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Cartoon characters in aggressive conflict") },
	{ "violence-cartoon",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic violence involving cartoon characters") },
	{ "violence-fantasy",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No fantasy violence") },
	{ "violence-fantasy",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Characters in unsafe situations easily distinguishable from reality") },
	{ "violence-fantasy",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Characters in aggressive conflict easily distinguishable from reality") },
	{ "violence-fantasy",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic violence easily distinguishable from reality") },
	{ "violence-realistic",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No realistic violence") },
	{ "violence-realistic", MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Mildly realistic characters in unsafe situations") },
	{ "violence-realistic",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Depictions of realistic characters in aggressive conflict") },
	{ "violence-realistic",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic violence involving realistic characters") },
	{ "violence-bloodshed",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No bloodshed") },
	{ "violence-bloodshed",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Unrealistic bloodshed") },
	{ "violence-bloodshed",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Realistic bloodshed") },
	{ "violence-bloodshed",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Depictions of bloodshed and the mutilation of body parts") },
	{ "violence-sexual",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No sexual violence") },
	{ "violence-sexual",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Rape or other violent sexual behavior") },
	{ "drugs-alcohol",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No references to alcohol") },
	{ "drugs-alcohol",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("References to alcoholic beverages") },
	{ "drugs-alcohol",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Use of alcoholic beverages") },
	{ "drugs-narcotics",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No references to illicit drugs") },
	{ "drugs-narcotics",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("References to illicit drugs") },
	{ "drugs-narcotics",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Use of illicit drugs") },
	{ "drugs-tobacco",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("References to tobacco products") },
	{ "drugs-tobacco",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Use of tobacco products") },
	{ "sex-nudity",		MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No nudity of any sort") },
	{ "sex-nudity",		MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Brief artistic nudity") },
	{ "sex-nudity",		MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Prolonged nudity") },
	{ "sex-themes",		MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No references or depictions of sexual nature") },
	{ "sex-themes",		MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Provocative references or depictions") },
	{ "sex-themes",		MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Sexual references or depictions") },
	{ "sex-themes",		MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic sexual behavior") },
	{ "language-profanity",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No profanity of any kind") },
	{ "language-profanity",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Mild or infrequent use of profanity") },
	{ "language-profanity",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Moderate use of profanity") },
	{ "language-profanity",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Strong or frequent use of profanity") },
	{ "language-humor",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No inappropriate humor") },
	{ "language-humor",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Slapstick humor") },
	{ "language-humor",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Vulgar or bathroom humor") },
	{ "language-humor",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Mature or sexual humor") },
	{ "language-discrimination", MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No discriminatory language of any kind") },
	{ "language-discrimination", MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Negativity towards a specific group of people") },
	{ "language-discrimination", MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Discrimination designed to cause emotional harm") },
	{ "language-discrimination", MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Explicit discrimination based on gender, sexuality, race or religion") },
	{ "money-advertising", MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No advertising of any kind") },
	{ "money-advertising", MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Product placement") },
	{ "money-advertising", MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Explicit references to specific brands or trademarked products") },
	{ "money-advertising", MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Users are encouraged to purchase specific real-world items") },
	{ "money-gambling",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No gambling of any kind") },
	{ "money-gambling",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Gambling on random events using tokens or credits") },
	{ "money-gambling",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Gambling using “play” money") },
	{ "money-gambling",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Gambling using real money") },
	{ "money-purchasing",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No ability to spend money") },
	{ "money-purchasing",	MCT_APP_FILTER_OARS_VALUE_MILD,		/* v1.1 */
	/* TRANSLATORS: content rating description */
	_("Users are encouraged to donate real money") },
	{ "money-purchasing",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Ability to spend real money in-game") },
	{ "social-chat",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No way to chat with other users") },
	{ "social-chat",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("User-to-user game interactions without chat functionality") },
	{ "social-chat",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Moderated chat functionality between users") },
	{ "social-chat",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Uncontrolled chat functionality between users") },
	{ "social-audio",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No way to talk with other users") },
	{ "social-audio",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Uncontrolled audio or video chat functionality between users") },
	{ "social-contacts",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No sharing of social network usernames or email addresses") },
	{ "social-contacts",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Sharing social network usernames or email addresses") },
	{ "social-info",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No sharing of user information with 3rd parties") },
	{ "social-info",	MCT_APP_FILTER_OARS_VALUE_MILD,		/* v1.1 */
	/* TRANSLATORS: content rating description */
	_("Checking for the latest application version") },
	{ "social-info",	MCT_APP_FILTER_OARS_VALUE_MODERATE,	/* v1.1 */
	/* TRANSLATORS: content rating description */
	_("Sharing diagnostic data that does not let others identify the user") },
	{ "social-info",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Sharing information that lets others identify the user") },
	{ "social-location",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No sharing of physical location to other users") },
	{ "social-location",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Sharing physical location to other users") },

	/* v1.1 */
	{ "sex-homosexuality",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No references to homosexuality") },
	{ "sex-homosexuality",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Indirect references to homosexuality") },
	{ "sex-homosexuality",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Kissing between people of the same gender") },
	{ "sex-homosexuality",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic sexual behavior between people of the same gender") },
	{ "sex-prostitution",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No references to prostitution") },
	{ "sex-prostitution",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Indirect references to prostitution") },
	{ "sex-prostitution",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Direct references to prostitution") },
	{ "sex-prostitution",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic depictions of the act of prostitution") },
	{ "sex-adultery",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No references to adultery") },
	{ "sex-adultery",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Indirect references to adultery") },
	{ "sex-adultery",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Direct references to adultery") },
	{ "sex-adultery",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic depictions of the act of adultery") },
	{ "sex-appearance",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No sexualized characters") },
	{ "sex-appearance",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Scantily clad human characters") },
	{ "sex-appearance",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Overtly sexualized human characters") },
	{ "violence-worship",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No references to desecration") },
	{ "violence-worship",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Depictions or references to historical desecration") },
	{ "violence-worship",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Depictions of modern-day human desecration") },
	{ "violence-worship",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic depictions of modern-day desecration") },
	{ "violence-desecration", MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No visible dead human remains") },
	{ "violence-desecration", MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Visible dead human remains") },
	{ "violence-desecration", MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Dead human remains that are exposed to the elements") },
	{ "violence-desecration", MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic depictions of desecration of human bodies") },
	{ "violence-slavery",	MCT_APP_FILTER_OARS_VALUE_NONE,
	/* TRANSLATORS: content rating description */
	_("No references to slavery") },
	{ "violence-slavery",	MCT_APP_FILTER_OARS_VALUE_MILD,
	/* TRANSLATORS: content rating description */
	_("Depictions or references to historical slavery") },
	{ "violence-slavery",	MCT_APP_FILTER_OARS_VALUE_MODERATE,
	/* TRANSLATORS: content rating description */
	_("Depictions of modern-day slavery") },
	{ "violence-slavery",	MCT_APP_FILTER_OARS_VALUE_INTENSE,
	/* TRANSLATORS: content rating description */
	_("Graphic depictions of modern-day slavery") },
	{ NULL, 0, NULL } };
	for (i = 0; tab[i].id != NULL; i++) {
		if (g_strcmp0 (tab[i].id, id) == 0 && tab[i].value == value)
			return tab[i].desc;
	}
	return NULL;
}

/* data obtained from https://en.wikipedia.org/wiki/Video_game_rating_system */
const gchar *
gs_utils_content_rating_age_to_str (GsContentRatingSystem system, guint age)
{
	if (system == GS_CONTENT_RATING_SYSTEM_INCAA) {
		if (age >= 18)
			return "+18";
		if (age >= 13)
			return "+13";
		return "ATP";
	}
	if (system == GS_CONTENT_RATING_SYSTEM_ACB) {
		if (age >= 18)
			return "R18+";
		if (age >= 15)
			return "MA15+";
		return "PG";
	}
	if (system == GS_CONTENT_RATING_SYSTEM_DJCTQ) {
		if (age >= 18)
			return "18";
		if (age >= 16)
			return "16";
		if (age >= 14)
			return "14";
		if (age >= 12)
			return "12";
		if (age >= 10)
			return "10";
		return "L";
	}
	if (system == GS_CONTENT_RATING_SYSTEM_GSRR) {
		if (age >= 18)
			return "限制";
		if (age >= 15)
			return "輔15";
		if (age >= 12)
			return "輔12";
		if (age >= 6)
			return "保護";
		return "普通";
	}
	if (system == GS_CONTENT_RATING_SYSTEM_PEGI) {
		if (age >= 18)
			return "18";
		if (age >= 16)
			return "16";
		if (age >= 12)
			return "12";
		if (age >= 7)
			return "7";
		if (age >= 3)
			return "3";
		return NULL;
	}
	if (system == GS_CONTENT_RATING_SYSTEM_KAVI) {
		if (age >= 18)
			return "18+";
		if (age >= 16)
			return "16+";
		if (age >= 12)
			return "12+";
		if (age >= 7)
			return "7+";
		if (age >= 3)
			return "3+";
		return NULL;
	}
	if (system == GS_CONTENT_RATING_SYSTEM_USK) {
		if (age >= 18)
			return "18";
		if (age >= 16)
			return "16";
		if (age >= 12)
			return "12";
		if (age >= 6)
			return "6";
		return "0";
	}
	/* Reference: http://www.esra.org.ir/ */
	if (system == GS_CONTENT_RATING_SYSTEM_ESRA) {
		if (age >= 18)
			return "+18";
		if (age >= 15)
			return "+15";
		if (age >= 12)
			return "+12";
		if (age >= 7)
			return "+7";
		if (age >= 3)
			return "+3";
		return NULL;
	}
	if (system == GS_CONTENT_RATING_SYSTEM_CERO) {
		if (age >= 18)
			return "Z";
		if (age >= 17)
			return "D";
		if (age >= 15)
			return "C";
		if (age >= 12)
			return "B";
		return "A";
	}
	if (system == GS_CONTENT_RATING_SYSTEM_OFLCNZ) {
		if (age >= 18)
			return "R18";
		if (age >= 16)
			return "R16";
		if (age >= 15)
			return "R15";
		if (age >= 13)
			return "R13";
		return "G";
	}
	if (system == GS_CONTENT_RATING_SYSTEM_RUSSIA) {
		if (age >= 18)
			return "18+";
		if (age >= 16)
			return "16+";
		if (age >= 12)
			return "12+";
		if (age >= 6)
			return "6+";
		return "0+";
	}
	if (system == GS_CONTENT_RATING_SYSTEM_MDA) {
		if (age >= 18)
			return "M18";
		if (age >= 16)
			return "ADV";
		return "General";
	}
	if (system == GS_CONTENT_RATING_SYSTEM_GRAC) {
		if (age >= 18)
			return "18";
		if (age >= 15)
			return "15";
		if (age >= 12)
			return "12";
		return "ALL";
	}
	if (system == GS_CONTENT_RATING_SYSTEM_ESRB) {
		if (age >= 18)
			return "Adults Only";
		if (age >= 17)
			return "Mature";
		if (age >= 13)
			return "Teen";
		if (age >= 10)
			return "Everyone 10+";
		if (age >= 6)
			return "Everyone";
		return "Early Childhood";
	}
	/* IARC = everything else */
	if (age >= 18)
		return "18+";
	if (age >= 16)
		return "16+";
	if (age >= 12)
		return "12+";
	if (age >= 7)
		return "7+";
	if (age >= 3)
		return "3+";
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
	const gchar *language, *territory;

	/* Default to IARC for locales which can’t be parsed. */
	if (!parse_locale (locale_copy, &language, &territory, NULL, NULL))
		return GS_CONTENT_RATING_SYSTEM_IARC;

	/* Argentina */
	if (g_strcmp0 (language, "ar") == 0)
		return GS_CONTENT_RATING_SYSTEM_INCAA;

	/* Australia */
	if (g_strcmp0 (language, "au") == 0)
		return GS_CONTENT_RATING_SYSTEM_ACB;

	/* Brazil */
	if (g_strcmp0 (language, "pt") == 0 &&
	    g_strcmp0 (territory, "BR") == 0)
		return GS_CONTENT_RATING_SYSTEM_DJCTQ;

	/* Taiwan */
	if (g_strcmp0 (language, "zh") == 0 &&
	    g_strcmp0 (territory, "TW") == 0)
		return GS_CONTENT_RATING_SYSTEM_GSRR;

	/* Europe (but not Finland or Germany), India, Israel,
	 * Pakistan, Quebec, South Africa */
	if ((g_strcmp0 (language, "en") == 0 &&
	     g_strcmp0 (territory, "GB") == 0) ||
	    g_strcmp0 (language, "gb") == 0 ||
	    g_strcmp0 (language, "al") == 0 ||
	    g_strcmp0 (language, "ad") == 0 ||
	    g_strcmp0 (language, "am") == 0 ||
	    g_strcmp0 (language, "at") == 0 ||
	    g_strcmp0 (language, "az") == 0 ||
	    g_strcmp0 (language, "by") == 0 ||
	    g_strcmp0 (language, "be") == 0 ||
	    g_strcmp0 (language, "ba") == 0 ||
	    g_strcmp0 (language, "bg") == 0 ||
	    g_strcmp0 (language, "hr") == 0 ||
	    g_strcmp0 (language, "cy") == 0 ||
	    g_strcmp0 (language, "cz") == 0 ||
	    g_strcmp0 (language, "dk") == 0 ||
	    g_strcmp0 (language, "ee") == 0 ||
	    g_strcmp0 (language, "fr") == 0 ||
	    g_strcmp0 (language, "ge") == 0 ||
	    g_strcmp0 (language, "gr") == 0 ||
	    g_strcmp0 (language, "hu") == 0 ||
	    g_strcmp0 (language, "is") == 0 ||
	    g_strcmp0 (language, "it") == 0 ||
	    g_strcmp0 (language, "kz") == 0 ||
	    g_strcmp0 (language, "xk") == 0 ||
	    g_strcmp0 (language, "lv") == 0 ||
	    g_strcmp0 (language, "fl") == 0 ||
	    g_strcmp0 (language, "lu") == 0 ||
	    g_strcmp0 (language, "lt") == 0 ||
	    g_strcmp0 (language, "mk") == 0 ||
	    g_strcmp0 (language, "mt") == 0 ||
	    g_strcmp0 (language, "md") == 0 ||
	    g_strcmp0 (language, "mc") == 0 ||
	    g_strcmp0 (language, "me") == 0 ||
	    g_strcmp0 (language, "nl") == 0 ||
	    g_strcmp0 (language, "no") == 0 ||
	    g_strcmp0 (language, "pl") == 0 ||
	    g_strcmp0 (language, "pt") == 0 ||
	    g_strcmp0 (language, "ro") == 0 ||
	    g_strcmp0 (language, "sm") == 0 ||
	    g_strcmp0 (language, "rs") == 0 ||
	    g_strcmp0 (language, "sk") == 0 ||
	    g_strcmp0 (language, "si") == 0 ||
	    g_strcmp0 (language, "es") == 0 ||
	    g_strcmp0 (language, "se") == 0 ||
	    g_strcmp0 (language, "ch") == 0 ||
	    g_strcmp0 (language, "tr") == 0 ||
	    g_strcmp0 (language, "ua") == 0 ||
	    g_strcmp0 (language, "va") == 0 ||
	    g_strcmp0 (language, "in") == 0 ||
	    g_strcmp0 (language, "il") == 0 ||
	    g_strcmp0 (language, "pk") == 0 ||
	    g_strcmp0 (language, "za") == 0)
		return GS_CONTENT_RATING_SYSTEM_PEGI;

	/* Finland */
	if (g_strcmp0 (language, "fi") == 0)
		return GS_CONTENT_RATING_SYSTEM_KAVI;

	/* Germany */
	if (g_strcmp0 (language, "de") == 0)
		return GS_CONTENT_RATING_SYSTEM_USK;

	/* Iran */
	if (g_strcmp0 (language, "ir") == 0)
		return GS_CONTENT_RATING_SYSTEM_ESRA;

	/* Japan */
	if (g_strcmp0 (language, "jp") == 0)
		return GS_CONTENT_RATING_SYSTEM_CERO;

	/* New Zealand */
	if (g_strcmp0 (language, "nz") == 0)
		return GS_CONTENT_RATING_SYSTEM_OFLCNZ;

	/* Russia: Content rating law */
	if (g_strcmp0 (language, "ru") == 0)
		return GS_CONTENT_RATING_SYSTEM_RUSSIA;

	/* Singapore */
	if (g_strcmp0 (language, "sg") == 0)
		return GS_CONTENT_RATING_SYSTEM_MDA;

	/* South Korea */
	if (g_strcmp0 (language, "kr") == 0)
		return GS_CONTENT_RATING_SYSTEM_GRAC;

	/* USA, Canada, Mexico */
	if ((g_strcmp0 (language, "en") == 0 &&
	     g_strcmp0 (territory, "US") == 0) ||
	    g_strcmp0 (language, "us") == 0 ||
	    g_strcmp0 (language, "ca") == 0 ||
	    g_strcmp0 (language, "mx") == 0)
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

const gchar * const *
gs_utils_content_rating_get_values (GsContentRatingSystem system)
{
	g_assert (system < GS_CONTENT_RATING_SYSTEM_LAST);
	return content_rating_strings[system];
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
