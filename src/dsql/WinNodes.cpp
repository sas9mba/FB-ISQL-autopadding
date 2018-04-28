/*
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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2010 Adriano dos Santos Fernandes <adrianosf@gmail.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../dsql/WinNodes.h"
#include "../dsql/make_proto.h"
#include "../dsql/pass1_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/par_proto.h"
#include "../jrd/recsrc/RecordSource.h"

using namespace Firebird;
using namespace Jrd;

namespace Jrd {


WinFuncNode::WinFuncNode(MemoryPool& pool, const AggInfo& aAggInfo, ValueExprNode* aArg)
	: AggNode(pool, aAggInfo, false, false, aArg)
{
}


//--------------------


static WinFuncNode::RegisterFactory0<DenseRankWinNode> denseRankWinInfo("DENSE_RANK");

DenseRankWinNode::DenseRankWinNode(MemoryPool& pool)
	: WinFuncNode(pool, denseRankWinInfo)
{
}

string DenseRankWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);
	return "DenseRankWinNode";
}

void DenseRankWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	if (dsqlScratch->clientDialect == 1)
		desc->makeDouble();
	else
		desc->makeInt64(0);
}

void DenseRankWinNode::getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* desc)
{
	desc->makeInt64(0);
}

ValueExprNode* DenseRankWinNode::copy(thread_db* tdbb, NodeCopier& /*copier*/) const
{
	return FB_NEW_POOL(*tdbb->getDefaultPool()) DenseRankWinNode(*tdbb->getDefaultPool());
}

void DenseRankWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(0, 0);
}

void DenseRankWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* /*request*/, dsc* /*desc*/) const
{
}

dsc* DenseRankWinNode::aggExecute(thread_db* /*tdbb*/, jrd_req* request) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	++impure->vlu_misc.vlu_int64;
	return &impure->vlu_desc;
}

AggNode* DenseRankWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) DenseRankWinNode(dsqlScratch->getPool());
}


//--------------------


static WinFuncNode::RegisterFactory0<RankWinNode> rankWinInfo("RANK");

RankWinNode::RankWinNode(MemoryPool& pool)
	: WinFuncNode(pool, rankWinInfo),
	  tempImpure(0)
{
}

string RankWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);

	NODE_PRINT(printer, tempImpure);

	return "RankWinNode";
}

void RankWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	if (dsqlScratch->clientDialect == 1)
		desc->makeDouble();
	else
		desc->makeInt64(0);
}

void RankWinNode::getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* desc)
{
	desc->makeInt64(0);
}

ValueExprNode* RankWinNode::copy(thread_db* tdbb, NodeCopier& /*copier*/) const
{
	return FB_NEW_POOL(*tdbb->getDefaultPool()) RankWinNode(*tdbb->getDefaultPool());
}

AggNode* RankWinNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	AggNode::pass2(tdbb, csb);
	tempImpure = CMP_impure(csb, sizeof(impure_value_ex));
	return this;
}

void RankWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(1, 0);
	impure->vlux_count = 0;
}

void RankWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* request, dsc* /*desc*/) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	++impure->vlux_count;
}

dsc* RankWinNode::aggExecute(thread_db* tdbb, jrd_req* request) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);

	dsc temp;
	temp.makeInt64(0, &impure->vlu_misc.vlu_int64);

	impure_value_ex* impureTemp = request->getImpure<impure_value_ex>(tempImpure);
	EVL_make_value(tdbb, &temp, impureTemp);

	impure->vlu_misc.vlu_int64 += impure->vlux_count;
	impure->vlux_count = 0;

	return &impureTemp->vlu_desc;
}

AggNode* RankWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) RankWinNode(dsqlScratch->getPool());
}


//--------------------


static WinFuncNode::RegisterFactory0<PercentRankWinNode> percentRankWinInfo("PERCENT_RANK");

PercentRankWinNode::PercentRankWinNode(MemoryPool& pool)
	: WinFuncNode(pool, percentRankWinInfo),
	  tempImpure(0)
{
}

string PercentRankWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);
	return "PercentRankWinNode";
}

void PercentRankWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	desc->makeDouble();
}

void PercentRankWinNode::getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* desc)
{
	desc->makeDouble();
}

ValueExprNode* PercentRankWinNode::copy(thread_db* tdbb, NodeCopier& /*copier*/) const
{
	return FB_NEW_POOL(*tdbb->getDefaultPool()) PercentRankWinNode(*tdbb->getDefaultPool());
}

AggNode* PercentRankWinNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	AggNode::pass2(tdbb, csb);
	tempImpure = CMP_impure(csb, sizeof(impure_value_ex));
	return this;
}

void PercentRankWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(1, 0);
	impure->vlux_count = 0;

	impure_value_ex* impureTemp = request->getImpure<impure_value_ex>(tempImpure);
	impureTemp->make_double(0);
	impureTemp->vlux_count = 0;
}

void PercentRankWinNode::aggPass(thread_db* /*tdbb*/, jrd_req* request, dsc* /*desc*/) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	++impure->vlux_count;
}

dsc* PercentRankWinNode::aggExecute(thread_db* /*tdbb*/, jrd_req* request) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure_value_ex* impureTemp = request->getImpure<impure_value_ex>(tempImpure);

	impureTemp->vlux_count = impure->vlu_misc.vlu_int64;

	impure->vlu_misc.vlu_int64 += impure->vlux_count;
	impure->vlux_count = 0;

	return NULL;
}

dsc* PercentRankWinNode::winPass(thread_db* /*tdbb*/, jrd_req* request, SlidingWindow* window) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure_value_ex* impureTemp = request->getImpure<impure_value_ex>(tempImpure);

	double partitionSize = window->getPartitionSize();

	impureTemp->vlu_misc.vlu_double = 1 / (partitionSize - 1) * (impureTemp->vlux_count - 1);
	return &impureTemp->vlu_desc;
}

AggNode* PercentRankWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) PercentRankWinNode(dsqlScratch->getPool());
}


//--------------------


static WinFuncNode::RegisterFactory0<CumeDistWinNode> cumeDistWinInfo("CUME_DIST");

CumeDistWinNode::CumeDistWinNode(MemoryPool& pool)
	: WinFuncNode(pool, cumeDistWinInfo)
{
}

string CumeDistWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);
	return "CumeDistWinNode";
}

void CumeDistWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	desc->makeDouble();
}

void CumeDistWinNode::getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* desc)
{
	desc->makeDouble();
}

ValueExprNode* CumeDistWinNode::copy(thread_db* tdbb, NodeCopier& /*copier*/) const
{
	return FB_NEW_POOL(*tdbb->getDefaultPool()) CumeDistWinNode(*tdbb->getDefaultPool());
}

AggNode* CumeDistWinNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	AggNode::pass2(tdbb, csb);
	return this;
}

void CumeDistWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_double(0);
}

dsc* CumeDistWinNode::winPass(thread_db* /*tdbb*/, jrd_req* request, SlidingWindow* window) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);

	double frameSize = window->getFrameSize();
	double partitionSize = window->getPartitionSize();

	impure->vlu_misc.vlu_double = frameSize / partitionSize;
	return &impure->vlu_desc;
}

AggNode* CumeDistWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) CumeDistWinNode(dsqlScratch->getPool());
}


//--------------------


static WinFuncNode::RegisterFactory0<RowNumberWinNode> rowNumberWinInfo("ROW_NUMBER");

RowNumberWinNode::RowNumberWinNode(MemoryPool& pool)
	: WinFuncNode(pool, rowNumberWinInfo)
{
}

string RowNumberWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);
	return "RowNumberWinNode";
}

void RowNumberWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	if (dsqlScratch->clientDialect == 1)
		desc->makeDouble();
	else
		desc->makeInt64(0);
}

void RowNumberWinNode::getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* desc)
{
	desc->makeInt64(0);
}

ValueExprNode* RowNumberWinNode::copy(thread_db* tdbb, NodeCopier& /*copier*/) const
{
	return FB_NEW_POOL(*tdbb->getDefaultPool()) RowNumberWinNode(*tdbb->getDefaultPool());
}

void RowNumberWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(0, 0);
}

dsc* RowNumberWinNode::winPass(thread_db* /*tdbb*/, jrd_req* request, SlidingWindow* window) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->vlu_misc.vlu_int64 = window->getRecordPosition() - window->getPartitionStart() + 1;
	return &impure->vlu_desc;
}

AggNode* RowNumberWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) RowNumberWinNode(dsqlScratch->getPool());
}


//--------------------


static WinFuncNode::RegisterFactory0<FirstValueWinNode> firstValueWinInfo("FIRST_VALUE");

FirstValueWinNode::FirstValueWinNode(MemoryPool& pool, ValueExprNode* aArg)
	: WinFuncNode(pool, firstValueWinInfo, aArg)
{
}

void FirstValueWinNode::parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned /*count*/)
{
	arg = PAR_parse_value(tdbb, csb);
}

string FirstValueWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);
	return "FirstValueWinNode";
}

void FirstValueWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	MAKE_desc(dsqlScratch, desc, arg);
	desc->setNullable(true);
}

void FirstValueWinNode::getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc)
{
	arg->getDesc(tdbb, csb, desc);
}

ValueExprNode* FirstValueWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	FirstValueWinNode* node = FB_NEW_POOL(*tdbb->getDefaultPool()) FirstValueWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	return node;
}

void FirstValueWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);
}

dsc* FirstValueWinNode::winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);

	if (!window->moveWithinFrame(-(window->getRecordPosition() - window->getFrameStart())))
		return NULL;

	dsc* desc = EVL_expr(tdbb, request, arg);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	return desc;
}

AggNode* FirstValueWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) FirstValueWinNode(dsqlScratch->getPool(), doDsqlPass(dsqlScratch, arg));
}


//--------------------


static WinFuncNode::RegisterFactory0<LastValueWinNode> lastValueWinInfo("LAST_VALUE");

LastValueWinNode::LastValueWinNode(MemoryPool& pool, ValueExprNode* aArg)
	: WinFuncNode(pool, lastValueWinInfo, aArg)
{
}

void LastValueWinNode::parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned /*count*/)
{
	arg = PAR_parse_value(tdbb, csb);
}

string LastValueWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);
	return "LastValueWinNode";
}

void LastValueWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	MAKE_desc(dsqlScratch, desc, arg);
	desc->setNullable(true);
}

void LastValueWinNode::getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc)
{
	arg->getDesc(tdbb, csb, desc);
}

ValueExprNode* LastValueWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	LastValueWinNode* node = FB_NEW_POOL(*tdbb->getDefaultPool()) LastValueWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	return node;
}

void LastValueWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);
}

dsc* LastValueWinNode::winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);

	if (!window->moveWithinFrame(window->getFrameEnd() - window->getRecordPosition()))
		return NULL;

	dsc* desc = EVL_expr(tdbb, request, arg);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	return desc;
}

AggNode* LastValueWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) LastValueWinNode(dsqlScratch->getPool(), doDsqlPass(dsqlScratch, arg));
}


//--------------------


static WinFuncNode::RegisterFactory0<NthValueWinNode> nthValueWinInfo("NTH_VALUE");

NthValueWinNode::NthValueWinNode(MemoryPool& pool, ValueExprNode* aArg, ValueExprNode* aRow,
			ValueExprNode* aFrom)
	: WinFuncNode(pool, nthValueWinInfo, aArg),
	  row(aRow),
	  from(aFrom)
{
}

void NthValueWinNode::parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned /*count*/)
{
	arg = PAR_parse_value(tdbb, csb);
	row = PAR_parse_value(tdbb, csb);
	from = PAR_parse_value(tdbb, csb);
}

string NthValueWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);

	NODE_PRINT(printer, row);
	NODE_PRINT(printer, from);

	return "NthValueWinNode";
}

void NthValueWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	MAKE_desc(dsqlScratch, desc, arg);
	desc->setNullable(true);
}

void NthValueWinNode::getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc)
{
	arg->getDesc(tdbb, csb, desc);
}

ValueExprNode* NthValueWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	NthValueWinNode* node = FB_NEW_POOL(*tdbb->getDefaultPool()) NthValueWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	node->row = copier.copy(tdbb, row);
	node->from = copier.copy(tdbb, from);
	return node;
}

void NthValueWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	impure->make_int64(0, 0);
}

dsc* NthValueWinNode::winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);

	dsc* desc = EVL_expr(tdbb, request, row);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	SINT64 records = MOV_get_int64(tdbb, desc, 0);
	if (records <= 0)
	{
		status_exception::raise(Arg::Gds(isc_sysf_argnmustbe_positive) <<
			Arg::Num(2) << Arg::Str(aggInfo.name));
	}

	desc = EVL_expr(tdbb, request, from);
	const SLONG fromPos = desc ? MOV_get_long(tdbb, desc, 0) : FROM_FIRST;

	if (fromPos == FROM_FIRST)
		records += -(window->getRecordPosition() - window->getFrameStart()) - 1;
	else
		records = window->getFrameEnd() - window->getRecordPosition() - records + 1;

	if (!window->moveWithinFrame(records))
		return NULL;

	desc = EVL_expr(tdbb, request, arg);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	return desc;
}

AggNode* NthValueWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) NthValueWinNode(dsqlScratch->getPool(),
		doDsqlPass(dsqlScratch, arg), doDsqlPass(dsqlScratch, row), doDsqlPass(dsqlScratch, from));
}


//--------------------


// A direction of -1 is LAG, and 1 is LEAD.
LagLeadWinNode::LagLeadWinNode(MemoryPool& pool, const AggInfo& aAggInfo, int aDirection,
			ValueExprNode* aArg, ValueExprNode* aRows, ValueExprNode* aOutExpr)
	: WinFuncNode(pool, aAggInfo, aArg),
	  direction(aDirection),
	  rows(aRows),
	  outExpr(aOutExpr)
{
	fb_assert(direction == -1 || direction == 1);
}

void LagLeadWinNode::parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned /*count*/)
{
	arg = PAR_parse_value(tdbb, csb);
	rows = PAR_parse_value(tdbb, csb);
	outExpr = PAR_parse_value(tdbb, csb);
}

string LagLeadWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);

	NODE_PRINT(printer, direction);
	NODE_PRINT(printer, rows);
	NODE_PRINT(printer, outExpr);

	return "LagLeadWinNode";
}

void LagLeadWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	MAKE_desc(dsqlScratch, desc, arg);
	desc->setNullable(true);
}

void LagLeadWinNode::getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc)
{
	arg->getDesc(tdbb, csb, desc);
}

void LagLeadWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);
}

dsc* LagLeadWinNode::winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const
{
	dsc* desc = EVL_expr(tdbb, request, rows);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	SINT64 records = MOV_get_int64(tdbb, desc, 0);
	if (records < 0)
	{
		status_exception::raise(Arg::Gds(isc_sysf_argnmustbe_nonneg) <<
			Arg::Num(2) << Arg::Str(aggInfo.name));
	}

	if (!window->moveWithinPartition(records * direction))
	{
		desc = EVL_expr(tdbb, request, outExpr);
		if (!desc || (request->req_flags & req_null))
			return NULL;

		return desc;
	}

	desc = EVL_expr(tdbb, request, arg);
	if (!desc || (request->req_flags & req_null))
		return NULL;

	return desc;
}


//--------------------


static WinFuncNode::RegisterFactory0<LagWinNode> lagWinInfo("LAG");

LagWinNode::LagWinNode(MemoryPool& pool, ValueExprNode* aArg, ValueExprNode* aRows,
			ValueExprNode* aOutExpr)
	: LagLeadWinNode(pool, lagWinInfo, -1, aArg, aRows, aOutExpr)
{
}

ValueExprNode* LagWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	LagWinNode* node = FB_NEW_POOL(*tdbb->getDefaultPool()) LagWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	node->rows = copier.copy(tdbb, rows);
	node->outExpr = copier.copy(tdbb, outExpr);
	return node;
}

AggNode* LagWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) LagWinNode(dsqlScratch->getPool(),
		doDsqlPass(dsqlScratch, arg),
		doDsqlPass(dsqlScratch, rows),
		doDsqlPass(dsqlScratch, outExpr));
}


//--------------------


static WinFuncNode::RegisterFactory0<LeadWinNode> leadWinInfo("LEAD");

LeadWinNode::LeadWinNode(MemoryPool& pool, ValueExprNode* aArg, ValueExprNode* aRows,
			ValueExprNode* aOutExpr)
	: LagLeadWinNode(pool, leadWinInfo, 1, aArg, aRows, aOutExpr)
{
}

ValueExprNode* LeadWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	LeadWinNode* node = FB_NEW_POOL(*tdbb->getDefaultPool()) LeadWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	node->rows = copier.copy(tdbb, rows);
	node->outExpr = copier.copy(tdbb, outExpr);
	return node;
}

AggNode* LeadWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	return FB_NEW_POOL(dsqlScratch->getPool()) LeadWinNode(dsqlScratch->getPool(),
		doDsqlPass(dsqlScratch, arg),
		doDsqlPass(dsqlScratch, rows),
		doDsqlPass(dsqlScratch, outExpr));
}


//--------------------


static WinFuncNode::RegisterFactory0<NTileWinNode> nTileWinInfo("NTILE");

NTileWinNode::NTileWinNode(MemoryPool& pool, ValueExprNode* aArg)
	: WinFuncNode(pool, nTileWinInfo, aArg),
	  thisImpureOffset(0)
{
}

void NTileWinNode::parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned /*count*/)
{
	arg = PAR_parse_value(tdbb, csb);
}

string NTileWinNode::internalPrint(NodePrinter& printer) const
{
	WinFuncNode::internalPrint(printer);

	NODE_PRINT(printer, thisImpureOffset);

	return "NTileWinNode";
}

void NTileWinNode::make(DsqlCompilerScratch* dsqlScratch, dsc* desc)
{
	dsc argDesc;
	MAKE_desc(dsqlScratch, &argDesc, arg);

	if (!argDesc.isExact() || argDesc.dsc_scale != 0)
	{
		status_exception::raise(
			Arg::Gds(isc_sysf_argmustbe_exact) << "NTILE");
	}

	if (dsqlScratch->clientDialect == 1)
		desc->makeDouble();
	else
		desc->makeInt64(0);
}

void NTileWinNode::getDesc(thread_db* /*tdbb*/, CompilerScratch* /*csb*/, dsc* desc)
{
	desc->makeInt64(0);
}

ValueExprNode* NTileWinNode::copy(thread_db* tdbb, NodeCopier& copier) const
{
	NTileWinNode* node = FB_NEW_POOL(*tdbb->getDefaultPool()) NTileWinNode(*tdbb->getDefaultPool());
	node->arg = copier.copy(tdbb, arg);
	return node;
}

AggNode* NTileWinNode::pass2(thread_db* tdbb, CompilerScratch* csb)
{
	AggNode::pass2(tdbb, csb);
	thisImpureOffset = CMP_impure(csb, sizeof(ThisImpure));
	return this;
}

void NTileWinNode::aggInit(thread_db* tdbb, jrd_req* request) const
{
	AggNode::aggInit(tdbb, request);

	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	ThisImpure* thisImpure = request->getImpure<ThisImpure>(thisImpureOffset);

	impure->make_int64(0, 0);
	impure->vlux_count = 0;

	dsc* desc = EVL_expr(tdbb, request, arg);

	if (!desc || (request->req_flags & req_null))
	{
		status_exception::raise(
			Arg::Gds(isc_sysf_argnmustbe_positive) <<
			Arg::Num(1) << Arg::Str(aggInfo.name));
	}

	thisImpure->buckets = MOV_get_int64(tdbb, desc, 0);

	if (thisImpure->buckets <= 0)
	{
		status_exception::raise(
			Arg::Gds(isc_sysf_argnmustbe_positive) <<
			Arg::Num(1) << Arg::Str(aggInfo.name));
	}
}

dsc* NTileWinNode::winPass(thread_db* /*tdbb*/, jrd_req* request, SlidingWindow* window) const
{
	impure_value_ex* impure = request->getImpure<impure_value_ex>(impureOffset);
	ThisImpure* thisImpure = request->getImpure<ThisImpure>(thisImpureOffset);

	SINT64& n = impure->vlux_count;
	const SINT64& buckets = thisImpure->buckets;
	SINT64 partitionSize = window->getPartitionSize();

	SINT64 topBoundary = partitionSize / buckets + 1;
	SINT64 bottomBoundary = partitionSize / buckets;
	SINT64 numTopBoundary = partitionSize % buckets;
	SINT64 result;

	if (n < topBoundary * numTopBoundary)
		result = (n / topBoundary) + 1;
	else
		result = numTopBoundary + (n - numTopBoundary * topBoundary) / bottomBoundary + 1;

	++n;

	impure->vlu_misc.vlu_int64 = result;
	return &impure->vlu_desc;
}


AggNode* NTileWinNode::dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/
{
	NTileWinNode* node = FB_NEW_POOL(dsqlScratch->getPool()) NTileWinNode(dsqlScratch->getPool(),
		doDsqlPass(dsqlScratch, arg));

	dsc argDesc;
	argDesc.makeInt64(0);
	PASS1_set_parameter_type(dsqlScratch, node->arg, &argDesc, false);

	return node;
}


}	// namespace Jrd
