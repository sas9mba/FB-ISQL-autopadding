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

#ifndef DSQL_WIN_NODES_H
#define DSQL_WIN_NODES_H

#include "../jrd/blr.h"
#include "../dsql/Nodes.h"
#include "../dsql/NodePrinter.h"

namespace Jrd {


// DENSE_RANK function.
class DenseRankWinNode : public WinFuncNode
{
public:
	explicit DenseRankWinNode(MemoryPool& pool);

	virtual unsigned getCapabilities() const
	{
		return CAP_SUPPORTS_WINDOW_FRAME | CAP_WANTS_AGG_CALLS;
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		// nothing
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;
	virtual void aggPass(thread_db* tdbb, jrd_req* request, dsc* desc) const;
	virtual dsc* aggExecute(thread_db* tdbb, jrd_req* request) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;
};

// RANK function.
class RankWinNode : public WinFuncNode
{
public:
	explicit RankWinNode(MemoryPool& pool);

	virtual unsigned getCapabilities() const
	{
		return CAP_SUPPORTS_WINDOW_FRAME | CAP_WANTS_AGG_CALLS;
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		// nothing
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual AggNode* pass2(thread_db* tdbb, CompilerScratch* csb);

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;
	virtual void aggPass(thread_db* tdbb, jrd_req* request, dsc* desc) const;
	virtual dsc* aggExecute(thread_db* tdbb, jrd_req* request) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;

private:
	USHORT tempImpure;
};

// PERCENT_RANK function.
class PercentRankWinNode : public WinFuncNode
{
public:
	explicit PercentRankWinNode(MemoryPool& pool);

	virtual unsigned getCapabilities() const
	{
		return CAP_SUPPORTS_WINDOW_FRAME | CAP_WANTS_AGG_CALLS | CAP_WANTS_WIN_PASS_CALL;
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		// nothing
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual AggNode* pass2(thread_db* tdbb, CompilerScratch* csb);

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;
	virtual void aggPass(thread_db* tdbb, jrd_req* request, dsc* desc) const;
	virtual dsc* aggExecute(thread_db* tdbb, jrd_req* request) const;

	virtual dsc* winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;

private:
	USHORT tempImpure;
};

// CUME_DIST function.
class CumeDistWinNode : public WinFuncNode
{
public:
	explicit CumeDistWinNode(MemoryPool& pool);

	virtual unsigned getCapabilities() const
	{
		return CAP_SUPPORTS_WINDOW_FRAME | CAP_WANTS_WIN_PASS_CALL;
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		// nothing
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual AggNode* pass2(thread_db* tdbb, CompilerScratch* csb);

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;

	virtual dsc* winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;
};

// ROW_NUMBER function.
class RowNumberWinNode : public WinFuncNode
{
public:
	explicit RowNumberWinNode(MemoryPool& pool);

	virtual unsigned getCapabilities() const
	{
		return CAP_SUPPORTS_WINDOW_FRAME | CAP_WANTS_WIN_PASS_CALL;
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		// nothing
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;

	virtual dsc* winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;
};

// FIRST_VALUE function.
class FirstValueWinNode : public WinFuncNode
{
public:
	explicit FirstValueWinNode(MemoryPool& pool, ValueExprNode* aArg = NULL);

	virtual unsigned getCapabilities() const
	{
		return CAP_RESPECTS_WINDOW_FRAME | CAP_WANTS_WIN_PASS_CALL;
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;

	virtual dsc* winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;

	virtual void parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned count);
};

// LAST_VALUE function.
class LastValueWinNode : public WinFuncNode
{
public:
	explicit LastValueWinNode(MemoryPool& pool, ValueExprNode* aArg = NULL);

	virtual unsigned getCapabilities() const
	{
		return CAP_RESPECTS_WINDOW_FRAME | CAP_WANTS_WIN_PASS_CALL;
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;

	virtual dsc* winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;

	virtual void parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned count);
};

// NTH_VALUE function.
class NthValueWinNode : public WinFuncNode
{
public:
	enum
	{
		FROM_FIRST = 0,
		FROM_LAST
	};

public:
	explicit NthValueWinNode(MemoryPool& pool, ValueExprNode* aArg = NULL,
		ValueExprNode* aRow = NULL, ValueExprNode* aFrom = NULL);

	virtual unsigned getCapabilities() const
	{
		return CAP_RESPECTS_WINDOW_FRAME | CAP_WANTS_WIN_PASS_CALL;
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		WinFuncNode::getChildren(holder, dsql);
		holder.add(row);
		holder.add(from);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;

	virtual dsc* winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;

	virtual void parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned count);

private:
	NestConst<ValueExprNode> row;
	NestConst<ValueExprNode> from;
};

// LAG/LEAD function.
class LagLeadWinNode : public WinFuncNode
{
public:
	explicit LagLeadWinNode(MemoryPool& pool, const AggInfo& aAggInfo, int aDirection,
		ValueExprNode* aArg = NULL, ValueExprNode* aRows = NULL, ValueExprNode* aOutExpr = NULL);

	virtual unsigned getCapabilities() const
	{
		return CAP_SUPPORTS_WINDOW_FRAME | CAP_WANTS_WIN_PASS_CALL;
	}

	virtual void getChildren(NodeRefsHolder& holder, bool dsql) const
	{
		WinFuncNode::getChildren(holder, dsql);
		holder.add(rows);
		holder.add(outExpr);
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const = 0;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;
	virtual dsc* winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const;

protected:
	virtual void parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned count);

protected:
	const int direction;
	NestConst<ValueExprNode> rows;
	NestConst<ValueExprNode> outExpr;
};

// LAG function.
class LagWinNode : public LagLeadWinNode
{
public:
	explicit LagWinNode(MemoryPool& pool, ValueExprNode* aArg = NULL, ValueExprNode* aRows = NULL,
		ValueExprNode* aOutExpr = NULL);

	virtual Firebird::string internalPrint(NodePrinter& printer) const
	{
		LagLeadWinNode::internalPrint(printer);
		return "LagWinNode";
	}

	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;
};

// LEAD function.
class LeadWinNode : public LagLeadWinNode
{
public:
	explicit LeadWinNode(MemoryPool& pool, ValueExprNode* aArg = NULL, ValueExprNode* aRows = NULL,
		ValueExprNode* aOutExpr = NULL);

	virtual Firebird::string internalPrint(NodePrinter& printer) const
	{
		LagLeadWinNode::internalPrint(printer);
		return "LeadWinNode";
	}

	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;
};

// NTILE function.
class NTileWinNode : public WinFuncNode
{
public:
	explicit NTileWinNode(MemoryPool& pool, ValueExprNode* aArg = NULL);

	virtual unsigned getCapabilities() const
	{
		return CAP_SUPPORTS_WINDOW_FRAME | CAP_WANTS_WIN_PASS_CALL;
	}

	virtual Firebird::string internalPrint(NodePrinter& printer) const;
	virtual void make(DsqlCompilerScratch* dsqlScratch, dsc* desc);
	virtual void getDesc(thread_db* tdbb, CompilerScratch* csb, dsc* desc);
	virtual ValueExprNode* copy(thread_db* tdbb, NodeCopier& copier) const;
	virtual AggNode* pass2(thread_db* tdbb, CompilerScratch* csb);

	virtual void aggInit(thread_db* tdbb, jrd_req* request) const;

	virtual dsc* winPass(thread_db* tdbb, jrd_req* request, SlidingWindow* window) const;

protected:
	virtual AggNode* dsqlCopy(DsqlCompilerScratch* dsqlScratch) /*const*/;
	virtual void parseArgs(thread_db* tdbb, CompilerScratch* csb, unsigned count);

private:
	struct ThisImpure
	{
		SINT64 buckets;
	};

	USHORT thisImpureOffset;
};


} // namespace

#endif // DSQL_WIN_NODES_H
