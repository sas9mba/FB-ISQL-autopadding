/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		MetaName.cpp
 *	DESCRIPTION:	metadata name holder
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Alexander Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2005 Alexander Peshkov <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include "firebird.h"

#include <stdarg.h>

#include "../common/classes/MetaName.h"

namespace Firebird {

	MetaName& MetaName::assign(const char* s, FB_SIZE_T l)
	{
		init();
		if (s)
		{
			adjustLength(s, l);
			count = l;
			memcpy(data, s, l);
		}
		else {
			count = 0;
		}
		return *this;
	}

	char* MetaName::getBuffer(const FB_SIZE_T l)
	{
		fb_assert (l < MAX_SQL_IDENTIFIER_SIZE);
		init();
		count = l;
		return data;
	}

	int MetaName::compare(const char* s, FB_SIZE_T l) const
	{
		if (s)
		{
			adjustLength(s, l);
			FB_SIZE_T x = length() < l ? length() : l;
			int rc = memcmp(c_str(), s, x);
			if (rc)
			{
				return rc;
			}
		}
		return length() - l;
	}

	void MetaName::adjustLength(const char* const s, FB_SIZE_T& l)
	{
		fb_assert(s);
		if (l > MAX_SQL_IDENTIFIER_LEN)
		{
			l = MAX_SQL_IDENTIFIER_LEN;
		}
		while (l)
		{
			if (s[l - 1] != ' ')
			{
				break;
			}
			--l;
		}
	}

	void MetaName::printf(const char* format, ...)
	{
		init();
		va_list params;
		va_start(params, format);
		int l = VSNPRINTF(data, MAX_SQL_IDENTIFIER_LEN, format, params);
		if (l < 0 || FB_SIZE_T(l) > MAX_SQL_IDENTIFIER_LEN)
		{
			l = MAX_SQL_IDENTIFIER_LEN;
		}
		data[l] = 0;
		count = l;
		va_end(params);
	}

	FB_SIZE_T MetaName::copyTo(char* to, FB_SIZE_T toSize) const
	{
		fb_assert(to);
		fb_assert(toSize);
		if (--toSize > length())
		{
			toSize = length();
		}
		memcpy(to, c_str(), toSize);
		to[toSize] = 0;
		return toSize;
	}

} // namespace Firebird
