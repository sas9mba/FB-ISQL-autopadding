/*
 *	PROGRAM:	SQL Trace plugin
 *	MODULE:		TracePluginImpl.cpp
 *	DESCRIPTION:	Plugin implementation
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *  2008 Khorsun Vladyslav
*/

#include <time.h>
#include <math.h>

#include "TracePluginImpl.h"
#include "PluginLogWriter.h"
#include "os/platform.h"
#include "consts_pub.h"
#include "codetext.h"
#include "../../common/isc_f_proto.h"
#include "../../jrd/RuntimeStatistics.h"
#include "../../common/dsc.h"
#include "../../common/utils_proto.h"
#include "../../common/UtilSvc.h"
#include "../../jrd/svc_undoc.h"
#include "../../jrd/constants.h"
#include "../../common/os/path_utils.h"
#include "../../jrd/inf_pub.h"
#include "../../dsql/sqlda_pub.h"
#include "../../common/classes/ImplementHelper.h"
#include "../../common/SimpleStatusVector.h"
#include "../../jrd/status.h"

using namespace Firebird;
using namespace Jrd;

static const char* const DEFAULT_LOG_NAME = "default_trace.log";

#ifdef WIN_NT
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif


/// TracePluginImpl

const char* TracePluginImpl::marshal_exception(const Firebird::Exception& ex)
{
	Firebird::StaticStatusVector st;
	ex.stuffException(st);
	const ISC_STATUS* status = st.begin();

	char buff[1024];
	char* p = buff;
	char* const end = buff + sizeof(buff) - 1;

	const ISC_STATUS* s = status;
	while (end > p && fb_interpret(p, end - p, &s))
	{
		p += strlen(p);
		if (p < end)
			*p++ = '\n';
	}
	*p = 0;

	set_error_string(buff);
	return get_error_string();
}

TracePluginImpl::TracePluginImpl(IPluginBase* plugin,
								 const TracePluginConfig& configuration,
								 ITraceInitInfo* initInfo) :
	factory(plugin),
	operational(false),
	session_id(initInfo->getTraceSessionID()),
	session_name(*getDefaultMemoryPool()),
	logWriter(initInfo->getLogWriter()),
	config(configuration),
	record(*getDefaultMemoryPool()),
	connections(getDefaultMemoryPool()),
	transactions(getDefaultMemoryPool()),
	statements(getDefaultMemoryPool()),
	services(getDefaultMemoryPool()),
	unicodeCollation(*getDefaultMemoryPool()),
	include_codes(*getDefaultMemoryPool()),
	exclude_codes(*getDefaultMemoryPool())
{
	const char* ses_name = initInfo->getTraceSessionName();
	session_name = ses_name && *ses_name ? ses_name : " ";

	if (!logWriter)
	{
		PathName logname(configuration.log_filename);
		if (logname.empty()) {
			logname = DEFAULT_LOG_NAME;
		}

		if (PathUtils::isRelative(logname))
		{
			PathName root(initInfo->getFirebirdRootDirectory());
			PathUtils::ensureSeparator(root);
			logname.insert(0, root);
		}

		logWriter = FB_NEW PluginLogWriter(logname.c_str(), config.max_log_size * 1024 * 1024);
		logWriter->addRef();
	}

	Jrd::TextType* textType = unicodeCollation.getTextType();

	// Compile filtering regular expressions
	const char* str = NULL;
	try
	{
		if (config.include_filter.hasData())
		{
			str = config.include_filter.c_str();
			string filter(config.include_filter);
			ISC_systemToUtf8(filter);

			include_matcher = FB_NEW TraceSimilarToMatcher(
				*getDefaultMemoryPool(), textType, (const UCHAR*) filter.c_str(),
				filter.length(), '\\', true);
		}

		if (config.exclude_filter.hasData())
		{
			str = config.exclude_filter.c_str();
			string filter(config.exclude_filter);
			ISC_systemToUtf8(filter);

			exclude_matcher = FB_NEW TraceSimilarToMatcher(
				*getDefaultMemoryPool(), textType, (const UCHAR*) filter.c_str(),
				filter.length(), '\\', true);
		}
	}
	catch (const Exception&)
	{
		if (config.db_filename.empty())
		{
			fatal_exception::raiseFmt(
				"error while compiling regular expression \"%s\"", str);
		}
		else
		{
			fatal_exception::raiseFmt(
				"error while compiling regular expression \"%s\" for database \"%s\"",
				str, config.db_filename.c_str());
		}
	}

	// parse filters for gds error codes
	if (!config.include_gds_codes.isEmpty())
		str2Array(config.include_gds_codes, include_codes);

	if (!config.exclude_gds_codes.isEmpty())
		str2Array(config.exclude_gds_codes, exclude_codes);

	operational = true;
	log_init();
}

TracePluginImpl::~TracePluginImpl()
{
	// All objects must have been free already, but in case something remained
	// deallocate tracking objects now.

	if (operational)
	{
		if (connections.getFirst())
		{
			do {
				connections.current().deallocate_references();
			} while (connections.getNext());
		}

		if (transactions.getFirst())
		{
			do {
				transactions.current().deallocate_references();
			} while (transactions.getNext());
		}

		if (statements.getFirst())
		{
			do {
				delete statements.current().description;
			} while (statements.getNext());
		}

		if (services.getFirst())
		{
			do {
				services.current().deallocate_references();
			} while (services.getNext());
		}

		log_finalize();
	}
}

void TracePluginImpl::logRecord(const char* action)
{
	// We use atomic file appends for logging. Do not try to break logging
	// to multiple separate file operations
	const Firebird::TimeStamp stamp(Firebird::TimeStamp::getCurrentTimeStamp());
	struct tm times;
	stamp.decode(&times);

	char buffer[100];
	SNPRINTF(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%04d (%d:%p) %s" NEWLINE,
		times.tm_year + 1900, times.tm_mon + 1, times.tm_mday, times.tm_hour,
		times.tm_min, times.tm_sec, (int) (stamp.value().timestamp_time % ISC_TIME_SECONDS_PRECISION),
		get_process_id(), this, action);

	record.insert(0, buffer);
	record.append(NEWLINE);
	// TODO: implement adjusting of line breaks
	// line.adjustLineBreaks();

	logWriter->write(record.c_str(), record.length());

	record = "";
}

void TracePluginImpl::logRecordConn(const char* action, ITraceDatabaseConnection* connection)
{
	// Lookup connection description
	const AttNumber conn_id = connection->getConnectionID();
	bool reg = false;

	while (true)
	{
		{
			ReadLockGuard lock(connectionsLock, FB_FUNCTION);
			ConnectionsTree::Accessor accessor(&connections);
			if (accessor.locate(conn_id))
			{
				record.insert(0, *accessor.current().description);
				break;
			}
		}

		if (reg)
		{
			string temp;
			temp.printf("\t%s (ATT_%" SQUADFORMAT", <unknown, bug?>)" NEWLINE,
				config.db_filename.c_str(), conn_id);
			record.insert(0, temp);
			break;
		}

		register_connection(connection);
		reg = true;
	}

	// don't keep failed connection
	if (!conn_id)
	{
		WriteLockGuard lock(connectionsLock, FB_FUNCTION);
		ConnectionsTree::Accessor accessor(&connections);
		if (accessor.locate(conn_id))
		{
			accessor.current().deallocate_references();
			accessor.fastRemove();
		}
	}

	logRecord(action);
}

void TracePluginImpl::logRecordTrans(const char* action, ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction)
{
	const TraNumber tra_id = transaction->getTransactionID();
	bool reg = false;
	while (true)
	{
		// Lookup transaction description
		{
			ReadLockGuard lock(transactionsLock, FB_FUNCTION);
			TransactionsTree::Accessor accessor(&transactions);
			if (accessor.locate(tra_id))
			{
				record.insert(0, *accessor.current().description);
				break;
			}
		}

		if (reg)
		{
			string temp;
			temp.printf("\t\t(TRA_%" SQUADFORMAT", <unknown, bug?>)" NEWLINE, transaction->getTransactionID());
			record.insert(0, temp);
			break;
		}

		register_transaction(transaction);
		reg = true;
	}

	logRecordConn(action, connection);
}

void TracePluginImpl::logRecordProcFunc(const char* action, ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, const char* obj_type, const char* obj_name)
{
	string temp;
	temp.printf(NEWLINE "%s %s:" NEWLINE, obj_type, obj_name);
	record.insert(0, temp);

	if (!transaction) {
		logRecordConn(action, connection);
	}
	else {
		logRecordTrans(action, connection, transaction);
	}
}

void TracePluginImpl::logRecordStmt(const char* action, ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceStatement* statement, bool isSQL)
{
	const StmtNumber stmt_id = statement->getStmtID();
	bool reg = false;
	bool log = true;

	while (true)
	{
		// Lookup description for statement
		{
			ReadLockGuard lock(statementsLock, FB_FUNCTION);

			StatementsTree::Accessor accessor(&statements);
			if (accessor.locate(stmt_id))
			{
				const string* description = accessor.current().description;

				log = (description != NULL);
				// Do not say anything about statements which do not fall under filter criteria
				if (log) {
					record.insert(0, *description);
				}
				break;
			}
		}

		if (reg)
		{
			string temp;
			temp.printf(NEWLINE "Statement %" SQUADFORMAT", <unknown, bug?>:" NEWLINE, stmt_id);
			record.insert(0, temp);
			break;
		}

		if (isSQL) {
			register_sql_statement((ITraceSQLStatement*) statement);
		}
		else {
			register_blr_statement((ITraceBLRStatement*) statement);
		}
		reg = true;
	}

	// don't need to keep failed statement
	if (!stmt_id)
	{
		WriteLockGuard lock(statementsLock, FB_FUNCTION);
		StatementsTree::Accessor accessor(&statements);
		if (accessor.locate(stmt_id))
		{
			delete accessor.current().description;
			accessor.fastRemove();
		}
	}

	if (!log)
	{
		record = "";
		return;
	}

	if (!transaction) {
		logRecordConn(action, connection);
	}
	else {
		logRecordTrans(action, connection, transaction);
	}
}

void TracePluginImpl::logRecordServ(const char* action, ITraceServiceConnection* service)
{
	ServiceId svc_id = service->getServiceID();
	bool reg = false;

	while (true)
	{
		// Lookup service description
		{
			ReadLockGuard lock(servicesLock, FB_FUNCTION);

			ServicesTree::Accessor accessor(&services);
			if (accessor.locate(svc_id))
			{
				record.insert(0, *accessor.current().description);
				break;
			}
		}

		if (reg)
		{
			string temp;
			temp.printf("\tService %p, <unknown, bug?>" NEWLINE, svc_id);
			record.insert(0, temp);
			break;
		}

		register_service(service);
		reg = true;
	}

	logRecord(action);
}

void TracePluginImpl::logRecordError(const char* action, ITraceConnection* connection,
	ITraceStatusVector* status)
{
	const char* err = status->getText();

	record.insert(0, err);

	if (connection)
	{
		switch (connection->getKind())
		{
		case ITraceConnection::KIND_DATABASE:
			logRecordConn(action, (ITraceDatabaseConnection*) connection);
			break;

		case ITraceConnection::KIND_SERVICE:
			logRecordServ(action, (ITraceServiceConnection*) connection);
			break;

		default:
			break;
		}
	}
	else
		logRecord(action);
}

void TracePluginImpl::appendGlobalCounts(const PerformanceInfo* info)
{
	string temp;

	temp.printf("%7" QUADFORMAT"d ms", info->pin_time);
	record.append(temp);

	ntrace_counter_t cnt;

	if ((cnt = info->pin_counters[RuntimeStatistics::PAGE_READS]) != 0)
	{
		temp.printf(", %" QUADFORMAT"d read(s)", cnt);
		record.append(temp);
	}

	if ((cnt = info->pin_counters[RuntimeStatistics::PAGE_WRITES]) != 0)
	{
		temp.printf(", %" QUADFORMAT"d write(s)", cnt);
		record.append(temp);
	}

	if ((cnt = info->pin_counters[RuntimeStatistics::PAGE_FETCHES]) != 0)
	{
		temp.printf(", %" QUADFORMAT"d fetch(es)", cnt);
		record.append(temp);
	}

	if ((cnt = info->pin_counters[RuntimeStatistics::PAGE_MARKS]) != 0)
	{
		temp.printf(", %" QUADFORMAT"d mark(s)", cnt);
		record.append(temp);
	}

	record.append(NEWLINE);
}

void TracePluginImpl::appendTableCounts(const PerformanceInfo *info)
{
	if (!config.print_perf || info->pin_count == 0)
		return;

	const TraceCounts* trc = info->pin_tables;
	const TraceCounts* trc_end = trc + info->pin_count;

	FB_SIZE_T max_len = 0;
	for (; trc < trc_end; trc++)
	{
		FB_SIZE_T len = fb_strlen(trc->trc_relation_name);
		if (max_len < len)
			max_len = len;
	}

	if (max_len < 32)
		max_len = 32;

	record.append(NEWLINE"Table");
	record.append(max_len - 5, ' ');
	record.append("   Natural     Index    Update    Insert    Delete   Backout     Purge   Expunge" NEWLINE);
	record.append(max_len + 80, '*');
	record.append(NEWLINE);

	string temp;
	for (trc = info->pin_tables; trc < trc_end; trc++)
	{
		record.append(trc->trc_relation_name);
		record.append(max_len - fb_strlen(trc->trc_relation_name), ' ');
		for (int j = 0; j < DBB_max_rel_count; j++)
		{
			if (trc->trc_counters[j] == 0)
			{
				record.append(10, ' ');
			}
			else
			{
				//fb_utils::exactNumericToStr(trc->trc_counters[j], 0, temp);
				//record.append(' ', 10 - temp.length());
				temp.printf("%10" QUADFORMAT"d", trc->trc_counters[j]);
				record.append(temp);
			}
		}
		record.append(NEWLINE);
	}
}


void TracePluginImpl::formatStringArgument(string& result, const UCHAR* str, size_t len)
{
	if (config.max_arg_length && len > config.max_arg_length)
	{
		/* CVC: We will wrap with the original code.
		len = config.max_arg_length - 3;
		if (len < 0)
			len = 0;
		*/
		if (config.max_arg_length < 3)
			len = 0;
		else
			len = config.max_arg_length - 3;

		result.printf("%.*s...", len, str);
		return;
	}
	result.printf("%.*s", len, str);
}


bool TracePluginImpl::filterStatus(const ISC_STATUS* status, GdsCodesArray& arr)
{
	FB_SIZE_T pos;

	while (*status != isc_arg_end)
	{
		const ISC_STATUS s = *status;

		switch (s)
		{
		case isc_arg_gds:
		case isc_arg_warning:
			if (arr.find(status[1], pos))
				return true;
			status += 2;
			break;

		case isc_arg_cstring:
			status += 3;
			break;

		default:
			status += 2;
			break;
		}
	}

	return false;
}


namespace {

class GdsName2CodeMap
{
public:
	GdsName2CodeMap(MemoryPool& pool) :
	  m_map(pool)
	{
		for (int i = 0; codes[i].code_string; i++)
			m_map.put(codes[i].code_string, codes[i].code_number);
	}

	bool find(const char* name, ISC_STATUS& code) const
	{
		return m_map.get(name, code);
	}

private:
	class NocaseCmp
	{
	public:
		static bool greaterThan(const char* i1, const char* i2)
		{
			return fb_utils::stricmp(i1, i2) > 0;
		}
	};

	GenericMap<Pair<NonPooled<const char*, ISC_STATUS> >, NocaseCmp > m_map;
};

}	// namespace

static InitInstance<GdsName2CodeMap> gdsNamesMap;

void TracePluginImpl::str2Array(const Firebird::string& str, GdsCodesArray& arr)
{
	// input: string with comma-delimited list of gds codes values and\or gds codes names
	// output: sorted array of gds codes values

	const char *sep = " ,";

	FB_SIZE_T p1 = 0, p2 = 0;
	while (p2 < str.length())
	{
		p2 = str.find_first_of(sep, p1);
		if (p2 == string::npos)
			p2 = str.length();

		string s = str.substr(p1, p2 - p1);

		ISC_STATUS code = atol(s.c_str());

		if (!code && !gdsNamesMap().find(s.c_str(), code))
		{
			fatal_exception::raiseFmt(
				"Error parsing error codes filter: \n"
				"\t%s\n"
				"\tbad item is: %s, at position: %d",
				str.c_str(), s.c_str(), p1 + 1);
		}

		// avoid duplicates

		FB_SIZE_T ins_pos;
		if (!arr.find(code, ins_pos))
			arr.insert(ins_pos, code);

		p1 = str.find_first_not_of(sep, p2);
	}
}


void TracePluginImpl::appendParams(ITraceParams* params)
{
	const FB_SIZE_T paramcount = params->getCount();
	if (!paramcount)
		return;

	// NS: Please, do not move strings inside the loop. This is performance-sensitive piece of code.
	string paramtype;
	string paramvalue;
	string temp;

	for (FB_SIZE_T i = 0; i < paramcount; i++)
	{
		const struct dsc* parameters = params->getParam(i);

		// See if we need to print any more arguments
		if (config.max_arg_count && i >= config.max_arg_count)
		{
			temp.printf("...%d more arguments skipped..." NEWLINE, paramcount - i);
			record.append(temp);
			break;
		}

		// Assign type name
		switch (parameters->dsc_dtype)
		{
			case dtype_text:
				paramtype.printf("char(%d)", parameters->dsc_length);
				break;
			case dtype_cstring:
				paramtype.printf("cstring(%d)", parameters->dsc_length - 1);
				break;
			case dtype_varying:
				paramtype.printf("varchar(%d)", parameters->dsc_length - 2);
				break;
			case dtype_blob:
				paramtype = "blob";
				break;
			case dtype_array:
				paramtype = "array";
				break;
			case dtype_quad:
				paramtype = "quad";
				break;

			case dtype_short:
				if (parameters->dsc_scale)
					paramtype.printf("smallint(*, %d)", parameters->dsc_scale);
				else
					paramtype = "smallint";
				break;
			case dtype_long:
				if (parameters->dsc_scale)
					paramtype.printf("integer(*, %d)", parameters->dsc_scale);
				else
					paramtype = "integer";
				break;
			case dtype_int64:
				if (parameters->dsc_scale)
					paramtype.printf("bigint(*, %d)", parameters->dsc_scale);
				else
					paramtype = "bigint";
				break;

			case dtype_real:
				paramtype = "float";
				break;
			case dtype_double:
				if (parameters->dsc_scale)
					paramtype.printf("double precision(*, %d)", parameters->dsc_scale);
				else
					paramtype = "double precision";
				break;

			case dtype_dec64:
				paramtype = "decfloat(16)";
				break;
			case dtype_dec128:
				paramtype = "decfloat(34)";
				break;
			case dtype_dec_fixed:
				paramtype = "decimal";
				break;

			case dtype_sql_date:
				paramtype = "date";
				break;
			case dtype_sql_time:
				paramtype = "time";
				break;
			case dtype_timestamp:
				paramtype = "timestamp";
				break;

			case dtype_dbkey:
				paramtype = "db_key";
				break;

			case dtype_boolean:
				paramtype = "boolean";
				break;

			default:
				paramtype.printf("<type%d>", parameters->dsc_dtype);
				break;
		}

		if (parameters->dsc_flags & DSC_null)
		{
			paramvalue = "<NULL>";
		}
		else
		{
			// Assign value
			switch (parameters->dsc_dtype)
			{
				// Handle potentially long string values
				case dtype_text:
				{
					FbLocalStatus status;
					const char* text = params->getTextUTF8(&status, i);

					if (status->getState() & IStatus::STATE_ERRORS)
					{
						formatStringArgument(paramvalue,
							parameters->dsc_address, parameters->dsc_length);
					}
					else
						formatStringArgument(paramvalue, (UCHAR*) text, strlen(text));

					break;
				}
				case dtype_cstring:
					formatStringArgument(paramvalue,
						parameters->dsc_address,
						strlen(reinterpret_cast<const char*>(parameters->dsc_address)));
					break;
				case dtype_varying:
				{
					FbLocalStatus status;
					const char* text = params->getTextUTF8(&status, i);

					if (status->getState() & IStatus::STATE_ERRORS)
					{
						formatStringArgument(paramvalue,
							parameters->dsc_address + 2,
							*(USHORT*)parameters->dsc_address);
					}
					else
						formatStringArgument(paramvalue, (UCHAR*) text, strlen(text));

					break;
				}

				// Handle quad
				case dtype_quad:
				case dtype_blob:
				case dtype_array:
				case dtype_dbkey:
				{
					ISC_QUAD *quad = (ISC_QUAD*) parameters->dsc_address;
					paramvalue.printf("%08X%08X", quad->gds_quad_high, quad->gds_quad_low);
					break;
				}

				case dtype_short:
					fb_utils::exactNumericToStr(*(SSHORT*) parameters->dsc_address, parameters->dsc_scale, paramvalue);
					break;

				case dtype_long:
					fb_utils::exactNumericToStr(*(SLONG*) parameters->dsc_address, parameters->dsc_scale, paramvalue);
					break;

				case dtype_int64:
					fb_utils::exactNumericToStr(*(SINT64*) parameters->dsc_address, parameters->dsc_scale, paramvalue);
					break;

				case dtype_real:
					if (!parameters->dsc_scale) {
						paramvalue.printf("%.7g", *(float*) parameters->dsc_address);
					}
					else {
						paramvalue.printf("%.7g",
							*(float*) parameters->dsc_address * pow(10.0f, -parameters->dsc_scale));
					}
					break;

				case dtype_double:
					if (!parameters->dsc_scale) {
						paramvalue.printf("%.15g", *(double*) parameters->dsc_address);
					}
					else {
						paramvalue.printf("%.15g",
							*(double*) parameters->dsc_address * pow(10.0, -parameters->dsc_scale));
					}
					break;

				case dtype_dec64:
					((Decimal64*) parameters->dsc_address)->toString(paramvalue);
					break;

				case dtype_dec128:
					((Decimal128*) parameters->dsc_address)->toString(paramvalue);
					break;

				case dtype_dec_fixed:
					try
					{
						DecimalStatus decSt(FB_DEC_Errors);
						((DecimalFixed*) parameters->dsc_address)->toString(decSt, parameters->dsc_scale, paramvalue);
					}
					catch (const Exception& ex)
					{
						StaticStatusVector status;
						ex.stuffException(status);
						paramvalue.printf("Conversion error %d\n", status[1]);
					}
					break;

				case dtype_sql_date:
				{
					struct tm times;
					Firebird::TimeStamp::decode_date(*(ISC_DATE*)parameters->dsc_address, &times);
					paramvalue.printf("%04d-%02d-%02d", times.tm_year + 1900, times.tm_mon + 1, times.tm_mday);
					break;
				}
				case dtype_sql_time:
				{
					int hours, minutes, seconds, fractions;
					Firebird::TimeStamp::decode_time(*(ISC_TIME*) parameters->dsc_address,
						&hours, &minutes, &seconds, &fractions);

					paramvalue.printf("%02d:%02d:%02d.%04d", hours,	minutes, seconds, fractions);
					break;
				}
				case dtype_timestamp:
				{
					Firebird::TimeStamp ts(*(ISC_TIMESTAMP*) parameters->dsc_address);
					struct tm times;

					ts.decode(&times);

					paramvalue.printf("%04d-%02d-%02dT%02d:%02d:%02d.%04d",
						times.tm_year + 1900, times.tm_mon + 1, times.tm_mday,
						times.tm_hour, times.tm_min, times.tm_sec,
						ts.value().timestamp_time % ISC_TIME_SECONDS_PRECISION);
					break;
				}

				case dtype_boolean:
					paramvalue = *parameters->dsc_address ? "<true>" : "<false>";
					break;

				default:
					paramvalue = "<unknown>";
			}
		}
		temp.printf("param%d = %s, \"%s\"" NEWLINE, i, paramtype.c_str(), paramvalue.c_str());
		record.append(temp);
	}
}

void TracePluginImpl::appendServiceQueryParams(size_t send_item_length,
		const ntrace_byte_t* send_items, size_t recv_item_length,
		const ntrace_byte_t* recv_items)
{
	string send_query;
	string recv_query;
	USHORT l;
	UCHAR item;
	//USHORT timeout = 0; // Unused

	const UCHAR* items = send_items;
	const UCHAR* const end_items = items + send_item_length;
	while (items < end_items && *items != isc_info_end)
	{
		switch ((item = *items++))
		{
		case isc_info_end:
			break;

		default:
			if (items + 2 <= end_items)
			{
				l = (USHORT) gds__vax_integer(items, 2);
				items += 2;
				if (items + l <= end_items)
				{
					switch (item)
					{
					case isc_info_svc_line:
						send_query.printf(NEWLINE "\t\t send line: %.*s", l, items);
						break;
					case isc_info_svc_message:
						send_query.printf(NEWLINE "\t\t send message: %.*s", l + 3, items - 3);
						break;
					case isc_info_svc_timeout:
						send_query.printf(NEWLINE "\t\t set timeout: %d",
							(USHORT) gds__vax_integer(items, l));
						break;
					case isc_info_svc_version:
						send_query.printf(NEWLINE "\t\t set version: %d",
							(USHORT) gds__vax_integer(items, l));
						break;
					}
				}
				items += l;
			}
			else
				items += 2;
			break;
		}
	}

	if (send_query.hasData())
	{
		record.append("\t Send portion of the query:");
		record.append(send_query);
	}

	items = recv_items;
	const UCHAR* const end_items2 = items + recv_item_length;

	if (*items == isc_info_length) {
		items++;
	}

	while (items < end_items2 && *items != isc_info_end)
	{
		switch ((item = *items++))
		{
			case isc_info_end:
				break;

			case isc_info_svc_svr_db_info:
				recv_query.printf(NEWLINE "\t\t retrieve number of attachments and databases");
				break;

			case isc_info_svc_svr_online:
				recv_query.printf(NEWLINE "\t\t set service online");
				break;

			case isc_info_svc_svr_offline:
				recv_query.printf(NEWLINE "\t\t set service offline");
				break;

			case isc_info_svc_get_env:
				recv_query.printf(NEWLINE "\t\t retrieve the setting of $FIREBIRD");
				break;

			case isc_info_svc_get_env_lock:
				recv_query.printf(NEWLINE "\t\t retrieve the setting of $FIREBIRD_LOCK");
				break;

			case isc_info_svc_get_env_msg:
				recv_query.printf(NEWLINE "\t\t retrieve the setting of $FIREBIRD_MSG");
				break;

			case isc_info_svc_dump_pool_info:
				recv_query.printf(NEWLINE "\t\t print memory counters");
				break;

			case isc_info_svc_get_config:
				recv_query.printf(NEWLINE "\t\t retrieve the parameters and values for IB_CONFIG");
				break;

			case isc_info_svc_default_config:
				recv_query.printf(NEWLINE "\t\t reset the config values to defaults");
				break;

			case isc_info_svc_set_config:
				recv_query.printf(NEWLINE "\t\t set the config values");
				break;

			case isc_info_svc_version:
				recv_query.printf(NEWLINE "\t\t retrieve the version of the service manager");
				break;

			case isc_info_svc_capabilities:
				recv_query.printf(NEWLINE "\t\t retrieve a bitmask representing the server's capabilities");
				break;

			case isc_info_svc_server_version:
				recv_query.printf(NEWLINE "\t\t retrieve the version of the server engine");
				break;

			case isc_info_svc_implementation:
				recv_query.printf(NEWLINE "\t\t retrieve the implementation of the Firebird server");
				break;

			case isc_info_svc_user_dbpath:
				recv_query.printf(NEWLINE "\t\t retrieve the path to the security database in use by the server");
				break;

			case isc_info_svc_response:
				recv_query.printf(NEWLINE "\t\t retrieve service response");
				break;

			case isc_info_svc_response_more:
				recv_query.printf(NEWLINE "\t\t retrieve service response more");
				break;

			case isc_info_svc_total_length:
				recv_query.printf(NEWLINE "\t\t retrieve total length");
				break;

			case isc_info_svc_line:
				recv_query.printf(NEWLINE "\t\t retrieve 1 line of service output per call");
				break;

			case isc_info_svc_to_eof:
				recv_query.printf(NEWLINE "\t\t retrieve as much of the server output as will fit in the supplied buffer");
				break;

			case isc_info_svc_limbo_trans:
				recv_query.printf(NEWLINE "\t\t retrieve the limbo transactions");
				break;

			case isc_info_svc_get_users:
				recv_query.printf(NEWLINE "\t\t retrieve the user information");
				break;

			case isc_info_svc_stdin:
				recv_query.printf(NEWLINE "\t\t retrieve the size of data to send to the server");
				break;
		}
	}

	if (recv_query.hasData())
	{
		record.append("\t Receive portion of the query:");
		record.append(recv_query);
	}
}

void TracePluginImpl::log_init()
{
	if (config.log_initfini)
	{
		record.printf("\tSESSION_%d %s" NEWLINE "\t%s" NEWLINE,
			session_id, session_name.c_str(), config.db_filename.c_str());
		logRecord("TRACE_INIT");
	}
}

void TracePluginImpl::log_finalize()
{
	if (config.log_initfini)
	{
		record.printf("\tSESSION_%d %s" NEWLINE "\t%s" NEWLINE,
			session_id, session_name.c_str(), config.db_filename.c_str());
		logRecord("TRACE_FINI");
	}

	logWriter->release();
	logWriter = NULL;
}

void TracePluginImpl::register_connection(ITraceDatabaseConnection* connection)
{
	ConnectionData conn_data;
	conn_data.id = connection->getConnectionID();
	conn_data.description = FB_NEW_POOL(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());

	string tmp(*getDefaultMemoryPool());

	conn_data.description->printf("\t%s (ATT_%" SQUADFORMAT,
		connection->getDatabaseName(), connection->getConnectionID());

	const char* user = connection->getUserName();
	if (user)
	{
		const char* role = connection->getRoleName();
		if (role && *role) {
			tmp.printf(", %s:%s", user, role);
		}
		else {
			tmp.printf(", %s", user);
		}
		conn_data.description->append(tmp);
	}
	else
	{
		conn_data.description->append(", <unknown_user>");
	}

	const char* charSet = connection->getCharSet();
	tmp.printf(", %s", charSet && *charSet ? charSet : "NONE");
	conn_data.description->append(tmp);

	const char* remProto = connection->getRemoteProtocol();
	const char* remAddr = connection->getRemoteAddress();
	if (remProto && *remProto)
	{
		tmp.printf(", %s:%s)", remProto, remAddr);
		conn_data.description->append(tmp);
	}
	else
	{
		conn_data.description->append(", <internal>)");
	}

	const char *prc_name = connection->getRemoteProcessName();
	if (prc_name && *prc_name)
	{
		tmp.printf(NEWLINE "\t%s:%d", prc_name, connection->getRemoteProcessID());
		conn_data.description->append(tmp);
	}
	conn_data.description->append(NEWLINE);

	// Adjust the list of connections
	{
		WriteLockGuard lock(connectionsLock, FB_FUNCTION);
		connections.add(conn_data);
	}
}

void TracePluginImpl::log_event_attach(ITraceDatabaseConnection* connection,
	FB_BOOLEAN create_db, ntrace_result_t att_result)
{
	if (config.log_connections)
	{
		const char* event_type;
		switch (att_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = create_db ? "CREATE_DATABASE" : "ATTACH_DATABASE";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = create_db ? "FAILED CREATE_DATABASE" : "FAILED ATTACH_DATABASE";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = create_db ? "UNAUTHORIZED CREATE_DATABASE" : "UNAUTHORIZED ATTACH_DATABASE";
				break;
			default:
				event_type = create_db ?
					"Unknown event in CREATE DATABASE ": "Unknown event in ATTACH_DATABASE";
				break;
		}

		logRecordConn(event_type, connection);
	}
}

void TracePluginImpl::log_event_detach(ITraceDatabaseConnection* connection, FB_BOOLEAN drop_db)
{
	if (config.log_connections)
	{
		logRecordConn(drop_db ? "DROP_DATABASE" : "DETACH_DATABASE", connection);
	}

	// Get rid of connection descriptor
	WriteLockGuard lock(connectionsLock, FB_FUNCTION);
	if (connections.locate(connection->getConnectionID()))
	{
		connections.current().deallocate_references();
		connections.fastRemove();
	}
}

void TracePluginImpl::register_transaction(ITraceTransaction* transaction)
{
	TransactionData trans_data;
	trans_data.id = transaction->getTransactionID();
	trans_data.description = FB_NEW_POOL(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());
	trans_data.description->printf("\t\t(TRA_%" SQUADFORMAT", ", trans_data.id);

	switch (transaction->getIsolation())
	{
	case ITraceTransaction::ISOLATION_CONSISTENCY:
		trans_data.description->append("CONSISTENCY");
		break;

	case ITraceTransaction::ISOLATION_CONCURRENCY:
		trans_data.description->append("CONCURRENCY");
		break;

	case ITraceTransaction::ISOLATION_READ_COMMITTED_RECVER:
		trans_data.description->append("READ_COMMITTED | REC_VERSION");
		break;

	case ITraceTransaction::ISOLATION_READ_COMMITTED_NORECVER:
		trans_data.description->append("READ_COMMITTED | NO_REC_VERSION");
		break;

	default:
		trans_data.description->append("<unknown>");
	}

	const int wait = transaction->getWait();
	if (wait < 0) {
		trans_data.description->append(" | WAIT");
	}
	else if (wait == 0) {
		trans_data.description->append(" | NOWAIT");
	}
	else
	{
		string s;
		s.printf(" | WAIT %d", wait);
		trans_data.description->append(s);
	}

	if (transaction->getReadOnly()) {
		trans_data.description->append(" | READ_ONLY");
	}
	else {
		trans_data.description->append(" | READ_WRITE");
	}

	trans_data.description->append(")" NEWLINE);

	// Remember transaction
	{
		WriteLockGuard lock(transactionsLock, FB_FUNCTION);
		transactions.add(trans_data);
	}
}


void TracePluginImpl::log_event_transaction_start(ITraceDatabaseConnection* connection,
		ITraceTransaction* transaction, size_t /*tpb_length*/,
		const ntrace_byte_t* /*tpb*/, ntrace_result_t tra_result)
{
	if (config.log_transactions)
	{
		const char* event_type;
		switch (tra_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = "START_TRANSACTION";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = "FAILED START_TRANSACTION";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = "UNAUTHORIZED START_TRANSACTION";
				break;
			default:
				event_type = "Unknown event in START_TRANSACTION";
				break;
		}
		logRecordTrans(event_type, connection, transaction);
	}
}

void TracePluginImpl::log_event_transaction_end(ITraceDatabaseConnection* connection,
		ITraceTransaction* transaction, FB_BOOLEAN commit,
		FB_BOOLEAN retain_context, ntrace_result_t tra_result)
{
	if (config.log_transactions)
	{
		PerformanceInfo* info = transaction->getPerf();
		if (info)
		{
			appendGlobalCounts(info);
			appendTableCounts(info);
		}

		const char* event_type;
		switch (tra_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = commit ?
					(retain_context ? "COMMIT_RETAINING"   : "COMMIT_TRANSACTION") :
					(retain_context ? "ROLLBACK_RETAINING" : "ROLLBACK_TRANSACTION");
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = commit ?
					(retain_context ? "FAILED COMMIT_RETAINING"   : "FAILED COMMIT_TRANSACTION") :
					(retain_context ? "FAILED ROLLBACK_RETAINING" : "FAILED ROLLBACK_TRANSACTION");
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = commit ?
					(retain_context ? "UNAUTHORIZED COMMIT_RETAINING"   : "UNAUTHORIZED COMMIT_TRANSACTION") :
					(retain_context ? "UNAUTHORIZED ROLLBACK_RETAINING" : "UNAUTHORIZED ROLLBACK_TRANSACTION");
				break;
			default:
				event_type = "Unknown event at transaction end";
				break;
		}
		logRecordTrans(event_type, connection, transaction);
	}

	if (!retain_context)
	{
		// Forget about the transaction
		WriteLockGuard lock(transactionsLock, FB_FUNCTION);
		if (transactions.locate(transaction->getTransactionID()))
		{
			transactions.current().deallocate_references();
			transactions.fastRemove();
		}
	}
}

void TracePluginImpl::log_event_set_context(ITraceDatabaseConnection* connection,
		ITraceTransaction* transaction, ITraceContextVariable* variable)
{
	const char* ns = variable->getNameSpace();
	const char* name = variable->getVarName();
	const char* value = variable->getVarValue();

	const size_t ns_len = strlen(ns);
	const size_t name_len = strlen(name);
	const size_t value_len = value ? strlen(value) : 0;

	if (config.log_context)
	{
		if (value == NULL) {
			record.printf("[%.*s] %.*s = NULL" NEWLINE, ns_len, ns, name_len, name);
		}
		else {
			record.printf("[%.*s] %.*s = \"%.*s\"" NEWLINE, ns_len, ns, name_len, name, value_len, value);
		}
		logRecordTrans("SET_CONTEXT", connection, transaction);
	}
}

void TracePluginImpl::log_event_proc_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceProcedure* procedure, bool started,
	ntrace_result_t proc_result)
{
	if (!config.log_procedure_start && started)
		return;

	if (!config.log_procedure_finish && !started)
		return;

	// Do not log operation if it is below time threshold
	const PerformanceInfo* info = started ? NULL : procedure->getPerf();
	if (config.time_threshold && info && info->pin_time < config.time_threshold)
		return;

	ITraceParams* params = procedure->getInputs();
	if (params && params->getCount())
	{
		appendParams(params);
		record.append(NEWLINE);
	}

	if (info)
	{
		if (info->pin_records_fetched)
		{
			string temp;
			temp.printf("%" QUADFORMAT"d records fetched" NEWLINE, info->pin_records_fetched);
			record.append(temp);
		}
		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type;
	switch (proc_result)
	{
		case ITracePlugin::RESULT_SUCCESS:
			event_type = started ? "EXECUTE_PROCEDURE_START" :
								   "EXECUTE_PROCEDURE_FINISH";
			break;
		case ITracePlugin::RESULT_FAILED:
			event_type = started ? "FAILED EXECUTE_PROCEDURE_START" :
								   "FAILED EXECUTE_PROCEDURE_FINISH";
			break;
		case ITracePlugin::RESULT_UNAUTHORIZED:
			event_type = started ? "UNAUTHORIZED EXECUTE_PROCEDURE_START" :
								   "UNAUTHORIZED EXECUTE_PROCEDURE_FINISH";
			break;
		default:
			event_type = "Unknown event at executing procedure";
			break;
	}

	logRecordProcFunc(event_type, connection, transaction, "Procedure", procedure->getProcName());
}

void TracePluginImpl::log_event_func_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceFunction* function, bool started,
	ntrace_result_t func_result)
{
	if (!config.log_function_start && started)
		return;

	if (!config.log_function_finish && !started)
		return;

	// Do not log operation if it is below time threshold
	const PerformanceInfo* info = started ? NULL : function->getPerf();
	if (config.time_threshold && info && info->pin_time < config.time_threshold)
		return;

	ITraceParams* params = function->getInputs();
	if (params && params->getCount())
	{
		appendParams(params);
		record.append(NEWLINE);
	}

	if (!started && func_result == ITracePlugin::RESULT_SUCCESS)
	{
		params = function->getResult();
		{
			record.append("returns:" NEWLINE);
			appendParams(params);
			record.append(NEWLINE);
		}
	}

	if (info)
	{
		if (info->pin_records_fetched)
		{
			string temp;
			temp.printf("%" QUADFORMAT"d records fetched" NEWLINE, info->pin_records_fetched);
			record.append(temp);
		}
		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type;
	switch (func_result)
	{
		case ITracePlugin::RESULT_SUCCESS:
			event_type = started ? "EXECUTE_FUNCTION_START" :
								   "EXECUTE_FUNCTION_FINISH";
			break;
		case ITracePlugin::RESULT_FAILED:
			event_type = started ? "FAILED EXECUTE_FUNCTION_START" :
								   "FAILED EXECUTE_FUNCTION_FINISH";
			break;
		case ITracePlugin::RESULT_UNAUTHORIZED:
			event_type = started ? "UNAUTHORIZED EXECUTE_FUNCTION_START" :
								   "UNAUTHORIZED EXECUTE_FUNCTION_FINISH";
			break;
		default:
			event_type = "Unknown event at executing function";
			break;
	}

	logRecordProcFunc(event_type, connection, transaction, "Function", function->getFuncName());
}

void TracePluginImpl::register_sql_statement(ITraceSQLStatement* statement)
{
	StatementData stmt_data;
	stmt_data.id = statement->getStmtID();

	bool need_statement = true;

	const char* sql = statement->getText();
	if (!sql)
		return;

	size_t sql_length = strlen(sql);
	if (!sql_length)
		return;

	if (config.include_filter.hasData())
	{
		include_matcher->reset();
		include_matcher->process((const UCHAR*) sql, sql_length);
		need_statement = include_matcher->result();
	}

	if (need_statement && config.exclude_filter.hasData())
	{
		exclude_matcher->reset();
		exclude_matcher->process((const UCHAR*) sql, sql_length);
		need_statement = !exclude_matcher->result();
	}

	if (need_statement)
	{
		stmt_data.description = FB_NEW_POOL(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());

		if (stmt_data.id) {
			stmt_data.description->printf(NEWLINE "Statement %d:", stmt_data.id);
		}

		string temp(*getDefaultMemoryPool());
		if (config.max_sql_length && sql_length > config.max_sql_length)
		{
			// Truncate too long SQL printing it out with ellipsis
			sql_length = (config.max_sql_length < 3) ? 0 : (config.max_sql_length - 3);
			temp.printf(NEWLINE
				"-------------------------------------------------------------------------------" NEWLINE
				"%.*s...", sql_length, sql);
		}
		else
		{
			temp.printf(NEWLINE
				"-------------------------------------------------------------------------------" NEWLINE
				"%.*s", sql_length, sql);
		}
		*stmt_data.description += temp;

		const char* access_path = config.print_plan ?
			(config.explain_plan ? statement->getExplainedPlan() : statement->getPlan())
			: NULL;

		if (access_path && *access_path)
		{
			const size_t access_path_length = strlen(access_path);
			temp.printf(NEWLINE
				"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
				"%.*s" NEWLINE, access_path_length, access_path);

			*stmt_data.description += temp;
		}
		else
		{
			*stmt_data.description += NEWLINE;
		}
	}
	else
	{
		stmt_data.description = NULL;
	}

	// Remember statement
	{
		WriteLockGuard lock(statementsLock, FB_FUNCTION);
		statements.add(stmt_data);
	}
}

void TracePluginImpl::log_event_dsql_prepare(ITraceDatabaseConnection* connection,
		ITraceTransaction* transaction, ITraceSQLStatement* statement,
		ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	if (config.log_statement_prepare)
	{
		const char* event_type;
		switch (req_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = "PREPARE_STATEMENT";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = "FAILED PREPARE_STATEMENT";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = "UNAUTHORIZED PREPARE_STATEMENT";
				break;
			default:
				event_type = "Unknown event in PREPARE_STATEMENT";
				break;
		}
		record.printf("%7d ms" NEWLINE, time_millis);
		logRecordStmt(event_type, connection, transaction, statement, true);
	}
}

void TracePluginImpl::log_event_dsql_free(ITraceDatabaseConnection* connection,
		ITraceSQLStatement* statement, unsigned short option)
{
	if (config.log_statement_free)
	{
		logRecordStmt(option == DSQL_drop ? "FREE_STATEMENT" : "CLOSE_CURSOR",
			connection, 0, statement, true);
	}

	if (option == DSQL_drop)
	{
		WriteLockGuard lock(statementsLock, FB_FUNCTION);
		if (statements.locate(statement->getStmtID()))
		{
			delete statements.current().description;
			statements.fastRemove();
		}
	}
}

void TracePluginImpl::log_event_dsql_execute(ITraceDatabaseConnection* connection,
		ITraceTransaction* transaction, ITraceSQLStatement* statement,
		bool started, ntrace_result_t req_result)
{
	if (started && !config.log_statement_start)
		return;

	if (!started && !config.log_statement_finish)
		return;

	// Do not log operation if it is below time threshold
	const PerformanceInfo* info = started ? NULL : statement->getPerf();
	if (config.time_threshold && info && info->pin_time < config.time_threshold)
		return;

	ITraceParams *params = statement->getInputs();
	if (params && params->getCount())
	{
		record.append(NEWLINE);
		appendParams(params);
		record.append(NEWLINE);
	}

	if (info)
	{
		string temp;
		temp.printf("%" QUADFORMAT"d records fetched" NEWLINE, info->pin_records_fetched);
		record.append(temp);

		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type;
	switch (req_result)
	{
		case ITracePlugin::RESULT_SUCCESS:
			event_type = started ? "EXECUTE_STATEMENT_START" :
								   "EXECUTE_STATEMENT_FINISH";
			break;
		case ITracePlugin::RESULT_FAILED:
			event_type = started ? "FAILED EXECUTE_STATEMENT_START" :
								   "FAILED EXECUTE_STATEMENT_FINISH";
			break;
		case ITracePlugin::RESULT_UNAUTHORIZED:
			event_type = started ? "UNAUTHORIZED EXECUTE_STATEMENT_START" :
								   "UNAUTHORIZED EXECUTE_STATEMENT_FINISH";
			break;
		default:
			event_type = "Unknown event at executing statement";
			break;
	}
	logRecordStmt(event_type, connection, transaction, statement, true);
}


void TracePluginImpl::register_blr_statement(ITraceBLRStatement* statement)
{
	string* description = FB_NEW_POOL(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());

	if (statement->getStmtID()) {
		description->printf(NEWLINE "Statement %" SQUADFORMAT":" NEWLINE, statement->getStmtID());
	}

	if (config.print_blr)
	{
		const char *text_blr = statement->getText();
		size_t text_blr_length = text_blr ? strlen(text_blr) : 0;
		if (!text_blr)
			text_blr = "";

		if (config.max_blr_length && text_blr_length > config.max_blr_length)
		{
			// Truncate too long BLR printing it out with ellipsis
			text_blr_length = config.max_blr_length < 3 ? 0 : config.max_blr_length - 3;
			description->printf(
				"-------------------------------------------------------------------------------" NEWLINE
				"%.*s..." NEWLINE,
				text_blr_length, text_blr);
		}
		else
		{
			description->printf(
				"-------------------------------------------------------------------------------" NEWLINE
				"%.*s" NEWLINE,
				text_blr_length, text_blr);
		}
	}

	StatementData stmt_data;
	stmt_data.id = statement->getStmtID();
	stmt_data.description = description;
	WriteLockGuard lock(statementsLock, FB_FUNCTION);

	statements.add(stmt_data);
}

void TracePluginImpl::log_event_blr_compile(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceBLRStatement* statement,
	ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	if (config.log_blr_requests)
	{
		{
			ReadLockGuard lock(statementsLock, FB_FUNCTION);
			StatementsTree::Accessor accessor(&statements);
			if (accessor.locate(statement->getStmtID()))
				return;
		}

		const char* event_type;
		switch (req_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = "COMPILE_BLR";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = "FAILED COMPILE_BLR";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = "UNAUTHORIZED COMPILE_BLR";
				break;
			default:
				event_type = "Unknown event in COMPILE_BLR";
				break;
		}

		record.printf("%7d ms", time_millis);

		logRecordStmt(event_type, connection, transaction, statement, false);
	}
}

void TracePluginImpl::log_event_blr_execute(ITraceDatabaseConnection* connection,
		ITraceTransaction* transaction, ITraceBLRStatement* statement,
		ntrace_result_t req_result)
{
	PerformanceInfo *info = statement->getPerf();

	// Do not log operation if it is below time threshold
	if (config.time_threshold && info->pin_time < config.time_threshold)
		return;

	if (config.log_blr_requests)
	{
		appendGlobalCounts(info);
		appendTableCounts(info);

		const char* event_type;
		switch (req_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = "EXECUTE_BLR";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = "FAILED EXECUTE_BLR";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = "UNAUTHORIZED EXECUTE_BLR";
				break;
			default:
				event_type = "Unknown event in EXECUTE_BLR";
				break;
		}

		logRecordStmt(event_type, connection, transaction, statement, false);
	}
}

void TracePluginImpl::log_event_dyn_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceDYNRequest* request,
	ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	if (config.log_dyn_requests)
	{
		string description;

		if (config.print_dyn)
		{
			const char *text_dyn = request->getText();
			size_t text_dyn_length = text_dyn ? strlen(text_dyn) : 0;
			if (!text_dyn) {
				text_dyn = "";
			}

			if (config.max_dyn_length && text_dyn_length > config.max_dyn_length)
			{
				// Truncate too long DDL printing it out with ellipsis
				text_dyn_length = config.max_dyn_length < 3 ? 0 : config.max_dyn_length - 3;
				description.printf(
					"-------------------------------------------------------------------------------" NEWLINE
					"%.*s...",
					text_dyn_length, text_dyn);
			}
			else
			{
				description.printf(
					"-------------------------------------------------------------------------------" NEWLINE
					"%.*s",
					text_dyn_length, text_dyn);
			}
		}

		const char* event_type;
		switch (req_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = "EXECUTE_DYN";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = "FAILED EXECUTE_DYN";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = "UNAUTHORIZED EXECUTE_DYN";
				break;
			default:
				event_type = "Unknown event in EXECUTE_DYN";
				break;
		}

		record.printf("%7d ms", time_millis);
		record.insert(0, description);

		logRecordTrans(event_type, connection, transaction);
	}
}


void TracePluginImpl::register_service(ITraceServiceConnection* service)
{
	string username(service->getUserName());
	string remote_address;
	string remote_process;

	const char* tmp = service->getRemoteAddress();
	if (tmp && *tmp) {
		remote_address.printf("%s:%s", service->getRemoteProtocol(), service->getRemoteAddress());
	}
	else
	{
		tmp = service->getRemoteProtocol();
		if (tmp && *tmp)
			remote_address = tmp;
		else
			remote_address = "internal";
	}

	if (username.isEmpty())
		username = "<user is unknown>";

	tmp = service->getRemoteProcessName();
	if (tmp && *tmp) {
		remote_process.printf(", %s:%d", tmp, service->getRemoteProcessID());
	}

	ServiceData serv_data;
	serv_data.id = service->getServiceID();
	serv_data.description = FB_NEW_POOL(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());
	serv_data.description->printf("\t%s, (Service %p, %s, %s%s)" NEWLINE,
		service->getServiceMgr(), serv_data.id,
		username.c_str(), remote_address.c_str(), remote_process.c_str());
	serv_data.enabled = true;

	// Adjust the list of services
	{
		WriteLockGuard lock(servicesLock, FB_FUNCTION);
		services.add(serv_data);
	}
}


bool TracePluginImpl::checkServiceFilter(ITraceServiceConnection* service, bool started)
{
	ReadLockGuard lock(servicesLock, FB_FUNCTION);

	ServiceData* data = NULL;
	ServicesTree::Accessor accessor(&services);
	if (accessor.locate(service->getServiceID()))
		data = &accessor.current();

	if (data && !started)
		return data->enabled;

	const char* svcName = service->getServiceName();
	const int svcNameLen = static_cast<int>(strlen(svcName));
	bool enabled = true;

	if (config.include_filter.hasData())
	{
		include_matcher->reset();
		include_matcher->process((const UCHAR*) svcName, svcNameLen);
		enabled = include_matcher->result();
	}

	if (enabled && config.exclude_filter.hasData())
	{
		exclude_matcher->reset();
		exclude_matcher->process((const UCHAR*) svcName, svcNameLen);
		enabled = !exclude_matcher->result();
	}

	if (data) {
		data->enabled = enabled;
	}

	return enabled;
}


void TracePluginImpl::log_event_service_attach(ITraceServiceConnection* service,
	ntrace_result_t att_result)
{
	if (config.log_services)
	{
		const char* event_type;
		switch (att_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = "ATTACH_SERVICE";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = "FAILED ATTACH_SERVICE";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = "UNAUTHORIZED ATTACH_SERVICE";
				break;
			default:
				event_type = "Unknown evnt in ATTACH_SERVICE";
				break;
		}

		logRecordServ(event_type, service);
	}
}

void TracePluginImpl::log_event_service_start(ITraceServiceConnection* service,
	size_t switches_length, const char* switches, ntrace_result_t start_result)
{
	if (config.log_services)
	{
		if (!checkServiceFilter(service, true))
			return;

		const char* event_type;
		switch (start_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = "START_SERVICE";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = "FAILED START_SERVICE";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = "UNAUTHORIZED START_SERVICE";
				break;
			default:
				event_type = "Unknown event in START_SERVICE";
				break;
		}

		const char* tmp = service->getServiceName();
		if (tmp && *tmp) {
			record.printf("\t\"%s\"" NEWLINE, tmp);
		}

		if (switches_length)
		{
			string sw;
			sw.printf("\t%.*s" NEWLINE, switches_length, switches);

			// Delete terminator symbols from service switches
			for (FB_SIZE_T i = 0; i < sw.length(); ++i)
			{
				if (sw[i] == Firebird::SVC_TRMNTR)
				{
					sw.erase(i, 1);
					if ((i < sw.length()) && (sw[i] != Firebird::SVC_TRMNTR))
						--i;
				}
			}
			record.append(sw);
		}

		logRecordServ(event_type, service);
	}
}

void TracePluginImpl::log_event_service_query(ITraceServiceConnection* service,
	size_t send_item_length, const ntrace_byte_t* send_items,
	size_t recv_item_length, const ntrace_byte_t* recv_items,
	ntrace_result_t query_result)
{
	if (config.log_services && config.log_service_query)
	{
		if (!checkServiceFilter(service, false))
			return;

		const char* tmp = service->getServiceName();
		if (tmp && *tmp) {
			record.printf("\t\"%s\"" NEWLINE, tmp);
		}
		appendServiceQueryParams(send_item_length, send_items, recv_item_length, recv_items);
		record.append(NEWLINE);

		const char* event_type;
		switch (query_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = "QUERY_SERVICE";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = "FAILED QUERY_SERVICE";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = "UNAUTHORIZED QUERY_SERVICE";
				break;
			default:
				event_type = "Unknown event in QUERY_SERVICE";
				break;
		}

		logRecordServ(event_type, service);
	}
}

void TracePluginImpl::log_event_service_detach(ITraceServiceConnection* service,
	ntrace_result_t detach_result)
{
	if (config.log_services)
	{
		const char* event_type;
		switch (detach_result)
		{
			case ITracePlugin::RESULT_SUCCESS:
				event_type = "DETACH_SERVICE";
				break;
			case ITracePlugin::RESULT_FAILED:
				event_type = "FAILED DETACH_SERVICE";
				break;
			case ITracePlugin::RESULT_UNAUTHORIZED:
				event_type = "UNAUTHORIZED DETACH_SERVICE";
				break;
			default:
				event_type = "Unknown event in DETACH_SERVICE";
				break;
		}
		logRecordServ(event_type, service);
	}

	// Get rid of connection descriptor
	{
		WriteLockGuard lock(servicesLock, FB_FUNCTION);
		if (services.locate(service->getServiceID()))
		{
			services.current().deallocate_references();
			services.fastRemove();
		}
	}
}

void TracePluginImpl::log_event_trigger_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceTrigger* trigger, bool started, ntrace_result_t trig_result)
{
	if (!config.log_trigger_start && started)
		return;

	if (!config.log_trigger_finish && !started)
		return;

	// Do not log operation if it is below time threshold
	const PerformanceInfo* info = started ? NULL : trigger->getPerf();
	if (config.time_threshold && info && info->pin_time < config.time_threshold)
		return;

	string trgname(trigger->getTriggerName());

	if (trgname.empty())
		trgname = "<unknown>";

	if ((trigger->getWhich() != ITraceTrigger::TYPE_ALL) && trigger->getRelationName())
	{
		string relation;
		relation.printf(" FOR %s", trigger->getRelationName());
		trgname.append(relation);
	}

	string action;
	switch (trigger->getWhich())
	{
		case ITraceTrigger::TYPE_ALL:
			action = "ON ";	//// TODO: Why ALL means ON (DATABASE) triggers?
			break;
		case ITraceTrigger::TYPE_BEFORE:
			action = "BEFORE ";
			break;
		case ITraceTrigger::TYPE_AFTER:
			action = "AFTER ";
			break;
		default:
			action = "<unknown> ";
			break;
	}

	switch (trigger->getAction())
	{
		case TRIGGER_INSERT:
			action.append("INSERT");
			break;
		case TRIGGER_UPDATE:
			action.append("UPDATE");
			break;
		case TRIGGER_DELETE:
			action.append("DELETE");
			break;
		case TRIGGER_CONNECT:
			action.append("CONNECT");
			break;
		case TRIGGER_DISCONNECT:
			action.append("DISCONNECT");
			break;
		case TRIGGER_TRANS_START:
			action.append("TRANSACTION_START");
			break;
		case TRIGGER_TRANS_COMMIT:
			action.append("TRANSACTION_COMMIT");
			break;
		case TRIGGER_TRANS_ROLLBACK:
			action.append("TRANSACTION_ROLLBACK");
			break;
		case TRIGGER_DDL:
			action.append("DDL");
			break;
		default:
			action.append("Unknown trigger action");
			break;
	}

	record.printf("\t%s (%s) " NEWLINE, trgname.c_str(), action.c_str());

	if (info)
	{
		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type;
	switch (trig_result)
	{
		case ITracePlugin::RESULT_SUCCESS:
			event_type = started ? "EXECUTE_TRIGGER_START" :
								   "EXECUTE_TRIGGER_FINISH";
			break;
		case ITracePlugin::RESULT_FAILED:
			event_type = started ? "FAILED EXECUTE_TRIGGER_START" :
								   "FAILED EXECUTE_TRIGGER_FINISH";
			break;
		case ITracePlugin::RESULT_UNAUTHORIZED:
			event_type = started ? "UNAUTHORIZED EXECUTE_TRIGGER_START" :
								   "UNAUTHORIZED EXECUTE_TRIGGER_FINISH";
			break;
		default:
			event_type = "Unknown event at executing trigger";
			break;
	}

	logRecordTrans(event_type, connection, transaction);
}

void TracePluginImpl::log_event_error(ITraceConnection* connection, ITraceStatusVector* status,
	const char* function)
{
	string event_type;
	if (config.log_errors && status->hasError())
	{
		const ISC_STATUS* errs = status->getStatus()->getErrors();

		if (!include_codes.isEmpty() && !filterStatus(errs, include_codes))
			return;

		if (!exclude_codes.isEmpty() && filterStatus(errs, exclude_codes))
			return;

		event_type.printf("ERROR AT %s", function);
	}
	else if (config.log_warnings && status->hasWarning())
	{
		const ISC_STATUS* warns = status->getStatus()->getWarnings();

		if (!include_codes.isEmpty() && !filterStatus(warns, include_codes))
			return;

		if (!exclude_codes.isEmpty() && filterStatus(warns, exclude_codes))
			return;

		event_type.printf("WARNING AT %s", function);
	}
	else
		return;

	logRecordError(event_type.c_str(), connection, status);
}

void TracePluginImpl::log_event_sweep(ITraceDatabaseConnection* connection, ITraceSweepInfo* sweep,
	ntrace_process_state_t sweep_state)
{
	if (!config.log_sweep)
		return;

	if (sweep_state == SWEEP_STATE_STARTED ||
		sweep_state == SWEEP_STATE_FINISHED)
	{
		record.printf("\nTransaction counters:\n"
			"\tOldest interesting %10" SQUADFORMAT"\n"
			"\tOldest active      %10" SQUADFORMAT"\n"
			"\tOldest snapshot    %10" SQUADFORMAT"\n"
			"\tNext transaction   %10" SQUADFORMAT"\n",
			sweep->getOIT(),
			sweep->getOAT(),
			sweep->getOST(),
			sweep->getNext()
			);
	}

	PerformanceInfo* info = sweep->getPerf();
	if (info)
	{
		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type = NULL;
	switch (sweep_state)
	{
	case SWEEP_STATE_STARTED:
		event_type = "SWEEP_START";
		break;

	case SWEEP_STATE_FINISHED:
		event_type = "SWEEP_FINISH";
		break;

	case SWEEP_STATE_FAILED:
		event_type = "SWEEP_FAILED";
		break;

	case SWEEP_STATE_PROGRESS:
		event_type = "SWEEP_PROGRESS";
		break;

	default:
		fb_assert(false);
		event_type = "Unknown SWEEP process state";
		break;
	}

	logRecordConn(event_type, connection);
}

//***************************** PLUGIN INTERFACE ********************************

int TracePluginImpl::release()
{
	if (--refCounter == 0)
	{
		delete this;
		return 0;
	}
	return 1;
}

const char* TracePluginImpl::trace_get_error()
{
	return get_error_string();
}

// Create/close attachment
FB_BOOLEAN TracePluginImpl::trace_attach(ITraceDatabaseConnection* connection,
	FB_BOOLEAN create_db, ntrace_result_t att_result)
{
	try
	{
		log_event_attach(connection, create_db, att_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_detach(ITraceDatabaseConnection* connection, FB_BOOLEAN drop_db)
{
	try
	{
		log_event_detach(connection, drop_db);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Start/end transaction
FB_BOOLEAN TracePluginImpl::trace_transaction_start(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, unsigned tpb_length, const ntrace_byte_t* tpb,
	ntrace_result_t tra_result)
{
	try
	{
		log_event_transaction_start(connection, transaction, tpb_length, tpb, tra_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_transaction_end(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, FB_BOOLEAN commit, FB_BOOLEAN retain_context,
	ntrace_result_t tra_result)
{
	try
	{
		log_event_transaction_end(connection, transaction, commit, retain_context, tra_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Assignment to context variables
FB_BOOLEAN TracePluginImpl::trace_set_context(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceContextVariable* variable)
{
	try
	{
		log_event_set_context(connection, transaction, variable);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Stored procedure executing
FB_BOOLEAN TracePluginImpl::trace_proc_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceProcedure* procedure,
	FB_BOOLEAN started, ntrace_result_t proc_result)
{
	try
	{
		log_event_proc_execute(connection, transaction, procedure, started, proc_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Stored function executing
FB_BOOLEAN TracePluginImpl::trace_func_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceFunction* function,
	FB_BOOLEAN started, ntrace_result_t func_result)
{
	try
	{
		log_event_func_execute(connection, transaction, function, started, func_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_trigger_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceTrigger* trigger,
	FB_BOOLEAN started, ntrace_result_t trig_result)
{
	try
	{
		log_event_trigger_execute(connection, transaction, trigger, started, trig_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}


// DSQL statement lifecycle
FB_BOOLEAN TracePluginImpl::trace_dsql_prepare(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceSQLStatement* statement, ISC_INT64 time_millis,
	ntrace_result_t req_result)
{
	try
	{
		log_event_dsql_prepare(connection, transaction, statement, time_millis, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_dsql_free(ITraceDatabaseConnection* connection,
	ITraceSQLStatement* statement, unsigned option)
{
	try
	{
		log_event_dsql_free(connection, statement, option);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_dsql_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceSQLStatement* statement,
	FB_BOOLEAN started, ntrace_result_t req_result)
{
	try
	{
		log_event_dsql_execute(connection, transaction, statement, started, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}


// BLR requests
FB_BOOLEAN TracePluginImpl::trace_blr_compile(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceBLRStatement* statement, ISC_INT64 time_millis,
	ntrace_result_t req_result)
{
	try
	{
		log_event_blr_compile(connection, transaction, statement, time_millis, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_blr_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceBLRStatement* statement, ntrace_result_t req_result)
{
	try
	{
		log_event_blr_execute(connection, transaction, statement, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// DYN requests
FB_BOOLEAN TracePluginImpl::trace_dyn_execute(ITraceDatabaseConnection* connection,
	ITraceTransaction* transaction, ITraceDYNRequest* request, ISC_INT64 time_millis,
	ntrace_result_t req_result)
{
	try
	{
		log_event_dyn_execute(connection, transaction, request, time_millis, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Using the services
FB_BOOLEAN TracePluginImpl::trace_service_attach(ITraceServiceConnection* service,
	ntrace_result_t att_result)
{
	try
	{
		log_event_service_attach(service, att_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_service_start(ITraceServiceConnection* service,
	unsigned switches_length, const char* switches, ntrace_result_t start_result)
{
	try
	{
		log_event_service_start(service, switches_length, switches, start_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_service_query(ITraceServiceConnection* service,
	unsigned send_item_length, const ntrace_byte_t* send_items, unsigned recv_item_length,
	const ntrace_byte_t* recv_items, ntrace_result_t query_result)
{
	try
	{
		log_event_service_query(service, send_item_length, send_items,
								recv_item_length, recv_items, query_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}

}

FB_BOOLEAN TracePluginImpl::trace_service_detach(ITraceServiceConnection* service,
	ntrace_result_t detach_result)
{
	try
	{
		log_event_service_detach(service, detach_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_event_error(ITraceConnection* connection,
	ITraceStatusVector* status, const char* function)
{
	try
	{
		log_event_error(connection, status, function);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

FB_BOOLEAN TracePluginImpl::trace_event_sweep(ITraceDatabaseConnection* connection,
	ITraceSweepInfo* sweep, ntrace_process_state_t sweep_state)
{
	try
	{
		log_event_sweep(connection, sweep, sweep_state);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}
